#include "modelwrapper.h"

ModelWrapper::ModelWrapper() {

	const int ROWS = 384;
	const int COLS = 384;
	model = new NeuralNetwork(T_TEXT("faster_rcnn.onnx"), ROWS, COLS);

}

ModelWrapper::~ModelWrapper() {
	delete model;
	model = nullptr;

	endProcessingLoop();
}

void ModelWrapper::loop(std::string fileName) {

	std::cout << "Starting processing" << fileName << std::endl;

	cv::VideoCapture cap(fileName);
	cv::Mat frame;

	int frameID = 0;
	while (keepThreadAlive) {
		if (!cap.read(frame)) break;

		if (frameID % EVERY_N_FRAMES != 0) {
			frameID++;
			continue;
		}

		model->predict(frame);
		Eigen::MatrixXf results = model->getCurrentTracking();
		model->outputTrackingImageWithBoxesAndLabels(frame, results, "frames/" + std::to_string(frameID) + ".png");

		lock.lock();
		
		data.push_back(results);

		lock.unlock();

		frameID++;
	}
}

void ModelWrapper::endProcessingLoop() {
	if (loopThread.joinable()) {
		keepThreadAlive = false;
		loopThread.join();
	}
}

void ModelWrapper::startProcessingLoop(std::string fileName) {
	keepThreadAlive = true;
	data = std::vector<Eigen::MatrixXf>();
	loopThread = std::thread(&ModelWrapper::loop, this, fileName);
}

Eigen::MatrixXf ModelWrapper::getFrameData(const int NthFrame) {

	Eigen::MatrixXf frameData;

	if (NthFrame < data.size()) {
		lock.lock();
		frameData = data[NthFrame];
		lock.unlock();
		return data[NthFrame];
	}
	else {
		return frameData;

	}

}
