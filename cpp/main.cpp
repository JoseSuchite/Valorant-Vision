
#include <QtWidgets>
#include <QVBoxLayout>
#include <QApplication>
#include <QLabel>
#include <QImage>
#include <QPushButton>
#include <QLayout>

#include <opencv2/opencv.hpp>

#include "../headers/mainwindow.h"


int main(int argc, char* argv[])
{

    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();
    //mainWindow.setWindowTitle("Valorant-Vision");
    //mainWindow.resize(600, 400);

    

    return app.exec();
}
