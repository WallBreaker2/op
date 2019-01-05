#pragma once
class Bkkeypad
{
public:
	Bkkeypad();
	~Bkkeypad();
	long Bind(HWND hwnd, long mode);
	long UnBind();
private:
	HWND _hwnd;
	int _mode;
};

