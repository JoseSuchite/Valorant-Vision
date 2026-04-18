#include "../headers/ocr.h"
#include <QDateTime>
#include <algorithm>
#include <cctype>

// Relative coordinate helpers (calibrated for 1920x1080 broadcast layout)
struct RelRect { double x1, y1, x2, y2; };

static cv::Rect rel_to_rect(const RelRect& r, int w, int h)
{
    return cv::Rect(
        int(r.x1 * w), int(r.y1 * h),
        int((r.x2 - r.x1) * w), int((r.y2 - r.y1) * h)
    );
}

static const RelRect KILLFEED_ROWS[] = {
    { 1291.0/1920,  90.0/1080, 1898.0/1920, 130.0/1080 },
    { 1291.0/1920, 130.0/1080, 1898.0/1920, 170.0/1080 },
    { 1291.0/1920, 170.0/1080, 1898.0/1920, 210.0/1080 },
    { 1291.0/1920, 210.0/1080, 1898.0/1920, 250.0/1080 },
};
static const int KILLFEED_ROW_COUNT = 4;

static const RelRect TEAM_LEFT_ROI   = { 720.0/1920,  10.0/1080,  790.0/1920,  38.0/1080 };
static const RelRect TEAM_RIGHT_ROI  = { 1130.0/1920, 10.0/1080, 1200.0/1920,  38.0/1080 };
static const RelRect SCORE_LEFT_ROI  = { 800.0/1920,   0.0/1080,  860.0/1920,  60.0/1080 };
static const RelRect SCORE_RIGHT_ROI = { 1060.0/1920,  0.0/1080, 1120.0/1920,  60.0/1080 };
static const RelRect ROUND_TIMER_ROI = { 880.0/1920,   0.0/1080, 1010.0/1920,  67.0/1080 };

static const int FRAME_INTERVAL_MS = 500;

// Constructor / destructor
OCRDetector::OCRDetector(const std::string& tessdataPath,
                         const std::string& lang,
                         QObject* parent)
    : QObject(parent)
{
    tessOk = (tess.Init(tessdataPath.c_str(), lang.c_str()) == 0);
    if (tessOk) {
        tess.SetVariable("debug_level", "0");
        tess.SetVariable("debug_file", "NUL");
    }

    teamNames = { "NRG","PRX","LOUD","SEN","EG","TL","FNC","T1","G2","KRU","LEV" };

    frameTimer = new QTimer(this);
    frameTimer->setInterval(FRAME_INTERVAL_MS);
    connect(frameTimer, &QTimer::timeout, this, &OCRDetector::processNextFrame);
}

OCRDetector::~OCRDetector()
{
    frameTimer->stop();
    cap.release();
    if (tessOk) tess.End();
}

// Public control
void OCRDetector::startVideo(const std::string& videoPath)
{
    frameTimer->stop();
    cap.release();

    teamDetected = false;
    seenKills.clear();
    lastLeftScore  = -1;
    lastRightScore = -1;
    leftTeamName.clear();
    rightTeamName.clear();

    if (!tessOk) return;

    cap.open(videoPath);
    if (cap.isOpened()) {
        capFps = cap.get(cv::CAP_PROP_FPS);
        if (capFps <= 0) capFps = 30.0;
        frameTimer->start();
    }
}

void OCRDetector::reset()
{
    frameTimer->stop();
    cap.release();
    teamDetected = false;
    seenKills.clear();
    lastLeftScore  = -1;
    lastRightScore = -1;
}

void OCRDetector::setTeamNames(const std::vector<std::string>& names)
{
    if (!names.empty()) teamNames = names;
}

void OCRDetector::setPlayerRecords(const std::vector<std::pair<std::string,std::string>>& nameTeamPairs)
{
    nameToTeam.clear();
    playerNames.clear();
    for (const auto& p : nameTeamPairs) {
        nameToTeam[p.first] = p.second;
        playerNames.push_back(p.first);
    }
    playerListValid = !playerNames.empty();
}

void OCRDetector::onVideoPlaying(qint64 positionMs)
{
    currentVideoPos = positionMs;
    if (!cap.isOpened()) return;
    frameTimer->start();
}

void OCRDetector::onVideoPaused()
{
    frameTimer->stop();
}

void OCRDetector::onVideoPositionChanged(qint64 positionMs)
{
    currentVideoPos = positionMs;
}

// ---------------------------------------------------------------------------
// Timer slot
// ---------------------------------------------------------------------------
void OCRDetector::processNextFrame()
{
    if (!cap.isOpened()) return;

    // Skip tick if the previous OCR is still running — avoids piling up work
    if (ocrBusy.load()) return;

    // Only seek when OCR has drifted more than 1.5 seconds from the video.
    // Sequential cap.read() is fast; H264 seeks are expensive (keyframe decode).
    if (capFps > 0) {
        double ocrPosMs = (cap.get(cv::CAP_PROP_POS_FRAMES) / capFps) * 1000.0;
        double drift    = std::abs(ocrPosMs - static_cast<double>(currentVideoPos.load()));
        if (drift > 1500.0) {
            double targetFrame = (currentVideoPos.load() / 1000.0) * capFps;
            cap.set(cv::CAP_PROP_POS_FRAMES, targetFrame);
        }
    }

    cv::Mat frame;
    if (!cap.read(frame)) {
        frameTimer->stop();
        return;
    }

    // Run all Tesseract work off the main thread so the UI stays responsive.
    // ocrBusy prevents a second step until this one finishes.
    ocrBusy.store(true);
    std::thread([this, f = std::move(frame)]() mutable {
        processFrame(f);
        ocrBusy.store(false);
    }).detach();
}

// Core OCR logic
std::string OCRDetector::normalize(const std::string& s)
{
    std::string out;
    for (char c : s)
        if (!std::isspace((unsigned char)c))
            out.push_back(std::tolower(c));
    return out;
}

int OCRDetector::levenshtein_distance(const std::string& a, const std::string& b)
{
    size_t m = a.size(), n = b.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    for (size_t i = 0; i <= m; i++) dp[i][0] = (int)i;
    for (size_t j = 0; j <= n; j++) dp[0][j] = (int)j;
    for (size_t i = 1; i <= m; i++)
        for (size_t j = 1; j <= n; j++) {
            int cost = (a[i-1] == b[j-1]) ? 0 : 1;
            dp[i][j] = std::min({ dp[i-1][j]+1, dp[i][j-1]+1, dp[i-1][j-1]+cost });
        }
    return dp[m][n];
}

std::string OCRDetector::ocr_region(const cv::Mat& roi)
{
    if (roi.empty()) return "";

    cv::Mat gray;
    if (roi.channels() == 3)
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    else
        gray = roi.clone();

    cv::Mat big;
    cv::resize(gray, big, cv::Size(), 3.0, 3.0, cv::INTER_CUBIC);
    cv::threshold(big, big, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    Pix* pix = pixCreate(big.cols, big.rows, 8);
    for (int y = 0; y < big.rows; ++y)
        for (int x = 0; x < big.cols; ++x)
            pixSetPixel(pix, x, y, big.at<uchar>(y, x));

    tess.SetImage(pix);
    char* out = tess.GetUTF8Text();
    std::string result = out ? out : "";
    if (out) delete[] out;
    pixDestroy(&pix);

    auto s = result.find_first_not_of(" \n\r\t");
    auto e = result.find_last_not_of(" \n\r\t");
    if (s == std::string::npos) return "";
    return result.substr(s, e - s + 1);
}

std::string OCRDetector::ocr_team_name(const cv::Mat& roi)
{
    tess.SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    tess.SetVariable("tessedit_char_whitelist",
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    return ocr_region(roi);
}

std::string OCRDetector::resolve_team_name(const std::string& raw,
                                           const std::vector<std::string>& teams)
{
    std::string token = normalize(raw);
    if (token.empty()) return "";

    int alpha = (int)std::count_if(token.begin(), token.end(), ::isalpha);
    if (alpha < (int)token.size() - 1) return "";

    int bestDist = 9999;
    std::string bestTeam;
    for (const auto& t : teams) {
        int d = levenshtein_distance(token, normalize(t));
        if (d < bestDist) { bestDist = d; bestTeam = t; }
    }
    return (bestDist <= 2) ? bestTeam : "";
}

std::vector<std::string> OCRDetector::extract_tokens(const std::string& raw)
{
    std::vector<std::string> tokens;
    std::string cur;
    for (char c : raw) {
        if (std::isalnum((unsigned char)c)) {
            cur.push_back(c);
        } else if (!cur.empty()) {
            tokens.push_back(cur);
            cur.clear();
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

KillEvent OCRDetector::resolve_killfeed_whole(const std::string& raw,
                                              const std::vector<std::string>& players,
                                              bool player_list_valid)
{
    KillEvent evt;
    auto tokens = extract_tokens(raw);
    if (tokens.empty()) return evt;

    if (!player_list_valid) {
        if (tokens.size() >= 2) { evt.killer = tokens[0]; evt.victim = tokens[1]; }
        return evt;
    }

    struct Match { std::string token, player; int dist; };
    std::vector<Match> good;

    for (const auto& t : tokens) {
        std::string norm_t = normalize(t);
        int best = 9999;
        std::string bestp;
        for (const auto& p : players) {
            int d = levenshtein_distance(norm_t, normalize(p));
            if (d < best) { best = d; bestp = p; }
        }
        if (best <= 3)
            good.push_back({ t, bestp, best });
    }

    if (good.size() != 2) return evt;

    evt.killer = good[0].player;
    evt.victim = good[1].player;
    if (evt.killer == evt.victim) { evt.killer.clear(); evt.victim.clear(); }
    return evt;
}

void OCRDetector::processFrame(const cv::Mat& frame)
{
    if (frame.empty() || !tessOk) return;

    double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    int img_w = frame.cols, img_h = frame.rows;

    // --- TEAM DETECTION (once) ---
    if (!teamDetected) {
        cv::Rect lr = rel_to_rect(TEAM_LEFT_ROI,  img_w, img_h) & cv::Rect(0,0,img_w,img_h);
        cv::Rect rr = rel_to_rect(TEAM_RIGHT_ROI, img_w, img_h) & cv::Rect(0,0,img_w,img_h);

        std::string left_team  = resolve_team_name(ocr_team_name(frame(lr)), teamNames);
        std::string right_team = resolve_team_name(ocr_team_name(frame(rr)), teamNames);

        if (!left_team.empty() && !right_team.empty()) {
            leftTeamName  = left_team;
            rightTeamName = right_team;
            emit ocrLog(QString("Teams detected: %1 vs %2")
                        .arg(QString::fromStdString(left_team))
                        .arg(QString::fromStdString(right_team)));
            emit teamsDetected(QString::fromStdString(left_team),
                               QString::fromStdString(right_team));
            teamDetected = true;
        }
    }

    // --- SCORE OCR ---
    if (teamDetected) {
        tess.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
        tess.SetVariable("tessedit_char_whitelist", "0123456789");

        cv::Rect lsr = rel_to_rect(SCORE_LEFT_ROI,  img_w, img_h) & cv::Rect(0,0,img_w,img_h);
        cv::Rect rsr = rel_to_rect(SCORE_RIGHT_ROI, img_w, img_h) & cv::Rect(0,0,img_w,img_h);

        std::string ls = ocr_region(frame(lsr));
        std::string rs = ocr_region(frame(rsr));

        try {
            if (!ls.empty() && !rs.empty()) {
                int leftScore  = std::stoi(ls);
                int rightScore = std::stoi(rs);

                if (leftScore != lastLeftScore || rightScore != lastRightScore) {
                    lastLeftScore  = leftScore;
                    lastRightScore = rightScore;
                    emit scoresChanged(
                        QString::fromStdString(leftTeamName),  leftScore,
                        QString::fromStdString(rightTeamName), rightScore
                    );
                    // Clear seen kills on new round
                    seenKills.clear();
                }
            }
        } catch (...) {}
    }

    // --- ROUND TIMER OCR ---
    {
        tess.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
        tess.SetVariable("tessedit_char_whitelist", "0123456789:");
        cv::Rect tr = rel_to_rect(ROUND_TIMER_ROI, img_w, img_h) & cv::Rect(0,0,img_w,img_h);
        std::string raw = ocr_region(frame(tr));

        // Strip everything except digits and colons
        std::string cleaned;
        for (char c : raw)
            if (std::isdigit((unsigned char)c) || c == ':') cleaned.push_back(c);

        // Validate format: one or two digits, colon, exactly two digits (00-59)
        // Accepts "1:15", "0:45", "12:00" etc.
        bool valid = false;
        auto col = cleaned.find(':');
        if (col != std::string::npos && col >= 1 && col <= 2) {
            std::string minPart = cleaned.substr(0, col);
            std::string secPart = cleaned.substr(col + 1);
            if (secPart.size() == 2) {
                int secs = std::stoi(secPart);
                valid = (secs >= 0 && secs <= 59);
            }
        }

        lastRoundTimer = valid ? cleaned : "[no time detected]";
    }

    // --- KILLFEED OCR ---
    tess.SetPageSegMode(tesseract::PSM_SPARSE_TEXT);
    tess.SetVariable("tessedit_char_whitelist",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    for (int row = 0; row < KILLFEED_ROW_COUNT; ++row) {
        cv::Rect roi = rel_to_rect(KILLFEED_ROWS[row], img_w, img_h)
                       & cv::Rect(0, 0, img_w, img_h);
        if (roi.width < 4 || roi.height < 4) continue;

        std::string raw = ocr_region(frame(roi));
        KillEvent evt = resolve_killfeed_whole(raw, playerNames, playerListValid);

        if (!evt.killer.empty() && !evt.victim.empty()) {
            std::string sig = evt.killer + "|" + evt.victim;
            if (!seenKills.count(sig)) {
                seenKills.insert(sig);

                // Look up team for each player
                auto killerTeamIt = nameToTeam.find(evt.killer);
                auto victimTeamIt = nameToTeam.find(evt.victim);
                QString killerTeam = (killerTeamIt != nameToTeam.end())
                                     ? QString::fromStdString(killerTeamIt->second) : "";
                QString victimTeam = (victimTeamIt != nameToTeam.end())
                                     ? QString::fromStdString(victimTeamIt->second) : "";

                QString prefix = lastRoundTimer.empty()
                                 ? ""
                                 : QString::fromStdString(lastRoundTimer) + " - ";

                QString logLine;
                if (!killerTeam.isEmpty() && !victimTeam.isEmpty())
                    logLine = prefix
                              + killerTeam + " " + QString::fromStdString(evt.killer)
                              + " killed "
                              + victimTeam + " " + QString::fromStdString(evt.victim);
                else
                    logLine = prefix
                              + QString::fromStdString(evt.killer)
                              + " killed "
                              + QString::fromStdString(evt.victim);

                emit killDetected(logLine);
            }
        }
    }
}
