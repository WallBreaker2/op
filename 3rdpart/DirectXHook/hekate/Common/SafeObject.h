#pragma once

#include "Common/Common.h"

#include <Windows.h>

namespace Hekate
{
namespace Common
{

namespace SafeObjectCleanupFnc
{
bool ClnCloseHandle(const HANDLE &handle);
bool ClnFreeLibrary(const HMODULE &handle);
bool ClnLocalFree(const HLOCAL &handle);
bool ClnGlobalFree(const HGLOBAL &handle);
bool ClnUnmapViewOfFile(const PVOID &handle);
bool ClnCloseDesktop(const HDESK &handle);
bool ClnCloseWindowStation(const HWINSTA &handle);
bool ClnCloseServiceHandle(const SC_HANDLE &handle);
bool ClnVirtualFree(const PVOID &handle);
}

template <typename T, bool(*Cleanup)(const T &), PVOID InvalidValue>
class SafeObject final
{
public:
    SafeObject() : m_obj{ InvalidValue }
    {
    }

    SafeObject(const SafeObject &copy) = delete;

    SafeObject(const T &obj) : m_obj{ obj }
    {
    }

    SafeObject(SafeObject &&obj) : m_obj{ InvalidValue }
    {
        *this = std::move(obj);
    }

    ~SafeObject()
    {
        if (IsValid())
        {
            (void)Cleanup(m_obj);
        }
    }

    const bool IsValid() const
    {
        return m_obj != (T)InvalidValue;
    }

    SafeObject &operator=(const SafeObject &copy) = delete;

    SafeObject &operator=(SafeObject &&obj)
    {
        if (IsValid())
        {
            (void)Cleanup(m_obj);
        }

        m_obj = std::move(obj.m_obj);
        obj.m_obj = InvalidValue;

        return *this;
    }

    T * const Ptr()
    {
        return &m_obj;
    }

    const T operator()() const
    {
        return m_obj;
    }

private:
    T m_obj;
};

using SafeHandle = SafeObject<HANDLE, SafeObjectCleanupFnc::ClnCloseHandle, INVALID_HANDLE_VALUE>;
using SafeLibrary = SafeObject<HMODULE, SafeObjectCleanupFnc::ClnFreeLibrary, nullptr>;
using SafeLocal = SafeObject<HLOCAL, SafeObjectCleanupFnc::ClnLocalFree, nullptr>;
using SafeGlobal = SafeObject<HGLOBAL, SafeObjectCleanupFnc::ClnGlobalFree, nullptr>;
using SafeMapView = SafeObject<PVOID, SafeObjectCleanupFnc::ClnUnmapViewOfFile, nullptr>;
using SafeDesktop = SafeObject<HDESK, SafeObjectCleanupFnc::ClnCloseDesktop, nullptr>;
using SafeWindowStation = SafeObject<HWINSTA, SafeObjectCleanupFnc::ClnCloseWindowStation, nullptr>;
using SafeService = SafeObject<SC_HANDLE, SafeObjectCleanupFnc::ClnCloseServiceHandle, nullptr>;
using SafeVirtual = SafeObject<PVOID, SafeObjectCleanupFnc::ClnVirtualFree, nullptr>;

}
}