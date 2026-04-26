#ifndef OCR_H
#define OCR_H

#include <QObject>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>

struct KillEvent {
    std::string killer;
    std::string victim;
};

class OCRDetector : public QObject {
    Q_OBJECT

public:
    explicit OCRDetector(const std::string& tessdataPath,
                         const std::string& lang,
                         QObject* parent = nullptr);
    ~OCRDetector();

    // Pass full records so kill events can include team names
    void setPlayerRecords(const std::vector<std::pair<std::string,std::string>>& nameTeamPairs);
    void setTeamNames(const std::vector<std::string>& names);

public slots:
    void startVideo(const std::string& videoPath);
    void reset();
    // User accepted / rejected the proposed team names from the dialog
    void confirmTeams(QString left, QString right);
    void rejectTeams();
    // Sync slots wired to VideoPlayer signals
    void onVideoPlaying(qint64 positionMs);
    void onVideoPaused();
    void onVideoPositionChanged(qint64 positionMs);

signals:
    void ocrLog(QString text);
    // OCR detected team names - MainWindow shows a confirmation dialog
    void teamsProposed(QString left, QString right);
    // Fired only after the user has confirmed the names (roster scrape trigger)
    void teamsDetected(QString left, QString right);
    void killDetected(QString logLine);
    void scoresChanged(QString leftTeam, int leftScore, QString rightTeam, int rightScore);

private slots:
    void processNextFrame();

private:
    tesseract::TessBaseAPI tess;
    bool tessOk = false;

    cv::VideoCapture cap;
    QTimer* frameTimer = nullptr;

    // name -> team lookup
    std::unordered_map<std::string, std::string> nameToTeam;
    bool playerListValid = false;
    std::vector<std::string> playerNames; // for matching

    std::vector<std::string> teamNames;
    bool teamDetected = false;
    bool awaitingTeamConfirmation = false;
    std::string leftTeamName;
    std::string rightTeamName;

    std::unordered_set<std::string> seenKills;

    int lastLeftScore  = -1;
    int lastRightScore = -1;

    double startTime = 0.0;
    std::atomic<qint64> currentVideoPos{0};
    double capFps = 0.0;
    std::string lastRoundTimer;
    std::atomic_bool ocrBusy{false};
    std::atomic_bool shuttingDown{false};
    // Serializes tess access between the OCR worker thread and destructor cleanup
    std::mutex tessMutex;

    std::string normalize(const std::string& s);
    int levenshtein_distance(const std::string& a, const std::string& b);
    std::string ocr_region(const cv::Mat& roi);
    std::string ocr_team_name(const cv::Mat& roi);
    std::string resolve_team_name(const std::string& raw,
                                  const std::vector<std::string>& teams);
    std::vector<std::string> extract_tokens(const std::string& raw);
    KillEvent resolve_killfeed_whole(const std::string& raw,
                                     const std::vector<std::string>& players,
                                     bool player_list_valid);
    void processFrame(const cv::Mat& frame);
};

#endif
