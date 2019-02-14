#include "op_test.h"
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <functional>
#include <qdir.h>
#include <QFileDialog>
#include <fstream>
#include <sstream>
QTextEdit* g_edit;
/*
function for lua traceprint int
*/
int mprint(const char* ss) {
	if (g_edit) {
		g_edit->append(ss);
		return 1;
	}
	return 0;

}
op_test::op_test(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	g_edit = ui.textEdit_2;
	_L = luaL_newstate();
	luaL_openlibs(_L);
	luabridge::getGlobalNamespace(_L).addFunction("mprint", mprint);
	QObject::connect(ui.pushButton, &QPushButton::clicked, this, [this]() {
		auto dir = QFileDialog::getOpenFileName(this, "lua path", "", tr("lua(*.lua)"));
		if (!dir.isEmpty()) {
			std::fstream file;
			file.open(dir.toStdString(), std::ios::in);
			if (file.is_open()) {
				std::ostringstream temp;
				temp << file.rdbuf();

				ui.textEdit->setText(temp.str().c_str());
				ui.lineEdit->setText(dir);
			}

			file.close();

		}
	});
	QObject::connect(ui.pushButton_2, &QPushButton::clicked, this, &op_test::save_script);
	QObject::connect(ui.pushButton_3, &QPushButton::clicked, this, &op_test::run_script);


}

op_test::~op_test() {
	lua_close(_L);
}


void op_test::run_script() {
	auto ss = ui.textEdit->toPlainText();
	ui.textEdit_2->clear();
	//luaL_dostring(_L, ss.toStdString().c_str());
	luaL_loadstring(_L, ss.toStdString().c_str());
	if (lua_pcall(_L, 0, 0, 0) != 0) {
		outputline(lua_tostring(_L, -1));
		lua_pop(_L, 1);
	}


}

int op_test::outputline(const char* ss) {
	ui.textEdit_2->append(ss);
	//ui.textEdit_2->append("\r\n");
	return 0;
}

void op_test::save_script() {
	auto dir = QFileDialog::getSaveFileName(this, "save path", "", tr("lua(*.lua)"));
	if (!dir.isEmpty()) {
		std::fstream file;
		file.open(dir.toStdString(), std::ios::out);
		if (file.is_open()) {
			file << ui.textEdit->toPlainText().toStdString();
		}
		file.close();
	}
}