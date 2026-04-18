#include <QtWidgets>

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QMediaMetadata>

#include <opencv2/opencv.hpp>

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

	connect(player, &QMediaPlayer::playbackStateChanged, this,
		[this](QMediaPlayer::PlaybackState state) {
			if (state == QMediaPlayer::PlayingState)
				emit videoPlaying(player->position());
			else if (state == QMediaPlayer::PausedState)
				emit videoPaused();
		});
}

qint64 VideoPlayer::currentPositionMs() const
{
	return player->position();
}

//Takes in the url to a video and displays it
void VideoPlayer::displayVideo(std::string source) {

	//Store what the video's FPS is
	//Using openCV to do it manually because Qt just gives the wrong FPS (it said a 60 fps video was 17 fps ????)
	cv::VideoCapture video(source);
	currentVideoFPS = video.get(cv::CAP_PROP_FPS);
	video.release();
	
	//End the model's last processing loop if it had one
	modelWrapper.endProcessingLoop();
	//Let the model start another thread for processing the video's frames
	int RUN_EVERY_N_FRAMES = currentVideoFPS / 6; //Run every third of a second
	while (RUN_EVERY_N_FRAMES % 5 != 0) { RUN_EVERY_N_FRAMES++; } //Round up to the nearest integer divisible by 5 (it personally would annoy be if it wasn't)

	modelWrapper.startProcessingLoop(source, RUN_EVERY_N_FRAMES, currentVideoFPS);

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
	emit videoPositionChanged(NthMillisecond);

	//Get frame rate of the video to convert milliseconds to frames
	qint64 frameNum = (NthMillisecond / 1000.0) * currentVideoFPS;
	
	Eigen::MatrixXf results = modelWrapper.getFrameData(frameNum);

	emit frameChanged(results);
}

