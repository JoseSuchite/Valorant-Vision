#include <QtWidgets>

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QMediaMetadata>

#include <string>
#include <iostream>

#include "videoplayer.h"
#include "modelwrapper.h"

// Takes in a pointer to the parent object
VideoPlayer::VideoPlayer(QWidget *parentAddress) : 
	QWidget(parentAddress)
{
	player = new QMediaPlayer(this);
	videoWidget = new QVideoWidget(this);
	audioOutput = new QAudioOutput(this);
	slider = new QSlider(Qt::Horizontal, this);

	player->setVideoOutput(videoWidget);
	player->setAudioOutput(audioOutput);

	connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
	connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
	connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(videoWidget);
	layout->addWidget(slider);

	connect(player, &QMediaPlayer::positionChanged, this, &VideoPlayer::onFrameChange);
}

//Takes in the url to a video and displays it
void VideoPlayer::displayVideo(std::string source) {

	//End the model's last processing loop if it had one
	modelWrapper.endProcessingLoop();
	//Let the model start another thread for processing the video's frames
	modelWrapper.startProcessingLoop(source);

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

void VideoPlayer::setVolume(float volume) {
    audioOutput->setVolume(volume);
}

void VideoPlayer::onFrameChange(qint64 NthMillisecond) {

	//Get frame rate of the video to convert milliseconds to frames
	QMediaMetaData metaData = player->metaData();
	int FPS = metaData.VideoFrameRate;
	qint64 frameNum = (NthMillisecond / 1000.0) * FPS;
	
	Eigen::MatrixXf results = modelWrapper.getFrameData(frameNum);

	emit frameChanged(results);
}

