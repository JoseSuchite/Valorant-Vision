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

MainWindow::MainWindow() :
    QMainWindow() {
    // adds the logo next to the title
    setWindowIcon(QIcon(":/images/logo.png"));

    centralWidget = new QWidget(this);
    mainVerticalLayout = new QVBoxLayout(centralWidget);

    logBar = new LogBar(this);

    addDockWidget(Qt::RightDockWidgetArea, logBar);
    minimap_wid = new Minimap(this);

    addDockWidget(Qt::RightDockWidgetArea, minimap_wid);
    minimap_wid->loadImage("map_layouts/Ascent_layout.png");

    player = new VideoPlayer(centralWidget);
    controlPanel = new QHBoxLayout(centralWidget);
    chooseFileButton = new QPushButton("Select Video", centralWidget);

    QPixmap pixmap(":/images/logo.png");
    picLabel = new QLabel();
    picLabel->setPixmap(pixmap);

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

    this->show();
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
    logBar->addLog("Video Loaded.");
}

void MainWindow::pauseOrPlayVideo() {
    player->pauseOrPlayVideo();
}

void MainWindow::resetBar() {
    logBar->resetBar();
}