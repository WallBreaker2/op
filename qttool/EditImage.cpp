#include "EditImage.h"
#include <QMouseEvent>
#include <qpainter.h>
#include <qmessagebox.h>
#include <qpen.h>
#include <Eigen/dense>

EditImage::EditImage(QWidget* parent):QDialog(parent)
{
	ui.setupUi(this);
	// = _ui.okButton->font();
	//::connect(_ui.pushButton, &QPushButton::clicked, this, &systemfont_dlg::on_select_font);
	//QObject::connect(_ui.okButton, &QPushButton::clicked, this, &systemfont_dlg::on_ok);
	curImg = ib._src;
	rawImg = ib._src;
	viewdlg = new ViewDlg(this, curImg,index);

	ui.scrollArea->setWidget(viewdlg);

	QObject::connect(ui.btnPointer, &QPushButton::clicked, this, [&]() {
		index = 0;
		selected = 0;
		viewdlg->update();
		//to do...
		});
	QObject::connect(ui.btnRect, &QPushButton::clicked, this, [&]() {
		index = 1;
		//to do...
		});
	QObject::connect(ui.btnMagic, &QPushButton::clicked, this, [&]() {
		index = 2;
		//to do...
		});
	QObject::connect(ui.btnPen, &QPushButton::clicked, this, [&]() {
		index = 3;
		//to do...
		});
	QObject::connect(ui.btnFill, &QPushButton::clicked, this, [&]() {
		index = 4;
		//to do...
		on_fill();
		});
	QObject::connect(ui.btnColor, &QPushButton::clicked, this, [&]() {
		index = 5;
		//to do...
		});
	QObject::connect(ui.btnBig, &QPushButton::clicked, this, [&]() {
		index = 6;
		//to do...
		if (bigX < 64) {
			bigX *= 2;
			on_resize();
			update();
		}
			
		});
	QObject::connect(ui.btnSrc, &QPushButton::clicked, this, [&]() {
		index = 7;
		//to do...
		if (bigX != 1) {
			bigX = 1;
			on_resize();
			
		}
	
		});
	QObject::connect(ui.btnSmall, &QPushButton::clicked, this, [&]() {
		index = 8;
		//to do...
		if (bigX > 1) {
			bigX /= 2;
			on_resize();
			
		}
			
		});
	QObject::connect(ui.btnCut, &QPushButton::clicked, this, [&]() {
		index = 9;
		//to do...
		});

	QObject::connect(ui.btnRoate, &QPushButton::clicked, this,&EditImage::on_roate);

	QObject::connect(viewdlg, &ViewDlg::on_selected, this, [&]() {selected = 1; });
	
}


EditImage::~EditImage()
{
	
}
void EditImage::paintEvent(QPaintEvent*) {
	QPainter paint(this);

	
}

void EditImage::do_init(Image& img) {

	curImg = img;
	rawImg = img;
	selected = 0;
	on_resize();
}

void EditImage::on_resize() {
	using namespace Eigen;
	if (bigX != 1) {
		curImg.create(rawImg.width * bigX, rawImg.height * bigX);
		using matrix_t = Array<uint, Dynamic, Dynamic, RowMajor>;
		Map<matrix_t> mat(curImg.ptr<uint>(0), curImg.height, curImg.width);
		for (int i = 0; i < rawImg.height; i++) {
			for (int j = 0; j < rawImg.width; j++) {
				mat.block(i * bigX, j * bigX, bigX, bigX) = rawImg.at<uint>(i, j);
			}
		}

	}
	else {
		curImg = rawImg;
	}
	viewdlg->update();
	
}



void EditImage::on_fill() {
	if (!selected)return;
	int x0 = viewdlg->select_rc.x();
	int y0 = viewdlg->select_rc.y();
	int w = viewdlg->select_rc.width();
	int h = viewdlg->select_rc.height();
	x0 /= bigX;
	y0 /= bigX;
	w /= bigX;
	h /= bigX;
	qDebug("%d %d %d %d", x0, y0, w, h);
}

void EditImage::on_roate() {
	static double theta = 0.0;
	theta += 0.2;
	int cw = sqrt(rawImg.width * rawImg.width + rawImg.height * rawImg.height);
	if (cw == 0)return;
	curImg.create(cw, cw);
	for (int y = 0; y < cw; y++) {
		for (int x = 0; x < cw; x++) {
			
			int rx = (x -cw/2) * cos(theta) - (y-cw/2) * sin(theta) + rawImg.width/2;
			int ry = (x -cw/2) * sin(theta) + (y-cw/2) * cos(theta) +rawImg.height/2;
		
			if (0 <= rx && rx < rawImg.width && 0 <= ry && ry < rawImg.height)
				curImg.at<uint>(y, x) = rawImg.at<uint>(ry, rx);
			else
				curImg.at<uint>(y, x) = 0xffffffffu;
		}
	}
	viewdlg->update();
}

