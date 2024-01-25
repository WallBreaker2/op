#ifndef __FRAME_INFO_H_
#define __FRAME_INFO_H_
#pragma pack(1)
struct FrameInfo {
	unsigned __int64 hwnd;
	unsigned __int32 frameId;
	unsigned __int32 time;
	unsigned __int32 width;
	unsigned __int32 height;
	unsigned __int32 chk;
	void fmtChk() {
		chk = (hwnd >> 32) ^ (hwnd & 0xffffffffull) ^ frameId ^ time ^ width ^ height;
	}

	void format(HWND hwnd_, int w_, int h_) {
		hwnd = (unsigned __int64)hwnd_;
		frameId++;
		time = ::GetTickCount();
		width = w_;
		height = h_;
		fmtChk();
	}
	
};
#pragma pack()
#endif // !__FRAME_INFO_H_
