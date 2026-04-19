#ifndef MODELWRAPPER_H
#define MODELWRAPPER_H

#include <thread>
#include <vector>
#include <QtWidgets>

#include "neuralnetwork.h"

class ModelWrapper {

private:

	NeuralNetwork *model = nullptr;
	int EVERY_N_FRAMES;

	// Thread for processing video data and a mutex for accessing the 'data' variable below
	std::thread loopThread;
	std::mutex lock;

	// Tracks whether or not the processing thread should be running
	std::atomic_bool keepThreadAlive;

	// Holds all of the data from processed frames
	// Note: we only store every N frames of data, so data[0] is frame 0, data[1] is frame N, data[2] is 2N, etc, etc
	// Should likely be changed in the future in a way that we can easily use with a file to store it
	std::vector<Eigen::MatrixXf> data;

	// Loop function that our thread uses
	void loop(std::string fileName);

public:

	ModelWrapper();
	~ModelWrapper();

	void endProcessingLoop(); // Ends the processing loop if it's running, if not does nothing
	void startProcessingLoop(std::string fileName, const int RUN_EVERY_N_FRAMES, const int FPS); // Takes in the file name that we want to process, how often we want to run the model, video FPS,and creates the thread for processing it
	// Returns the data corresponding to the Nth frame of the video
	// Note: pass in the actual frame in the video you want. The method will handle it's actually stored
	Eigen::MatrixXf getFrameData(const int NthFrame); 

};

#endif