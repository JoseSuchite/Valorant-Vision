#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include <filesystem>
#include <string>

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

//Make sure we use the right type of char, because ONNX on Windows has to be unique and use wchar_t...
//Example usage: const T_CHAR *str = T_TEXT("Chikorita is the best pokemon.")
#ifdef _WIN32
typedef wchar_t T_CHAR;
#define T_TEXT(quote) L##quote
#else
typedef char T_CHAR;
#define T_TEXT(quote) quote
#endif

struct Box {
	int xmin;
	int ymin;
	int xmax;
	int ymax;
};

struct Prediction {
	size_t length;
	std::vector<Box> boundingBoxes;
	std::vector<int64_t> labels;
	std::vector<float> scores;
};

class NeuralNetwork {

private:

	Ort::Env *env;
	Ort::SessionOptions sessionOptions;
	Ort::Session *model; // Has to be dynamically created because we can't create it with no parameters
	int ROWS; //Height of images to use
	int COLS; //Width of images to use

	//Storing an std::string version and a c-style version separaretly
	//We need c-style to pass into the model but that gets really annoying and yes I know this is ineffecient but like, it's like 4 extra strings
	//The model has over 40 million parameters
	//I think we will live
	std::vector<std::string> inputNodeNames;
	std::vector<const char *> inputNodeNamesCStyle;
	std::vector<std::string> outputNodeNames;
	std::vector<const char *> outputNodeNamesCStyle;

	//Takes in a reference to an opencv image file, does needed preprocessing, and returns it as a vector of floats
	std::vector<float> prepareImage(const cv::Mat &);

public:

	NeuralNetwork(const T_CHAR *onnxFilePath, const int ROWS, const int COLS);
	~NeuralNetwork();

	//Takes in an image in opencv BGR format and returns prediction on it
	Prediction predict(cv::Mat imageBGR);

	//Outputs an image file with predictions drawn on it. Takes in the image, arrays of bounding boxes, labels, and scores, the amount of predictions (N), and a threshold for scores to show up
	void outputImageWithBoxesAndLabels(const cv::Mat image, const Prediction prediction, const float threshold);

};

#endif