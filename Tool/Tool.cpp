#include "Tool.h"

#include <QBrush>
#include <QMouseEvent>
#include <qmessagebox.h>
#include <qdebug.h>
#include <qdir.h>
#include <QFileDialog>
#include <Windows.h>

Tool::Tool(QWidget *parent)
	: QMainWindow(parent),_motive(this),_cap_dlg(this)
{
	_is_edit = 0;
	_is_press = 0;
	_ocr_sim = 1.0;
	ui.setupUi(this);
	const int chigh = 20, cwidth = 80;
	int w = ui.lineEdit->height();
	for (int i = 0; i < 10; ++i) {
		_checkbox[i] = new QCheckBox(this);
		_checkbox[i]->resize(w * 2, w);
		_checkbox[i]->move(ui.label_5->pos().x() + cwidth + 5,
			ui.label_5->pos().y() + i * chigh);
		QObject::connect(_checkbox[i], &QCheckBox::stateChanged, this, &Tool::on_state_changed);
		
		_df_edit[i] = new QLineEdit(this);
		_df_edit[i]->resize(50, w);
		_df_edit[i]->move(ui.label_5->pos().x() + cwidth / 4 + 5,
			ui.label_5->pos().y() + i * chigh);
		_df_edit[i]->setText("000000");
	}
	_checkbox[0]->setChecked(true);
	_motive.hide();
	//_motive.move(0, 0);
	_timer.setInterval(300);
	//QObject::connect(&_timer,QTimer::)
	//cv::namedWindow("SRC_IMAGE");
	//cv::namedWindow("BIN_IMAGE");
	_itemmodel = new QStandardItemModel(this);
	_model = nullptr;
	//_pimagedata = nullptr;
	QObject::connect(ui.pushButton, &QPushButton::clicked, this, &Tool::load_image);
	QObject::connect(ui.pushButton_4, &QPushButton::clicked, this, &Tool::to_binary);
	QObject::connect(ui.pushButton_5, &QPushButton::clicked, this, &Tool::hist);
	//QObject::connect(ui.pushButton_6, &QPushButton::clicked, this, &Tool::show_next_char);
	QObject::connect(ui.pushButton_2, &QPushButton::clicked, this, &Tool::add_word);
	QObject::connect(ui.listView, &QListView::clicked, this, &Tool::show_char);
	QObject::connect(ui.pushButton_7, &QPushButton::clicked, this, &Tool::load_dict);
	QObject::connect(ui.pushButton_8, &QPushButton::clicked, this, &Tool::save_dict);
	QObject::connect(ui.pushButton_9, &QPushButton::clicked, this, &Tool::edit_dict);
	QObject::connect(ui.lineEdit, &QLineEdit::editingFinished, this, &Tool::edit_enter);
	QObject::connect(ui.pushButton_6, &QPushButton::clicked, this, &Tool::del_word);
	QObject::connect(ui.horizontalSlider, &QSlider::sliderReleased, this, &Tool::on_slider);
	QObject::connect(ui.checkBox, &QCheckBox::clicked, this, &Tool::on_auto);
	QObject::connect(ui.pushButton_10, &QPushButton::clicked, this, &Tool::on_capture);
	QObject::connect(&_cap_dlg, &cap_dialog::sendCapture, this, &Tool::to_mat);
	//connect(maptable,)
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
	for (int i = 0; i <= max_height; ++i) {
		paint.drawLine(p.x() + offsetx, p.y() + i * pixel_w + offset,
			p.x() + pixel_w * max_height + offsetx, p.y() + i * pixel_w + offset);
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
				if (GET_BIT(_curr_word.clines[j], 31 - i))
					br.setColor(Qt::black);
				else
					br.setColor(Qt::white);
				paint.fillRect(p.x() + j * pixel_w + 1 + offsetx, p.y() + i * pixel_w + 1 + offset,
					pixel_w - 1, pixel_w - 1, br);
			}
		}
	}
	p = ui.label_5->pos();
	const int chigh = 20, cwidth = 80;
	for (int i = 0; i < 10; ++i) {
		//Qt::BGR
		auto cr = _color_info[i].color;
		br.setColor(QColor(cr.r, cr.g, cr.b));
		paint.fillRect(p.x(), p.y() + i * chigh,
			cwidth/4 - 1, chigh - 1, br);
	}
	//draw_line(_ys, paint,0, ui.groupBox_3);
	//draw_line(_xs, paint,1, ui.groupBox_6);
	//qDebug("pos=%d,%d", p.x(), p.y());
	//QString ss = QString::asprintf("pos=%d,%d", p.x(), p.y());
	//QMessageBox::about(nullptr, ss, "dd");
}

void Tool::load_image() {

	auto dir = QFileDialog::getOpenFileName(this, "image path", "", tr("Images(*.png *.bmp *.jpg *.tif *.GIF)"));
	if (!dir.isEmpty()) {
		_src = cv::imread(dir.toStdString());
		if (!_src.empty()) {
			//cv::cvtColor(_src, _src, CV_BGR2RGB);
			//QImage img = QImage(_src.data, _src.cols, _src.rows, QImage::Format_RGB888);

			//QMessageBox::about(this, "info", "load ok");
			//cv::imshow("SRC_IMAGE", _src);
			_qimage.load(dir);
			ui.label_3->setPixmap(QPixmap::fromImage(_qimage));
			to_binary();
		}
		else {
			QMessageBox::about(this, "info", "load false");
		}
	}

}

void Tool::to_binary() {
	if (_src.empty())
		return;
	int ncols = _src.cols, nrows = _src.rows;
	cv::cvtColor(_src, _gray, CV_BGR2GRAY);
	if (ui.checkBox->isChecked())
		graytobinary(_gray, _binary);
	else {
		_binary.create(nrows, ncols, CV_8UC1);
		std::vector<color_df_t> colors;
		for (int i = 0; i < 10; ++i) {
			if (_checkbox[i]->isChecked())
				colors.push_back(_color_info[i]);
		}
		for (int i = 0; i < nrows; ++i) {
			uchar* p = _src.ptr<uchar>(i);
			uchar* p2 = _binary.ptr<uchar>(i);
			for (int j = 0; j < ncols; ++j) {
				*p2 = 0xff;
				for (auto&it : colors) {//对每个颜色描述
					if ((*(color_t*)p - it.color) <= it.df) {
						*p2 = 0;
						break;
					}
				}
				++p2;
				p += 3;
			}
		}
	}
	
	cv::imwrite("binary.png", _binary);
	_qbinary.load("binary.png");
	ui.label_4->setPixmap(QPixmap::fromImage(_qbinary));
	std::wstring ss;
	bin_ocr(_binary, _record, _dict,_ocr_sim, ss);
	ui.textEdit->setText(QString::fromStdWString(ss));
}

void Tool::hist() {
	if (_binary.empty()) {
		return;
	}
	_is_edit = 0;
	std::string pic_name;
	std::vector<rect_t> out_y, out_x;
	rect_t rc;
	rc.x1 = rc.y1 = 0;
	rc.x2 = _binary.cols; rc.y2 = _binary.rows;
	//qDebug("rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
	//step1. 水平分割
	binshadowy(_binary, rc, out_y);
	for (auto&ity : out_y) {
		//step 2. 垂直分割
		binshadowx(_binary, ity, out_x);
		for (auto&itx : out_x) {
			//pic_name = std::to_string((i << 16) | j);
			//cv::imshow(pic_name, out_x[j]);
			//裁剪
			bin_image_cut(_binary, itx, itx);

			_dict.add_word(_binary, itx);
			//_chars.push_back(out_x[j]);
		}
	}
	//
	QStringList strlist;
	QString tp, t;
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
	if (!_dict.words.empty()) {
		_curr_word = _dict.words[0];
		update();
	}
}


void Tool::draw_line(const std::vector<int>&lines, QPainter& paint, int isy, QGroupBox* gp, Qt::GlobalColor cr) {
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
		int x0 = pt.x(), y0 = pt.y() + 10;
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
	if (id >= 0 && id < _dict.words.size()) {
		_curr_word = _dict.words[id];
		update();
	}
}
void Tool::load_dict() {
	auto dir = QFileDialog::getOpenFileName(this, "dict path", "", tr("dict(*.dict)"));
	if (!dir.isEmpty()) {
		_dict.read_dict(dir.toStdString());
		ui.lineEdit_2->setText(dir);
		std::wstring ss;
		bin_ocr(_binary,_record, _dict,_ocr_sim, ss);
		ui.textEdit->setText(QString::fromStdWString(ss));
		edit_dict();
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
	int idx = ui.listView->currentIndex().row();
	if (idx >= 0 && idx < _dict.words.size()) {
		auto midx = ui.listView->currentIndex();
		QString text = _model->data(midx, Qt::DisplayRole).toString();
		int idx = text.indexOf("|");
		if (idx >= 0)
			text = text.replace(0, idx, str);
		_model->setData(midx, text, Qt::EditRole);
		_curr_word.set_chars(str.toStdWString());
		_dict.add_word(_curr_word);
		ui.lineEdit->clear();
		idx = ui.listView->currentIndex().row();
		idx = (idx + 1) % _model->rowCount();
		QModelIndex next = _model->index(idx);
		ui.listView->setCurrentIndex(next);
		show_char(next);
		std::wstring ss;
		bin_ocr(_binary,_record, _dict,_ocr_sim, ss);
		ui.textEdit->setText(QString::fromStdWString(ss));
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

void Tool::edit_enter() {
	add_word();
}

void Tool::del_word() {
	int idx = ui.listView->currentIndex().row();
	if (idx >= 0 && idx < _dict.words.size()) {
		_model->removeRow(idx);
		_dict.erase(_dict.words[idx]);
		auto midx = ui.listView->currentIndex();
		show_char(midx);
		std::wstring ss;
		bin_ocr(_binary,_record, _dict,_ocr_sim, ss);
		ui.textEdit->setText(QString::fromStdWString(ss));
	}
}

void Tool::mousePressEvent(QMouseEvent* event) {

	if (event->button() == Qt::LeftButton) {
		//
		auto p = ui.label_5->pos();
		const int chigh = 20, cwidth = 80;
		if (event->x() < p.x() || event->x() > p.x() + cwidth || event->y() < p.y() || event->y() > p.y() + chigh * 10)
			return;
		_is_press = 1;
		_color_idx = (event->y() - p.y()) / chigh;
		auto global = mapToGlobal(event->pos());
		_motive.move(global.x() + 20, global.y());
		_motive.show();
		release_res();
		capture_full_screen();
	}
}

void Tool::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton&&_is_press) {
		
		auto global = mapToGlobal(event->pos());
		_is_press = 0;
		//HDC hdc = ::GetDC(NULL);
		if (_hdc&&global.x()<_width&&global.y()<_height) {
			auto pcr = _imagedata.data() + (_width *(_height - global.y()) + global.x()) * 4;
			auto& color = _color_info[_color_idx].color;
			color.b = pcr[0];
			color.g = pcr[1];
			color.r = pcr[2];
			
			//::ReleaseDC(NULL, hdc);
			std::string ss;
			for (int i = 0; i < 10; ++i) {
				if (_checkbox[i]->isChecked()) {
					ss += _color_info[i].color.tostr();
					ss += "-";
					ss += _color_info[i].df.tostr();
					ss += "|";
				}
			
			}
			if (ss.length() > 0 && ss.back() == L'|')
				ss.pop_back();
			ui.lineEdit_3->setText(QString::fromStdString(ss));
			update();
		}
		to_binary();
		_motive.hide();
		
		//qDebug() << "move" << event->x() << ":" << event->y();
	}
}

void Tool::on_state_changed(int st) {
	std::string ss;
	for (int i = 0; i < 10; ++i) {
		if (_checkbox[i]->isChecked()) {
			_color_info[i].df.str2color(_df_edit[i]->text().toStdWString());
			ss += _color_info[i].color.tostr();
			ss += "-";
			ss += _color_info[i].df.tostr();
			ss += "|";
		}

	}
	if (ss.length() > 0 && ss.back() == L'|')
		ss.pop_back();
	ui.lineEdit_3->setText(QString::fromStdString(ss));
	to_binary();
}

void Tool::mouseMoveEvent(QMouseEvent* event) {
	static DWORD rt = 0;
	if (_is_press&&clock()>rt+100) {
		rt = clock();
		//HDC hdc = ::GetDC(NULL);
		GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)_height, (LPBYTE)_imagedata.data(), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);
		auto global = mapToGlobal(event->pos());
		int x, y;
		for (int i = 0; i < my_dialog::crows; ++i) {
			for (int j = 0; j < my_dialog::ccols; ++j) {
				x = global.x() + j - my_dialog::ccols/2;
				y = global.y() + i - my_dialog::ccols/2;
				if (x >= 0 && y >= 0&&x<_width&&y<_height) {
					auto ptr = _imagedata.data()+ (_width * (_height-y) + x)*4;
					_motive._color[i*my_dialog::ccols + j] = QColor(ptr[2],ptr[1],ptr[0]);
				}
				
			}
		}
		//::ReleaseDC(NULL, hdc);
		auto ptr = _imagedata.data() + (_width * (_height - global.y()) + global.x()) * 4;
		_motive._label.setText(QString::asprintf("POS=%d,%d\r\nRGB=%02X%02X%02X",global.x(),global.y(), ptr[2], ptr[1], ptr[0]));
		_motive.move(global.x() + 20, global.y());
		_motive.update();
		//_motive.s
	}
}

void Tool::on_slider() {
	//qDebug() << ui.horizontalSlider->value();
	_ocr_sim = ui.horizontalSlider->value() / 100.;
	ui.label_6->setText(QString::asprintf("sim:%lf", _ocr_sim));
	std::wstring ss;
	bin_ocr(_binary, _record, _dict, _ocr_sim, ss);
	ui.textEdit->setText(QString::fromStdWString(ss));
}

void Tool::on_auto(bool checked) {
	to_binary();
}

void Tool::on_capture() {
	this->showMinimized();
	Sleep(300);
	release_res();
	capture_full_screen();
	_cap_dlg._src = QImage(_imagedata.data(), _width, _height, QImage::Format_RGB32).mirrored(false, true);
	_cap_dlg._img = _cap_dlg._src;
	
	_cap_dlg.AdjLight(0.4);
	_cap_dlg._screen_w = _width;
	//_cap_dlg._pscreen = _imagedata.data();
	_cap_dlg.show();
	_cap_dlg.showFullScreen();
	
}

void Tool::capture_full_screen() {
	_hdc = ::GetDC(NULL);
	RECT rc;

	::GetWindowRect(GetDesktopWindow(), &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	//qDebug() << _width << "," << _height;
	_hmdc = CreateCompatibleDC(_hdc); //创建一个与指定设备兼容的内存设备上下文环境	
	if (_hmdc == NULL) {
		//Tool::setlog("CreateCompatibleDC false");
		//return -2;
	}
	_hbmpscreen = CreateCompatibleBitmap(_hdc, _width, _height); //创建与指定的设备环境相关的设备兼容的位图

	_holdbmp = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //选择一对象到指定的设备上下文环境中


	GetObject(_hbmpscreen, sizeof(_bm), (LPSTR)&_bm); //得到指定图形对象的信息	
	//BITMAPINFOHEADER _bih;
	_bih.biBitCount = _bm.bmBitsPixel;//每个像素字节大小
	_bih.biCompression = BI_RGB;
	_bih.biHeight = _bm.bmHeight;//高度
	_bih.biPlanes = 1;
	_bih.biSize = sizeof(BITMAPINFOHEADER);
	_bih.biSizeImage = _bm.bmWidthBytes * _bm.bmHeight;//图像数据大小
	_bih.biWidth = _bm.bmWidth;//宽度
	BitBlt(_hmdc, 0, 0, _width, _height, _hdc, 0, 0, SRCCOPY);
	//if (_pimagedata)
	//	delete[] _pimagedata;
	//_pimagedata = new byte[_width*_height * 4];
	_imagedata.reserve(_width*_height * 4);
	//函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent Bitmap）使用的指定格式复制到一个缓冲区中
	GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)_height, (LPBYTE)_imagedata.data(), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);

}

void Tool::release_res() {
	if (_holdbmp&&_hmdc)
		_hbmpscreen = (HBITMAP)SelectObject(_hmdc, _holdbmp);
	//delete[dwLen_2]hDib;
	if (_hdc)DeleteDC(_hdc); _hdc = NULL;
	if (_hmdc)DeleteDC(_hmdc); _hmdc = NULL;

	if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
	if (_holdbmp)DeleteObject(_holdbmp); _holdbmp = NULL;
}

void Tool::to_mat() {
	int ch, cw;
	ch = _cap_dlg._rect.height();
	cw = _cap_dlg._rect.width();
	_src.create(ch,cw, CV_8UC3);
	uchar *p, *p2;
	int x1 = _cap_dlg._rect.left();
	int y1 = _cap_dlg._rect.top();
	for (int i = 0; i < ch; ++i) {
		p = _src.ptr<uchar>(i);
		p2 = _imagedata.data() + (_height - i - 1 - y1) * _width * 4 + x1 * 4;//偏移
		for (int j = 0; j < cw; ++j) {
			*p++ = *p2++; *p++ = *p2++;
			*p++ = *p2++; ++p2;
		}
	}
	cv::imwrite("_src.bmp", _src);
	_qimage.load("_src.bmp");
	ui.label_3->setPixmap(QPixmap::fromImage(_qimage));
	to_binary();
}