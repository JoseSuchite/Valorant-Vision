#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <string>

class VideoPlayer {

private:
	QWidget *parentPointer;

public:
	// Takes in a pointer to the parent object
	VideoPlayer(QWidget *parent);

	//Takes in the url to a video and displays its
	void displayVideo(std::string url);
};

#endif