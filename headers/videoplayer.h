#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <string>
#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QSlider>
#include <QtGlobal>
#include <QMetaObject>

#include "modelwrapper.h"

class VideoPlayer : public QWidget {

	Q_OBJECT

private:

	QMediaPlayer *player;
	QVideoWidget *videoWidget;
	QAudioOutput *audioOutput;
	QSlider *slider;

	ModelWrapper modelWrapper;

public:
	// Takes in a pointer to the parent object
	VideoPlayer(QWidget *parent);

	//Takes in the url to a video and displays its
	void displayVideo(std::string url);

	//Pauses or plays video (depends on state)
	void pauseOrPlayVideo();

	//Sets volume for the player
	void setVolume(float volume);


	void onFrameChange(qint64 NthMillisecond);

signals:
	void frameChanged(Eigen::MatrixXf frameData);
};

#endif