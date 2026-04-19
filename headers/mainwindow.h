#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QPixmap>
#include <QLabel>
#include "../headers/logbar.h"
#include "../headers/videoplayer.h"
#include "../headers/minimap.h"
#include "../headers/ocr.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QWidget* centralWidget;
    QVBoxLayout* mainVerticalLayout;
    QHBoxLayout* controlPanel;
    VideoPlayer* player;
    QPushButton* chooseFileButton;
    QPushButton* pauseButton;
    QPushButton* logButton;
    LogBar* logBar;
    Minimap* minimap_wid;
    QSlider* volumeSlider;
    QPixmap* pixmap;
    QLabel* picLabel;
    OCRDetector* ocrDetector;

    void openAndPlayVideoOnClick();
<<<<<<< HEAD
    //Pauses or plays the video (depends on the state)
    void pauseOrPlayVideo();
    //Resets the side bar to its original place
=======
    void pauseOrPlayVideo();
>>>>>>> 9a4bdb20a0f49958b16e022deb754a6b7435a051
    void resetBar();

private slots:
    void onTeamsDetected(QString left, QString right);
    void onScoresChanged(QString leftTeam, int leftScore, QString rightTeam, int rightScore);

public:
    MainWindow();
};
#endif
