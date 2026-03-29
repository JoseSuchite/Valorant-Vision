#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <string>
#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QSlider>

class VideoPlayer : public QWidget {

private:
	QMediaPlayer *player;
	QVideoWidget *videoWidget;
	QAudioOutput *audioOutput;
	QSlider *slider;

public:
	// Takes in a pointer to the parent object
	VideoPlayer(QWidget *parent);

	//Takes in the url to a video and displays its
	void displayVideo(std::string url);

	//Pauses or plays video (depends on state)
	void pauseOrPlayVideo();
};

#endif