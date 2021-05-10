#pragma once
#include <qdialog.h>
#include "ui_system_font.h"
#include <qfont.h>
#include <qfontdialog.h>
#include "../libop/include/Dict.h"

class systemfont_dlg :
	public QDialog
{
public:
	systemfont_dlg();
	~systemfont_dlg();
	void on_select_font();
	QFont& font() {
		return _font;
	}
	void on_ok();
	void GeneratingPeriod(HDC hDC, wchar_t chText, HFONT hFont);
	void FontPeriodLoop(std::string fileName);
private:
	Ui::Dialog _ui;
	QFont _font;
	HFONT _hfont;
	Dict _dict;
};

