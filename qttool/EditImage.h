#ifndef __EDIT_IMAGE_H_
#define __EDIT_IMAGE_H_

#include <qdialog.h>
#include "ui_editImg.h"
#include <qfont.h>
#include <qfontdialog.h>
#include <qimage.h>
#include <qwidget.h>
#include <qpainter.h>
#include "../libop/imageProc/ImageLoc.h"
#include <qdebug.h>
#include <QMouseEvent>
class ViewDlg :public QWidget {
	Q_OBJECT;
public:
	ViewDlg(QWidget* parent, Image& img,int& index) :QWidget(parent, Qt::WindowFlags::enum_type::FramelessWindowHint), viewImg(img),
	m_index(index){
		resize(1920,1080);

	}

	void paintEvent(QPaintEvent*) {
		QPainter paint(this);
		
		
		auto qimg = QImage(viewImg.pdata, viewImg.width, viewImg.height, viewImg.width * 4, QImage::Format_ARGB32);
		paint.drawImage(QPoint(0,0),qimg);
		if (pressed&&m_index==1) {
			paint.drawRect(select_rc);
		}
	}
	void mousePressEvent(QMouseEvent* event) {
		if (event->pos().x() < viewImg.width && event->pos().y() < viewImg.height) {
			pressed = 1;
			point1 = event->pos();
		}
		
	}
	void mouseReleaseEvent(QMouseEvent* event) {
		if (pressed &&m_index==1&& event->pos().x() < viewImg.width && event->pos().y() < viewImg.height) {
			point2 = event->pos();
			select_rc = QRect(point1, event->pos());
			//qDebug() << select_rc;
			update();
			on_selected();
			pressed = 0;
		}

	}
	void mouseMoveEvent(QMouseEvent* event) {
		if (pressed&&event->pos().x()<viewImg.width&&event->pos().y()<viewImg.height) {
			
			select_rc = QRect(point1, event->pos());
			//qDebug() << select_rc;
			update();
		}
	}
signals:
	void on_selected();
public:
	Image& viewImg;
	int& m_index;
	QPoint point1, point2;
	QRect select_rc;
	bool pressed=false;
};

class EditImage:
	public QDialog
{
public:
	EditImage(QWidget* parent);
	~EditImage();

	virtual void paintEvent(QPaintEvent*);
	void do_init(Image& img);
	void on_resize();


	void on_magic();
	void on_fill();

	void on_roate();
	
	

	ImageBase ib;
private:
	Ui::EditImgDlg ui;
	ViewDlg* viewdlg;
	int index = 0;
	QImage qimg;
	
	int bigX=1, smallX=1;
	Image curImg;
	Image rawImg;
	int viewW=0, viewH=0;
	int selected = 0;
};

#endif

