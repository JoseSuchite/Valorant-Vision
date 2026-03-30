#include <string>
#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>

#include "../headers/logbar.h"
#include "../headers/mainwindow.h"
#include "../headers/videoplayer.h"
#include "../headers/minimap.h"
#include "../headers/mapdetector.h"

MainWindow::MainWindow() :
    QMainWindow() {

    setWindowIcon(QIcon(":/images/logo.png"));

    centralWidget = new QWidget(this);
    topRow = new QHBoxLayout();
    mainVerticalLayout = new QVBoxLayout(centralWidget);
    bottomRow = new QHBoxLayout();

    logBar = new LogBar(this);
    addDockWidget(Qt::RightDockWidgetArea, logBar);
    minimap_wid = new Minimap(this);
    addDockWidget(Qt::RightDockWidgetArea, minimap_wid);
    minimap_wid->loadImage("map_layouts/Ascent_layout.png");

    player = new VideoPlayer(centralWidget);
    chooseFileButton = new QPushButton("Select Video", centralWidget);
    pauseButton = new QPushButton("Pause/Play", centralWidget);
    logButton = new QPushButton("Reset Log Bar", centralWidget);

    connect(chooseFileButton, &QPushButton::clicked, this, &MainWindow::openAndPlayVideoOnClick);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseOrPlayVideo);
    connect(logButton, &QPushButton::clicked, this, &MainWindow::resetBar);

    this->setCentralWidget(centralWidget);

    topRow->addWidget(chooseFileButton);
    topRow->addWidget(logButton);
    bottomRow->addWidget(pauseButton);

    mainVerticalLayout->addLayout(topRow);
    mainVerticalLayout->addWidget(player);
    mainVerticalLayout->addLayout(bottomRow);

    chooseFileButton->setFixedSize(120, 40);
    chooseFileButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    logButton->setFixedSize(120, 40);
    logButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pauseButton->setFixedSize(120, 40);
    pauseButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    this->show();
}

void MainWindow::openAndPlayVideoOnClick() {

    QString QSource = QFileDialog::getOpenFileName(
        this, tr("Open Video"), "C:", tr("*.mp4 *.mp3"));
    if (QSource.isEmpty()) return;
    std::string source = QSource.toStdString();

    QString layoutPath = detectMapFromVideo(source);
    if (!layoutPath.isEmpty()) {
        minimap_wid->loadImage(layoutPath);
        logBar->addLog("Map detected: " + layoutPath);
    }
    else {
        logBar->addLog("Map not detected.");
    }

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