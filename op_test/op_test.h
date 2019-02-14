#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_op_test.h"
#include <lua.hpp>

class op_test : public QMainWindow
{
	Q_OBJECT

public:
	op_test(QWidget *parent = Q_NULLPTR);
	~op_test();
	void run_script();
	int outputline(const char* ss);
	void save_script();
private:
	Ui::op_testClass ui;
	lua_State* _L;
};


