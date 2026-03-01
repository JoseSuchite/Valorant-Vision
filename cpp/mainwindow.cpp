#include <string>

#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QObject>


#include "../headers/mainwindow.h"
#include "../headers/videoplayer.h"

MainWindow::MainWindow() :

    QMainWindow() {

        // adds the logo next to the title
        setWindowIcon(QIcon(":/images/logo.png"));

        centralWidget = new QWidget(this);
        player = new VideoPlayer(centralWidget);
        mainVerticalLayout = new QVBoxLayout(centralWidget);
        chooseFileButton = new QPushButton("CLICK MEEEE", centralWidget);
        pauseButton = new QPushButton("Pause/Play", centralWidget);

        QObject::connect(chooseFileButton, &QPushButton::clicked, this, &MainWindow::openAndPlayVideoOnClick);
        QObject::connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseOrPlayVideo);

        this->setCentralWidget(centralWidget);

        mainVerticalLayout->addWidget(player);
        mainVerticalLayout->addWidget(chooseFileButton);
        mainVerticalLayout->addWidget(pauseButton);


        player->hide();
        this->show();
}

void MainWindow::openAndPlayVideoOnClick() {

    QString QSource = QFileDialog::getOpenFileName(this, tr("Open Video"), "C:", tr("*.mp4 *.mp3"));
    std::string source = QSource.toStdString();

    player->show();
    player->displayVideo(source);
}

void MainWindow::pauseOrPlayVideo() {
    player->pauseOrPlayVideo();
}