#include <QApplication>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QtWidgets>

#include <opencv2/opencv.hpp>

#include "../headers/videoplayer.h"

// Convert cv::Mat → QImage
QImage matToQImage(const cv::Mat& mat)
{
    switch (mat.type())
    {
    case CV_8UC1:
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);

    case CV_8UC3:
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_BGR888);

    case CV_8UC4:
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);

    default:
        return QImage();
    }
}

int main(int argc, char* argv[])
{



    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Valorant-Vision");
    mainWindow.resize(400, 300);
    //mainWindow.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainWindow.show();

    QWidget box(&mainWindow);
    box.show();

    VideoPlayer player(&mainWindow);
    std::string source = "D:/Downloads 2/videoplayback.mp4";
    player.displayVideo(source);



    //// Load an image with OpenCV
    //cv::Mat img = cv::imread("../../../download.png");
    //if (img.empty()) {
    //    qWarning("Failed to load image!");
    //    return -1;
    //}

    //// Convert to QImage
    //QImage qimg = matToQImage(img);

    //// Display in a QLabel
    //QLabel label;
    //label.setPixmap(QPixmap::fromImage(qimg));
    //label.setWindowTitle("Qt + OpenCV Example");
    //label.resize(qimg.size());
    //label.show();



    return app.exec();
}
