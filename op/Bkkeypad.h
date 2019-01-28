#pragma once
class bkkeypad
{
public:
	bkkeypad();

	virtual ~bkkeypad();

	virtual long Bind(HWND hwnd, long mode);

	virtual long UnBind();

	virtual long GetKeyState(long vk_code);

	virtual long KeyDown(long vk_code);

	//virtual long GetKeyState(long vk_code);

	virtual long KeyUp(long vk_code);

	virtual long WaitKey(long vk_code,long time_out);
private:
	HWND _hwnd;
	int _mode;
};

