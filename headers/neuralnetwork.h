#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include <filesystem>
#include <onnxruntime_cxx_api.h>
#include <string>

//Make sure we use the right type of char, because ONNX on Windows has to be unique and use wchar_t...
//Example usage: const T_CHAR *str = T_TEXT("Chikorita is the best pokemon.")
#ifdef _WIN32
typedef wchar_t T_CHAR;
#define T_TEXT(quote) L##quote
#else
typedef char T_CHAR;
#define T_TEXT(quote) quote
#endif

class NeuralNetwork {

private:

	Ort::Env *env;
	Ort::SessionOptions sessionOptions;
	Ort::Session *model; // Has to be dynamically created because we can't create it with no parameters

	//Storing an std::string version and a c-style version separaretly
	//We need c-style to pass into the model but that gets really annoying and yes I know this is ineffecient but like, it's like 4 extra strings
	//The model has over 40 million parameters
	//I think we will live
	std::vector<std::string> inputNodeNames;
	std::vector<const char *> inputNodeNamesCStyle;
	std::vector<std::string> outputNodeNames;
	std::vector<const char *> outputNodeNamesCStyle;

public:

	NeuralNetwork(const T_CHAR *onnxFilePath);
	~NeuralNetwork();
	void forwardPass();

};

#endif