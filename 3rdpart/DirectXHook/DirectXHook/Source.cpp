#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG
#pragma comment(lib, "hekate_d.lib")
#else
#pragma comment(lib, "hekate.lib")
#endif

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "D3dx9.lib")

#include <cstdio>
#include <memory>

#include <Windows.h>
#include <Dbghelp.h>

#include <d3d9.h>
#include <D3dx9core.h>
//#include <d3d9helper.h>

#include "Hook/InlineHook.h"
#include "Capstone/CapstoneDll.h"
#include <fstream>
void setlog(const char* pstr) {
	std::fstream file;
	file.open("hk.log", std::ios::app | std::ios::out);
	if (!file.is_open())
		return;
	file << pstr << std::endl;
	file.close();

}
std::unique_ptr<Hekate::Hook::InlineHook> pHook;

const DWORD_PTR GetAddressFromSymbols()
{
    BOOL success = SymInitialize(GetCurrentProcess(), nullptr, true);
    if (!success)
    {
        fprintf(stderr, "Could not load symbols for process.\n");
        return 0;
    }

    SYMBOL_INFO symInfo = { 0 };
    symInfo.SizeOfStruct = sizeof(SYMBOL_INFO);

    success = SymFromName(GetCurrentProcess(), "d3d9!CD3DBase::EndScene", &symInfo);
    if (!success)
    {
        fprintf(stderr, "Could not get symbol address.\n");
        return 0;
    }

    return (DWORD_PTR)symInfo.Address;
}

const bool Hook(const DWORD_PTR address, const DWORD_PTR hookAddress)
{
    pHook = std::unique_ptr<Hekate::Hook::InlineHook>(new Hekate::Hook::InlineHook(address, hookAddress));

    if (!pHook->Install())
    {
        fprintf(stderr, "Could not hook address 0x%X -> 0x%X\n", address, hookAddress);
    }

    return pHook->IsHooked();
}

HRESULT WINAPI EndSceneHook(void *pDevicePtr)
{
    using pFncOriginalEndScene = HRESULT (WINAPI *)(void *pDevicePtr);
    pFncOriginalEndScene EndSceneTrampoline =
        (pFncOriginalEndScene)pHook->TrampolineAddress();

    IDirect3DDevice9 *pDevice = (IDirect3DDevice9 *)pDevicePtr;
    ID3DXFont *pFont = nullptr;

    HRESULT result = D3DXCreateFont(pDevice, 30, 0, FW_NORMAL, 1, false,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Consolas", &pFont);
    if (FAILED(result))
    {
		setlog("Could not create font. Error ");
        fprintf(stderr, "Could not create font. Error = 0x%X\n", result);
    }
    else
    {
        RECT rect = { 0 };
        (void)SetRect(&rect, 0, 0, 300, 100);
        int height = pFont->DrawText(nullptr, L"Hello, World!", -1, &rect,
            DT_LEFT | DT_NOCLIP, -1);
        if (height == 0)
        {
			setlog("Could not draw text.\n");
            fprintf(stderr, "Could not draw text.\n");
        }
        (void)pFont->Release();
    }

    return EndSceneTrampoline(pDevicePtr);
}

const bool Initialize()
{
    DWORD_PTR EndSceneAddress = GetAddressFromSymbols();
    if (EndSceneAddress == 0)
    {
		setlog("Could not resolve EndScene address.\n");
        fprintf(stderr, "Could not resolve EndScene address.\n");
        return false;
    }

    return Hook(EndSceneAddress, (DWORD_PTR)EndSceneHook);
}

const bool Release()
{
    return pHook->Remove();
}

int APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        (void)DisableThreadLibraryCalls(hModule);
        if (AllocConsole())
        {
            freopen("CONOUT$", "w", stderr);
            SetConsoleTitle(L"Console");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            fprintf(stderr, "DLL loaded.\n");
			setlog("DLL loaded.\n");
        }

        Hekate::Capstone::ResolveCapstoneImports();

        Initialize();
    }
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
    {
        const bool unhooked = Release();
        if (!unhooked)
        {
			setlog("Could not remove hook.\n");
            fprintf(stderr, "Could not remove hook.\n");
        }
        if (!FreeConsole())
        {
			setlog("Could not free console.\n");
            fprintf(stderr, "Could not free console.\n");
        }

        break;
    }

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}
