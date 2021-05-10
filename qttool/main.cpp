#include "Tool.h"
#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <memory>
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setWindowIcon(QIcon(":/Tool/Resources/tool.ico"));
	std::unique_ptr<MainWindow> w(new MainWindow);
	w->show();
	//op_test t;
	//t.show();
	return a.exec();
}
