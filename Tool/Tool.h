#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Tool.h"
#include <QPainter>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QModelIndex>
#include "ximage.h"
#include "../op/include/color.h"
#include <qcheckbox.h>
#include <qdialog.h>
#include <qtimer.h>
#include <windows.h>
#include <vector>
class my_dialog : public QDialog {
public:
	my_dialog(QWidget* parent) :QDialog(parent,Qt::WindowFlags::enum_type::FramelessWindowHint)
		,_label(this)
	{
		resize(my_dialog::ccols*my_dialog::cw+100, my_dialog::crows*my_dialog::cw);
		_label.resize(70, my_dialog::crows*my_dialog::cw);
		_label.move(my_dialog::ccols*my_dialog::cw + 15, 0);
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
			paint.drawLine(0, i*cw, ccols*cw, i*cw);
		}
		for (int j = 0; j <= ccols; ++j) {
			paint.drawLine(j*cw, 0, j*cw, crows*cw);
		}
		for (int i = 0; i < crows; ++i) {
			for (int j = 0; j < ccols; ++j) {
				br.setColor(_color[i*ccols + j]);
				paint.fillRect(j*cw + 1, i*cw + 1, cw - 1, cw - 1, br);
			}
		}
		pen.setColor(Qt::red);
		paint.setPen(pen);
		paint.drawRect((ccols / 2 - 1)*cw, (crows / 2 - 1)*cw, 3 * cw , 3 * cw);
	}
	static const int cw = 10;
	static const int crows = 15;
	static const int ccols = 15;
	QColor _color[crows*ccols];
	QLabel _label;
};

class Tool : public QMainWindow
{
	Q_OBJECT

public:
	Tool(QWidget *parent = Q_NULLPTR);
	virtual void paintEvent(QPaintEvent*);
	void load_image();
	void to_binary();
	void hist();
	void draw_line(const std::vector<int>&lines, QPainter& paint,int isy,QGroupBox*,Qt::GlobalColor cr=Qt::black);
	void show_char(const QModelIndex& idx);
	void load_dict();
	void save_dict();
	void add_word();
	void edit_dict();
	void edit_enter();
	void del_word();
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void on_state_changed(int st);
	void mouseMoveEvent(QMouseEvent* event);
	void on_slider();
	void on_auto(bool checked = false);
private:
	Ui::ToolClass ui;
	cv::Mat _src;
	cv::Mat _gray;
	cv::Mat _binary;
	cv::Mat _record;
	int _is_edit;
	QStringListModel* _model;
	QStandardItemModel* _itemmodel;
	Dict _dict;
	word_t _curr_word;
	QImage _qimage, _qbinary;
	color_df_t _color_info[10];
	int _color_idx, _is_press;
	QCheckBox* _checkbox[10];
	QLineEdit* _df_edit[10];
	my_dialog _motive;
	QTimer _timer;
	//设备句柄
	HDC _hdc;
	HDC _hmdc;
	//位图句柄
	HBITMAP _hbmpscreen;
	HBITMAP _holdbmp;
	//位图信息
	BITMAP _bm;
	BITMAPINFOHEADER _bih;
	int _width, _height;
	std::vector<unsigned char> _imagedata;
	double _ocr_sim;
};


