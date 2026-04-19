#include "modelwrapper.h"

ModelWrapper::ModelWrapper() {
	// Model is loaded in startProcessingLoop to avoid crashing on startup
	// if the working directory is not set correctly.
}

ModelWrapper::~ModelWrapper() {
	delete model;
	model = nullptr;

	endProcessingLoop();
}

void ModelWrapper::loop(std::string fileName) {

	const float LEFT_SIDE = 0.024;
	const float RIGHT_SIDE = 0.24;
	const float TOP_SIDE = 0.04;
	const float BOTTOM_SIDE = 0.43;

	std::cout << "Starting frame processing" << fileName << std::endl;

	cv::VideoCapture cap(fileName);
	cv::Mat frame;

	int frameID = 0;
	while (keepThreadAlive) {

		if (!cap.read(frame)) break;

		if (frameID % EVERY_N_FRAMES != 0) {
			frameID++;
			continue;
		}

		cv::Size s = frame.size();
		int height = s.height, width = s.width;

		cv::Rect ROI(LEFT_SIDE * width, TOP_SIDE * height, (RIGHT_SIDE - LEFT_SIDE) * width, (BOTTOM_SIDE - TOP_SIDE) * height);
		cv::Mat croppedFrame = frame(ROI);


		model->predict(croppedFrame);
		Eigen::MatrixXf results = model->getCurrentTracking();
		//model->outputTrackingImageWithBoxesAndLabels(croppedFrame, results, "frames/" + std::to_string(frameID) + ".png");
		//model->outputClassificationImageWithBoxesAndLabels(croppedFrame, results, 0.6, "frames/" + std::to_string(frameID) + ".png");

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

void ModelWrapper::startProcessingLoop(const std::string fileName, const int RUN_EVERY_N_FRAMES, const int FPS) {

	EVERY_N_FRAMES = RUN_EVERY_N_FRAMES;

	if (model == nullptr) { //TODO: Change this back to being done in the constructor. I don't know why someone changed it to be here. The constructor should be responsible for initializaing the model...
		const int ROWS = 384;
		const int COLS = 384;
		try {
			model = new NeuralNetwork(T_TEXT("faster_rcnn.onnx"), ROWS, COLS);
		}
		catch (const Ort::Exception& e) {
			std::cerr << "[ModelWrapper] Failed to load ONNX model: " << e.what() << std::endl;
			return;
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelWrapper] Failed to load ONNX model: " << e.what() << std::endl;
			return;
		}
	}

	model->resetTracker(FPS / RUN_EVERY_N_FRAMES);
	keepThreadAlive = true;

	data = std::vector<Eigen::MatrixXf>(); //We're processing from the start, so we empty out previous data
	loopThread = std::thread(&ModelWrapper::loop, this, fileName);
}

Eigen::MatrixXf ModelWrapper::getFrameData(const int NthFrame) {

	int indexForThisFrame = NthFrame / EVERY_N_FRAMES;

	Eigen::MatrixXf frameData;

	if (indexForThisFrame < data.size()) {

		lock.lock();
		frameData = data[indexForThisFrame];
		lock.unlock();

		return data[indexForThisFrame];
	}
	else {
		return frameData;

	}

}
