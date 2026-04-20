#include <string>
#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QObject>
#include <QFileInfo>
#include <QPixmap>
#include <QLabel>
#include "../headers/logbar.h"
#include "../headers/mainwindow.h"
#include "../headers/videoplayer.h"
#include "../headers/minimap.h"
#include "../headers/mapdetector.h"
#include "../headers/ocr.h"
#include "../headers/webScraper.h"
#include <thread>

MainWindow::MainWindow() :
    QMainWindow() {
    // adds the logo next to the title
    setWindowIcon(QIcon(":/images/logo.png"));

    centralWidget = new QWidget(this);
    mainVerticalLayout = new QVBoxLayout(centralWidget);

    logBar = new LogBar(this);
    logBar->setObjectName("rightDockItem");

    addDockWidget(Qt::RightDockWidgetArea, logBar);
    minimap_wid = new Minimap(this);
    minimap_wid->setObjectName("rightDockItem");

    addDockWidget(Qt::RightDockWidgetArea, minimap_wid);
    minimap_wid->loadImage("map_layouts/Ascent_layout.png");

    player = new VideoPlayer(centralWidget);
    player->setObjectName("mainVideoPlayer");
    player->setAttribute(Qt::WA_StyledBackground);
    controlPanel = new QHBoxLayout(centralWidget);
    chooseFileButton = new QPushButton("Select Video", centralWidget);

    QPixmap pixmap(":/images/logo.png");
    picLabel = new QLabel();
    picLabel->setPixmap(pixmap);
    picLabel->setAlignment(Qt::AlignCenter);

    pauseButton = new QPushButton("Pause/Play", centralWidget);

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(100);

    logButton = new QPushButton("Reset Log Bar", centralWidget);

    // removed QObject as it was not needed, as they are calling their own functions.
    connect(chooseFileButton, &QPushButton::clicked, this, &MainWindow::openAndPlayVideoOnClick);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseOrPlayVideo);
    connect(logButton, &QPushButton::clicked, this, &MainWindow::resetBar);

    //Connect frame data (minimap icon predictions) to the minimap widget
    connect(player, &VideoPlayer::frameChanged, minimap_wid, &Minimap::redrawAgents);

    QObject::connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {player->setVolume(value / 100.0f); });
    this->setCentralWidget(centralWidget);

    controlPanel->addWidget(pauseButton);
    controlPanel->addWidget(volumeSlider);

    mainVerticalLayout->addWidget(picLabel);
    mainVerticalLayout->addWidget(player);
    mainVerticalLayout->addWidget(chooseFileButton);
    mainVerticalLayout->addLayout(controlPanel);
    mainVerticalLayout->addWidget(logButton);

    minimap_wid->hide();
    volumeSlider->hide();
    pauseButton->hide();
    logBar->hide();
    logButton->hide();
    player->hide();

    // Set up OCR detector (tessdata folder sits next to the exe)
    ocrDetector = new OCRDetector("tessdata", "eng", this);

    // Pre-load all team abbreviations from VLR.gg ranking pages so OCR can
    // recognise any pro team, not just the hardcoded fallback list.
    std::thread([this]() {
        std::vector<std::string> abbrevs = WebScraper::scrapeAllTeamAbbreviations();
        if (!abbrevs.empty()) {
            QMetaObject::invokeMethod(ocrDetector, [this, abbrevs]() {
                ocrDetector->setTeamNames(abbrevs);
            }, Qt::QueuedConnection);
            QMetaObject::invokeMethod(logBar, [this, abbrevs]() {
                logBar->addLog(QString("Loaded %1 team names from VLR.gg").arg(abbrevs.size()));
            }, Qt::QueuedConnection);
        }
    }).detach();
    connect(ocrDetector, &OCRDetector::ocrLog,
            logBar, &LogBar::addLog);
    connect(ocrDetector, &OCRDetector::teamsDetected,
            this, &MainWindow::onTeamsDetected);
    connect(ocrDetector, &OCRDetector::killDetected,
            logBar, &LogBar::addLog);
    connect(ocrDetector, &OCRDetector::scoresChanged,
            this, &MainWindow::onScoresChanged);

    // Sync OCR to video playback state and position
    connect(player, &VideoPlayer::videoPlaying,
            ocrDetector, &OCRDetector::onVideoPlaying);
    connect(player, &VideoPlayer::videoPaused,
            ocrDetector, &OCRDetector::onVideoPaused);
    connect(player, &VideoPlayer::videoPositionChanged,
            ocrDetector, &OCRDetector::onVideoPositionChanged);

    this->show();
}

void MainWindow::onTeamsDetected(QString left, QString right)
{
    logBar->addLog("Scraping roster for " + left + " vs " + right + "...");

    std::string a = left.toStdString();
    std::string b = right.toStdString();
    std::thread([this, a, b]() {
        bool ok = WebScraper::prepareForMatch(a, b);
        if (!ok) return;

        // Build name->team pairs for OCRDetector
        std::vector<std::pair<std::string,std::string>> records;
        for (const auto& p : WebScraper::getPlayers())
            records.emplace_back(p.name, p.team);

        QMetaObject::invokeMethod(ocrDetector, [this, records]() {
            ocrDetector->setPlayerRecords(records);
        }, Qt::QueuedConnection);

        QMetaObject::invokeMethod(logBar, [this, records]() {
            logBar->addLog(QString("Roster loaded: %1 players").arg(records.size()));
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onScoresChanged(QString leftTeam, int leftScore, QString rightTeam, int rightScore)
{
    logBar->addLog(QString("[Score] %1 %2 - %3 %4")
                   .arg(leftTeam).arg(leftScore)
                   .arg(rightScore).arg(rightTeam));
}

void MainWindow::openAndPlayVideoOnClick() {
    QString QSource = QFileDialog::getOpenFileName(this, tr("Open Video"), "C:", tr("*.mp4 *.mp3"));
    QFileInfo fileInfo(QSource);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        logBar->addLog("Invalid file selected.");
        return;
    }
    std::string source = QSource.toStdString();

    // Detect the map and load the correct layout image
    QString layoutPath = detectMapFromVideo(source);
    if (!layoutPath.isEmpty()) {
        minimap_wid->loadImage(layoutPath);
        logBar->addLog("Map detected: " + layoutPath);
    }
    else {
        logBar->addLog("Map not detected.");
    }

    chooseFileButton->hide();
    picLabel->hide();
    volumeSlider->show();
    minimap_wid->show();
    pauseButton->show();
    logBar->show();
    logButton->show();
    player->show();
    player->displayVideo(source);
    ocrDetector->startVideo(source);
    logBar->addLog("Video Loaded.");
}

void MainWindow::pauseOrPlayVideo() {
    player->pauseOrPlayVideo();
}

void MainWindow::resetBar() {
    logBar->resetBar();
}
