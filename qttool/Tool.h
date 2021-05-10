#pragma once

#include <QtWidgets/QMainWindow>
//#include "ui_Tool.h"
#include "ui_toolWidget.h"
#include <QPainter>

#include <QStringListModel>
#include <QStandardItemModel>
#include <QModelIndex>
#include <qscrollarea.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qtimer.h>
#include <qlabel.h>
#include <windows.h>
#include <vector>
#include "cap_dialog.h"
#include "EditImage.h"
#include "../libop/ImageProc/ImageLoc.h"
#include "../libop/include/color.h"
#include "get_color_dlg.h"


class ArrayW :public QWidget {
public:
	const int pixel_w = 7;
	ArrayW(QWidget* parent, word1_t& wd_) :QWidget(parent, Qt::WindowFlags::enum_type::FramelessWindowHint), wd(wd_) {
		resize(pixel_w * 255, pixel_w * 255);

	}

	void paintEvent(QPaintEvent*) {
		QPainter paint(this);
		QBrush br(Qt::BrushStyle::SolidPattern);
		QPen pen;
		pen.setColor(Qt::gray);
		paint.setPen(pen);

		const int offset = 0;
		const int offsetx = 0;
		const int max_height = 255;
		auto p = QPoint(2, 2);
		for (int i = 0; i <= max_height; ++i) {
			paint.drawLine(p.x() + offsetx, p.y() + i * pixel_w + offset,
				p.x() + pixel_w * max_height + offsetx, p.y() + i * pixel_w + offset);
		}
		for (int j = 0; j <= max_height; ++j) {
			paint.drawLine(p.x() + j * pixel_w + offsetx, p.y() + offset,
				p.x() + j * pixel_w + offsetx, p.y() + max_height * pixel_w + offset);
		}
		//qDebug("word:%d,%d", _curr_word.info.h, _curr_word.info.w);
		if (wd.info.w) {
			//qDebug("word:%d,%d", _curr_word.info.h, _curr_word.info.w);
			int rows = wd.info.h;
			int cols = wd.info.w;
			int idx = 0;
			for (int j = 0; j < cols; ++j) {
				for (int i = 0; i < rows; ++i) {
					if (GET_BIT(wd.data[idx / 8], idx & 7))
						br.setColor(Qt::black);
					else
						br.setColor(Qt::white);
					idx++;
					paint.fillRect(p.x() + j * pixel_w + 1 + offsetx, p.y() + i * pixel_w + 1 + offset,
						pixel_w - 1, pixel_w - 1, br);
				}
			}
		}
	}
public:
	word1_t& wd;
};

class Tool : public QWidget
{
	Q_OBJECT

public:
	Tool(QWidget* parent = Q_NULLPTR);
	virtual void paintEvent(QPaintEvent*);
	virtual void resizeEvent(QResizeEvent*)override;
	void create_control();
	void create_layout();
	void load_image();
	void edit_image();
	void save_image();
	void to_binary();
	void hist();
	//void draw_line(const std::vector<int>&lines, QPainter& paint,int isy,QGroupBox*,Qt::GlobalColor cr=Qt::black);
	void show_char(const QModelIndex& idx);
	void load_dict();

	void save_dict();
	void add_word();
	void edit_dict();
	//void edit_enter();
	void del_word();
	//void mousePressEvent(QMouseEvent* event);
	//void mouseReleaseEvent(QMouseEvent* event);
	void on_state_changed(int st);
	//void mouseMoveEvent(QMouseEvent* event);
	void on_simChanged(double sim);
	void on_chkbk(bool checked = false);
	void on_capture();
	void on_system_dict();
	void on_editChanged(const QString& s);
	void on_btnFind();
	void closeEvent(QCloseEvent* e);
	void on_newdict();
	void on_item_clicked(const QModelIndex& index);
	void on_item_changed(const QStandardItem* item);
private slots: void to_mat();
private slots: void got_color(QPoint pt);
private:
	Ui::Form ui;

	//Ui::

	ImageBase _imgloc;
	int edit_mode;
	QStringListModel* _model;
	QStandardItemModel* _itemmodel;
	//QStandardItemModel* _itemmodel;
	Dict file_dict;
	Dict dict_new;
	word1_t _curr_word;
	QImage _qimage, _qbinary;
	color_df_t _color_info[10];
	int _color_idx, _is_press;
	//QCheckBox* _checkbox[10];
	//QLineEdit* edit_color[10];
	//QLineEdit* _df_edit[10];
	//QLabel* m_labelcolor[10];
	//QLabel* m_array_lb, * m_array_desc;

	QTimer _timer;
	//�豸���
	HDC _hdc;
	HDC _hmdc;
	//λͼ���
	HBITMAP _hbmpscreen;
	HBITMAP _holdbmp;
	//λͼ��Ϣ
	BITMAP _bm;
	BITMAPINFOHEADER _bih;
	int _width, _height;
	std::vector<unsigned char> _imagedata;
	double _ocr_sim;
	cap_dialog _cap_dlg;
	get_color_dlg* m_getcolor_dlg;
	bool saved = true;
	EditImage editDlg;
	//QScrollArea* scroll_area;
	ArrayW* awidget;

private:
	void capture_full_screen();
	void release_res();
	void readCfg();
	void writeCfg();

};


