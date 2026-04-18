#include "minimap.h"

#include <nlohmann/json.hpp>

#include <fstream>

Minimap::Minimap(QWidget *parentAddress)
    : QDockWidget("Minimap", parentAddress)
{
    // creates a widget for the minimap
    minimap_wid = new QLabel(this);
    // makes the minimap stay in the centre
    minimap_wid->setAlignment(Qt::AlignCenter);
    // sets the minimum window size 
    minimap_wid->setMinimumSize(250, 250);
    // doesn't allow the map to be stretched from all the ways
    minimap_wid->setScaledContents(false);
    // allows the window to be resized as user wants
    minimap_wid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // allows the window be popped out and free to move
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    // only allows to be docked on the right side right now!!!
    setAllowedAreas(Qt::RightDockWidgetArea);
    // draws the windows
    setWidget(minimap_wid);

    std::ifstream f("id_to_name.json");
    // was crashing when it couldn't find the file, so added this check and warning message
    if (!f.is_open()) {
<<<<<<< HEAD
        qWarning("Minimap: could not open id_to_name.json — check working directory");
=======
        qWarning("Minimap: could not open id_to_name.json â€” check working directory");
>>>>>>> 9a4bdb20a0f49958b16e022deb754a6b7435a051
        return;
    }
    nlohmann::json idToNameJSON = nlohmann::json::parse(f);
    f.close();

    int numClasses = idToNameJSON["categories"].size();
    for (int i = 0; i < numClasses; i++) {
        idToName.push_back(idToNameJSON["categories"][i]["name"]);
    }
    agentIcons.push_back(cv::Mat());
    for (int i = 0; i < numClasses; i++) {
        cv::Mat img = cv::imread("agent_icons/" + idToName[i] + ".png", cv::IMREAD_UNCHANGED);
        if (img.empty()) {
            qWarning("Minimap: could not load agent icon for '%s'", idToName[i].c_str());
            agentIcons.push_back(cv::Mat());
            continue;
        }
        cv::Mat resizedImg;
<<<<<<< HEAD
        cv::resize(img, resizedImg, cv::Size(20, 20)); //This isn't *really* hardcoding, as this will get layered onto the image when it's resized to match the model's predictions
=======
        cv::resize(img, resizedImg, cv::Size(20, 20));
>>>>>>> 9a4bdb20a0f49958b16e022deb754a6b7435a051
        agentIcons.push_back(resizedImg);
    }
}

void Minimap::loadImage(const QString& filePath)
{
    mapFile = filePath;

    if (!pixmap.load(filePath)) {
        // loads split as default if path error
        pixmap.load("map_layouts/Sunset_layout.png");
        mapFile = filePath;
        minimap_wid->setPixmap(pixmap);
    }
    // loads the image after image selected
    minimap_wid->setPixmap(pixmap);
}

void Minimap::resizeEvent(QResizeEvent *event)
{
    // if image present, resize without losing qaulity
    if (!pixmap.isNull()) {
        minimap_wid->setPixmap(
            pixmap.scaled(
                minimap_wid->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            )
        );
    }

    QDockWidget::resizeEvent(event);
}

QPixmap Minimap::cvMatToQPixmap(const cv::Mat &inMat) {
    cv::Mat rgbMat;
    // OpenCV uses BGR, Qt uses RGB
    cv::cvtColor(inMat, rgbMat, cv::COLOR_BGR2RGB);

    QImage qtImage((const unsigned char *)rgbMat.data,
        rgbMat.cols, rgbMat.rows,
        rgbMat.step,
        QImage::Format_RGB888);

    return QPixmap::fromImage(qtImage).copy();
}

void Minimap::redrawAgents(Eigen::MatrixXf frameData) {

    if (frameData.rows() == 0) { return; }
    if ((frameData.rows() == lastPredictions.rows()) && 
        (frameData.cols() == lastPredictions.cols()) &&  
        (frameData.isApprox(lastPredictions))) { return; }

    lastPredictions = frameData;

    cv::Mat image = cv::imread(mapFile.toStdString());

    cv::Mat resizedImg;
    cv::resize(image, resizedImg, cv::Size(384, 384)); //Resize to 384x384 since that's what the model's predictions are based on

    for (int i = 0; i < frameData.rows(); i++) {

        int xmin = frameData(i, 0);
        int ymin = frameData(i, 1);
        int xmax = frameData(i, 2);
        int ymax = frameData(i, 3);
        int trackID = frameData(i, 4);
        int classID = frameData(i, 6);

        cv::Mat agent = agentIcons[classID];

        std::vector<cv::Mat> channels;
        cv::split(agent, channels);

        cv::Mat alpha;
        alpha = channels[3]; // Extract the 4th channel
        cv::Mat bgr_foreground;
        cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, bgr_foreground);

        //If the area to draw on is OOB, don't do it (should change it later to just resize it to fit within bounds)
        if (xmin < 0 || ymin < 0 || xmin + agent.cols > resizedImg.cols || ymin + agent.rows > resizedImg.rows) {
            continue;
        }
        cv::Rect rect(xmin, ymin, agent.cols, agent.rows);
        cv::Mat ROI = resizedImg(rect);

        bgr_foreground.copyTo(ROI, alpha);

        cv::String text = std::to_string(trackID);

        cv::putText(resizedImg, text, cv::Point(xmin, ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
        cv::putText(resizedImg, text, cv::Point(xmin, ymin - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
    }

    // TODO: Need to update so that it goes to whatever size the pixmap was at before instead of staying at the model's prediction size...
    QPixmap newPixmap = cvMatToQPixmap(resizedImg);

    if (newPixmap.toImage() != pixmap.toImage()) {
        pixmap = newPixmap;
        minimap_wid->setPixmap(pixmap);
    }

}
