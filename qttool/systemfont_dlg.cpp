#include "systemfont_dlg.h"
#include <qmessagebox.h>
#include <Windows.h>
#include <qfiledialog.h>
#include <commdlg.h>
int swapBinary(int data)
{
	int sign = 1;
	int result = 0;
	for (int i = 0; i <= 31; i++)
	{
		result += ((data & (sign << i)) >> i) << (31 - i);
	}
	return result;
}

FIXED FixedFromDouble(double d)
{
	long l;
	l = (long)(d * 65536L);
	return *(FIXED *)&l;
}

//设置字体图形变换矩阵。
void SetMat(LPMAT2 lpMat)
{
	lpMat->eM11 = FixedFromDouble(1);
	lpMat->eM12 = FixedFromDouble(0);
	lpMat->eM21 = FixedFromDouble(0);
	lpMat->eM22 = FixedFromDouble(1);
}

systemfont_dlg::systemfont_dlg()
{
	_ui.setupUi(this);
	_font = _ui.okButton->font();
	QObject::connect(_ui.pushButton, &QPushButton::clicked, this, &systemfont_dlg::on_select_font);
	QObject::connect(_ui.okButton, &QPushButton::clicked, this, &systemfont_dlg::on_ok);
}


systemfont_dlg::~systemfont_dlg()
{
	
}

void systemfont_dlg::on_select_font() {

	CHOOSEFONT cf;            // common dialog box structure
	static LOGFONT lf;        // logical font structure
	static DWORD rgbCurrent;   // current text color
	HFONT hfont, hfontPrev;
	DWORD rgbPrev;

	// Initialize CHOOSEFONT
	ZeroMemory(&cf, sizeof(CHOOSEFONT));
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = (HWND)this->winId();
	cf.lpLogFont = &lf;
	cf.rgbColors = rgbCurrent;
	cf.Flags = CF_SCREENFONTS | CF_EFFECTS;

	if (ChooseFont(&cf) == TRUE) {
		if (_hfont)
			DeleteObject(_hfont);
		_hfont = CreateFontIndirect(cf.lpLogFont);
	}

}

void systemfont_dlg::on_ok() {
	auto dir = QFileDialog::getSaveFileName(this, "save path", "", tr("dict(*.dict)"));
	if (!dir.isEmpty()) {
		FontPeriodLoop(dir.toStdString());
	}
	
}

void systemfont_dlg::GeneratingPeriod(HDC hDC, wchar_t chText, HFONT hFont) {
	MAT2 mat2;
	SetMat(&mat2);
	GLYPHMETRICS gm;
	DWORD dwNeedSize = GetGlyphOutline(hDC, chText, GGO_BITMAP, &gm, 0, NULL, &mat2);
	if (dwNeedSize > 0 && dwNeedSize < 0xFFFF)
	{
		LPBYTE lpBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNeedSize);
		if (lpBuf)
		{
			GetGlyphOutline(hDC, chText, GGO_BITMAP, &gm, dwNeedSize, lpBuf, &mat2);
			word1_t _word;
			_word.info.w = gm.gmBlackBoxX;
			_word.info.h = gm.gmBlackBoxY;
			_word.info.name[0] = chText;
			_word.info.name[1] = L'\0';
			_word.init();
			int nByteCount = ((gm.gmBlackBoxX + 31) >> 5) << 2;
			int nIndex = 0;
			int ck = 0;
			int idx = 0;
			for (int j = 0; j < nByteCount; j++)
			{
				for (int k = 0; k < 8; k++)
				{
					int var = 0;
					for (int i = 0; i < gm.gmBlackBoxY; i++)
					{
						BYTE btCode = lpBuf[i*nByteCount + j];
						if (btCode & (0x80 >> k))
						{
							var |= (1 << i);
							ck++;
							//OutputDebugString(_T("●"));
							SET_BIT(_word.data[idx / 8], idx & 7);

						}
						else
						{
							var &= ~(1 << i);
							//OutputDebugString(_T("○"));
						}
						idx++;
					}
					//CString str;
					//str.Format(L" %02d%08X\r\n", k, swapBinary(var));
					//OutputDebugString(str);
					//_word.clines[nIndex] = swapBinary(var);
					if (nIndex >= gm.gmBlackBoxX)
					{
						//if (nIndex<31)
						//{
						//	nIndex++;
						//	_word.clines[nIndex] = 0x0;
						//}
						break;
					}
					nIndex++;
				}
			}
			_word.info.bit_cnt = ck;
			_dict.add_word(_word);
		}
	}
	
}

void systemfont_dlg::FontPeriodLoop(std::string fileName) {
	//CProgressCtrl *myProCtrl2 = (CProgressCtrl *)GetDlgItem(IDC_PROGRESS1);
	HDC hDC = ::GetDC((HWND)this->winId());
	HFONT hOldFont = (HFONT)SelectObject(hDC, (HFONT)_hfont);
	int type = _ui.comboBox->currentIndex();
		if (type == 0)//chinese
		{
			//myProCtrl2->SetRange(0, 20902);
			for (int i = 0; i < 20902; i++)
			{
				GeneratingPeriod(hDC, 0x4E00 + i, _hfont);
				
			}
		}
	if (type == 1||type==4)//char
	{
		
		for (int i = 0; i < 26; i++)
		{
			GeneratingPeriod(hDC, 0x41 + i, _hfont);
			
		}
		for (int i = 0; i < 26; i++)
		{
			GeneratingPeriod(hDC, 0x61 + i, _hfont);
			
		}
	}
	if (type == 2||type==4)//num
	{
		
		for (int i = 0; i < 10; i++)
		{
			GeneratingPeriod(hDC, 0x30 + i, _hfont);
			
		}
	}
	if (type == 3||type==4)//symbol
	{
		
		for (int i = 0; i < 16; i++)
		{
			GeneratingPeriod(hDC, 0x20 + i, _hfont);
			
		}
		for (int i = 0; i < 6; i++)
		{
			GeneratingPeriod(hDC, 0x3A + i, _hfont);
			
		}
		for (int i = 0; i < 6; i++)
		{
			GeneratingPeriod(hDC, 0x5B + i, _hfont);
		}
		for (int i = 0; i < 4; i++)
		{
			GeneratingPeriod(hDC, 0x7B + i, _hfont);
		}
	}
	SelectObject(hDC, hOldFont);
	::ReleaseDC((HWND)this->winId(), hDC);
	_dict.write_dict(fileName);
}