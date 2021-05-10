#pragma once
#include <QtWidgets/QPushButton>
//#include <QDebug>

class Mbutton :public QPushButton {
	Q_OBJECT
public:

	Mbutton(QWidget* parent) :QPushButton(parent) {
		_pressed = false;;
	}
	void mousePressEvent(QMouseEvent *e) {
		this->setCursor(Qt::CrossCursor);
		_pressed = true;
	}

	void mouseMoveEvent(QMouseEvent *e) {
		if (_pressed) {
			//qDebug() << e->pos();
			on_mousemove();
		}
	}

	void mouseReleaseEvent(QMouseEvent *e) {
		this->setCursor(Qt::ArrowCursor);
		_pressed = false;
	}
private:
	bool _pressed;
signals:
	void on_mousemove();
	
};
