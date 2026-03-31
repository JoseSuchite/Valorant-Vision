#include "../headers/neuralnetwork.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <onnxruntime_cxx_api.h>


NeuralNetwork::NeuralNetwork(const T_CHAR *onnxFilePath) {

    //Set up environment and session options
	env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "Valorant-Vision Model");
	sessionOptions.SetIntraOpNumThreads(1);
	sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    //Create session
    try {
        model = new Ort::Session(*env, onnxFilePath, sessionOptions);
    }
    catch (Ort::Exception &e) {
        std::cerr << "ONNX Runtime error while creating model: " << e.what() << std::endl;
        std::exit(-1);
    }

    //Get and store input names for the model
    size_t numInputNodes = model->GetInputCount();
    std::vector<Ort::ValueInfo> inputs = model->GetInputs();

    for (int i = 0; i < numInputNodes; i++) {
        inputNodeNames.push_back(inputs[i].GetName());
    }
    for (auto &name : inputNodeNames) {
        inputNodeNamesCStyle.push_back(name.c_str());
    }

    //Get and store output names for the model
    std::vector<Ort::ValueInfo> outputs = model->GetOutputs();
    size_t numOutputNodes = model->GetOutputCount();

    for (int i = 0; i < numOutputNodes; i++) {
        outputNodeNames.push_back(outputs[i].GetName());
    }
    for (auto &name : outputNodeNames) {
        outputNodeNamesCStyle.push_back(name.c_str());
    }

	
}

NeuralNetwork::~NeuralNetwork() {
	delete model;
	model = nullptr;
	delete env;
	env = nullptr;
}

void NeuralNetwork::forwardPass() {

    size_t input_tensor_size = 3 * 384 * 384;
    std::vector<float> input_tensor_values(input_tensor_size);
    for (unsigned int i = 0; i < input_tensor_size; i++) input_tensor_values[i] = (float)i / (input_tensor_size + 1);

    auto type_info = model->GetInputTypeInfo(0);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> input_node_dims = tensor_info.GetShape();

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_size, input_node_dims.data(), 4);

    std::vector<Ort::Value> outputTensor;

    try {
        std::cout << "Starting inference" << std::endl;
        outputTensor = model->Run(Ort::RunOptions{ nullptr },
            inputNodeNamesCStyle.data(),
            &input_tensor,
            1,
            outputNodeNamesCStyle.data(),
            1);
        std::cout << "Inference over" << std::endl;
    }
    catch (const Ort::Exception &e) {
        std::cerr << "ONNX Runtime error during forward pass: " << e.what() << std::endl;
    }
}