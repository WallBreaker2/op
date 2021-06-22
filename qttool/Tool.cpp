#include "Tool.h"

#include <QBrush>
#include <QMouseEvent>
#include <qmessagebox.h>
#include <qdebug.h>
#include <qdir.h>
#include <QFileDialog>
#include <Windows.h>
#include <fstream>
#include <qtextcodec.h>
#include "systemfont_dlg.h"
#include <qgridlayout.h>
#include <qfile>
Tool::Tool(QWidget* parent)
	: QWidget(parent), _cap_dlg(this),
	editDlg(this)

{
	edit_mode = 0;
	_is_press = 0;
	_ocr_sim = 1.0;
	ui.setupUi(this);
	m_getcolor_dlg = new get_color_dlg(_width, _height, _imagedata, this);
	//editDlg.pimg = &_imgloc;
	QPalette pe;
	pe.setColor(QPalette::Window, Qt::black);
	pe.setColor(QPalette::Base, Qt::black);
	pe.setColor(QPalette::WindowText, Qt::white);
	ui.label_src->setAutoFillBackground(true);
	ui.label_bin->setAutoFillBackground(true);
	ui.label_src->setPalette(pe);
	ui.label_bin->setPalette(pe);

	//---------------------create cotrol---------------------------

	create_control();
	//----------------------set layout----------------------------
	create_layout();
	awidget = new ArrayW(nullptr, _curr_word);
	ui.scrollArea->setWidget(awidget);
	//auto p = ui.groupBoxArray->pos();
	//auto rc = m_array_lb->rect();
	//scroll_area->setGeometry(p.x() + 5, p.y() + 30, rc.width() - 10, rc.height() - 10);
	//scroll_area->setWidget(awidget);

	_itemmodel = new QStandardItemModel(this);
	ui.tableView->setModel(_itemmodel);

	for (int i = 0; i < 10; i++) {
		auto p0 = new QStandardItem();
		p0->setData(QColor(0, 0, 0), Qt::BackgroundColorRole);
		_itemmodel->setItem(i, 0, p0);

		QString ss = QString::asprintf("%06x", 0);

		auto p1 = new QStandardItem();
		p1->setData(ss, Qt::EditRole);
		_itemmodel->setItem(i, 1, p1);
		auto p2 = new QStandardItem();
		p2->setData(ss, Qt::EditRole);
		_itemmodel->setItem(i, 2, p2);
		auto p3 = new QStandardItem();
		p3->setCheckable(true);
		p3->setEditable(false);
		_itemmodel->setItem(i, 3, p3);
	}
	_itemmodel->setHorizontalHeaderItem(0, new QStandardItem("select"));
	_itemmodel->setHorizontalHeaderItem(1, new QStandardItem("color"));
	_itemmodel->setHorizontalHeaderItem(2, new QStandardItem("delta"));
	_itemmodel->setHorizontalHeaderItem(3, new QStandardItem("check"));
	ui.tableView->setSelectionMode(QAbstractItemView::NoSelection);
	_model = nullptr;
	readCfg();
	//_pimagedata = nullptr;
	/*for (int i = 0; i < 10; i++) {
		QObject::connect(_df_edit[i], &QLineEdit::textChanged, this, &Tool::on_editChanged);
		QObject::connect(_checkbox[i], &QCheckBox::stateChanged, this, &Tool::on_state_changed);

	}*/
	connect(ui.tableView, &QTableView::clicked, this, &Tool::on_item_clicked);
	connect(_itemmodel, &QStandardItemModel::itemChanged, this, &Tool::on_item_changed);
	QObject::connect(ui.btnLoadImg, &QPushButton::clicked, this, &Tool::load_image);
	QObject::connect(ui.btnCapture, &QPushButton::clicked, this, &Tool::on_capture);
	QObject::connect(ui.btnEditImg, &QPushButton::clicked, this, &Tool::edit_image);
	QObject::connect(ui.btnSaveImg, &QPushButton::clicked, this, &Tool::save_image);


	QObject::connect(ui.pushButton_5, &QPushButton::clicked, this, &Tool::hist);
	//QObject::connect(ui.pushButton_6, &QPushButton::clicked, this, &Tool::show_next_char);
	QObject::connect(ui.btnFind, &QPushButton::clicked, this, &Tool::on_btnFind);
	QObject::connect(ui.listView, &QListView::clicked, this, &Tool::show_char);
	QObject::connect(ui.pushButton_7, &QPushButton::clicked, this, &Tool::load_dict);
	QObject::connect(ui.pushButton_8, &QPushButton::clicked, this, &Tool::save_dict);
	QObject::connect(ui.pushButton_9, &QPushButton::clicked, this, &Tool::edit_dict);
	QObject::connect(ui.lineEdit, &QLineEdit::editingFinished, this, &Tool::add_word);
	QObject::connect(ui.pushButton_6, &QPushButton::clicked, this, &Tool::del_word);
	QObject::connect(ui.doubleSpinBoxSim, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &Tool::on_simChanged);
	QObject::connect(ui.chkBk, &QCheckBox::clicked, this, &Tool::on_chkbk);

	QObject::connect(&_cap_dlg, &cap_dialog::sendCapture, this, &Tool::to_mat);
	QObject::connect(m_getcolor_dlg, &get_color_dlg::got_color, this, &Tool::got_color);
	QObject::connect(ui.pushButton_11, &QPushButton::clicked, this, &Tool::on_system_dict);
	QObject::connect(ui.btnnewdict, &QPushButton::clicked, this, &Tool::on_newdict);
	ui.listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	//connect(maptable,)

	//_motive.hide();
	//_motive.move(0, 0);
	_timer.setInterval(300);

}
void Tool::paintEvent(QPaintEvent* event) {


}

void Tool::resizeEvent(QResizeEvent*) {
	auto p = ui.group_parray->pos();
	auto rc = ui.group_parray->rect();
	qDebug() << rc;
	//int w = min(rc.width(), rc.height());
	//scroll_area->setGeometry(p.x() , p.y(), rc.width(), rc.height());
}

void Tool::create_control() {
	//m_array_lb = new QLabel(ui.group_parray);
	//m_array_desc = new QLabel("0x0");
	//m_array_desc->setAlignment(Qt::AlignHCenter);
	//m_array_desc->setMaximumHeight(20);
	const int chigh = 20, cwidth = 80;
	int w = ui.lineEdit->height();

}

void Tool::create_layout() {


}

void Tool::load_image() {

	auto dir = QFileDialog::getOpenFileName(this, "image path", "", tr("Images(*.png *.bmp *.jpg *.tif *.GIF)"));
	if (!dir.isEmpty()) {
		_imgloc._src.read(dir.toStdWString().data());
		if (!_imgloc._src.empty()) {
			//cv::cvtColor(_src, _src, CV_BGR2RGB);
			//QImage img = QImage(_src.data, _src.cols, _src.rows, QImage::Format_RGB888);

			//QMessageBox::about(this, "info", "load ok");
			//cv::imshow("SRC_IMAGE", _src);
			auto& img = _imgloc._src;
			//_imgloc._gray.fromImage4(img);
			//_qimage = _qbinary = QImage(_imgloc._gray.data(), img.width, img.height, img.width, QImage::Format_Grayscale8);
			_qimage = _qbinary = QImage(img.pdata, img.width, img.height, img.width * 4, QImage::Format_ARGB32);
			ui.label_src->setPixmap(QPixmap::fromImage(_qimage));
			to_binary();
		}
		else {
			QMessageBox::about(this, "info", "load false");
		}
	}

}

void Tool::edit_image() {
	editDlg.do_init(_imgloc._src);
	editDlg.exec();
}

void Tool::save_image() {
	if (_imgloc._src.empty())return;
	auto dir = QFileDialog::getSaveFileName(this, "path", "", tr("image(*.bmp"));
	if (!dir.isEmpty()) {
		_imgloc._src.write(dir.toStdWString().data());

	}
}

void Tool::to_binary() {

	if (_imgloc._src.empty())
		return;
	int ncols = _imgloc._src.width, nrows = _imgloc._src.height;
	//cv::Mat _gray;
	//cv::cvtColor(_src, _gray, CV_BGR2GRAY);
	//cv::imwrite("gray.bmp", _gray);

	std::vector<color_df_t> colors;
	for (int i = 0; i < 10; ++i) {
		if (_itemmodel->item(i, 3)->checkState() == 2)
			colors.push_back(_color_info[i]);
	}
	if (ui.chkBk->isChecked())
		_imgloc.bgr2binarybk(colors);
	else {
		_imgloc.bgr2binary(colors);
	}

	int A = 0;
	///0--------------
	auto bin = _imgloc._binary;
	for (auto& it : bin)
		if (it)it = 0xff;
	_qbinary = QImage(bin.data(), bin.width, bin.height, bin.width, QImage::Format_Grayscale8);
	//_imgloc._binary.write(L"_binary.bmp");
	//_qbinary.load("_binary.bmp");
	ui.label_bin->setPixmap(QPixmap::fromImage(_qbinary));
	std::map<point_t, std::wstring> ps;
	std::wstring ss;
	auto tempDict = file_dict;
	tempDict.sort_dict();
	_imgloc.bin_ocr(tempDict, _ocr_sim, ps);
	for (auto& it : ps)ss += it.second;
	ui.textEdit->setText(QString::fromStdWString(ss));


}

void Tool::hist() {

	if (_imgloc._binary.empty()) {
		return;
	}

	std::string pic_name;
	std::vector<rect_t> vroi;
	rect_t rc;
	auto& _binary = _imgloc._binary;
	rc.x1 = rc.y1 = 0;
	rc.x2 = _binary.width; rc.y2 = _binary.height;
	dict_new.clear();
	if (ui.checkBox_2->isChecked()) {
		auto orc = rc;

		_imgloc.bin_image_cut(2, rc, orc);
		//check is too large
		if (orc.width() > 255) {
			orc.x2 = orc.x1 + 255;
			rc = orc;
			_imgloc.bin_image_cut(2, rc, orc);
		}
		if (orc.height() > 255) {
			orc.y2 = orc.y1 + 255;
			rc = orc;
			_imgloc.bin_image_cut(2, rc, orc);
		}
		dict_new.add_word(_binary, orc);
	}
	else {
		_imgloc.get_rois(5, vroi);
		for (auto& it : vroi) {

			dict_new.add_word(_binary, it);
			//_chars.push_back(out_x[j]);
		}
	}

	edit_mode = 0;
	//edit_mode = 1;
	QStringList strlist;
	QString tp, t;
	for (auto& it : dict_new.words) {

		tp = QString::asprintf("|%d,%d,%d|", it.info.h, it.info.w, it.info.bit_cnt);
		tp = QString::fromWCharArray(it.info.name) + tp;
		int n = it.info.h;
		for (int j = 0; j <= it.data.size(); ++j) {

			t = QString::asprintf("%02X", it.data[j]);
			tp += t;
		}
		strlist.append(tp);
	}
	if (_model)
		delete _model;

	_model = new QStringListModel(strlist);

	ui.listView->setModel(_model);
	if (!dict_new.words.empty()) {


		QModelIndex index = _model->index(0);
		ui.listView->setCurrentIndex(index);
		//_curr_word = dict_new.words[0];
		//
		////update();
		//awidget->update();
		show_char(index);
	}
}


//void Tool::draw_line(const std::vector<int>& lines, QPainter& paint, int isy, QGroupBox* gp, Qt::GlobalColor cr) {
//	if (lines.empty())
//		return;
//	auto pt = gp->pos();
//	auto rect = gp->rect();
//	int w = rect.right() - rect.left();
//	int h = rect.bottom() - rect.top();
//	//int max_val = *std::max_element(lines.begin(), lines.end());
//	//if (max_val == 0)return;
//
//	int n = lines.size(), idx;
//	QPen pen;
//	pen.setColor(cr);
//	paint.setPen(pen);
//	if (isy) {
//		int x0 = pt.x(), y0 = pt.y() + h + 10;
//		for (int j = 0; j < w; ++j) {
//			idx = j % n;
//			if (lines[idx]) {
//				paint.drawLine(x0 + j, y0 - (lines[idx] % h), x0 + j, y0);
//			}
//		}
//	}
//	else {
//		int x0 = pt.x(), y0 = pt.y() + 10;
//		for (int i = 0; i < h; ++i) {
//			idx = i % n;
//			if (lines[idx]) {
//				paint.drawLine(x0, y0 + i, x0 + lines[idx] % w, y0 + i);
//			}
//		}
//	}
//
//}

void Tool::show_char(const QModelIndex& idx) {
	int id = idx.row();
	auto& cDict = edit_mode ? file_dict : dict_new;
	if (id >= 0 && id < cDict.words.size()) {
		_curr_word = cDict.words[id];
		QString text = _model->data(idx, Qt::DisplayRole).toString();
		int idx = text.indexOf("|");
		if (idx > 0)ui.lineEdit->setText(text.left(idx));
		//m_array_desc->setText(QString::asprintf("array(%dx%d)", _curr_word.info.h, _curr_word.info.w));
		//ui.groupBoxArray->setTitle(QString::asprintf("array(%dx%d)", _curr_word.info.h, _curr_word.info.w));
		//ui.lineEdit->setFocus();
		awidget->update();
	}
}

void Tool::on_newdict() {
	bool ok = QMessageBox::question(this, "", tr("new dict?")) == QMessageBox::Yes;
	if (ok) {
		file_dict.clear();
		dict_new.clear();
		ui.editPath->clear();
		edit_dict();
	}

}

void Tool::load_dict() {
	auto dir = QFileDialog::getOpenFileName(this, "dict path", "", tr("dict(*.dict *.txt)"));
	if (!dir.isEmpty()) {
		//QTextCodec::setCodecForTr(QTextCodec::codecForName("GBK"));
		QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
		QByteArray ba = dir.toLocal8Bit();
		//auto fname = dir.toStdString();
		file_dict.read_dict(ba.data());
		ui.editPath->setText(dir);
		std::wstring ss;

		to_binary();


		edit_dict();


	}
}

void Tool::save_dict() {
    QString dictfile = ui.editPath->text();

    if (dictfile.isEmpty() == false)
    {
        saved = true;
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
        QByteArray ba = dictfile.toLocal8Bit();
        file_dict.write_dict(ba.data());
    }
    else {
        auto dir = QFileDialog::getSaveFileName(this, "save path", "", tr("dict(*.dict)"));
        if (!dir.isEmpty()) {
            saved = true;
            QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
            QByteArray ba = dir.toLocal8Bit();
            file_dict.write_dict(ba.data());
        }
    }
}

void Tool::add_word() {
	auto str = ui.lineEdit->text();
	if (str.isEmpty())
		return;
	saved = false;
	auto& cDict = edit_mode ? file_dict : dict_new;
	int idx = ui.listView->currentIndex().row();
	if (idx >= 0 && idx < cDict.words.size()) {
		auto midx = ui.listView->currentIndex();
		QString text = _model->data(midx, Qt::DisplayRole).toString();
		int idx = text.indexOf("|");
		if (idx >= 0)
			text = text.replace(0, idx, str);
		_model->setData(midx, text, Qt::EditRole);
		_curr_word.set_chars(str.toStdWString());
		if (edit_mode == 0) {//append to file dict
			file_dict.add_word(_curr_word);
			ui.label_7->setText(QString::asprintf("Dict size %d", file_dict.size()));
		}
		//append or update to current
		cDict.add_word(_curr_word);
		ui.lineEdit->clear();
		idx = ui.listView->currentIndex().row();
		idx = (idx + 1) % _model->rowCount();
		QModelIndex next = _model->index(idx);
		ui.listView->setCurrentIndex(next);
		show_char(next);
		std::wstring ss;
		to_binary();
	}

}

void Tool::edit_dict() {

	edit_mode = 1;
	QStringList strlist;
	QString tp, t;

	for (auto& it : file_dict.words) {
		tp = QString::asprintf("|%d,%d,%d|", it.info.h, it.info.w, it.info.bit_cnt);
		tp = QString::fromWCharArray(it.info.name) + tp;
		int n = it.info.h;
		for (int j = 0; j < it.data.size(); ++j) {

			t = QString::asprintf("%02X", it.data[j]);
			tp += t;
		}
		strlist.append(tp);
	}
	ui.label_7->setText(QString::asprintf("Dict size %d", file_dict.size()));
	if (_model)
		delete _model;
	_model = new QStringListModel(strlist);
	ui.listView->setModel(_model);
}



void Tool::del_word() {
	auto& cDict = edit_mode ? file_dict : dict_new;
	int idx = ui.listView->currentIndex().row();
	if (idx >= 0 && idx < cDict.size()) {
		saved = false;
		_model->removeRow(idx);
		cDict.erase(file_dict.words[idx]);
		auto midx = ui.listView->currentIndex();
		show_char(midx);
		to_binary();
	}
}

//void Tool::mousePressEvent(QMouseEvent* event) {
//	//const int chigh = 30, cwidth = 30;
//	//qDebug("press");
//	//if (event->button() == Qt::LeftButton && event->pos().x() <= edit_color[0]->x() ) {
//	//	//
//	//	for (int i = 0; i < 10; i++) {
//	//		auto p = edit_color[i]->pos();
//
//	//		if (event->x() < p.x()-cwidth || event->x() > p.x() || event->y() < p.y() || event->y() > p.y() + chigh)
//	//			continue;
//	//		_is_press = 1;
//	//		_color_idx = i;
//	//		auto global = mapToGlobal(event->pos());
//	//		_motive.move(global.x() + 20, global.y());
//	//		_motive.show();
//	//		release_res();
//	//		capture_full_screen();
//	//		break;
//	//	}
//
//	//}
//}

void Tool::got_color(QPoint pt) {
	m_getcolor_dlg->hide();
	auto pcr = _imagedata.data() + (_width * (_height - pt.y()) + pt.x()) * 4;
	auto& color = _color_info[_color_idx].color;
	color.b = pcr[0];
	color.g = pcr[1];
	color.r = pcr[2];
	//QPixmap pix(50, 60);
	//pix.fill(QColor(color.r, color.g, color.b));
	//_itemmodel->item(_color_idx, 0)->setIcon(QIcon(pix));
	_itemmodel->item(_color_idx, 0)->setData(QColor(color.r, color.g, color.b), Qt::BackgroundColorRole);
	_itemmodel->item(_color_idx, 1)->setData(color.tostr().data(), Qt::EditRole);
	//edit_color[_color_idx]->setText(color.tostr().data());
	//::ReleaseDC(NULL, hdc);
	std::string ss;
	for (int i = 0; i < 10; ++i) {

		auto item = _itemmodel->item(_color_idx, 3);
		if (item->checkState() == Qt::Checked) {
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

	to_binary();
	//qDebug() << "move" << event->x() << ":" << event->y();
	//ui.tableView->setAttribute(Qt::WA_TransparentForMouseEvents, false);


}

void Tool::on_state_changed(int st) {
	std::string ss;
	if (ui.chkBk->isChecked()) {
		ss.append("@");
	}
	for (int i = 0; i < 10; ++i) {
		if (_itemmodel->item(i, 3)->checkState() == 2) {
			_color_info[i].df.str2color(_itemmodel->item(i, 2)->data(Qt::EditRole).toString().toStdWString());
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

//void Tool::mouseMoveEvent(QMouseEvent* event) {
//	
//}

void Tool::on_simChanged(double sim) {
	//qDebug() << ui.horizontalSlider->value();
	_ocr_sim = sim;
	//ui.label_6->setText(QString::asprintf("sim:%lf", _ocr_sim));
	//std::wstring ss;
	to_binary();
}

void Tool::on_chkbk(bool checked) {

	//to_binary();
	on_state_changed(0);
}

void Tool::on_capture() {
	this->showMinimized();
	Sleep(300);
	release_res();
	_cap_dlg.firstCap = true;
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
	_imagedata.reserve(_width * _height * 4);
	//函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent Bitmap）使用的指定格式复制到一个缓冲区中
	GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)_height, (LPBYTE)_imagedata.data(), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);

}

void Tool::release_res() {
	if (_holdbmp && _hmdc)
		_hbmpscreen = (HBITMAP)SelectObject(_hmdc, _holdbmp);
	//delete[dwLen_2]hDib;
	if (_hdc)DeleteDC(_hdc); _hdc = NULL;
	if (_hmdc)DeleteDC(_hmdc); _hmdc = NULL;

	if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
	if (_holdbmp)DeleteObject(_holdbmp); _holdbmp = NULL;
}

void Tool::to_mat() {
	int ch, cw;
	ch = _cap_dlg.srect.height();
	cw = _cap_dlg.srect.width();
	_imgloc._src.create(cw, ch);
	uchar* p, * p2;
	int x1 = _cap_dlg.srect.left();
	int y1 = _cap_dlg.srect.top();
	for (int i = 0; i < ch; ++i) {
		p = _imgloc._src.ptr<uchar>(i);
		p2 = _imagedata.data() + (_height - i - 1 - y1) * _width * 4 + x1 * 4;//偏移
		for (int j = 0; j < cw; ++j) {
			*p++ = *p2++; *p++ = *p2++;
			*p++ = *p2++; *p++ = *p2++;
		}
	}
	auto& img = _imgloc._src;
	_qimage = _qbinary = QImage(img.pdata, img.width, img.height, img.width * 4, QImage::Format_ARGB32);
	ui.label_src->setPixmap(QPixmap::fromImage(_qimage));
	to_binary();
}

void Tool::on_editChanged(const QString& s) {
	if (s.length() == 6) {
		on_state_changed(0);
	}
}

void Tool::on_btnFind() {
	QString t = ui.editFind->text();
	if (t.length()) {
		for (int i = 0; i < file_dict.size(); i++) {
			QString s = QString::fromWCharArray(file_dict.words[i].info.name);
			if (t == s) {

				QModelIndex next = _model->index(i);
				ui.listView->setCurrentIndex(next);
				break;
			}
		}
	}
}

void Tool::closeEvent(QCloseEvent* e) {
	if (!saved && file_dict.size())
		if (ui.editPath->text().isEmpty()
			&& QMessageBox::question(this, "quit", "the dict is not saved,save dict to new file?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
			save_dict();
		}
		else if (QMessageBox::question(this, "quit", "save dict to current file?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {


			auto s = ui.editPath->text().toStdString();
			if (s.length() == 0)s = "temp.dict";
			file_dict.write_dict(s);

		}
	writeCfg();
}

void Tool::on_system_dict() {
	systemfont_dlg dlg;
	int hr = dlg.exec();


}

void Tool::on_item_clicked(const QModelIndex& index) {
	if (index.column() == 0) {
		//_is_press = 1;
		//m_clicked_count = 1;
		_color_idx = index.row();
		qDebug("idx=%d", _color_idx);
		//ui.tableView->setAttribute(Qt::WA_TransparentForMouseEvents, true);
		//auto global = mapToGlobal(event->pos());
		//_motive.move(global.x() + 20, global.y());
		//this->setFocus();
		//_motive.show();
		release_res();
		capture_full_screen();
		m_getcolor_dlg->prepare();
		m_getcolor_dlg->show();
		m_getcolor_dlg->showFullScreen();
	/*	ui.tableView->clearFocus();
		m_getcolor_dlg->setFocus();*/
	}

}

void Tool::on_item_changed(const QStandardItem* item) {
	bool need = item->column() == 3
		|| item->data(Qt::EditRole).toString().length() == 6;
	if (item->column() == 1&& item->data(Qt::EditRole).toString().length() == 6) {
		QString s = item->data(Qt::EditRole).toString();
		qDebug("new color");
		_itemmodel->item(item->row(), 0)->setData(
			QColor(s.mid(0,2).toInt(nullptr,16),
				s.mid(2,2).toInt(nullptr,16),
				s.mid(4,2).toInt(nullptr,16)), 
			Qt::BackgroundColorRole);
	}
	if (need) {
		std::string ss;
		if (ui.chkBk->isChecked()) {
			ss.append("@");
		}
		for (int i = 0; i < 10; ++i) {
			if (_itemmodel->item(i, 3)->checkState() == 2) {
				_color_info[i].df.str2color(_itemmodel->item(i, 2)->data(Qt::EditRole).toString().toStdWString());
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
	else {

	}
}

void Tool::readCfg() {
	std::fstream f;
	f.open("cfg.txt");
	char buff[256];
	if (f.is_open()) {
		int idx = 0;
		while (f.getline(buff, 256)) {
			QString ss = buff;
			if (ss.indexOf("color-df:") == 0 && ss.length() >= 24 && idx < 10) {
				QColor cr;
				uchar r, g, b;
				sscanf(buff+9,"%02X%02X%02X", &r, &g, &b);
				_itemmodel->item(idx, 0)->setData(QColor(r, g, b), Qt::BackgroundColorRole);
				_itemmodel->item(idx, 1)->setData(ss.mid(9, 6),Qt::EditRole);
				_itemmodel->item(idx, 2)->setData(ss.mid(16, 6),Qt::EditRole);
				_itemmodel->item(idx, 3)->setData(ss.right(1).toInt(), Qt::CheckStateRole);

				_color_info[idx].color.str2color(string(buff + 9, buff + 15));
				_color_info[idx].df.str2color(string(buff + 16, buff + 22));
				idx++;
			}
			else if (ss.indexOf("path:") == 0) {
				ui.editPath->setText(ss.mid(5));
			}
			else {
				break;
			}
		}

		f.close();
		//std::string ss = buff;
		if (ui.editPath->text().right(4) == ".txt")
			file_dict.read_dict_dm(buff);
		else
			file_dict.read_dict(buff);
		if (file_dict.size())
			edit_dict();
	}
}

void Tool::writeCfg() {
	QFile f;
	f.setFileName("cfg.txt");
	f.open(QIODevice::WriteOnly);
	//f.open("cfg.txt", std::ios::out);
	QTextStream stream(&f);
	for (int i = 0; i < 10; i++) {
		stream << "color-df:";
	/*	qDebug() << _itemmodel->item(i, 1);
		qDebug() << _itemmodel->item(i, 2);
		qDebug() << _itemmodel->item(i, 3);*/
		stream << _itemmodel->item(i, 1)->data(Qt::EditRole).toString() << " ";
		stream <<_itemmodel->item(i, 2)->data(Qt::EditRole).toString() << " ";
		stream << _itemmodel->item(i, 3)->checkState() << "\n";

	}
	char buff[256];
	
		stream << "path:" << ui.editPath->text().toStdString().data() <<"\n";
		f.close();
	
}

