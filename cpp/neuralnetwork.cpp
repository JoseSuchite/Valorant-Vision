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
        throw Ort::Exception("ONNX Runtime error while creating model: " + e.what());
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
    //std::string reidWeightsName = "E:/My Program Files/git/repos/Valorant-Vision_old/build/_deps/motcpp-src/scripts/models/osnet_x1_0_dukemtmcreid.onnx";

    tracker = new motcpp::trackers::ByteTrack(false,
        false,
        0.4f,
        30,
        50,
        3,
        0.8f);
}

NeuralNetwork::~NeuralNetwork() {
	delete model;
	model = nullptr;
	delete env;
	env = nullptr;
    delete tracker;
    tracker = nullptr;
}

Eigen::MatrixXf NeuralNetwork::getCurrentPrediction() const {
    return currentPrediction;
}

Eigen::MatrixXf NeuralNetwork::getCurrentTracking() const {
    return currentTracking;
}

//Takes in an image, standardizes it, and returns it as an array of pixel values
std::vector<float> NeuralNetwork::prepareImage(const cv::Mat &imageBGR) {
    cv::Mat imageBlob = cv::dnn::blobFromImage(imageBGR,
        1.0 / 255.0, //Standardize pixel values
        cv::Size(COLS, ROWS), //Standardize image size
        cv::Scalar(),
        true, //Convert from BGR to RGB (which is what the model uses)
        false);

    //Turn the image into a vector of floats
    std::vector<float> imageVector(imageBlob.total());
    std::memcpy(imageVector.data(), imageBlob.ptr<float>(), imageBlob.total() * sizeof(float));

    return imageVector;
}

void NeuralNetwork::predict(cv::Mat imageBGR) {

    size_t input_tensor_size = 1 * 3 * ROWS * COLS;

    auto type_info = model->GetInputTypeInfo(0);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> input_node_dims = tensor_info.GetShape();

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

    currentPrediction = Eigen::MatrixXf(labelsLength, 6);
    for (int i = 0; i < labelsLength; i++) {
        currentPrediction(i, 0) = boxesArr[4 * i];
        currentPrediction(i, 1) = boxesArr[4 * i + 1];
        currentPrediction(i, 2) = boxesArr[4 * i + 2];
        currentPrediction(i, 3) = boxesArr[4 * i + 3];
        currentPrediction(i, 4) = scoresArr[i];
        currentPrediction(i, 5) = labelsArr[i];
    }

    currentTracking = tracker->update(currentPrediction, imageBGR);
}


void NeuralNetwork::outputClassificationImageWithBoxesAndLabels(const cv::Mat image, const Eigen::MatrixXf prediction, const float threshold, std::string outFileName) {

    //Resize the image to what's used internally
    cv::Mat resizedImg;
    cv::resize(image, resizedImg, cv::Size(COLS, ROWS));

    for (int i = 0; i < prediction.rows(); i++) {
        if (prediction(i, 4) < threshold) { continue; }

        int xmin = prediction(i, 0);
        int ymin = prediction(i, 1);
        int xmax = prediction(i, 2);
        int ymax = prediction(i, 3);

        cv::String text = std::to_string(prediction(i, 5)) + ": " + std::to_string((int)(prediction(i, 4) * 100)) + "%";

        //Put label text on screen (the first one is to give an outline to the text (it is really hard to read otherwise))
        cv::putText(resizedImg, text, cv::Point(xmin, ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
        cv::putText(resizedImg, text, cv::Point(xmin, ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

        cv::rectangle(resizedImg, cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin), cv::Scalar(0, 0, 255), 2);

    }
    cv::imwrite(outFileName, resizedImg);
}

void NeuralNetwork::outputTrackingImageWithBoxesAndLabels(const cv::Mat image, Eigen::MatrixXf tracks, std::string outFileName) {
    //Resize the image to what's used internally
    cv::Mat resizedImg;
    cv::resize(image, resizedImg, cv::Size(COLS, ROWS));

    for (int i = 0; i < tracks.rows(); i++) {

        int xmin = tracks(i, 0);
        int ymin = tracks(i, 1);
        int xmax = tracks(i, 2);
        int ymax = tracks(i, 3);

        cv::String text = "#" + std::to_string((int)tracks(i, 4)) + ": " + std::to_string((int)tracks(i, 6));

        //Put label text on screen (the first one is to give an outline to the text (it is really hard to read otherwise))
        cv::putText(resizedImg, text, cv::Point(xmin, ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
        cv::putText(resizedImg, text, cv::Point(xmin, ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

        cv::rectangle(resizedImg, cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin), cv::Scalar(0, 0, 255), 2);

    }
    cv::imwrite(outFileName, resizedImg);
}