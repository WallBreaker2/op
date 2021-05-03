#pragma once
#ifndef __BKDISPLAY_H_
#define __BKDISPLAY_H_
#include <thread>
#include "./core/optype.h"
#include "IDisplay.h"
struct Image;
class bkgdi:public IDisplay
{
public:
	bkgdi();
	~bkgdi();
	//��
	long BindEx(HWND _hwnd, long render_type) override;
	//long UnBind(HWND hwnd);
	//���
	long UnBindEx() override;
	
	
	//long updata_screen();

	//byte* get_data() override;

	virtual bool requestCapture(int x1, int y1, int w, int h, Image& img)override;
	
private:
	//�豸���
	HDC _hdc = NULL;

	HDC _hmdc = NULL;
	//λͼ���
	HBITMAP _hbmpscreen = NULL;
	HBITMAP _hbmp_old = NULL;
	//bmp �ļ�ͷ
	BITMAPFILEHEADER _bfh = { 0 };
	BITMAPINFOHEADER _bih = { 0 };//λͼ��Ϣͷ
	int dx_, dy_;//ȥ��������
	//bytearray temp_src;
	FrameInfo m_frameInfo;
	void fmtFrameInfo(void* dst,HWND hwnd, int w, int h);
};

#endif

