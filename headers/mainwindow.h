#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "../headers/logbar.h"
#include "../headers/videoplayer.h"
#include "../headers/minimap.h"

// This class acts as the main window for the application and acts like a normal QWidget
class MainWindow : public QMainWindow {

private:

	QWidget *centralWidget;
	QVBoxLayout *mainVerticalLayout;
	QHBoxLayout *topRow;
	QHBoxLayout *bottomRow;
	VideoPlayer *player;
	QPushButton *chooseFileButton;
	QPushButton *pauseButton;
	QPushButton *logButton;
	LogBar *logBar;
	Minimap* minimap_wid;


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