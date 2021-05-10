#pragma once
#ifndef _GET_COLOR_FLG_H
#define _GET_COLOR_DLG_H
#include <qwidget.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qpainter>
class my_dialog : public QDialog {
public:
	my_dialog(QWidget* parent) :QDialog(parent, Qt::WindowFlags::enum_type::FramelessWindowHint)
		, _label(this)
	{
		resize(my_dialog::ccols * my_dialog::cw + 100, my_dialog::crows * my_dialog::cw);
		_label.resize(70, my_dialog::crows * my_dialog::cw);
		_label.move(my_dialog::ccols * my_dialog::cw + 15, 0);
		_label.setText("pos:[0,0]\r\ncolor:000000");
	}
	void paintEvent(QPaintEvent*) {
		QPainter paint(this);
		QBrush br(Qt::BrushStyle::SolidPattern);
		QPen pen;
		pen.setColor(Qt::black);
		paint.setPen(pen);
		br.setColor(Qt::gray);
		paint.fillRect(rect(), br);
		for (int i = 0; i <= crows; ++i) {
			paint.drawLine(0, i * cw, ccols * cw, i * cw);
		}
		for (int j = 0; j <= ccols; ++j) {
			paint.drawLine(j * cw, 0, j * cw, crows * cw);
		}
		for (int i = 0; i < crows; ++i) {
			for (int j = 0; j < ccols; ++j) {
				br.setColor(_color[i * ccols + j]);
				paint.fillRect(j * cw + 1, i * cw + 1, cw - 1, cw - 1, br);
			}
		}
		//pen.setColor(Qt::red);
		//paint.setPen(pen);
		//paint.drawRect((ccols / 2 - 1)*cw, (crows / 2 - 1)*cw, 3 * cw , 3 * cw);
	}
	static const int cw = 12;
	static const int crows = 11;
	static const int ccols = 11;
	QColor _color[crows * ccols];
	QLabel _label;
};

class get_color_dlg :public QDialog
{
	Q_OBJECT
public:
	explicit get_color_dlg(int& w,int& h, std::vector<unsigned char>&img, QWidget* parent = nullptr);

	void prepare();
	void paintEvent(QPaintEvent* event)override;
	/*void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);*/
	void mouseMoveEvent(QMouseEvent* event);
	void mouseDoubleClickEvent(QMouseEvent* e)override;
	void keyReleaseEvent(QKeyEvent* event)override;

signals:void got_color(QPoint pt);
private:
	void move_cursor(QPoint pos);
	my_dialog* _motive;
	int m_clicked_count;
	int& _width, &_height;
	std::vector<unsigned char>& _imagedata;
	QPoint m_pt;
	QImage m_src;
};
#endif // !_


