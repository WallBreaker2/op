#pragma once
#include <QtWidgets/QMainWindow>
#include <QPainter>
#include <qdialog.h>
#include <qmessagebox.h>
#include <qevent.h>
#include <qdebug.h>
#include <qpushbutton.h>
/*
capture dialog
*/
class cap_dialog : public QDialog {
	Q_OBJECT
public:
	cap_dialog(QWidget* parent) :QDialog(parent, Qt::WindowFlags::enum_type::FramelessWindowHint),_parent(parent) {
		//showFullScreen();
		_screen_w = 0;
		_pressed = 0;
		btn[0] = new QPushButton("ok", this);
		btn[1] = new QPushButton("cancel", this);
		//btn[0]->setBaseSize(40, 20);
		//btn[1]->setBaseSize(40, 20);
		btn[0]->hide();	btn[1]->hide();
		QObject::connect(btn[0], &QPushButton::clicked, this, &cap_dialog::on_ok);
		QObject::connect(btn[1], &QPushButton::clicked, this, &cap_dialog::on_no);
	}
	virtual void paintEvent(QPaintEvent*) {
		QPainter paint(this);
		paint.drawImage(rect(), _img);
		if (_pressed) {
			//paint.drawRect(QRect(_startpt,_endpt));
			//QRect rc(_startpt, _endpt);
			//paint.fillRect(_rect, QBrush(Qt::SolidPattern));
			paint.drawImage(srect, _src,srect);
			QPen pen(Qt::SolidLine);
			pen.setColor(Qt::blue);
			paint.setPen(pen);
			paint.drawRect(srect);
			
			QRect r[8];
			
			QBrush br(Qt::BrushStyle::SolidPattern);
			br.setColor(Qt::blue);
			for (int i = 0; i < 8; i++) {
				r[i].setTopLeft(dpt[i] - QPoint(2, 2));
				r[i].setBottomRight(dpt[i] + QPoint(2, 2));
				paint.fillRect(r[i], br);
			}
			//paint.drawRects(r, 8);
			
			//paint.drawPoints(p, 8);
		}
	}
	virtual void closeEvent(QCloseEvent* e) {
		//QMessageBox::information(this, "info", "dd");
	}
	void mousePressEvent(QMouseEvent* event) {
		if (firstCap) {
			spt = event->pos();
		}
		else {
			dir = -1;
			for (int i = 0; i < 8; i++) {
				auto d = dpt[i] - event->pos();
				if (QPoint::dotProduct(d, d) <= 8)
					dir = i;
			}
		}
		
		_pressed = 1;
	}
	void mouseReleaseEvent(QMouseEvent* event) {
		_pressed = 0;
		if (firstCap) {
			ept = event->pos();
		
		}
		auto sz = btn[0]->size();
		btn[0]->move(ept + QPoint(-sz.width() * 2, 0));
		btn[1]->move(ept + QPoint(-sz.width(), 0));
		btn[0]->showNormal(); btn[1]->showNormal();
		firstCap = false;
	
	}
	void mouseMoveEvent(QMouseEvent* event) {
		if (_pressed) {
			dpt[0] = srect.topLeft();
			dpt[1].setX(srect.right()); dpt[1].setY(srect.top());
			dpt[2].setX(srect.left()); dpt[2].setY(srect.bottom());
			dpt[3] = srect.bottomRight();
			dpt[4] = (dpt[0] + dpt[1]) / 2;
			dpt[5] = (dpt[0] + dpt[2]) / 2;
			dpt[6] = (dpt[3] + dpt[1]) / 2;
			dpt[7] = (dpt[3] + dpt[2]) / 2;
			if (firstCap) {
				ept = event->pos();
				get_rect(spt, ept);

			}
			else if(dir!=-1){
				update_rect(dir, event->pos());
			}
			update();
		}
		
	}
	void mouseDoubleClickEvent(QMouseEvent*event) {
		/*this->hide();
		
		if (_parent) {
			sendCapture();
			_parent->showNormal();
			_parent
		}*/
			
	}

	void on_ok() {
		this->hide();
		btn[0]->hide();	btn[1]->hide();
		if (_parent) {
			sendCapture();
			_parent->showNormal();
			//_parent
		}
	}

	void on_no() {
		this->hide();
		btn[0]->hide();	btn[1]->hide();
		if (_parent) {
			_parent->showNormal();
			//_parent
		}
	}
	void AdjLight(float val = 0.5) {
		int red, green, blue;
		int pixels = _img.width() * _img.height();
		unsigned int *data = (unsigned int *)_img.bits();

		for (int i = 0; i < pixels; ++i)
		{
			red = qRed(data[i]) * val;
			//red = (red < 0x00) ? 0x00 : (red > 0xff) ? 0xff : red;
			green = qGreen(data[i]) * val;
			//green = (green < 0x00) ? 0x00 : (green > 0xff) ? 0xff : green;
			blue = qBlue(data[i]) * val;
			//blue = (blue < 0x00) ? 0x00 : (blue > 0xff) ? 0xff : blue;
			data[i] = qRgba(red, green, blue, qAlpha(data[i]));
		}
		
	}
	void get_rect(QPoint pt1, QPoint pt2) {
		int x1 = pt1.x(), y1 = pt1.y();
		int x2 = pt2.x(), y2 = pt2.y();
		if (x1 > x2)std::swap(x1, x2);
		if (y1 > y2)std::swap(y1, y2);
		srect = QRect(x1, y1, x2 - x1, y2 - y1);
		
	}

	void update_rect(int dir_,QPoint pt) {
		if (dir_ == 0)spt = pt;
		else if (dir_ == 1) {
			spt.setY(pt.y());
			ept.setX(pt.x());
		}
		else if (dir_ == 2) {
			spt.setX(pt.x());
			ept.setY(pt.y());
		}
		else if (dir_ == 3) {
			ept = pt;
		}
		else if (dir_ == 4) {
			spt.setY(pt.y());
		}
		else if (dir_ == 5) {
			spt.setX(pt.x());
		}
		else if (dir_ == 6) {
			ept.setX(pt.x());
		}
		else if (dir_ == 7) {
			ept.setY(pt.y());
		}
		get_rect(spt, ept);
	}
	QImage _img;
	QImage _src;
	QPoint spt;
	QPoint ept;
	QRect srect;
	int _pressed;
	int _screen_w;
	//unsigned char* _pscreen;
	QWidget* _parent;
	QPushButton* btn[2];
	bool firstCap=true;
	int dir;
	QPoint dpt[8];
private:

signals:
	void sendCapture();
};

