#include <QtWidgets>

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>

#include <string>
#include <iostream>
#include "../headers/videoplayer.h"

// Takes in a pointer to the parent object
VideoPlayer::VideoPlayer(QWidget *parentAddress) : 
	QWidget(parentAddress)
{
	player = new QMediaPlayer(this);
	videoWidget = new QVideoWidget(this);
	audioOutput = new QAudioOutput(this);

	player->setVideoOutput(videoWidget);
	player->setAudioOutput(audioOutput);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(videoWidget);

}

//Takes in the url to a video and displays it
void VideoPlayer::displayVideo(std::string source) {

	//Convert our string into a QString (which Qt can use)
	QString QSource = QString::fromStdString(source);
	player->setSource(QUrl::fromLocalFile(QSource));

	player->play();
}

void VideoPlayer::pauseOrPlayVideo() {
	if (player->playbackState() == QMediaPlayer::PlaybackState::PausedState) {
		player->play();
	}
	else {
		player->pause();
	}
}