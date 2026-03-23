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

MainWindow::MainWindow() :

    QMainWindow() {

        // adds the logo next to the title
        setWindowIcon(QIcon(":/images/logo.png"));

        centralWidget = new QWidget(this);
        logBar = new LogBar(this);
        addDockWidget(Qt::RightDockWidgetArea, logBar);
        player = new VideoPlayer(centralWidget);
        mainVerticalLayout = new QVBoxLayout(centralWidget);
        controlPanel = new QHBoxLayout(centralWidget);
        chooseFileButton = new QPushButton("Select Video", centralWidget);

        QPixmap pixmap(":/images/logo.png");
        picLabel = new QLabel();
        picLabel->setPixmap(pixmap);

        pauseButton = new QPushButton("Pause/Play", centralWidget);
        pauseButton->setStyleSheet(QString("background-color: red;"));

        volumeSlider = new QSlider(Qt::Horizontal, this);
        volumeSlider->setRange(0, 100);
        volumeSlider->setValue(100);

        logButton = new QPushButton("Reset Log Bar", centralWidget);

        QObject::connect(chooseFileButton, &QPushButton::clicked, this, &MainWindow::openAndPlayVideoOnClick);
        QObject::connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseOrPlayVideo);
        QObject::connect(logButton, &QPushButton::clicked, this, &MainWindow::resetBar);

        QObject::connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {player->setVolume(value / 100.0f);});

        this->setCentralWidget(centralWidget);

        controlPanel->addWidget(pauseButton);
        controlPanel->addWidget(volumeSlider);

        mainVerticalLayout->addWidget(picLabel);
        mainVerticalLayout->addWidget(player);
        mainVerticalLayout->addWidget(chooseFileButton);
        mainVerticalLayout->addLayout(controlPanel);
        mainVerticalLayout->addWidget(logButton);

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

    volumeSlider->show();
    chooseFileButton->hide();
    picLabel->hide();
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