#include "BindWidget.h"
#include <qpainter.h>
double xscale = 1.0, yscale = 1.0;
BindWidget::BindWidget(QWidget* parent, QPixmap& pixmap) :QWidget(parent) , m_pixmap(pixmap){

}

void BindWidget::paintEvent(QPaintEvent* event) {
	QPainter pant(this);
	xscale = 1.0, yscale = 1.0;
	if (m_pixmap.width() > 1200) {
		xscale = 1200.0 / m_pixmap.width();
		//qDebug("xscaled:%lf", xscale);
	}
	if (m_pixmap.height() > 800) {
		yscale = 800.0 / m_pixmap.height();
		//qDebug("yscale:%lf", yscale);
	}
	pant.scale(xscale, yscale);
	
	pant.drawPixmap(QPoint(0, 0), m_pixmap);

}

void BindWidget::mouseReleaseEvent(QMouseEvent* event) {
	//qDebug() << event->pos();
	emit mouseClicked(QPoint(event->pos().x()/xscale, event->pos().y()/yscale));
}