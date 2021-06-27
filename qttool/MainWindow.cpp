#include "MainWindow.h"
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <functional>
#include <QtWidgets/QFileDialog>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <QtWidgets/QTreeWidgetItem>
#include <qdir.h>
#include <qmessagebox.h>
#include <qdesktopservices.h>
#include "Tool.h"
#include "BindWidget.h"
#define URL_DOC "https://gitee.com/wallbreaker2/op/wikis/"
#define URL_UPDATE "https://gitee.com/wallbreaker2/op/releases"
using std::string;
QTextEdit* g_edit;
QComboBox* g_combox[4];
/*
function for lua traceprint int
*/
int lua_traceprint(const char* ss) {
	if (g_edit) {
		g_edit->append(ss);
		return 1;
	}
	return 0;

}

static int lua_print(lua_State* L) {
	int n = lua_gettop(L);  /* number of arguments */
	int i;
	lua_getglobal(L, "tostring");
	string fs;
	for (i = 1; i <= n; i++) {
		const char* s;
		size_t l;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tolstring(L, -1, &l);  /* get result */
		if (s == NULL)
			return luaL_error(L, "'tostring' must return a string to 'print'");
		if (i > 1) fs.push_back('\t');
		fs += s;
		lua_pop(L, 1);  /* pop result */
	}
	lua_traceprint(fs.c_str());
	return 0;
}

int lua_GetModuleHandle(const char* name)
{
	return (int)::GetModuleHandleA(name);
}

int lua_write_int(size_t address, int val) {
	*(int*)address = val;
	return 1;
}

int lua_append_item(int idx, const char* s) {
	if (s && 0 <= idx && idx < 4) {
		g_combox[idx]->addItem(s);
		return 1;
	}
	return 0;
}

const char htmlKeyWords[] =
"a abbr acronym address applet area b base basefont "
"bdo big blockquote body br button caption center "
"cite code col colgroup dd del dfn dir div dl dt em "
"fieldset font form frame frameset h1 h2 h3 h4 h5 h6 "
"head hr html i iframe img input ins isindex kbd label "
"legend li link map menu meta noframes noscript "
"object ol optgroup option p param pre q s samp "
"script select small span strike strong style sub sup "
"table tbody td textarea tfoot th thead title tr tt u ul "
"var xmlns "
"abbr accept-charset accept accesskey action align alink "
"alt archive axis background bgcolor border "
"cellpadding cellspacing char charoff charset checked cite "
"class classid clear codebase codetype color cols colspan "
"compact content coords "
"data datafld dataformatas datapagesize datasrc datetime "
"declare defer dir disabled enctype "
"face for frame frameborder "
"headers height href hreflang hspace http-equiv "
"id ismap label lang language link longdesc "
"marginwidth marginheight maxlength media method multiple "
"name nohref noresize noshade nowrap "
"object onblur onchange onclick ondblclick onfocus "
"onkeydown onkeypress onkeyup onload onmousedown "
"onmousemove onmouseover onmouseout onmouseup "
"onreset onselect onsubmit onunload "
"profile prompt readonly rel rev rows rowspan rules "
"scheme scope shape size span src standby start style "
"summary tabindex target text title type usemap "
"valign value valuetype version vlink vspace width "
"text password checkbox radio submit reset "
"file hidden image "
"public !doctype xml";

const char jsKeyWords[] =
"break case catch continue default "
"do else for function if return throw try var while";

const char vbsKeyWords[] =
"and as byref byval case call const "
"continue dim do each else elseif end error exit false for function global "
"goto if in loop me new next not nothing on optional or private public "
"redim rem resume select set sub then to true type while with "
"boolean byte currency date double integer long object single string type "
"variant";

const char* luaKeywords = {
	"and break do else elseif end for function if "
	"in local not or "
	"repeat return then until while"
};

const char* luaKeywords2 = {
	"false true nil"
};

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent),m_bindWindow(nullptr),_bind_ret(0)
{
	ui.setupUi(this);

	_mbtn = new Mbutton(ui.groupBox_2);
	_mbtn->resize(33, 28);
	_mbtn->move(400, 50);
	_mbtn->setStyleSheet("background-image: url(:/Tool/Resources/catch.bmp);");
	g_edit = ui.textEdit_2;
	g_combox[0] = ui.comboBox; g_combox[1] = ui.comboBox_2;
	g_combox[2] = ui.comboBox_3; g_combox[3] = ui.comboBox_4;
	_pressed = false;
	m_tool = new Tool();
	m_tool->hide();
	auto layout = qobject_cast<QVBoxLayout*>(ui.tab_5->layout());
	m_sci = new ScintillaEdit();
	layout->addWidget(m_sci);
	//m_sci->show();
	m_timer = new QTimer();
	m_timer->setInterval(300);


	//using setdllpathw_t = int(const wchar_t*, int);
	//------------------lua init------------------------
	lua_init();
	sci_init();
	//------------------event bind---------------------
	QObject::connect(ui.pushButton, &QPushButton::clicked, this, &MainWindow::load_script);
	QObject::connect(ui.pushButton_2, &QPushButton::clicked, this, &MainWindow::save_script);
	QObject::connect(ui.pushButton_3, &QPushButton::clicked, this, &MainWindow::run_script);
	QObject::connect(_mbtn, &Mbutton::on_mousemove, this, &MainWindow::on_mousemove);
	QObject::connect(ui.pushButton_5, &QPushButton::clicked, this, &MainWindow::bind_unbind);
	QObject::connect(ui.pushButton_6, &QPushButton::clicked, this, &MainWindow::capture);
	QObject::connect(ui.pushButton_7, &QPushButton::clicked, this, &MainWindow::getcolor);
	QObject::connect(ui.pushButton_8, &QPushButton::clicked, this, &MainWindow::findpic);
	QObject::connect(ui.pushButton_4, &QPushButton::clicked, this, &MainWindow::moveto);
	QObject::connect(ui.pushButton_9, &QPushButton::clicked, this, &MainWindow::keypress);
	QObject::connect(ui.treeWidget, &QTreeWidget::itemClicked, this, &MainWindow::on_item_clickd);
	QObject::connect(ui.actiontool, &QAction::triggered, this, [this](bool) {this->m_tool->show(); });
	QObject::connect(ui.actioninfowindow, &QAction::triggered, this, [this](bool) {this->ui.dockWidget->show(); });
	//QObject::connect(ui.actiondocument, &QAction::triggered, this, [this](bool) {this->m_tool->show(); });
	QObject::connect(m_timer, &QTimer::timeout, this, &MainWindow::getBindScreen);
	

	connect(m_sci, &ScintillaEdit::marginClicked, this, &MainWindow::marginClicked);
	setWindowTitle("OP Test-v0.4");
	//ui.treeWidget.
	//ui.treeWidget->add1
	ui.treeWidget->setHeaderHidden(true);
	
}

MainWindow::~MainWindow() {
	if (_L)
		lua_close(_L);
	_L = nullptr;
}



void MainWindow::lua_init() {
	/*auto dir=QFileDialog::getOpenFileName(this, tr("op dll path"), "", tr("dll(*.dll)"));
	auto hdll = LoadLibrary(L"opreg.dll");
	if (hdll) {
		using SetDllPathW_t = int(const wchar_t*);
		SetDllPathW_t* pf = (SetDllPathW_t*)GetProcAddress(hdll, "SetDllPathW");
		if (pf) {
			pf(dir.toStdWString().c_str());
		}
	}

	if (!dir.isEmpty())
		this->setWindowTitle(dir);
*/
	_L = luaL_newstate();
	luaL_openlibs(_L);

	luabridge::getGlobalNamespace(_L).addFunction("TracePrint", lua_traceprint);
	luabridge::getGlobalNamespace(_L).addCFunction("print", lua_print);
	luabridge::getGlobalNamespace(_L).addFunction("GetModuleHandle", lua_GetModuleHandle);
	luabridge::getGlobalNamespace(_L).addFunction("write_int", lua_write_int);
	luabridge::getGlobalNamespace(_L).addFunction("append_item", lua_append_item);
	//luabridge::getGlobalNamespace(_L).addFunction("setupA", setupA);
	//luaL_dostring(_L, "require(\"luacom\")");
	//do_string("require(\"luacom\");\n");
	//do_string("op = luacom.CreateObject(\"op.opsoft\");\n");
//	QString fname = QDir::currentPath() + "__init__.lua";
	if (luaL_dofile(_L, "__init__.lua")) {
		outputline(lua_tostring(_L, -1));
		lua_pop(_L, 1);
		QMessageBox::warning(this, "error",QString::fromLocal8Bit(""));
		exit(-1);
	}

}

void MainWindow::sci_init() {
	//global style
	m_sci->send(SCI_STYLESETFONT, STYLE_DEFAULT, (sptr_t)"Courier New");
	m_sci->send(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
	m_sci->send(SCI_STYLECLEARALL);
	//language exer
	m_sci->send(SCI_SETLEXER, SCLEX_LUA);
	m_sci->send(SCI_SETKEYWORDS, 0,
		reinterpret_cast<LPARAM>(luaKeywords));
	m_sci->send(SCI_SETKEYWORDS, 1,
		reinterpret_cast<LPARAM>(luaKeywords2));
	m_sci->send(SCI_STYLESETFORE, SCE_C_WORD, 0X00FF0000);//KEY
	m_sci->send(SCI_STYLESETFORE, SCE_C_WORD2, 0X00800080);//KEY
	m_sci->send(SCI_STYLESETBOLD, SCE_C_WORD2, TRUE);//KEY

	m_sci->send(SCI_STYLESETFORE, SCE_C_STRING, 0X001515A3);//STRING
	m_sci->send(SCI_STYLESETFORE, SCE_C_CHARACTER, 0X001515A3);//CHAR

	m_sci->send(SCI_STYLESETFORE, SCE_C_COMMENT, 0x00008000);//BLOCK
	m_sci->send(SCI_STYLESETFORE, SCE_C_COMMENTLINE, 0x00008000);// //
	m_sci->send(SCI_STYLESETFORE, SCE_C_COMMENTDOC, 0x00008000);// //

	m_sci->send(SCI_SETCARETLINEVISIBLE, TRUE);//
	m_sci->send(SCI_SETCARETLINEBACK, 0xb0ffff);// //
	/*m_sci->send(SCI_SETSTYLEBITS, 7);
	m_sci->*/
	m_sci->send(SCI_SETMARGINTYPEN, 2, SC_MARGIN_NUMBER);//line number
	m_sci->send(SCI_SETMARGINWIDTHN, 2, 20);
	m_sci->send(SCI_MARKERSETFORE,2,0xff0000);

	m_sci->send(SCI_SETTABWIDTH, 4);
	//=-------------------
	m_sci->send(SCI_SETPROPERTY, (sptr_t)"fold", (sptr_t)"1");
	m_sci->send(SCI_SETPROPERTY, (sptr_t)"fold", (sptr_t)"1");
	const int MARGIN_FOLD_INDEX = 0;
	m_sci->send(SCI_SETMARGINTYPEN, MARGIN_FOLD_INDEX, SC_MARGIN_SYMBOL);//ҳ������  
	m_sci->send(SCI_SETMARGINMASKN, MARGIN_FOLD_INDEX, SC_MASK_FOLDERS); //ҳ������  
	m_sci->send(SCI_SETMARGINWIDTHN, MARGIN_FOLD_INDEX, 11); //ҳ�߿���  
	m_sci->send(SCI_SETMARGINSENSITIVEN, MARGIN_FOLD_INDEX, TRUE); //��Ӧ�����Ϣ  

	// �۵���ǩ��ʽ  
	m_sci->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_CIRCLEPLUS);
	m_sci->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_CIRCLEMINUS);
	m_sci->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_CIRCLEPLUSCONNECTED);
	m_sci->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_CIRCLEMINUSCONNECTED);
	m_sci->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNERCURVE);
	m_sci->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	m_sci->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNERCURVE);

	// �۵���ǩ��ɫ  
	m_sci->send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERSUB, 0xa0a0a0);
	m_sci->send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERMIDTAIL, 0xa0a0a0);
	m_sci->send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERTAIL, 0xa0a0a0);

	m_sci->send(SCI_SETFOLDFLAGS, 16 | 4, 0); //����۵������۵��е����¸���һ������
	//m_sci->send(SCI_SETMARGINMASKN, 0, 0x01);

}

void MainWindow::marginClicked(int position, int modifiers, int margin) {
	int number = m_sci->send(SCI_LINEFROMPOSITION, position);
	m_sci->send(SCI_TOGGLEFOLD, number);
}


void MainWindow::run_script() {
	auto ss = m_sci->textRange(0,m_sci->textLength());
	//ui.textEdit_2->clear();
	do_string(ss.toStdString().c_str());


}

int MainWindow::outputline(const char* ss) {
	ui.textEdit_2->append(ss);
	//ui.textEdit_2->append("\r\n");
	return 0;
}

void MainWindow::load_script() {
	auto dir = QFileDialog::getOpenFileName(this, "lua path", "", tr("lua(*.lua)"));
	if (!dir.isEmpty()) {
		std::fstream file;
		file.open(dir.toStdString(), std::ios::in);
		if (file.is_open()) {
			std::ostringstream temp;
			temp << file.rdbuf();
			std::string s = temp.str();
			m_sci->clear();
			m_sci->appendText(s.length(), s.data());
			//ui.textEdit->setText(temp.str().c_str());
			ui.lineEdit->setText(dir);
		}

		file.close();

	}
}

void MainWindow::save_script() {
	auto dir = QFileDialog::getSaveFileName(this, "save path", "", tr("lua(*.lua)"));
	if (!dir.isEmpty()) {
		std::fstream file;
		file.open(dir.toStdString(), std::ios::out);
		if (file.is_open()) {
			file << m_sci->targetText().toStdString();
		}
		file.close();
	}
}


void MainWindow::timerEvent(QTimerEvent* event) {
	if (event->timerId() == _timer_id) {

	}
}

void MainWindow::get_window_info(HWND hwnd) {
	_hwnd = (int)hwnd;
	wchar_t buff[256];
	::GetWindowText(hwnd, buff, 256);
	//wchar_t classtext[256];
	ui.lineEdit_2->setText(QString::fromStdWString(buff));
	::GetClassName(hwnd, buff, 256);
	ui.lineEdit_3->setText(QString::fromStdWString(buff));
	ui.lineEdit_4->setText(QString::asprintf("%d", hwnd));
	DWORD id;
	::GetWindowThreadProcessId(hwnd, &id);
	ui.lineEdit_5->setText(QString::asprintf("%d", id));
}



void MainWindow::on_mousemove() {
	POINT pt;
	::GetCursorPos(&pt);
	auto hwnd = ::WindowFromPoint(pt);
	if (hwnd) {
		get_window_info(hwnd);
		/*FLASHWINFO info;
		info.cbSize = sizeof(FLASHWINFO);
		info.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
		info.dwTimeout = 0;
		info.hwnd = hwnd;
		info.uCount = 0xffffff;*/
		//::FlashWindow(hwnd,0);
		//::SetForegroundWindow(hwnd);
		add_tree_item();
	}
}

int MainWindow::do_string(const char* ss) {

	if (luaL_loadstring(_L, ss) != 0) {
		outputline(lua_tostring(_L, -1));
		lua_pop(_L, 1);
		return 0;
	}
	if (lua_pcall(_L, 0, 0, 0) != 0) {
		outputline(lua_tostring(_L, -1));
		lua_pop(_L, 1);
		return 0;
	}

	return 1;
}

void MainWindow::bind_unbind() {
	if (!_bind_ret) {
		qDebug() << "begin...";
		QString ss("bind_ret=op:BindWindow(");
		ss += QString::asprintf("%d", _hwnd);
		ss += ",\"" + ui.comboBox->currentText() + "\"";
		ss += ",\"" + ui.comboBox_2->currentText() + "\"";
		ss += ",\"" + ui.comboBox_3->currentText() + "\"";
		ss += ",0);\n";
		outputline(ss.toStdString().c_str());
		do_string(ss.toStdString().c_str());
		qDebug() << "BindWindow do_string end...";
		lua_getglobal(_L, "bind_ret");
		_bind_ret = lua_tointeger(_L, -1);
		if (_bind_ret == 1) {
			ui.pushButton_5->setText("UnBindWindow");
			outputline("1");
			//to do
			m_timer->start();
		}
		else {
			outputline("0");
			m_timer->stop();
		}
	}
	else {
		m_timer->stop();
		qDebug() << "UnBindWindow do_string...";
		do_string("op:UnBindWindow()\n");
		qDebug() << "UnBindWindow do_string end";
		outputline("op:UnBindWindow()");
		ui.pushButton_5->setText("BindWindow");
		_bind_ret = 0;
	}

}

void MainWindow::getBindScreen(){
	QString ss("ret,data,size=op:GetScreenDataBmp(0,0,2000,2000);");
	//outputline(ss.toStdString().c_str());
	do_string(ss.toStdString().c_str());
	lua_getglobal(_L, "ret");
	int ret = lua_tointeger(_L, -1);
	lua_getglobal(_L, "data");
	unsigned int data = lua_tointeger(_L, -1);
	lua_getglobal(_L, "size");
	int size = lua_tointeger(_L, -1);
	char buffer[256] = { 0 };
	sprintf(buffer, "%d,%x,%d", ret, data, size);
	//outputline(buffer);
	if (ret && data && size) {
		BITMAPFILEHEADER bfh = { 0 };//bmp file header
		BITMAPINFOHEADER bih = { 0 };//bmp info header
		memcpy(&bfh, (uchar*)data, sizeof(bfh));
		memcpy(&bih, (uchar*)data+ sizeof(bfh), sizeof(bih));
		uchar* pixelsData = (uchar*)data + sizeof(bfh) + sizeof(bih);
		if (m_bindWindow == nullptr) {
			m_bindWindow = new BindWidget(nullptr, m_bindWindowPixmap);
			ui.dockWidgetBindWindow->setWidget(m_bindWindow);
			QObject::connect(m_bindWindow, &BindWidget::mouseClicked, this, &MainWindow::do_click);
		}
		
		QImage img(pixelsData, bih.biWidth, abs(bih.biHeight), QImage::Format::Format_ARGB32);
		
	
	
		
	
	
		//m_bindWindow->setScaledContents(true);
		m_bindWindowPixmap = QPixmap::fromImage(img.mirrored(false));
		
		/*QPainter pant(&m_bindWindowPixmap);
		
		drawElement(pant);
		pant.end();
		m_bindWindow->setPixmap(m_bindWindowPixmap);*/
		
		m_bindWindow->show();
		m_bindWindow->update();
	}
	else {
		m_timer->stop();
	}
}

void MainWindow::drawElement(QPainter& pant) {
	
	for (int i = 0; i < m_element.size(); i++) {
		
		pant.setPen(Qt::red);
		pant.drawRect(m_element[i]);
	}
	
}

void MainWindow::do_click(QPoint p) {
	if (_bind_ret) {
		char buffer[256] = { 0 };
		int rx = p.x() , ry = p.y();
		sprintf(buffer, "ret=op:MoveTo(%d,%d);",rx , ry);
		do_string(buffer);
		outputline(buffer);
		do_string("ret=op:LeftClick();");
		outputline("ret=op:LeftClick();");
	}

}

void MainWindow::capture() {
	QString ss("ret=op:Capture(");
	ss += ui.lineEdit_6->text() + ",";
	ss += ui.lineEdit_7->text() + ",";
	ss += ui.lineEdit_8->text() + ",";
	ss += ui.lineEdit_9->text() + ",";
	ss += "\"screen.bmp\"";
	ss += ");\n";
	outputline(ss.toStdString().c_str());
	do_string(ss.toStdString().c_str());
	lua_getglobal(_L, "ret");
	int ret = lua_tointeger(_L, -1);
	if (ret == 1) {
		outputline("1");
	}
	else {
		outputline("0");
	}


}



void MainWindow::getcolor() {
	QString ss("retstr=op:GetColor(");
	ss += "" + ui.lineEdit_10->text() + ",";
	ss += "" + ui.lineEdit_11->text();
	ss += ");\n";
	outputline(ss.toStdString().c_str());
	do_string(ss.toStdString().c_str());
	lua_getglobal(_L, "retstr");
	auto retstr = lua_tostring(_L, -1);
	if (retstr) {
		outputline(retstr);
	}
	else {
		outputline("");
	}
}

void MainWindow::findpic() {
	QString ss("ret,ret_x,ret_y=op:FindPic(");
	ss += ui.lineEdit_12->text() + ",";
	ss += ui.lineEdit_13->text() + ",";
	ss += ui.lineEdit_14->text() + ",";
	ss += ui.lineEdit_15->text() + ",";
	ss += "\"" + ui.lineEdit_16->text() + "\",";
	ss += "\"" + ui.lineEdit_17->text() + "\",";
	ss += ui.lineEdit_18->text() + ",";
	ss += ui.lineEdit_19->text();
	ss += ");\n";
	outputline(ss.toStdString().c_str());
	do_string(ss.toStdString().c_str());
	lua_getglobal(_L, "ret");
	int ret = lua_tointeger(_L, -1);
	int x, y;
	lua_getglobal(_L, "ret_x");
	x = lua_tointeger(_L, -1);
	lua_getglobal(_L, "ret_y");
	y = lua_tointeger(_L, -1);
	char buff[20];
	sprintf(buff, "%d,%d,%d", ret, x, y);
	outputline(buff);
	QImage img(ui.lineEdit_16->text());
	qDebug() << img.size();
	if (ret != -1) {
		
		QRect rc(QPoint(x, y), img.size());
		m_element.push_back(rc);
	}
	else {
		m_element.clear();
	}
	/*auto pstr = "op:CapturePre(\"pre.bmp\")";
	do_string(pstr);
	outputline(pstr);*/
}

void MainWindow::moveto() {
	QString ss("ret=op:MoveTo(");
	ss += "" + ui.lineEdit_20->text() + ",";
	ss += "" + ui.lineEdit_21->text();
	ss += ");\n";
	outputline(ss.toStdString().c_str());
	do_string(ss.toStdString().c_str());
	lua_getglobal(_L, "ret");
	int ret = lua_tointeger(_L, -1);
	char buff[20];
	sprintf(buff, "%d", ret);
	outputline(buff);
	if (ui.checkBox->isChecked()) {
		ss = "ret=op:" + ui.comboBox_5->currentText() + "();";
		outputline(ss.toStdString().c_str());
		do_string(ss.toStdString().c_str());
		lua_getglobal(_L, "ret");
		ret = lua_tointeger(_L, -1);
		sprintf(buff, "%d", ret);
		outputline(buff);
	}
}

void MainWindow::keypress() {
	QString ss("ret=op:KeyPressChar(");
	ss += "\"" + ui.lineEdit_22->text() + "\");\n";
	outputline(ss.toStdString().c_str());
	do_string(ss.toStdString().c_str());
	lua_getglobal(_L, "ret");
	int ret = lua_tointeger(_L, -1);
	char buff[20];
	sprintf(buff, "%d", ret);
	outputline(buff);
}

void MEnumWindow(HWND parent, QTreeWidgetItem* pitem) {
	wchar_t buff[256];

	auto s = QString::asprintf("%08X  \"", parent);
	::GetWindowTextW(parent, buff, 256);
	s += QString::fromWCharArray(buff) + "\"";

	::GetClassNameW(parent, buff, 256);
	s += "  \"" + QString::fromWCharArray(buff) + "\"";
	pitem->setText(0, s);

	HWND chd = ::GetWindow(parent, GW_CHILD);
	if (chd) {
		for (HWND it = ::GetWindow(chd, GW_HWNDFIRST); it; it = ::GetWindow(it, GW_HWNDNEXT)) {
			auto itc = new QTreeWidgetItem;
			pitem->addChild(itc);

			MEnumWindow(it, itc);
		}

	}
}

void del_node(QTreeWidgetItem* node) {
	if (node) {
		for (int i = 0; i < node->childCount(); ++i)
			del_node(node->child(i));
		delete node;
	}
}

void MainWindow::add_tree_item() {


	auto old = ui.treeWidget->takeTopLevelItem(0);
	if (old)del_node(old);
	auto item = new QTreeWidgetItem;
	HWND hwnd = (HWND)_hwnd, pre, desk = ::GetDesktopWindow();
	pre = hwnd;
	while (hwnd && hwnd != desk)
	{
		pre = hwnd;
		hwnd = GetParent(hwnd);
	}
	MEnumWindow(pre, item);

	ui.treeWidget->insertTopLevelItem(0, item);
	ui.treeWidget->expandAll();
}

void MainWindow::on_item_clickd(QTreeWidgetItem* item, int col) {
	if (item) {
		auto s = item->text(0);

		if (s.length() > 8) {
			s = s.left(8);
			long hd = s.toLong(nullptr, 16);
			get_window_info((HWND)hd);

		}

	}
}

void MainWindow::on_actiondocument_triggered() {
	QDesktopServices::openUrl(QUrl(URL_DOC));
}

void MainWindow::on_pushButton_5_clicked() {
	qDebug("on_pushButton_5_clicked...");
}

void MainWindow::on_actionabout_triggered() {
	QMessageBox::about(this, "about", "op-tool v0.4.0\nAutohr:DeepFire\nEmail:784942619@qq.com\nBuild:2020-09-02\n");
}
void MainWindow::on_actionupdate_triggered() {
	QDesktopServices::openUrl(QUrl(URL_UPDATE));
}