#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Tool.h"
#include <QPainter>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
class Tool : public QMainWindow
{
	Q_OBJECT

public:
	Tool(QWidget *parent = Q_NULLPTR);
	virtual void paintEvent(QPaintEvent*);
	void load_image();
	void to_binary();
	void hist();
	void show_next_char();
	void draw_line(const std::vector<int>&lines, QPainter& paint,int isy,QGroupBox*,Qt::GlobalColor cr=Qt::black);
private:
	Ui::ToolClass ui;
	cv::Mat _src;
	cv::Mat _gray;
	cv::Mat _binary;
	std::vector<cv::Mat>_chars;
	int _char_idx;
	std::vector<int> _ys, _xs;
};
