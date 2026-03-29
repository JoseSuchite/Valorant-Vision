#include "../headers/minimap.h"

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
}

void Minimap::loadImage(const QString& filePath)
{
    if (!pixmap.load(filePath)) {
        // loads split as default if path error
        pixmap.load("map_layouts/Split_layout.png");
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
