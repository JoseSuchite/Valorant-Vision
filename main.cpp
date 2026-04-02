#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <unordered_set>

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

// ------------------------------------------------------------
// RELATIVE COORDINATE SYSTEM
// ------------------------------------------------------------
struct RelRect {
    double x1, y1, x2, y2;
};

RelRect to_rel_from_abs(int x1, int y1, int x2, int y2,
                        int base_w = 1920, int base_h = 1080) {
    return {
        double(x1) / base_w,
        double(y1) / base_h,
        double(x2) / base_w,
        double(y2) / base_h
    };
}

cv::Rect rel_to_rect(const RelRect& r, int img_w, int img_h) {
    int x = int(r.x1 * img_w);
    int y = int(r.y1 * img_h);
    int w = int((r.x2 - r.x1) * img_w);
    int h = int((r.y2 - r.y1) * img_h);
    return cv::Rect(x, y, w, h);
}

std::vector<std::string> extract_tokens(const std::string& raw) {
    std::vector<std::string> tokens;
    std::string cur;

    for (char c : raw) {
        if (std::isalnum((unsigned char)c)) {
            cur.push_back(c);
        } else {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
        }
    }
    if (!cur.empty()) tokens.push_back(cur);

    return tokens;
}

// ------------------------------------------------------------
// OCR HELPER
// ------------------------------------------------------------
std::string ocr_region(tesseract::TessBaseAPI& tess, const cv::Mat& roi) {
    cv::Mat gray;
    cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);

    // upscale
    cv::Mat big;
    cv::resize(gray, big, cv::Size(), 3.0, 3.0, cv::INTER_CUBIC);

    // LIGHT threshold (important)
    cv::Mat bin;
    cv::threshold(big, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    
    Pix* pix = pixCreate(big.cols, big.rows, 8);
    for (int y = 0; y < big.rows; ++y)
    for (int x = 0; x < big.cols; ++x)
    pixSetPixel(pix, x, y, big.at<uchar>(y, x));
    
    tess.SetImage(pix);
    char* out = tess.GetUTF8Text();
    std::string result = out ? out : "";
    if (out) delete[] out;
    pixDestroy(&pix);
    
    // trim
    auto start = result.find_first_not_of(" \n\r\t");
    auto end   = result.find_last_not_of(" \n\r\t");
    if (start == std::string::npos) return "";
    return result.substr(start, end - start + 1);
}


// ------------------------------------------------------------
// PLAYER LIST + MATCHING
// ------------------------------------------------------------
std::vector<std::string> load_player_list(const std::string& path) {
    std::vector<std::string> players;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open player list: " << path << "\n";
        return players;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty())
        players.push_back(line);
    }
    return players;
}

std::string normalize(const std::string& s) {
    std::string out;
    for (char c : s)
    if (!std::isspace((unsigned char)c))
    out.push_back(std::tolower(c));
    return out;
}

int levenshtein_distance(const std::string& a, const std::string& b) {
    size_t m = a.size(), n = b.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    
    for (size_t i = 0; i <= m; i++) dp[i][0] = i;
    for (size_t j = 0; j <= n; j++) dp[0][j] = j;
    
    for (size_t i = 1; i <= m; i++)
    for (size_t j = 1; j <= n; j++) {
        int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
        dp[i][j] = std::min({
            dp[i - 1][j] + 1,
            dp[i][j - 1] + 1,
            dp[i - 1][j - 1] + cost
        });
    }
    
    return dp[m][n];
}

std::string match_player(const std::string& ocr_text,
    const std::vector<std::string>& players) {
        if (players.empty()) return "";
        
        std::string norm = normalize(ocr_text);
        int best_score = 9999;
        std::string best_match;
        
        for (const auto& p : players) {
            int dist = levenshtein_distance(norm, normalize(p));
            if (dist < best_score) {
                best_score = dist;
                best_match = p;
            }
        }
        
        if (best_score > 6)  // threshold
        return "";
        
        return best_match;
    }
    
    std::string match_from_tokens(const std::vector<std::string>& tokens,
        const std::vector<std::string>& players) {
            std::string best_match;
            int best_score = 9999;
            
            for (const auto& t : tokens) {
                std::string norm_t = normalize(t);
                for (const auto& p : players) {
                    int dist = levenshtein_distance(norm_t, normalize(p));
                    if (dist < best_score) {
                        best_score = dist;
                        best_match = p;
                    }
                }
            }
            
            if (best_score > 3) return "";
            return best_match;
        }
        struct KillEvent {
            std::string killer;
            std::string victim;
        };
        
        KillEvent resolve_killfeed_whole(const std::string& raw,
                                 const std::vector<std::string>& players)
        {
            KillEvent evt;

            // 1. Extract tokens from OCR text
            auto tokens = extract_tokens(raw);
            if (tokens.empty())
                return evt;

            struct Match {
                std::string token;
                std::string player;
                int dist;
            };

            std::vector<Match> good_matches;

            // 2. Match each token to the closest player
            for (auto& t : tokens) {
                std::string norm_t = normalize(t);

                int best_dist = 9999;
                std::string best_player;

                for (auto& p : players) {
                    int d = levenshtein_distance(norm_t, normalize(p));
                    if (d < best_dist) {
                        best_dist = d;
                        best_player = p;
                    }
                }

                // 3. Only accept GOOD matches (distance <= 3)
                if (best_dist <= 3) {
                    good_matches.push_back({ t, best_player, best_dist });
                }
            }

            // 4. Require EXACTLY two matched players
            if (good_matches.size() != 2)
                return evt;

            // 5. Preserve left→right order based on token order
            evt.killer = good_matches[0].player;
            evt.victim = good_matches[1].player;

            // 6. Reject self-kills (unless the game actually allows them)
            if (evt.killer == evt.victim) {
                evt.killer.clear();
                evt.victim.clear();
            }

            return evt;
        }


        // ------------------------------------------------------------
        // MAIN
        // ------------------------------------------------------------
        int main(int argc, char** argv) {
        if (argc < 4) {
            std::cerr << "Usage: " << argv[0]
                    << " <video_path> <tessdata_path> <player_list.txt> [lang]\n";
            return 1;
        }

        std::string video_path = argv[1];
        std::string tessdata_path = argv[2];
        std::string player_list_path = argv[3];
        std::string lang = (argc >= 5) ? argv[4] : "eng";

        // Load player list
        auto players = load_player_list(player_list_path);
        if (players.empty()) {
            std::cerr << "Player list is empty.\n";
            return 1;
        }

        // Open video
        cv::VideoCapture cap(video_path);
        if (!cap.isOpened()) {
            std::cerr << "Failed to open video: " << video_path << "\n";
            return 1;
        }

        // Init Tesseract
        tesseract::TessBaseAPI tess;
        if (tess.Init(tessdata_path.c_str(), lang.c_str())) {
            std::cerr << "Could not initialize tesseract.\n";
            return 1;
        }
        tess.SetVariable("debug_level", "0");
        tess.SetVariable("debug_file", "NUL");


        // Define ROIs (relative)
        RelRect left_score_rel  = to_rel_from_abs(770, 0, 840, 60);
        RelRect right_score_rel = to_rel_from_abs(1040, 0, 1110, 60);
        RelRect round_timer_rel = to_rel_from_abs(880, 0, 1010, 67);

        // WHOLE killfeed area
        RelRect killfeed_rel_0 = to_rel_from_abs(1291,  90, 1898, 130);  // newest
        RelRect killfeed_rel_1 = to_rel_from_abs(1291, 130, 1898, 170);  // 1 below
        RelRect killfeed_rel_2 = to_rel_from_abs(1291, 170, 1898, 210);  // 2 below
        RelRect killfeed_rel_3 = to_rel_from_abs(1291, 210, 1898, 250);  // 3 below


        std::cout << "Press Q to quit.\n";

        double last_kill_time = 0;
        const double KILL_COOLDOWN = 1; // seconds
        std::string last_kill_signature = "";
        std::unordered_set<std::string> seen_kills;

        while (true) {
            cv::Mat frame;
            if (!cap.read(frame)) {
                std::cout << "End of video.\n";
                break;
            }

            int img_w = frame.cols;
            int img_h = frame.rows;

            // Convert relative ROIs to absolute
            cv::Rect left_score_roi  = rel_to_rect(left_score_rel,  img_w, img_h);
            cv::Rect right_score_roi = rel_to_rect(right_score_rel, img_w, img_h);
            cv::Rect round_timer_roi = rel_to_rect(round_timer_rel, img_w, img_h);
            cv::Rect killfeed_roi_0 = rel_to_rect(killfeed_rel_0, img_w, img_h);
            cv::Rect killfeed_roi_1 = rel_to_rect(killfeed_rel_1, img_w, img_h);
            cv::Rect killfeed_roi_2 = rel_to_rect(killfeed_rel_2, img_w, img_h);
            cv::Rect killfeed_roi_3 = rel_to_rect(killfeed_rel_3, img_w, img_h);


            // Clamp ROIs
            auto clamp = [&](cv::Rect& r) { r &= cv::Rect(0, 0, img_w, img_h); };
            clamp(left_score_roi);
            clamp(right_score_roi);
            clamp(round_timer_roi);
            clamp(killfeed_roi_0);
            clamp(killfeed_roi_1);
            clamp(killfeed_roi_2);
            clamp(killfeed_roi_3);

            // --- SCORE OCR ---
            tess.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
            tess.SetVariable("tessedit_char_whitelist", "0123456789");

            std::string left_score  = ocr_region(tess, frame(left_score_roi));
            std::string right_score = ocr_region(tess, frame(right_score_roi));

            // --- TIMER OCR ---
            tess.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
            tess.SetVariable("tessedit_char_whitelist",
                "0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

            std::string round_timer = ocr_region(tess, frame(round_timer_roi));

            // --- KILLFEED OCR (whole area) ---
            tess.SetPageSegMode(tesseract::PSM_SPARSE_TEXT);
            tess.SetVariable("tessedit_char_whitelist",
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

            std::string raw0 = ocr_region(tess, frame(killfeed_roi_0));
            std::string raw1 = ocr_region(tess, frame(killfeed_roi_1));
            std::string raw2 = ocr_region(tess, frame(killfeed_roi_2));
            std::string raw3 = ocr_region(tess, frame(killfeed_roi_3));

            // Resolve killfeed
            KillEvent evt0 = resolve_killfeed_whole(raw0, players);
            KillEvent evt1 = resolve_killfeed_whole(raw1, players);
            KillEvent evt2 = resolve_killfeed_whole(raw2, players);
            KillEvent evt3 = resolve_killfeed_whole(raw3, players);

            auto try_print = [&](const KillEvent& evt) {
                if (evt.killer.empty() || evt.victim.empty())
                    return;

                std::string sig = evt.killer + "|" + evt.victim;

                // If we've already printed this killfeed, skip it
                if (seen_kills.count(sig))
                    return;

                // Otherwise print it and remember it
                std::cout << evt.killer << " killed " << evt.victim << "\n";
                seen_kills.insert(sig);
            };

            try_print(evt0);
            try_print(evt1);
            try_print(evt2);
            try_print(evt3);

            // --- Debug view ---
            cv::rectangle(frame, left_score_roi,  cv::Scalar(0,255,0), 2);
            cv::rectangle(frame, right_score_roi, cv::Scalar(255,0,0), 2);
            cv::rectangle(frame, round_timer_roi, cv::Scalar(0,0,255), 2);
            cv::rectangle(frame, killfeed_roi_0,    cv::Scalar(255,255,255), 2);
            cv::rectangle(frame, killfeed_roi_1,    cv::Scalar(255,255,255), 2);
            cv::rectangle(frame, killfeed_roi_2,    cv::Scalar(255,255,255), 2);
            cv::rectangle(frame, killfeed_roi_3,    cv::Scalar(255,255,255), 2);

            cv::imshow("debug", frame);

            char key = (char)cv::waitKey(1);
            if (key == 'q' || key == 'Q') {
                break;
            }
        }

        tess.End();
        return 0;
    }

