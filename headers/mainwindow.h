#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>

#include "../headers/videoplayer.h"

// This class acts as the main window for the application and acts like a normal QWidget
class MainWindow : public QMainWindow {

private:

	QWidget *centralWidget;
	QVBoxLayout *mainVerticalLayout;
	VideoPlayer *player;
	QPushButton *chooseFileButton;
	QPushButton *pauseButton;

	//Prompts user to select a file from the file browser and then calls the video player to play it
	void openAndPlayVideoOnClick();

	//Pauses or plays the video (depends on the state)
	void pauseOrPlayVideo();
	
public:

	MainWindow();

};

#endif