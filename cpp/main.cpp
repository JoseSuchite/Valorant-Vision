#include <QApplication>
#include <QLabel>
#include <QImage>

#include <QtWidgets>

#include <opencv2/opencv.hpp>

#include "../headers/videoplayer.h"


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

    return app.exec();
}
