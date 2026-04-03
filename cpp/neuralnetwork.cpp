#include "../headers/neuralnetwork.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>


#include <onnxruntime_cxx_api.h>

NeuralNetwork::NeuralNetwork(const T_CHAR *onnxFilePath, const int ROWS, const int COLS) {

    this->ROWS = ROWS;
    this->COLS = COLS;

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

std::vector<float> NeuralNetwork::prepareImage(const cv::Mat &imageBGR) {
    cv::Mat imageBlob = cv::dnn::blobFromImage(imageBGR,
        1.0 / 255.0, //Standardize images 
        cv::Size(COLS, ROWS),
        cv::Scalar(),
        true, //Convert from BGR to RGB (which is what the model uses)
        false);

    //Turn the image into a vector of floats
    std::vector<float> imageVector(imageBlob.total());
    std::memcpy(imageVector.data(), imageBlob.ptr<float>(), imageBlob.total() * sizeof(float));

    return imageVector;
}

std::vector<Ort::Value> NeuralNetwork::predict(cv::Mat imageBGR) {

    size_t input_tensor_size = 1 * 3 * ROWS * COLS;
    //std::vector<float> input_tensor_values(input_tensor_size);
    //for (unsigned int i = 0; i < input_tensor_size; i++) input_tensor_values[i] = (float)i / (input_tensor_size + 1);

    auto type_info = model->GetInputTypeInfo(0);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> input_node_dims = tensor_info.GetShape();
    for (auto d : input_node_dims)
        std::cout << d << " ";

    std::vector<float> imageVector = prepareImage(imageBGR);

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, imageVector.data(), input_tensor_size, input_node_dims.data(), 4);

    std::vector<Ort::Value> outputTensor;

    try {
        std::cout << "Starting inference" << std::endl;
        outputTensor = model->Run(Ort::RunOptions{ nullptr },
            inputNodeNamesCStyle.data(),
            &input_tensor,
            inputNodeNamesCStyle.size(),
            outputNodeNamesCStyle.data(),
            outputNodeNamesCStyle.size());
        std::cout << "Inference over" << std::endl;
        std::cout << "Output vector names:" << std::endl;
        for (auto name : outputNodeNames) {
            std::cout << name << std::endl;
        }


    }
    catch (const Ort::Exception &e) {
        std::cerr << "ONNX Runtime error during forward pass: " << e.what() << std::endl;
        throw(e);
    }

    return outputTensor;

   

   // return outputTensor;


}

void NeuralNetwork::outputImageWithBoxesAndLabels(const cv::Mat image, const float *boundingBoxes, const int64_t *labels, const float *scores, const int N, const float threshold) {
    
    //Resize the image to what's used internally
    cv::Mat resizedImg;
    cv::resize(image, resizedImg, cv::Size(COLS, ROWS));

    for (int i = 0; i < N; i++) {
        if (scores[i] < threshold) { continue; }

        int xmin = boundingBoxes[4 * i];
        int ymin = boundingBoxes[4 * i + 1];
        int xmax = boundingBoxes[4 * i + 2];
        int ymax = boundingBoxes[4 * i + 3];

        cv::String text = std::to_string(labels[i]) + ": " + std::to_string((int) (scores[i] * 100)) + "%";

        cv::putText(resizedImg, text, cv::Point(xmin, ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
        cv::putText(resizedImg, text, cv::Point(xmin, ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

        cv::rectangle(resizedImg, cv::Rect(xmin, ymin, xmax - xmin, ymax-ymin), cv::Scalar(0, 0, 255), 2);

    }
    cv::imwrite("image_labeled.png", resizedImg);
}