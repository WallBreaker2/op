//#include "stdafx.h"
#include "Bkgdi.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
#include <fstream>
#include "./include/Image.hpp"

bkgdi::bkgdi()
{
	_render_type = 0;
	dx_ = 0; dy_ = 0;
	//4*2^22=16*2^20=16MB
	//_image_data = new byte[MAX_IMAGE_WIDTH*MAX_IMAGE_WIDTH * 4];
}

bkgdi::~bkgdi()
{
	//SAFE_DELETE_ARRAY(_image_data);
}

long bkgdi::BindEx(HWND hwnd, long render_type) {
	if (!::IsWindow(hwnd))
		return 0;
	_hwnd = hwnd; _render_type = render_type;

	if (render_type == RDT_NORMAL) {
		RECT rc, rc2;
		::GetWindowRect(_hwnd, &rc);
		::GetClientRect(hwnd, &rc2);

		_width = rc2.right - rc2.left;
		_height = rc2.bottom - rc2.top;
		POINT pt = { 0 };
		::ClientToScreen(hwnd, &pt);
		dx_ = pt.x - rc.left;
		dy_ = pt.y - rc.top;
		_hdc = ::GetDC(::GetDesktopWindow());
	}
	else {//client size
		RECT rc,rc2;
		::GetWindowRect(_hwnd, &rc);
		::GetClientRect(hwnd, &rc2);
		_width = rc2.right - rc2.left;
		_height = rc2.bottom - rc2.top;
		POINT pt = { 0 };
		::ClientToScreen(hwnd, &pt);
		dx_ = pt.x - rc.left;
		dy_ = pt.y - rc.top;
		/*setlog("dx=%d dy=%d", dx_, dy_);*/
		if (_render_type == RDT_GDI) {
			_hdc = ::GetDC(_hwnd);
		}
		else {
			_hdc = ::GetDCEx(_hwnd, NULL, DCX_PARENTCLIP);
		}
	}


	if (_hdc == NULL) {
		setlog("hdc == NULL", _hdc);
		return 0;
	}

	//����һ����ָ���豸���ݵ��ڴ��豸�����Ļ���	
	_hmdc = CreateCompatibleDC(_hdc);
	if (_hmdc == NULL) {
		setlog("CreateCompatibleDC false");
		return -2;
	}

	//updata_screen();
	return 1;
}

//long bkgdi::UnBind(HWND hwnd) {
//	_hwnd = hwnd;
//	return UnBind();
//}

long bkgdi::UnBindEx() {
	//setlog("bkgdi::UnBindEx()");
	_hbmpscreen = (HBITMAP)SelectObject(_hmdc, _hbmp_old);
	//delete[dwLen_2]hDib;
	if (_hdc)DeleteDC(_hdc); _hdc = NULL;
	if (_hmdc)DeleteDC(_hmdc); _hmdc = NULL;

	if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
	//if (_hbmp_old)DeleteObject(_hbmp_old); _hbmp_old = NULL;
	return 1;
}







//byte* bkgdi::get_data() {
//	//����  ˢ������
//	//updata_screen();
//
//	return _shmem->data<byte>();
//}

//long bkgdi::updata_screen() {
//	//step 1.�ж� �����Ƿ����
//	if (!::IsWindow(_hwnd))
//		return 0;
//	if (_render_type == RDT_NORMAL) {//normal �����Ĵ�СΪʵ����Ҫ�Ĵ�С
//		//
//		int w = rect.right - rect.left;
//		int h = rect.bottom - rect.top;
//		_hbmpscreen = CreateCompatibleBitmap(_hdc, w, h); //������ָ�����豸������ص��豸���ݵ�λͼ
//		_hbmp_old = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //ѡ��һ����ָ�����豸�����Ļ�����
//
//		_bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
//		_bfh.bfSize = _bfh.bfOffBits + w * h * 4;
//		_bfh.bfType = static_cast<WORD>(0x4d42);
//
//		_bih.biBitCount = 32;//ÿ�������ֽڴ�С
//		_bih.biCompression = BI_RGB;
//		_bih.biHeight = h;//�߶�
//		_bih.biPlanes = 1;
//		_bih.biSize = sizeof(BITMAPINFOHEADER);
//		_bih.biSizeImage = w * h * 4;//ͼ�����ݴ�С
//		_bih.biWidth = w;//����
//
//		//��ָ����Դ�豸���������е����ؽ���λ�飨bit_block��ת��
//
//		RECT rc;
//		::GetWindowRect(_hwnd, &rc);
//		int src_x = rect.left + rc.left + dx_;
//		int src_y = rect.top + rc.top + dy_;
//		if (BitBlt(_hmdc, 0, 0, w, h, _hdc, src_x, src_y, SRCCOPY)) {
//			//ok
//		}
//		else {
//			setlog("error in bitbit");
//		}
//
//
//
//		//������ȡָ������λͼ��λ��Ȼ������һ��DIB���豸�޹�λͼ��Device-Independent Bitmap��ʹ�õ�ָ����ʽ���Ƶ�һ����������
//		_pmutex->lock();
//		GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)h, _shmem->data<byte>(), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);
//
//		_pmutex->unlock();
//
//		if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
//	}
//	else {//gdi ... ����printwindow ������ԭ�� ��ȡ��СΪʵ�ʵĴ��ڴ�С���ں����Ĵ����У���Ҫת���ɿͻ�����С
//		//
//		RECT rc;
//		::GetWindowRect(_hwnd, &rc);
//		int w = rc.right - rc.left;
//		int h = rc.bottom - rc.top;
//		//setlog("_w w=%d %d _h h=%d %d,dx=%d dy=%d", _width, w, _height, h, dx_, dy_);
//		_hbmpscreen = CreateCompatibleBitmap(_hdc, w, h); //������ָ�����豸������ص��豸���ݵ�λͼ
//		_hbmp_old = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //ѡ��һ����ָ�����豸�����Ļ�����
//
//		_bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
//		_bfh.bfSize = _bfh.bfOffBits + w * h * 4;
//		_bfh.bfType = static_cast<WORD>(0x4d42);
//
//		_bih.biBitCount = 32;//ÿ�������ֽڴ�С
//		_bih.biCompression = BI_RGB;
//		_bih.biHeight = h;//�߶�
//		_bih.biPlanes = 1;
//		_bih.biSize = sizeof(BITMAPINFOHEADER);
//		_bih.biSizeImage = w * h * 4;//ͼ�����ݴ�С
//		_bih.biWidth = w;//����
//
//		//��ָ����Դ�豸���������е����ؽ���λ�飨bit_block��ת��
//	
//		if (_render_type == RDT_GDI) {
//			::PrintWindow(_hwnd, _hmdc, 0);
//
//		}
//		else {
//			::UpdateWindow(_hwnd);
//			//::RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
//			::PrintWindow(_hwnd, _hmdc, 0);
//		}
//		//������ȡָ������λͼ��λ��Ȼ������һ��DIB���豸�޹�λͼ��Device-Independent Bitmap��ʹ�õ�ָ����ʽ���Ƶ�һ����������
//		temp_src.resize(_bih.biSizeImage);
//		GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)h, temp_src.data(), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);
//		_pmutex->lock();
//		auto pdst = _shmem->data<uchar>();
//		for (int i = 0; i < _height; i++) {
//			memcpy(pdst + i * _width * 4,
//				temp_src.data() + (i + 0) * w * 4 + dx_ * 4,
//				_width * 4);
//		}
//			
//		_pmutex->unlock();
//
//		if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
//		
//	}
//	return 1;
//
//}

bool bkgdi::requestCapture(int x1,int y1,int w,int h,Image& img) {
	//step 1.�ж� �����Ƿ����
	if (!::IsWindow(_hwnd))
		return 0;
	img.create(w, h);
	if (_render_type == RDT_NORMAL) {//normal �����Ĵ�СΪʵ����Ҫ�Ĵ�С
		//
	/*	int w = rect.right - rect.left;
		int h = rect.bottom - rect.top;*/
		_hbmpscreen = CreateCompatibleBitmap(_hdc, w, h); //������ָ�����豸������ص��豸���ݵ�λͼ
		_hbmp_old = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //ѡ��һ����ָ�����豸�����Ļ�����

		_bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		_bfh.bfSize = _bfh.bfOffBits + w * h * 4;
		_bfh.bfType = static_cast<WORD>(0x4d42);

		_bih.biBitCount = 32;//ÿ�������ֽڴ�С
		_bih.biCompression = BI_RGB;
		_bih.biHeight = h;//�߶�
		_bih.biPlanes = 1;
		_bih.biSize = sizeof(BITMAPINFOHEADER);
		_bih.biSizeImage = w * h * 4;//ͼ�����ݴ�С
		_bih.biWidth = w;//����

		//��ָ����Դ�豸���������е����ؽ���λ�飨bit_block��ת��

		RECT rc;
		::GetWindowRect(_hwnd, &rc);
		int src_x = x1 + rc.left + dx_;
		int src_y = y1 + rc.top + dy_;
		if (BitBlt(_hmdc, 0, 0, w, h, _hdc, src_x, src_y, SRCCOPY)) {
			//ok
		}
		else {
			setlog("error in bitbit");
		}



		//������ȡָ������λͼ��λ��Ȼ������һ��DIB���豸�޹�λͼ��Device-Independent Bitmap��ʹ�õ�ָ����ʽ���Ƶ�һ����������
		//_pmutex->lock();
		uchar* pshare = _shmem->data<byte>();
		fmtFrameInfo(pshare, _hwnd, w, h);
		GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)h,pshare+sizeof(FrameInfo), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);

		//_pmutex->unlock();

		if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;

		//�����ݿ�����Ŀ��ע��ʵ�������Ƿ���
		
		for (int i = 0; i < h; i++) {
			memcpy(img.ptr<uchar>(i), _shmem->data<byte>() + (h - 1 - i) * 4 * w, 4 * w);
		}
	}
	else {//gdi ... ����printwindow ������ԭ�� ��ȡ��СΪʵ�ʵĴ��ڴ�С���ں����Ĵ����У���Ҫת���ɿͻ�����С
		//
		RECT rc;
		::GetWindowRect(_hwnd, &rc);
		int ww = rc.right - rc.left;
		int wh = rc.bottom - rc.top;
		//setlog("_w w=%d %d _h h=%d %d,dx=%d dy=%d", _width, w, _height, h, dx_, dy_);
		_hbmpscreen = CreateCompatibleBitmap(_hdc, ww, wh); //������ָ�����豸������ص��豸���ݵ�λͼ
		_hbmp_old = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //ѡ��һ����ָ�����豸�����Ļ�����

		_bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		_bfh.bfSize = _bfh.bfOffBits + ww * wh * 4;
		_bfh.bfType = static_cast<WORD>(0x4d42);

		_bih.biBitCount = 32;//ÿ�������ֽڴ�С
		_bih.biCompression = BI_RGB;
		_bih.biHeight = wh;//�߶�
		_bih.biPlanes = 1;
		_bih.biSize = sizeof(BITMAPINFOHEADER);
		_bih.biSizeImage = ww * wh * 4;//ͼ�����ݴ�С
		_bih.biWidth = ww;//����

		//��ָ����Դ�豸���������е����ؽ���λ�飨bit_block��ת��

		if (_render_type == RDT_GDI) {
			::PrintWindow(_hwnd, _hmdc, 0);

		}
		else {
			::UpdateWindow(_hwnd);
			//::RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
			::PrintWindow(_hwnd, _hmdc, 0);
		}
		//������ȡָ������λͼ��λ��Ȼ������һ��DIB���豸�޹�λͼ��Device-Independent Bitmap��ʹ�õ�ָ����ʽ���Ƶ�һ����������
		//_pmutex->lock();
		uchar* pshare = _shmem->data<byte>();
		fmtFrameInfo(pshare, _hwnd, w, h);
		GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)wh, pshare+ sizeof(FrameInfo), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);

		if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;

		//�����ݿ�����Ŀ��ע��ʵ�������Ƿ���(ע��ƫ��)
		auto ppixels = _shmem->data<byte>()+sizeof(FrameInfo);
		for (int i = 0; i < h; i++) {
			memcpy(img.ptr<uchar>(i), ppixels + (wh - 1 - i - y1 - dy_) * 4 * ww + (x1 + dx_) * 4, 4 * w);
		}

	}
	return 1;

}
void bkgdi::fmtFrameInfo(void* dst,HWND hwnd, int w, int h) {
	m_frameInfo.hwnd = (unsigned __int64)hwnd;
	m_frameInfo.frameId++;
	m_frameInfo.time = ::GetTickCount();
	m_frameInfo.width = w;
	m_frameInfo.height = h;
	m_frameInfo.fmtChk();
	memcpy(dst, &m_frameInfo, sizeof(m_frameInfo));
}





