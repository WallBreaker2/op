#include "get_color_dlg.h"
#include <QBrush>
#include <QMouseEvent>

#include <qdebug.h>
get_color_dlg::get_color_dlg(int& w, int& h,
	std::vector<unsigned char>& img, QWidget* parent) :QDialog(parent, Qt::WindowFlags::enum_type::FramelessWindowHint),
	_width(w), _height(h), _imagedata(img) {
	_motive = new my_dialog(this);
	m_pt = this->cursor().pos();
	setMouseTracking(true);
}

void get_color_dlg::prepare() {
	m_src = QImage(_imagedata.data(), _width, _height, QImage::Format_RGB32).mirrored(false, true);
	m_pt = this->cursor().pos();
	_motive->show();
}

void get_color_dlg::paintEvent(QPaintEvent* event) {

	QPainter paint(this);
	paint.drawImage(0, 0, m_src);
}

//void get_color_dlg::mousePressEvent(QMouseEvent* event) {
//	
//}
//
//void get_color_dlg::mouseReleaseEvent(QMouseEvent* event) {
//	qDebug() << "released";
//	
//
//}

void get_color_dlg::mouseDoubleClickEvent(QMouseEvent* e) {
	
	_motive->hide();
	this->hide();
	got_color(m_pt);
}

void get_color_dlg::move_cursor(QPoint pos) {
	if (pos.x() < 0 || pos.x() >= _width)return;
	if (pos.y() < 0 || pos.y() >= _height)return;
	auto global = pos;
	m_pt = global;
	int x, y;
	for (int i = 0; i < my_dialog::crows; ++i) {
		for (int j = 0; j < my_dialog::ccols; ++j) {
			x = global.x() + j - my_dialog::ccols / 2;
			y = global.y() + i - my_dialog::crows / 2;
			if (x >= 0 && y >= 0 && x < _width && y < _height) {
				auto ptr = _imagedata.data() + (_width * (_height - 1 - y) + x) * 4;
				_motive->_color[i * my_dialog::ccols + j] = QColor(ptr[2], ptr[1], ptr[0]);
			}

		}
	}
	int w = my_dialog::ccols, h = my_dialog::crows;
	_motive->_color[(h - 2) / 2 * w + w / 2] = Qt::blue;
	_motive->_color[(h + 2) / 2 * w + w / 2] = Qt::blue;
	_motive->_color[h / 2 * w + (w - 2) / 2] = Qt::red;
	_motive->_color[h / 2 * w + (w + 2) / 2] = Qt::red;
	if (global.x() < _width && global.y() < _height) {
		auto ptr = _imagedata.data() + (_width * (_height - 1 - global.y()) + global.x()) * 4;

		_motive->_label.setText(QString::asprintf("POS=%d,%d\r\nRGB=%02X%02X%02X", global.x(), global.y(), ptr[2], ptr[1], ptr[0]));
		_motive->move(global.x() + 20, global.y());
		_motive->update();
		cursor().setPos(global);
	}
	
}

void get_color_dlg::mouseMoveEvent(QMouseEvent* event) {
	
	move_cursor(mapToGlobal(event->pos()));
}

void get_color_dlg::keyReleaseEvent(QKeyEvent* event) {
	if (event->key() == Qt::Key_Up) {
		auto pt=m_pt - QPoint(0, 1);
		move_cursor(pt);
	}
	else if (event->key() == Qt::Key_Down) {
		auto pt = m_pt + QPoint(0, 1);
		move_cursor(pt);
	}
	else if (event->key() == Qt::Key_Left) {
		auto pt = m_pt - QPoint(1, 0);
		move_cursor(pt);
	}
	else if (event->key() == Qt::Key_Right) {
		auto pt = m_pt + QPoint(1, 0);
		move_cursor(pt);
	}
}
