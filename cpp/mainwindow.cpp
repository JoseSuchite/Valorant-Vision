#include <string>

#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QObject>

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
        chooseFileButton = new QPushButton("Select Video", centralWidget);
        pauseButton = new QPushButton("Pause/Play", centralWidget);
        logButton = new QPushButton("Reset Log Bar", centralWidget);

        QObject::connect(chooseFileButton, &QPushButton::clicked, this, &MainWindow::openAndPlayVideoOnClick);
        QObject::connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseOrPlayVideo);
        QObject::connect(logButton, &QPushButton::clicked, this, &MainWindow::resetBar);

        this->setCentralWidget(centralWidget);

        mainVerticalLayout->addWidget(player);
        mainVerticalLayout->addWidget(chooseFileButton);
        mainVerticalLayout->addWidget(pauseButton);
        mainVerticalLayout->addWidget(logButton);


        player->hide();
        this->show();
}

void MainWindow::openAndPlayVideoOnClick() {

    QString QSource = QFileDialog::getOpenFileName(this, tr("Open Video"), "C:", tr("*.mp4 *.mp3"));
    std::string source = QSource.toStdString();

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