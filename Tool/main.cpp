#include "Tool.h"
#include <QtWidgets/QApplication>
#include <memory>
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	std::unique_ptr<Tool> w(new Tool);
	w->show();
	return a.exec();
}
