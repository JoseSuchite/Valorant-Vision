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

Prediction NeuralNetwork::predict(cv::Mat imageBGR) {

    size_t input_tensor_size = 1 * 3 * ROWS * COLS;

    auto type_info = model->GetInputTypeInfo(0);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> input_node_dims = tensor_info.GetShape();
    /*for (auto d : input_node_dims)
        std::cout << d << " ";*/

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
    }
    catch (const Ort::Exception &e) {
        std::cerr << "ONNX Runtime error during forward pass: " << e.what() << std::endl;
        throw(e);
    }


    // Get bounding boxes
    float *boxesArr = outputTensor[0].GetTensorMutableData<float>();
    auto info = outputTensor[0].GetTensorTypeAndShapeInfo();
    std::cout << info.GetElementCount() << std::endl;

    // Get labels
    int64_t *labelsArr = outputTensor[1].GetTensorMutableData<int64_t>();
    size_t labelsLength = outputTensor[1].GetTensorTypeAndShapeInfo().GetElementCount();

    // Get scores
    float *scoresArr = outputTensor[2].GetTensorMutableData<float>();
    size_t scoresLength = outputTensor[2].GetTensorTypeAndShapeInfo().GetElementCount();

    Prediction prediction;
    prediction.length = 0;
    for (int i = 0; i < labelsLength; i++) { //Length of labels and scores *should* be equal, so I just arbitrarily picked one
        prediction.length++;

        //Each bounding box is stored as 4 points, so add them
        Box box;
        box.xmin = boxesArr[4 * i];
        box.ymin = boxesArr[4 * i + 1];
        box.xmax = boxesArr[4 * i + 2];
        box.ymax = boxesArr[4 * i + 3];
        prediction.boundingBoxes.push_back(box);

        //Add the label and score
        prediction.labels.push_back(labelsArr[i]);
        prediction.scores.push_back(scoresArr[i]);
    }

    return prediction;

}

void NeuralNetwork::outputImageWithBoxesAndLabels(const cv::Mat image, const Prediction prediction, const float threshold) {
    
    //Resize the image to what's used internally
    cv::Mat resizedImg;
    cv::resize(image, resizedImg, cv::Size(COLS, ROWS));

    for (int i = 0; i < prediction.length; i++) {
        if (prediction.scores[i] < threshold) { continue; }

        Box box = prediction.boundingBoxes[i];

        cv::String text = std::to_string(prediction.labels[i]) + ": " + std::to_string((int) (prediction.scores[i] * 100)) + "%";

        cv::putText(resizedImg, text, cv::Point(box.xmin, box.ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
        cv::putText(resizedImg, text, cv::Point(box.xmin, box.ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

        cv::rectangle(resizedImg, cv::Rect(box.xmin, box.ymin, box.xmax - box.xmin, box.ymax - box.ymin), cv::Scalar(0, 0, 255), 2);

    }
    cv::imwrite("image_labeled.png", resizedImg);
}