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
	
};
#pragma pack()
#endif // !__FRAME_INFO_H_
