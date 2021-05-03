#pragma once
#ifndef __DX9HOOK_H_
#define __DX9HOOK_H_
#include "./core/globalVar.h"


namespace xhook {
	
	/*target window hwnd*/
	extern HWND render_hwnd;
	extern int render_type;
	/*name of ...*/
	extern wchar_t shared_res_name[256];
	extern wchar_t mutex_name[256];
	extern void* old_address;
	//
	int setup(HWND hwnd_, int render_type_);
	int release();


};
//���º�������HOOK DX9

//�˺��������¹���
/*
1.hook��غ���
2.���ù����ڴ�,������
3.��ͼ(hook)�������ڴ�
*/

//����ֵ:1 �ɹ���0ʧ��
DLL_API long SetXHook(HWND hwnd_,int render_type_);

DLL_API long UnXHook();
#endif // !__DX9HOOK_H_
