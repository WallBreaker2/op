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
private:
	Ui::ToolClass ui;
	cv::Mat _src;
	cv::Mat _gray;
	cv::Mat _binary;
	int _is_edit;
	QStringListModel* _model;
	QStandardItemModel* _itemmodel;
	Dict _dict;
	Dict _dict_tmp;
	word_t _curr_word;
};
