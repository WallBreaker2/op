#include "Tool.h"

#include <QBrush>
#include <QMouseEvent>
#include <qmessagebox.h>
#include <qdebug.h>
#include <qdir.h>
#include <QFileDialog>

Tool::Tool(QWidget *parent)
	: QMainWindow(parent)
{
	_is_edit =0;
	ui.setupUi(this);
	//cv::namedWindow("SRC_IMAGE");
	//cv::namedWindow("BIN_IMAGE");
	_itemmodel = new QStandardItemModel(this);
	_model = nullptr;
	QObject::connect(ui.pushButton, &QPushButton::clicked, this, &Tool::load_image);
	QObject::connect(ui.pushButton_4, &QPushButton::clicked, this, &Tool::to_binary);
	QObject::connect(ui.pushButton_5, &QPushButton::clicked, this, &Tool::hist);
	//QObject::connect(ui.pushButton_6, &QPushButton::clicked, this, &Tool::show_next_char);
	QObject::connect(ui.pushButton_2, &QPushButton::clicked, this, &Tool::add_word);
	QObject::connect(ui.listView, &QListView::clicked, this, &Tool::show_char);
	QObject::connect(ui.pushButton_7, &QPushButton::clicked, this, &Tool::load_dict);
	QObject::connect(ui.pushButton_8, &QPushButton::clicked, this, &Tool::save_dict);
	QObject::connect(ui.pushButton_9, &QPushButton::clicked, this, &Tool::edit_dict);

}
void Tool::paintEvent(QPaintEvent* event) {
	QPainter paint(this);
	QBrush br(Qt::BrushStyle::SolidPattern);
	QPen pen;
	pen.setColor(Qt::gray);
	paint.setPen(pen);
	const int pixel_w = 7;
	const int offset = 30;
	const int offsetx = 10;
	const int max_height = 32;
	auto p = ui.groupBox_4->pos();
	for (int i = 0; i <=max_height; ++i) {
		paint.drawLine(p.x()+offsetx, p.y() + i * pixel_w+offset,
			p.x() + pixel_w*max_height+offsetx, p.y() + i * pixel_w+offset);
	}
	for (int j = 0; j <= max_height; ++j) {
		paint.drawLine(p.x() + j * pixel_w + offsetx, p.y() + offset,
			p.x() + j * pixel_w + offsetx, p.y() + max_height * pixel_w + offset);
	}
	//qDebug("word:%d,%d", _curr_word.info.height, _curr_word.info.width);
	if (_curr_word.info.width) {
		//qDebug("word:%d,%d", _curr_word.info.height, _curr_word.info.width);
		int rows = _curr_word.info.height;
		int cols = _curr_word.info.width;
		for (int j = 0; j < cols; ++j) {
			for (int i = 0; i < rows; ++i) {
				if (GET_BIT(_curr_word.clines[j],31-i))
					br.setColor(Qt::black);
				else
					br.setColor(Qt::white);
				paint.fillRect(p.x() + j * pixel_w+1+offsetx, p.y() + i * pixel_w+1 + offset,
					pixel_w-1,pixel_w-1, br);
			}
		}
	}
	//draw_line(_ys, paint,0, ui.groupBox_3);
	//draw_line(_xs, paint,1, ui.groupBox_6);
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
			//cv::imshow("SRC_IMAGE", _src);
			_qimage.load(dir);
			ui.label_3->setPixmap(QPixmap::fromImage(_qimage));
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
	//cv::imshow("BIN_IMAGE", _binary);
	cv::imwrite("binary.png", _binary);
	_qbinary.load("binary.png");
	ui.label_4->setPixmap(QPixmap::fromImage(_qimage));

}

void Tool::hist() {
	if (_binary.empty()) {
		return;
	}
	_is_edit = 0;
	_dict_tmp.clear();
	std::string pic_name;
	std::vector<rect_t> out_y, out_x;
	rect_t rc;
	rc.x1 = rc.y1 = 0;
	rc.x2 = _binary.cols; rc.y2 = _binary.rows;
	//qDebug("rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
	//step1. 水平分割
	binshadowy(_binary,rc, out_y);
	for (auto&ity:out_y) {
		//step 2. 垂直分割
		binshadowx(_binary,ity, out_x);
		for (auto&itx:out_x) {
			//pic_name = std::to_string((i << 16) | j);
			//cv::imshow(pic_name, out_x[j]);
			//裁剪
			bin_image_cut(_binary, itx, itx);
			_dict_tmp.add_word(_binary, itx, L"");
			//_chars.push_back(out_x[j]);
		}
	}
	//
	QStringList strlist;
	QString tp, t;
	for (auto&it : _dict_tmp.words) {
		tp = QString::asprintf("|%d,%d,%d|", it.info.width, it.info.height, it.info.bit_count);
		tp = QString::fromWCharArray(it.info._char, 3) + tp;
		int n = it.info.height;
		for (int j = 0; j <= it.info.width; ++j) {

			t = QString::asprintf("%08X", it.clines[j]);
			tp += t;
		}
		strlist.append(tp);
	}
	if (_model)
		delete _model;
	_model = new QStringListModel(strlist);
	ui.listView->setModel(_model);
	if (!_dict_tmp.words.empty()) {
		_curr_word = _dict_tmp.words[0];
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

void Tool::show_char(const QModelIndex& idx) {
	int id = idx.row();
	if (_is_edit) {
		if (id >= 0 && id < _dict.words.size()) {
			_curr_word = _dict.words[id];
			update();
		}
	}
	else {
		if (id >= 0 && id < _dict_tmp.words.size()) {
			_curr_word = _dict_tmp.words[id];
			update();
		}
	}
}
void Tool::load_dict() {
	auto dir = QFileDialog::getOpenFileName(this, "dict path", "", tr("dict(*.dict)"));
	if (!dir.isEmpty()) {
		_dict.read_dict(dir.toStdString());
		ui.lineEdit_2->setText(dir);
	}
}

void Tool::save_dict() {
	auto dir = QFileDialog::getSaveFileName(this, "save path", "", tr("dict(*.dict)"));
	if (!dir.isEmpty()) {
		_dict.write_dict(dir.toStdString());
	}
}

void Tool::add_word() {
	auto str = ui.lineEdit->text();
	if (str.isEmpty())
		return;
	int idx=ui.listView->currentIndex().row();
	Dict* cd = _is_edit ? &_dict : &_dict_tmp;
	if (idx >= 0 && idx < cd->words.size()) {
		auto midx = ui.listView->currentIndex();
		QString text = _model->data(midx, Qt::DisplayRole).toString();
		int idx=text.indexOf("|");
		if (idx >= 0)
			text = text.replace(0, idx, str);
		_model->setData(midx, text, Qt::EditRole);
		_curr_word.set_chars(str.toStdWString());
		_dict.add_word(_curr_word);
	}
}

void Tool::edit_dict() {
	QStringList strlist;
	QString tp, t;
	_is_edit = 1;
	for (auto&it : _dict.words) {
		tp = QString::asprintf("|%d,%d,%d|", it.info.width, it.info.height, it.info.bit_count);
		tp = QString::fromWCharArray(it.info._char) + tp;
		int n = it.info.height;
		for (int j = 0; j <= it.info.width; ++j) {
			
			t = QString::asprintf("%08X", it.clines[j]);
			tp += t;
		}
		strlist.append(tp);
	}

	if (_model)
		delete _model;
	_model = new QStringListModel(strlist);
	ui.listView->setModel(_model);
	
	//ui.listView->setModel(_model);
}