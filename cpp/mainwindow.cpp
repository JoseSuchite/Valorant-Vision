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
#include "../headers/minimap.h"

MainWindow::MainWindow() :

    QMainWindow() {

        // adds the logo next to the title
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

        // removed QObject as it was not needed, as they are calling their own functions.
        connect(chooseFileButton, &QPushButton::clicked, this, &MainWindow::openAndPlayVideoOnClick);
        connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseOrPlayVideo);
        connect(logButton, &QPushButton::clicked, this, &MainWindow::resetBar);

        this->setCentralWidget(centralWidget);

        // add the buttons to their respective layout like top or bottom
        topRow->addWidget(chooseFileButton);
        topRow->addWidget(logButton);
        bottomRow->addWidget(pauseButton);

        // format the layout of the buttons and player
        mainVerticalLayout->addLayout(topRow);
        mainVerticalLayout->addWidget(player);
        mainVerticalLayout->addLayout(bottomRow);

        // resizes the select video button
        chooseFileButton->setFixedSize(120, 40);
        chooseFileButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // resizes the reset log button
        logButton->setFixedSize(120, 40);
        logButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        //resizes the pause/play button
        pauseButton->setFixedSize(120, 40);
        pauseButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        //player->hide();
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