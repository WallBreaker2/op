//#include "stdafx.h"
#include "opMouseDx.h"
#include "../core/globalVar.h"
#include "../core/helpfunc.h"
#include "BlackBone/Process/Process.h"
#include "BlackBone/Process/RPC/RemoteFunction.hpp"
#include "../core/opEnv.h"
#include "../HOOK/opMessage.h"
opMouseDx::opMouseDx()
	: _hwnd(NULL), _mode(0), _x(0), _y(0), _dpi(getDPI())
{
}

opMouseDx::~opMouseDx()
{
	_hwnd = NULL;
}

long opMouseDx::Bind(HWND h, int mode)
{
	_hwnd = h;
	_mode = mode;
	DWORD id;
	::GetWindowThreadProcessId(_hwnd, &id);

	//attach 进程
	blackbone::Process proc;
	NTSTATUS hr;

	hr = proc.Attach(id);
	long ret = 0;
	if (NT_SUCCESS(hr))
	{
		wstring dllname = opEnv::getOpName();
		//检查是否与插件相同的32/64位,如果不同，则使用另一种dll
		BOOL is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
		if (is64 != OP64)
		{
			dllname = is64 ? L"op_x64.dll" : L"op_x86.dll";
		}

		bool injected = false;
		//判断是否已经注入
		auto _dllptr = proc.modules().GetModule(dllname);
		auto mods = proc.modules().GetAllModules();
		if (_dllptr)
		{
			injected = true;
		}
		else
		{
			wstring opFile = opEnv::getBasePath() + L"\\" + dllname;
			if (::PathFileExistsW(opFile.data()))
			{
				auto iret = proc.modules().Inject(opFile);
				injected = (iret ? true : false);
			}
			else
			{
				setlog(L"file:<%s> not exists!", opFile.data());
			}
		}
		if (injected)
		{
			using my_func_t = long(__stdcall *)(HWND, int);
			auto PSetInputHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "SetInputHook");
			if (PSetInputHook)
			{
				//setlog("after MakeRemoteFunction");
				auto cret = PSetInputHook(_hwnd, _mode);
				//setlog("after pSetXHook");
				ret = cret.result();
				//setlog("after result");
			}
			else
			{
				setlog(L"remote function 'SetInputHook' not found.");
			}
		}
		else
		{
			setlog(L"Inject false.");
		}
	}
	else
	{
		setlog(L"attach false.");
	}

	proc.Detach();
	//setlog("after Detach");

	return 1;
}

long opMouseDx::UnBind()
{
	DWORD id;
	::GetWindowThreadProcessId(_hwnd, &id);

	//attach 进程
	blackbone::Process proc;
	NTSTATUS hr;

	hr = proc.Attach(id);
	long ret = 0;
	if (NT_SUCCESS(hr))
	{
		wstring dllname = opEnv::getOpName();
		//检查是否与插件相同的32/64位,如果不同，则使用另一种dll
		BOOL is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
		if (is64 != OP64)
		{
			dllname = is64 ? L"op_x64.dll" : L"op_x86.dll";
		}
		using my_func_t = long(__stdcall *)();
		auto pReleaseInputHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "ReleaseInputHook");
		if (pReleaseInputHook)
		{
			//setlog("after MakeRemoteFunction");
			auto cret = pReleaseInputHook();
			//setlog("after pSetXHook");
			ret = cret.result();
			//setlog("after result");
		}
		else
		{
			setlog(L"remote function 'ReleaseInputHook' not found.");
		}
	}
	else
	{
		setlog(L"attach false.");
	}

	proc.Detach();
	//setlog("after Detach");
	_hwnd = 0;
	_mode = 0;
	return 1;
}

long opMouseDx::GetCursorPos(long &x, long &y)
{
	BOOL ret = FALSE;
	POINT pt;
	ret = ::GetCursorPos(&pt);
	if (_hwnd != ::GetDesktopWindow())
	{
		ret = ::ScreenToClient(_hwnd, &pt);
	}
	x = pt.x;
	y = pt.y;
	return ret;
}

long opMouseDx::MoveR(int rx, int ry)
{
	return MoveTo(_x + rx, _y + ry);
}

long opMouseDx::MoveTo(int x, int y)
{
	x = x * _dpi;
	y = y * _dpi;
	long ret = 0;
	ret = ::SendMessage(_hwnd, OP_WM_MOUSEMOVE, 0, MAKELPARAM(x, y)) == 0 ? 1 : 0;

	_x = x, _y = y;
	return ret;
}

long opMouseDx::MoveToEx(int x, int y, int w, int h)
{

	if (w >= 2 && h >= 2)
		return MoveTo(x + rand() % w, y + rand() % h);
	else
		return MoveTo(x, y);
}

long opMouseDx::LeftClick()
{
	long ret = 0, ret2 = 0;

	///ret=::PostMessage(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));
	ret = ::SendMessageTimeout(_hwnd, OP_WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y), SMTO_BLOCK, 2000, nullptr);
	//ret = ::SendNotifyMessage(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));
	//::Sleep(100);
	//ret = ::SendMessage(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));
	ret2 = ::SendMessageTimeout(_hwnd, OP_WM_LBUTTONUP, 0, MAKELPARAM(_x, _y), SMTO_BLOCK, 2000, nullptr);

	return ret && ret2 ? 1 : 0;
}

long opMouseDx::LeftDoubleClick()
{
	long r1, r2;
	r1 = LeftClick();
	::Sleep(1);
	r2 = LeftClick();
	return r1 & r2 ? 1 : 0;
}

long opMouseDx::LeftDown()
{
	long ret = 0;

	ret = ::SendMessage(_hwnd, OP_WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));

	return ret;
}

long opMouseDx::LeftUp()
{
	long ret = 0;

	ret = ::SendMessage(_hwnd, OP_WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(_x, _y));

	return ret;
}

long opMouseDx::MiddleClick()
{
	long r1, r2;
	r1 = MiddleDown();
	::Sleep(1);
	r2 = MiddleUp();
	return r1 & r2 ? 1 : 0;
}

long opMouseDx::MiddleDown()
{
	long ret = 0;

	ret = ::SendMessage(_hwnd, OP_WM_MBUTTONDOWN, MK_MBUTTON, MAKELPARAM(_x, _y));

	return ret;
}

long opMouseDx::MiddleUp()
{
	long ret = 0;

	ret = ::SendMessage(_hwnd, OP_WM_MBUTTONUP, MK_MBUTTON, MAKELPARAM(_x, _y));

	return ret;
}

long opMouseDx::RightClick()
{
	long ret = 0;
	long r1, r2;

	r1 = ::SendMessage(_hwnd, OP_WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(_x, _y));
	r2 = ::SendMessage(_hwnd, OP_WM_RBUTTONUP, MK_RBUTTON, MAKELPARAM(_x, _y));
	ret = r1 == 0 && r2 == 0 ? 1 : 0;

	return ret;
}

long opMouseDx::RightDown()
{
	long ret = 0;

	ret = ::PostMessage(_hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(_x, _y)) == 0 ? 1 : 0;

	return ret;
}

long opMouseDx::RightUp()
{
	long ret = 0;

	ret = ::PostMessage(_hwnd, OP_WM_RBUTTONUP, MK_RBUTTON, MAKELPARAM(_x, _y));

	return ret;
}

long opMouseDx::WheelDown()
{
	long ret = 0;

	/*
		wParam
		The high-order word indicates the distance the wheel is rotated, 
		expressed in multiples or divisions of WHEEL_DELTA, which is 120. 
		A positive value indicates that the wheel was rotated forward, away from the user;
		a negative value indicates that the wheel was rotated backward, toward the user.
		The low-order word indicates whether various virtual keys are down.
		This parameter can be one or more of the following values.
		lParam
		The low-order word specifies the x-coordinate of the pointer,
		relative to the upper-left corner of the screen.
		The high-order word specifies the y-coordinate of the pointer, 
		relative to the upper-left corner of the screen.
		*/
	//If an application processes this message, it should return zero.
	ret = ::SendMessage(_hwnd, OP_WM_MOUSEWHEEL, MAKEWPARAM(-WHEEL_DELTA, 0), MAKELPARAM(_x, _y)) == 0 ? 1 : 0;

	return ret;
}

long opMouseDx::WheelUp()
{
	long ret = 0;

	ret = ::SendMessage(_hwnd, OP_WM_MOUSEWHEEL, MAKEWPARAM(WHEEL_DELTA, 0), MAKELPARAM(_x, _y)) == 0 ? 1 : 0;

	return ret;
}
