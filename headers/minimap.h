#ifndef MINIMAP_H
#define MINIMAP_H

#include <QDockWidget>
#include <QLabel>
#include <QPixmap>

#include <Eigen/dense>

// This class is used by the side bar to show logs and reset bar
class Minimap : public QDockWidget {

private:
	QLabel* minimap_wid;
	QPixmap pixmap;

public:
	// sets the parent to nullptr if no main parent
	explicit Minimap(QWidget* parent = nullptr);

	// allows to load an image to the minimap widget
	void loadImage(const QString &file);

	// allows the image in the widget to resize without losing the quality
	void resizeEvent(QResizeEvent *event);

	void redrawAgents(Eigen::MatrixXf frameData);
};

#endif