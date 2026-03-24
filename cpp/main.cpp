
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

    QFile stylesheet(":/style/style.qss");
    if (stylesheet.open(QIODevice::ReadOnly | QIODevice::Text)){
        qApp->setStyleSheet(stylesheet.readAll());
        stylesheet.close();
    }

    MainWindow mainWindow;
    mainWindow.show();
    //mainWindow.setWindowTitle("Valorant-Vision");
    mainWindow.resize(900, 600); // initial window size for better viewing
    mainWindow.setMinimumSize(700, 400); // set a minimum size so, it keeps the UI clean

    

    return app.exec();
}
