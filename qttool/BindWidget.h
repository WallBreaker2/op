#pragma once
#include <QtWidgets/QMainWindow>
#include <QPainter>
#include <qdialog.h>
#include <qmessagebox.h>
#include <qevent.h>
#include <qdebug.h>
#include <qpushbutton.h>
#include <qlabel.h>
class BindWidget:public QWidget
{
	Q_OBJECT
public:
	BindWidget(QWidget* parent, QPixmap& pixmap);
	void mouseReleaseEvent(QMouseEvent* event)override;
	void paintEvent(QPaintEvent* event)override;
 signals:
	void mouseClicked(QPoint pt);
private:
	QPixmap& m_pixmap;
};

