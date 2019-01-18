#include "Tool.h"

#include <QBrush>
#include <QMouseEvent>
#include <qmessagebox.h>
#include <qdebug.h>
#include <qdir.h>
#include <QFileDialog>
#include "ximage.h"
Tool::Tool(QWidget *parent)
	: QMainWindow(parent)
{
	_char_idx =0;
	ui.setupUi(this);
	cv::namedWindow("SRC_IMAGE");
	cv::namedWindow("BIN_IMAGE");
	
	QObject::connect(ui.pushButton, &QPushButton::clicked, this, &Tool::load_image);
	QObject::connect(ui.pushButton_4, &QPushButton::clicked, this, &Tool::to_binary);
	QObject::connect(ui.pushButton_5, &QPushButton::clicked, this, &Tool::hist);
	QObject::connect(ui.pushButton_6, &QPushButton::clicked, this, &Tool::show_next_char);

}
void Tool::paintEvent(QPaintEvent* event) {
	QPainter paint(this);
	QBrush br(Qt::BrushStyle::SolidPattern);
	QPen pen;
	pen.setColor(Qt::gray);
	paint.setPen(pen);
	const int pixel_w = 6;
	const int offset = 25;
	const int max_height = 32;
	auto p = ui.groupBox_4->pos();
	for (int i = 0; i <=max_height; ++i) {
		paint.drawLine(p.x(), p.y() + i * pixel_w+offset, p.x() + pixel_w*max_height, p.y() + i * pixel_w+offset);
	}
	for (int j = 0; j <= max_height; ++j) {
		paint.drawLine(p.x() + j * pixel_w, p.y() + offset, p.x() + j * pixel_w, p.y() + max_height * pixel_w + offset);
	}
	if (!_chars.empty()) {
		int rows = std::min(max_height, _chars[_char_idx].rows);
		int cols = std::min(max_height, _chars[_char_idx].cols);
		for (int i = 0; i < rows; ++i) {
			auto pline = _chars[_char_idx].ptr<uchar>(i);
			for (int j = 0; j < cols; ++j) {
				if (*(pline + j) == 0)
					br.setColor(Qt::black);
				else
					br.setColor(Qt::white);
				paint.fillRect(p.x() + j * pixel_w+1, p.y() + i * pixel_w+1 + offset,
					pixel_w-1,pixel_w-1, br);
			}
		}
	}
	draw_line(_ys, paint,0, ui.groupBox_3);
	draw_line(_xs, paint,1, ui.groupBox_6);
	//qDebug("pos=%d,%d", p.x(), p.y());
	//QString ss = QString::asprintf("pos=%d,%d", p.x(), p.y());
	//QMessageBox::about(nullptr, ss, "dd");
}

void Tool::load_image() {

	auto dir = QFileDialog::getOpenFileName(this, "image path","",tr("Images(*.png *.bmp *.jpg *.tif *.GIF)"));
	if (!dir.isEmpty()) {
		_src=cv::imread(dir.toStdString());
		if (!_src.empty()) {
			//cv::cvtColor(_src, _src, CV_BGR2RGB);
			//QImage img = QImage(_src.data, _src.cols, _src.rows, QImage::Format_RGB888);
		
			//QMessageBox::about(this, "info", "load ok");
			cv::imshow("SRC_IMAGE", _src);
		}
		else {
			QMessageBox::about(this, "info", "load false");
		}
	}
}

void Tool::to_binary() {
	if (_src.empty())
		return;
	cv::cvtColor(_src, _gray, CV_BGR2GRAY);
	_binary = cv::Mat::zeros(_gray.size(), CV_8UC1);
	thresholdIntegral(_gray, _binary);
	cv::imshow("BIN_IMAGE", _binary);

}

void Tool::hist() {
	if (_binary.empty()) {
		return;
	}
	_chars.clear();
	std::string pic_name;
	std::vector<cv::Mat> out_y, out_x;
	
	picshadowy(_binary, out_y,_ys);
	for (int i = 0; i < out_y.size(); ++i) {
		picshadowx(out_y[i], out_x,_xs);
		for (int j = 0; j < out_x.size(); ++j) {
			_chars.push_back(out_x[j]);
		}
	}
	show_next_char();
}

void Tool::show_next_char() {
	//QMessageBox::warning(this, "show", "d");
	//qDebug("%d", _chars.size());
	if (!_chars.empty()) {
		_char_idx = (_char_idx + 1) % _chars.size();
		//QMessageBox::warning(this, "show", "d");
		//qDebug("idx=%d", _char_idx);
		update();
	}
	
}

void Tool::draw_line(const std::vector<int>&lines, QPainter& paint,int isy, QGroupBox* gp, Qt::GlobalColor cr) {
	if (lines.empty())
		return;
	auto pt = gp->pos();
	auto rect = gp->rect();
	int w = rect.right() - rect.left();
	int h = rect.bottom() - rect.top();
	//int max_val = *std::max_element(lines.begin(), lines.end());
	//if (max_val == 0)return;
	
	int n = lines.size(), idx;
	QPen pen;
	pen.setColor(cr);
	paint.setPen(pen);
	if (isy) {
		int x0 = pt.x(), y0 = pt.y() + h + 10;
		for (int j = 0; j < w; ++j) {
			idx = j % n;
			if (lines[idx]) {
				paint.drawLine(x0 + j, y0 - (lines[idx] % h), x0 + j, y0);
			}
		}
	}
	else {
		int x0 = pt.x(), y0 = pt.y()+10;
		for (int i = 0; i < h; ++i) {
			idx = i % n;
			if (lines[idx]) {
				paint.drawLine(x0, y0 + i, x0 + lines[idx] % w, y0 + i);
			}
		}
	}
	
}