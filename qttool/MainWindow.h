#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_op_test.h"
#include <lua.hpp>
//#include <QMouseEvent>
//#include <Qtimer>
#include "Mbutton.hpp"
#include "./scintlla/include/ScintillaEdit.h"
#include "./scintlla/include/Scintilla.h"
#include "./scintlla/include/SciLexer.h"
//#pragma comment(lib,"./scintlla/lib/ScintillaEdit4.lib")
class Tool;
class BindWidget;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = Q_NULLPTR);
	~MainWindow();
	void run_script();
	int outputline(const char* ss);
	void load_script();
	void save_script();



	void timerEvent(QTimerEvent* event);
	void lua_init();
	void sci_init();
	/*ִ��lua ����*/
	int do_string(const char* ss);
	void bind_unbind();
	void capture();
	void getcolor();
	void findpic();
	void moveto();
	void keypress();
	void add_tree_item();
	void get_window_info(HWND hwnd);
	void on_item_clickd(QTreeWidgetItem* item, int col);
	void getBindScreen();
	void drawElement(QPainter& pant);
	//void mouseReleaseEvent(QMouseEvent* event)override;
	
public Q_SLOTS:
	void on_mousemove();
	void marginClicked(int position, int modifiers, int margin);
	void on_actiondocument_triggered();
	void on_pushButton_5_clicked();
	void on_actionabout_triggered();
	void on_actionupdate_triggered();
	void do_click(QPoint p);
private:
	Ui::op_testClass ui;
	lua_State* _L;
	int _timer_id;
	bool _pressed;
	Mbutton* _mbtn;
	int _hwnd = 0;
	int _bind_ret = 0;
	Tool* m_tool;
	ScintillaEdit* m_sci;
	BindWidget* m_bindWindow;
	QPixmap m_bindWindowPixmap;
	QTimer* m_timer;
	QList<QRect> m_element;
};


