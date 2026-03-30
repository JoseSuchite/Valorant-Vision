#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "../headers/logbar.h"
#include "../headers/videoplayer.h"
#include "../headers/minimap.h"

class MainWindow : public QMainWindow {

private:
    QWidget* centralWidget;
    QVBoxLayout* mainVerticalLayout;
    QHBoxLayout* topRow;
    QHBoxLayout* bottomRow;
    VideoPlayer* player;
    QPushButton* chooseFileButton;
    QPushButton* pauseButton;
    QPushButton* logButton;
    LogBar* logBar;
    Minimap* minimap_wid;

    void openAndPlayVideoOnClick();
    void pauseOrPlayVideo();
    void resetBar();

public:
    MainWindow();
};

#endif