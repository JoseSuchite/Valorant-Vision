#ifndef MODELWRAPPER_H
#define MODELWRAPPER_H

#include <thread>
#include <vector>
#include <QtWidgets>

#include "neuralnetwork.h"

class ModelWrapper {

private:

	NeuralNetwork *model = nullptr;
	const int EVERY_N_FRAMES = 30;

	std::thread loopThread;
	std::mutex lock;

	std::atomic_bool keepThreadAlive = true;

	std::vector<Eigen::MatrixXf> data;

	void loop(std::string fileName);

public:

	ModelWrapper();
	~ModelWrapper();

	void startProcessingLoop(std::string fileName);
	void endProcessingLoop();
	Eigen::MatrixXf getFrameData(const int NthFrame);

};

#endif