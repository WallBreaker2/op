#pragma once
#ifndef __DXBACKGROUND_H_
#define __DXBACKGROUND_H_

#include "IDisplay.h"
struct Image;
class opDxGL : public IDisplay
{
public:
	opDxGL();
	~opDxGL();
	//1
	long BindEx(HWND hwnd, long render_type) override;

	/*long UnBind(HWND hwnd);*/

	long UnBindEx() override;

	virtual bool requestCapture(int x1, int y1, int w, int h, Image &img) override;

	//nox mode
	long BindNox(HWND hwnd, long render_type);
	//
	long UnBindNox();

private:
	//blackbone::Process _process;
	wstring m_opPath;
};

#endif
