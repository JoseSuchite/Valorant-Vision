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

// This class acts as the main window for the application and acts like a normal QWidget
class MainWindow : public QMainWindow {

private:

	QWidget *centralWidget;
	QVBoxLayout *mainVerticalLayout;
	QHBoxLayout *controlPanel;
	VideoPlayer *player;
	QPushButton *chooseFileButton;
	QPushButton *pauseButton;
	QPushButton *logButton;
	LogBar *logBar;
	QSlider *volumeSlider;
	QPixmap *pixmap;
	QLabel *picLabel;


	//Prompts user to select a file from the file browser and then calls the video player to play it
	void openAndPlayVideoOnClick();

	//Pauses or plays the video (depends on the state)
	void pauseOrPlayVideo();

	//Resets the side bar to its original place
	void resetBar();

public:

	MainWindow();

};

#endif