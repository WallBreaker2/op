#include "Tool.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Tool w;
	w.show();
	return a.exec();
}
