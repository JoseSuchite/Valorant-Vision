#include <QtWidgets>

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>

#include <string>
#include <iostream>
#include "../headers/videoplayer.h"

// Takes in a pointer to the parent object
VideoPlayer::VideoPlayer(QWidget *parentAddress) {
	this->parentPointer = parentAddress;
}

//Takes in the url to a video and displays its
void VideoPlayer::displayVideo(std::string source) {

	//Convert our string into a QString (which Qt can use)
	QString QSource = QString::fromStdString(source);

	QMediaPlayer *player = new QMediaPlayer(this->parentPointer);
	player->setSource(QUrl::fromLocalFile(QSource));

	QVideoWidget *videoWidget = new QVideoWidget(this->parentPointer);
	player->setVideoOutput(videoWidget);

	QAudioOutput *audioOutput = new QAudioOutput(this->parentPointer);
	player->setAudioOutput(audioOutput);

	videoWidget->show();
	player->play();
}