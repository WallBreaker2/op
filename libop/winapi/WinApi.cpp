//#include "stdafx.h"
#include "WinApi.h"

#include <Tlhelp32.h>
#include <psapi.h>

#include <memory>
#pragma comment(lib, "psapi.lib")

WinApi::WinApi(void) {
  retstringlen = 0;
  WindowVerion = 0;
  IsEuemprosuccess = 0;
  memset(npid, 0, MAX_PATH);
}

WinApi::~WinApi(void) {}

BOOL WinApi::EnumProcessbyName(DWORD dwPID, LPCWSTR ExeName, LONG type) {
  if (IsEuemprosuccess == 0) {
    int nItem = 0;  // 项计数
    PROCESSENTRY32 pe32 = {sizeof(PROCESSENTRY32)};
    HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return FALSE;
    if (::Process32First(hProcessSnap, &pe32)) {
      do {
        if (type == 1) {
          if (wcsstr(pe32.szExeFile, ExeName) != NULL)  //模糊匹配
          {
            npid[nItem] = pe32.th32ProcessID;
            IsEuemprosuccess++;
            nItem++;
          }
        } else {
          if (!_wcsicmp(pe32.szExeFile, ExeName)) {
            npid[nItem] = pe32.th32ProcessID;
            IsEuemprosuccess++;
            nItem++;
          }
        }

      } while (::Process32Next(hProcessSnap, &pe32));
    }
    ::CloseHandle(hProcessSnap);
    if (IsEuemprosuccess > 0) return TRUE;
  } else {
    for (int i = 0; i < IsEuemprosuccess; i++) {
      if (dwPID == npid[i]) return TRUE;
    }
  }

  return FALSE;
}

DWORD WinApi::FindChildWnd(HWND hchile, const wchar_t *title,
                           const wchar_t *classname, wchar_t *retstring,
                           bool isGW_OWNER, bool isVisible,
                           const wchar_t *process_name) {
  hchile = ::GetWindow(hchile, GW_HWNDFIRST);
  while (hchile != NULL) {
    if (isGW_OWNER)  //判断是否要匹配所有者窗口为0的窗口,即顶级窗口
      if (::GetWindow(hchile, GW_OWNER) != 0) {
        hchile = ::GetWindow(hchile, GW_HWNDNEXT);  //获取下一个窗口
        continue;
      }

    if (isVisible)  //判断是否匹配可视窗口
      if (::IsWindowVisible(hchile) == false) {
        hchile = ::GetWindow(hchile, GW_HWNDNEXT);  //获取下一个窗口
        continue;
      }
    if (title == NULL && classname == NULL) {
      if (process_name) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hchile, &pid);
        if (EnumProcessbyName(pid, process_name)) {
          if (retstring) {
            if (retstringlen == 0) retstringlen = wcslen(retstring);
            if (retstringlen > 1)
              swprintf(retstring, L"%s,%d", retstring, hchile);
            else
              swprintf(retstring, L"%d", hchile);
          } else
            return (DWORD)hchile;
        }
      } else {
        if (retstring) {
          int len = wcslen(retstring);
          if (len > 1)
            swprintf(retstring, L"%s,%d", retstring, hchile);
          else
            swprintf(retstring, L"%d", hchile);
        } else
          return (DWORD)hchile;
      }
    } else if (title != NULL && classname != NULL) {
      wchar_t WindowClassName[MAX_PATH] = {0};
      ::GetClassName(hchile, WindowClassName, MAX_PATH);
      wchar_t WindowTitle[MAX_PATH] = {0};
      ::GetWindowText(hchile, WindowTitle, MAX_PATH);
      if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
        wchar_t *strfindclass = wcsstr(WindowClassName, classname);  //模糊匹配
        wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
        if (strfindclass && strfindtitle) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(hchile, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (retstring) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, hchile);
                else
                  swprintf(retstring, L"%d", hchile);
              } else
                return (DWORD)hchile;
            }
          } else {
            if (retstring) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, hchile);
              else
                swprintf(retstring, L"%d", hchile);
            } else
              return (DWORD)hchile;
          }
        }
      }

    } else if (title != NULL) {
      wchar_t WindowTitle[MAX_PATH] = {0};
      ::GetWindowText(hchile, WindowTitle, MAX_PATH);
      if (wcslen(WindowTitle) > 1) {
        wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
        if (strfind) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(hchile, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (retstring) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, hchile);
                else
                  swprintf(retstring, L"%d", hchile);
              } else
                return (DWORD)hchile;
            }
          } else {
            if (retstring) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, hchile);
              else
                swprintf(retstring, L"%d", hchile);
            } else
              return (DWORD)hchile;
          }
        }
      }

    } else if (classname != NULL) {
      wchar_t WindowClassName[MAX_PATH] = {0};
      ::GetClassName(hchile, WindowClassName, MAX_PATH);
      if (wcslen(WindowClassName) > 1) {
        wchar_t *strfind = wcsstr(WindowClassName, classname);  //模糊匹配
        if (strfind) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(hchile, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (retstring) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, hchile);
                else
                  swprintf(retstring, L"%d", hchile);
              } else
                return (DWORD)hchile;
            }
          } else {
            if (retstring) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, hchile);
              else
                swprintf(retstring, L"%d", hchile);
            } else
              return (DWORD)hchile;
          }
        }
      }
    }

    HWND hchilechile = ::GetWindow(hchile, GW_CHILD);
    if (hchilechile != NULL) {
      DWORD dret = FindChildWnd(hchilechile, title, classname, retstring,
                                isGW_OWNER, isVisible, process_name);
      if (dret > 0) break;
    }

    hchile = ::GetWindow(hchile, GW_HWNDNEXT);  //获取下一个窗口
  }
  return 0;
}

// TSEnumWindow:filter整形数: 取值定义如下
//
// 1 : 匹配窗口标题,参数title有效
//
// 2 : 匹配窗口类名,参数class_name有效.
//
// 4 : 只匹配指定父窗口的第一层孩子窗口
//
// 8 : 匹配所有者窗口为0的窗口,即顶级窗口
//
// 16 : 匹配可见的窗口
//
// 32 : 匹配出的窗口按照窗口打开顺序依次排列
bool WinApi::EnumWindow(HWND parent, const wchar_t *title,
                        const wchar_t *class_name, LONG filter,
                        wchar_t *retstring, const wchar_t *process_name) {
  bool bret = false;
  bool bZwindow = false;  //匹配出的窗口按照窗口打开顺序依次排列
  if (parent == 0) {
    parent = GetDesktopWindow();
  }
  if (filter > 32) {
    bZwindow = true;  //说明要排序窗口句柄
    filter = filter - 32;
  }

  DWORD procpids[MAX_PATH] = {0};
  int indexpid = 0;
  if (process_name)  // EnumWindowByProcess
  {
    if (wcslen(process_name) < 1) return false;
    memset(npid, 0, MAX_PATH);
    IsEuemprosuccess = 0;
    if (EnumProcessbyName(0, process_name) == false) return false;
  }

  DWORD processpid = 0;
  retstringlen = 0;
  switch (filter) {
    case 0:  //所有模式
    {
      if (process_name)  // EnumWindowByProcess
      {
        return false;
      }

      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (retstringlen == 0) retstringlen = wcslen(retstring);
        if (retstringlen > 1)
          swprintf(retstring, L"%s,%d", retstring, p);
        else
          swprintf(retstring, L"%d", p);
        bret = true;
        HWND hchile = ::GetWindow(p, GW_CHILD);
        if (hchile != NULL) {
          FindChildWnd(hchile, NULL, NULL, retstring);
        }

        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 1:  // 1 : 匹配窗口标题,参数title有效
    {
      if (wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        wchar_t WindowTitle[MAX_PATH] = {0};
        ::GetWindowText(p, WindowTitle, MAX_PATH);
        if (wcslen(WindowTitle) > 1) {
          wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
          if (strfind) {
            if (process_name)  // EnumWindowByProcess
            {
              DWORD pid = 0;
              GetWindowThreadProcessId(p, &pid);
              if (EnumProcessbyName(pid, process_name)) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            } else {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, NULL, retstring);
              }
            }
          }
        }

        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 2:  // 2 : 匹配窗口类名,参数class_name有效.
    {
      if (wcslen(class_name) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        wchar_t WindowClassName[MAX_PATH] = {0};
        ::GetClassName(p, WindowClassName, MAX_PATH);
        if (wcslen(WindowClassName) > 1) {
          wchar_t *strfind = wcsstr(WindowClassName, class_name);  //模糊匹配
          if (strfind) {
            if (process_name)  // EnumWindowByProcess
            {
              DWORD pid = 0;
              GetWindowThreadProcessId(p, &pid);
              if (EnumProcessbyName(pid, process_name)) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            } else {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, NULL, class_name, retstring);
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 3:  // 1.窗口标题+2.窗口类名
    {
      if (wcslen(class_name) < 1 && wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        wchar_t WindowClassName[MAX_PATH] = {0};
        ::GetClassName(p, WindowClassName, MAX_PATH);
        wchar_t WindowTitle[MAX_PATH] = {0};
        ::GetWindowText(p, WindowTitle, MAX_PATH);
        if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
          wchar_t *strfindclass =
              wcsstr(WindowClassName, class_name);             //模糊匹配
          wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
          if (strfindclass && strfindtitle) {
            if (process_name)  // EnumWindowByProcess
            {
              DWORD pid = 0;
              GetWindowThreadProcessId(p, &pid);
              if (EnumProcessbyName(pid, process_name)) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            } else {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, class_name, retstring);
              }
            }
          }
        }

        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 4:  // 4 : 只匹配指定父窗口的第一层孩子窗口
    {
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (process_name)  // EnumWindowByProcess
        {
          DWORD pid = 0;
          GetWindowThreadProcessId(p, &pid);
          if (EnumProcessbyName(pid, process_name)) {
            if (processpid !=
                pid)  //只匹配指定映像的所对应的第一个进程.
                      //可能有很多同映像名的进程，只匹配第一个进程的.
            {
              if (indexpid < IsEuemprosuccess) {
                indexpid++;
                processpid = pid;
                memset(retstring, 0, retstringlen);  //清空返回字符串
                retstringlen = 0;
              }
            }
            if (processpid == pid) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, NULL, NULL, retstring, false, false,
                             process_name);
              }
            }
          }
        } else {
          if (retstringlen == 0) retstringlen = wcslen(retstring);
          if (retstringlen > 1)
            swprintf(retstring, L"%s,%d", retstring, p);
          else
            swprintf(retstring, L"%d", p);
          bret = true;
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 5:  // 1.匹配窗口标题+//4 : 只匹配指定父窗口的第一层孩子窗口
    {
      if (wcslen(title) < 1) return false;

      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (process_name)  // EnumWindowByProcess
        {
          DWORD pid = 0;
          GetWindowThreadProcessId(p, &pid);
          if (EnumProcessbyName(pid, process_name)) {
            if (processpid !=
                pid)  //只匹配指定映像的所对应的第一个进程.
                      //可能有很多同映像名的进程，只匹配第一个进程的.
            {
              if (indexpid < IsEuemprosuccess) {
                indexpid++;
                processpid = pid;
                memset(retstring, 0, retstringlen);  //清空返回字符串
                retstringlen = 0;
              }
            }
            if (processpid == pid) {
              wchar_t WindowTitle[MAX_PATH] = {0};
              ::GetWindowText(p, WindowTitle, MAX_PATH);
              if (wcslen(WindowTitle) > 1) {
                if (wcsstr(WindowTitle, title)) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, NULL, retstring, false, false,
                             process_name);
              }
            }
          }

        } else {
          wchar_t WindowTitle[MAX_PATH] = {0};
          ::GetWindowText(p, WindowTitle, MAX_PATH);
          if (wcslen(WindowTitle) > 1) {
            if (wcsstr(WindowTitle, title)) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
            }
          }
        }

        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 6:  // 2 : 匹配窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口
    {
      if (wcslen(class_name) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (process_name)  // EnumWindowByProcess
        {
          DWORD pid = 0;
          GetWindowThreadProcessId(p, &pid);
          if (EnumProcessbyName(pid, process_name)) {
            if (indexpid < IsEuemprosuccess) {
              indexpid++;
              processpid = pid;
              memset(retstring, 0, retstringlen);  //清空返回字符串
              retstringlen = 0;
            }
          }
          if (processpid == pid) {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            if (wcslen(WindowClassName) > 1) {
              if (wcsstr(WindowClassName, class_name)) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, NULL, class_name, retstring, false, false,
                           process_name);
            }
          }
        } else {
          wchar_t WindowClassName[MAX_PATH] = {0};
          ::GetClassName(p, WindowClassName, MAX_PATH);
          if (wcslen(WindowClassName) > 1) {
            if (wcsstr(WindowClassName, class_name)) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 7:  // 1.窗口标题+2.窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口
    {
      if (wcslen(class_name) < 1 && wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (process_name)  // EnumWindowByProcess
        {
          DWORD pid = 0;
          GetWindowThreadProcessId(p, &pid);
          if (EnumProcessbyName(pid, process_name)) {
            if (indexpid < IsEuemprosuccess) {
              indexpid++;
              processpid = pid;
              memset(retstring, 0, retstringlen);  //清空返回字符串
              retstringlen = 0;
            }
          }
          if (processpid == pid) {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfindclass && strfindtitle) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, title, class_name, retstring, false, false,
                           process_name);
            }
          }
        } else {
          wchar_t WindowClassName[MAX_PATH] = {0};
          ::GetClassName(p, WindowClassName, MAX_PATH);
          wchar_t WindowTitle[MAX_PATH] = {0};
          ::GetWindowText(p, WindowTitle, MAX_PATH);
          if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
            wchar_t *strfindclass =
                wcsstr(WindowClassName, class_name);             //模糊匹配
            wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
            if (strfindclass && strfindtitle) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
            }
          }
        }

        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 8:  // 8 : 匹配所有者窗口为0的窗口,即顶级窗口
    {
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, NULL, NULL, retstring, true, false,
                             process_name);
              }
            }
          } else {
            if (retstringlen == 0) retstringlen = wcslen(retstring);
            if (retstringlen > 1)
              swprintf(retstring, L"%s,%d", retstring, p);
            else
              swprintf(retstring, L"%d", p);
            bret = true;
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, NULL, NULL, retstring, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 9:  // 1.窗口标题+8 : 匹配所有者窗口为0的窗口,即顶级窗口
    {
      if (wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowTitle[MAX_PATH] = {0};
              ::GetWindowText(p, WindowTitle, MAX_PATH);
              if (wcslen(WindowTitle) > 1) {
                wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
                if (strfind) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, NULL, retstring, true, false,
                             process_name);
              }
            }
          } else {
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowTitle) > 1) {
              wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, title, NULL, retstring, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 10:  // 2.窗口类名+8 : 匹配所有者窗口为0的窗口,即顶级窗口
    {
      if (wcslen(class_name) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowClassName[MAX_PATH] = {0};
              ::GetClassName(p, WindowClassName, MAX_PATH);
              if (wcslen(WindowClassName) > 1) {
                wchar_t *strfind =
                    wcsstr(WindowClassName, class_name);  //模糊匹配
                if (strfind) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, NULL, class_name, retstring, true, false,
                             process_name);
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            if (wcslen(WindowClassName) > 1) {
              wchar_t *strfind =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }

            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, NULL, class_name, retstring, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 11:  ////1.窗口标题+2.窗口类名+8 : 匹配所有者窗口为0的窗口,即顶级窗口
    {
      if (wcslen(class_name) < 1 && wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      if (p == NULL) return false;
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowClassName[MAX_PATH] = {0};
              ::GetClassName(p, WindowClassName, MAX_PATH);
              wchar_t WindowTitle[MAX_PATH] = {0};
              ::GetWindowText(p, WindowTitle, MAX_PATH);
              if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                wchar_t *strfindclass =
                    wcsstr(WindowClassName, class_name);  //模糊匹配
                wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
                if (strfindclass && strfindtitle) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, class_name, retstring, true, false,
                             process_name);
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfindclass && strfindtitle) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, title, class_name, retstring, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 12:  // //4 : 只匹配指定父窗口的第一层孩子窗口+8 :
              // 匹配所有者窗口为0的窗口,即顶级窗口
    {
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, NULL, NULL, retstring, true, false,
                               process_name);
                }
              }
            }
          } else {
            if (retstringlen == 0) retstringlen = wcslen(retstring);
            if (retstringlen > 1)
              swprintf(retstring, L"%s,%d", retstring, p);
            else
              swprintf(retstring, L"%d", p);
            bret = true;
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 13:  // 1.窗口标题+4 : 只匹配指定父窗口的第一层孩子窗口+8 :
              // 匹配所有者窗口为0的窗口,即顶级窗口
    {
      if (wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowTitle[MAX_PATH] = {0};
                ::GetWindowText(p, WindowTitle, MAX_PATH);
                if (wcslen(WindowTitle) > 1) {
                  wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
                  if (strfind) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, title, NULL, retstring, true, false,
                               process_name);
                }
              }
            }
          } else {
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowTitle) > 1) {
              wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 14:  // 2.窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口+8 :
              // 匹配所有者窗口为0的窗口,即顶级窗口
    {
      if (wcslen(class_name) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowClassName[MAX_PATH] = {0};
                ::GetClassName(p, WindowClassName, MAX_PATH);
                if (wcslen(WindowClassName) > 1) {
                  wchar_t *strfind =
                      wcsstr(WindowClassName, class_name);  //模糊匹配
                  if (strfind) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, NULL, class_name, retstring, true, false,
                               process_name);
                }
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            if (wcslen(WindowClassName) > 1) {
              wchar_t *strfind =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 15:  ////1.窗口标题+2.窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口+8 :
              ///匹配所有者窗口为0的窗口,即顶级窗口
    {
      if (wcslen(class_name) < 1 && wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowClassName[MAX_PATH] = {0};
                ::GetClassName(p, WindowClassName, MAX_PATH);
                wchar_t WindowTitle[MAX_PATH] = {0};
                ::GetWindowText(p, WindowTitle, MAX_PATH);
                if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                  wchar_t *strfindclass =
                      wcsstr(WindowClassName, class_name);  //模糊匹配
                  wchar_t *strfindtitle =
                      wcsstr(WindowTitle, title);  //模糊匹配
                  if (strfindclass && strfindtitle) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, title, class_name, retstring, true,
                               false, process_name);
                }
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfindclass && strfindtitle) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 16:  //匹配可见的窗口
    {
      parent = GetDesktopWindow();
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p)) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, NULL, NULL, retstring, false, true,
                             process_name);
              }
            }
          } else {
            if (retstringlen == 0) retstringlen = wcslen(retstring);
            if (retstringlen > 1)
              swprintf(retstring, L"%s,%d", retstring, p);
            else
              swprintf(retstring, L"%d", p);
            bret = true;
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, NULL, NULL, retstring, false, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 17:  // 1.窗口标题+//匹配可见的窗口
    {
      if (wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p)) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowTitle[MAX_PATH] = {0};
              ::GetWindowText(p, WindowTitle, MAX_PATH);
              if (wcslen(WindowTitle) > 1) {
                wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
                if (strfind) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, NULL, retstring, false, true,
                             process_name);
              }
            }
          } else {
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowTitle) > 1) {
              wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, title, NULL, retstring, false, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 18:  // 2.窗口类名+//匹配可见的窗口
    {
      if (wcslen(class_name) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p)) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowClassName[MAX_PATH] = {0};
              ::GetClassName(p, WindowClassName, MAX_PATH);
              if (wcslen(WindowClassName) > 1) {
                wchar_t *strfind =
                    wcsstr(WindowClassName, class_name);  //模糊匹配
                if (strfind) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, NULL, class_name, retstring, false, true,
                             process_name);
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            if (wcslen(WindowClassName) > 1) {
              wchar_t *strfind =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, NULL, class_name, retstring, false, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 19:  ////1.窗口标题+2.窗口类名+匹配可见的窗口
    {
      if (wcslen(class_name) < 1 && wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p)) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowClassName[MAX_PATH] = {0};
              ::GetClassName(p, WindowClassName, MAX_PATH);
              wchar_t WindowTitle[MAX_PATH] = {0};
              ::GetWindowText(p, WindowTitle, MAX_PATH);
              if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                wchar_t *strfindclass =
                    wcsstr(WindowClassName, class_name);  //模糊匹配
                wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
                if (strfindclass && strfindtitle) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, class_name, retstring, false, true,
                             process_name);
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfindclass && strfindtitle) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, title, class_name, retstring, false, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 20:  // 4 : 只匹配指定父窗口的第一层孩子窗口+匹配可见的窗口
    {
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p)) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, NULL, NULL, retstring, false, true,
                               process_name);
                }
              }
            }
          } else {
            if (retstringlen == 0) retstringlen = wcslen(retstring);
            if (retstringlen > 1)
              swprintf(retstring, L"%s,%d", retstring, p);
            else
              swprintf(retstring, L"%d", p);
            bret = true;
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 21:  // 1.窗口标题+4 : 只匹配指定父窗口的第一层孩子窗口+匹配可见的窗口
    {
      if (wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p)) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowTitle[MAX_PATH] = {0};
                ::GetWindowText(p, WindowTitle, MAX_PATH);
                if (wcslen(WindowTitle) > 1) {
                  wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
                  if (strfind) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, title, NULL, retstring, false, true,
                               process_name);
                }
              }
            }
          } else {
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowTitle) > 1) {
              wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 22:  // 2.窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口+匹配可见的窗口
    {
      if (wcslen(class_name) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p)) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowClassName[MAX_PATH] = {0};
                ::GetClassName(p, WindowClassName, MAX_PATH);
                if (wcslen(WindowClassName) > 1) {
                  wchar_t *strfind =
                      wcsstr(WindowClassName, class_name);  //模糊匹配
                  if (strfind) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, NULL, class_name, retstring, false, true,
                               process_name);
                }
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            if (wcslen(WindowClassName) > 1) {
              wchar_t *strfind =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 23:  // 1.窗口标题+2.窗口类名+4 :
              // 只匹配指定父窗口的第一层孩子窗口+匹配可见的窗口
    {
      if (wcslen(class_name) < 1 && wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p)) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowClassName[MAX_PATH] = {0};
                ::GetClassName(p, WindowClassName, MAX_PATH);
                wchar_t WindowTitle[MAX_PATH] = {0};
                ::GetWindowText(p, WindowTitle, MAX_PATH);
                if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                  wchar_t *strfindclass =
                      wcsstr(WindowClassName, class_name);  //模糊匹配
                  wchar_t *strfindtitle =
                      wcsstr(WindowTitle, title);  //模糊匹配
                  if (strfindclass && strfindtitle) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, title, class_name, retstring, false,
                               true, process_name);
                }
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfindclass && strfindtitle) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 24:  // 8 : 匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (retstringlen == 0) retstringlen = wcslen(retstring);
              if (retstringlen > 1)
                swprintf(retstring, L"%s,%d", retstring, p);
              else
                swprintf(retstring, L"%d", p);
              bret = true;
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, NULL, NULL, retstring, true, true,
                             process_name);
              }
            }
          } else {
            if (retstringlen == 0) retstringlen = wcslen(retstring);
            if (retstringlen > 1)
              swprintf(retstring, L"%s,%d", retstring, p);
            else
              swprintf(retstring, L"%d", p);
            bret = true;

            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, NULL, NULL, retstring, true, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 25:  // 1.窗口标题+
              // 8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
      if (wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowTitle[MAX_PATH] = {0};
              ::GetWindowText(p, WindowTitle, MAX_PATH);
              if (wcslen(WindowTitle) > 1) {
                wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
                if (strfind) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, NULL, retstring, true, true,
                             process_name);
              }
            }
          } else {
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowTitle) > 1) {
              wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, title, NULL, retstring, true, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 26:  // 2.窗口类名+
              // 8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
      if (wcslen(class_name) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowClassName[MAX_PATH] = {0};
              ::GetClassName(p, WindowClassName, MAX_PATH);
              if (wcslen(WindowClassName) > 1) {
                wchar_t *strfind =
                    wcsstr(WindowClassName, class_name);  //模糊匹配
                if (strfind) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, NULL, class_name, retstring, true, true,
                             process_name);
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            if (wcslen(WindowClassName) > 1) {
              wchar_t *strfind =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, NULL, class_name, retstring, true, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 27:  // 1.窗口标题+2.窗口类名+8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
      if (wcslen(class_name) < 1 && wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              wchar_t WindowClassName[MAX_PATH] = {0};
              ::GetClassName(p, WindowClassName, MAX_PATH);
              wchar_t WindowTitle[MAX_PATH] = {0};
              ::GetWindowText(p, WindowTitle, MAX_PATH);
              if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                wchar_t *strfindclass =
                    wcsstr(WindowClassName, class_name);  //模糊匹配
                wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
                if (strfindclass && strfindtitle) {
                  if (retstringlen == 0) retstringlen = wcslen(retstring);
                  if (retstringlen > 1)
                    swprintf(retstring, L"%s,%d", retstring, p);
                  else
                    swprintf(retstring, L"%d", p);
                  bret = true;
                }
              }
              HWND hchile = ::GetWindow(p, GW_CHILD);
              if (hchile != NULL) {
                FindChildWnd(hchile, title, class_name, retstring, true, true,
                             process_name);
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfindclass && strfindtitle) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              FindChildWnd(hchile, title, class_name, retstring, true, true);
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 28:  // 4 :
              // 只匹配指定父窗口的第一层孩子窗口+8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, NULL, NULL, retstring, true, true,
                               process_name);
                }
              }
            }
          } else {
            if (retstringlen == 0) retstringlen = wcslen(retstring);
            if (retstringlen > 1)
              swprintf(retstring, L"%s,%d", retstring, p);
            else
              swprintf(retstring, L"%d", p);
            bret = true;
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 29:  ////1.窗口标题+4 :
              ///只匹配指定父窗口的第一层孩子窗口+8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
      if (wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowTitle[MAX_PATH] = {0};
                ::GetWindowText(p, WindowTitle, MAX_PATH);
                if (wcslen(WindowTitle) > 1) {
                  wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
                  if (strfind) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, title, NULL, retstring, true, true,
                               process_name);
                }
              }
            }
          } else {
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowTitle) > 1) {
              wchar_t *strfind = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 30:  // 2.窗口类名+4 :
              // 只匹配指定父窗口的第一层孩子窗口+8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
      if (wcslen(class_name) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowClassName[MAX_PATH] = {0};
                ::GetClassName(p, WindowClassName, MAX_PATH);
                if (wcslen(WindowClassName) > 1) {
                  wchar_t *strfind =
                      wcsstr(WindowClassName, class_name);  //模糊匹配
                  if (strfind) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, NULL, class_name, retstring, true, true,
                               process_name);
                }
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            if (wcslen(WindowClassName) > 1) {
              wchar_t *strfind =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              if (strfind) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    case 31:  // 1.窗口标题+2.窗口类名+4 :
              // 只匹配指定父窗口的第一层孩子窗口+8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
      if (wcslen(class_name) < 1 && wcslen(title) < 1) return false;
      HWND p = ::GetWindow(parent, GW_CHILD);  //获取桌面窗口的子窗口
      p = ::GetWindow(p, GW_HWNDFIRST);
      while (p != NULL) {
        if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
          if (process_name)  // EnumWindowByProcess
          {
            DWORD pid = 0;
            GetWindowThreadProcessId(p, &pid);
            if (EnumProcessbyName(pid, process_name)) {
              if (processpid !=
                  pid)  //只匹配指定映像的所对应的第一个进程.
                        //可能有很多同映像名的进程，只匹配第一个进程的.
              {
                if (indexpid < IsEuemprosuccess) {
                  indexpid++;
                  processpid = pid;
                  memset(retstring, 0, retstringlen);  //清空返回字符串
                  retstringlen = 0;
                }
              }
              if (processpid == pid) {
                wchar_t WindowClassName[MAX_PATH] = {0};
                ::GetClassName(p, WindowClassName, MAX_PATH);
                wchar_t WindowTitle[MAX_PATH] = {0};
                ::GetWindowText(p, WindowTitle, MAX_PATH);
                if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                  wchar_t *strfindclass =
                      wcsstr(WindowClassName, class_name);  //模糊匹配
                  wchar_t *strfindtitle =
                      wcsstr(WindowTitle, title);  //模糊匹配
                  if (strfindclass && strfindtitle) {
                    if (retstringlen == 0) retstringlen = wcslen(retstring);
                    if (retstringlen > 1)
                      swprintf(retstring, L"%s,%d", retstring, p);
                    else
                      swprintf(retstring, L"%d", p);
                    bret = true;
                  }
                }
                HWND hchile = ::GetWindow(p, GW_CHILD);
                if (hchile != NULL) {
                  FindChildWnd(hchile, title, class_name, retstring, true, true,
                               process_name);
                }
              }
            }
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if (strfindclass && strfindtitle) {
                if (retstringlen == 0) retstringlen = wcslen(retstring);
                if (retstringlen > 1)
                  swprintf(retstring, L"%s,%d", retstring, p);
                else
                  swprintf(retstring, L"%d", p);
                bret = true;
              }
            }
          }
        }
        p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
      }
      break;
    }
    default:
      return bret;
  }

  return bret;
}

bool WinApi::EnumWindowSuper(wchar_t *spec1, LONG flag1, LONG type1,
                             wchar_t *spec2, LONG flag2, LONG type2, LONG sort,
                             wchar_t *retstring) {
  bool bret = false;
  wchar_t findhwnd1[MAX_PATH * 100] = {0};
  wchar_t findhwnd2[MAX_PATH * 100] = {0};
  bool bfindhwnd1 = false;
  bool bfindhwnd2 = false;
  retstringlen = 0;
  HWND parent = GetDesktopWindow();
  HWND p = ::GetWindow(parent, GW_CHILD);
  p = ::GetWindow(p, GW_HWNDFIRST);
  while (p != NULL) {
    if (flag1 == 0)  // 0表示spec1的内容是标题
    {
      wchar_t WindowTitle[MAX_PATH] = {0};
      ::GetWindowText(p, WindowTitle, MAX_PATH);
      if (wcslen(WindowTitle) > 0) {
        if (type1 == 0)  // 0精确判断,1模糊判断
        {
          if (wcscmp(spec1, WindowTitle) == 0) bfindhwnd1 = true;
        } else if (type1 == 1) {
          if (wcsstr(WindowTitle, spec1) != NULL) bfindhwnd1 = true;
        }
      }
    } else if (flag1 == 1)  // 1表示spec1的内容是程序名字
    {
      DWORD pid = 0;
      ::GetWindowThreadProcessId(p, &pid);
      wchar_t proname[MAX_PATH] = {0};
      GetProcesspath(pid, proname);
    } else if (flag1 == 2)  // 2表示spec1的内容是类名
    {
      wchar_t WindowClassName[MAX_PATH] = {0};
      ::GetClassName(p, WindowClassName, MAX_PATH);
      if (wcslen(WindowClassName) > 0) {
        if (type1 == 0)  // 0精确判断,1模糊判断
        {
          if (wcscmp(spec1, WindowClassName) == 0) bfindhwnd1 = true;
        } else {
          if (wcsstr(WindowClassName, spec1) != NULL) bfindhwnd1 = true;
        }
      }
    }
    if (bfindhwnd1) {
      if (retstringlen == 0) retstringlen = wcslen(retstring);
      if (retstringlen > 1)
        swprintf(retstring, L"%s,%d", retstring, p);
      else
        swprintf(retstring, L"%d", p);
      bfindhwnd1 = false;
    }

    HWND hchile = ::GetWindow(p, GW_CHILD);
    if (hchile != NULL) {
      FindChildWnd(hchile, NULL, NULL, findhwnd1);
    }
    p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
  }

  return bret;
}

bool WinApi::EnumProcess(const wchar_t *name, wchar_t *retstring) {
  bool bret = false;
  retstringlen = 0;
  if (wcslen(name) < 1) return false;
  IsEuemprosuccess = 0;
  if (EnumProcessbyName(0, name) == true) {
    bret = true;
    for (int i = 0; i < IsEuemprosuccess; i++) {
      if (retstringlen == 0) retstringlen = wcslen(retstring);
      if (retstringlen > 1)
        swprintf(retstring, L"%s,%d", retstring, npid[i]);
      else
        swprintf(retstring, L"%d", npid[i]);
    }
  }
  return bret;
}
bool WinApi::ClientToScreen(LONG hwnd, LONG &x, LONG &y) {
  POINT point;

  ::ClientToScreen((HWND)hwnd, &point);

  x = point.x;
  y = point.y;

  return true;
}
long WinApi::FindWindow(const wchar_t *class_name, const wchar_t *title) {
  if (class_name[0] == L'\0') class_name = nullptr;
  if (title[0] == L'\0') title = nullptr;
  return (LONG)::FindWindowW(class_name, title);
}

long WinApi::FindWindowEx(long parent, const wchar_t *class_name,
                          const wchar_t *title) {
  if (class_name[0] == L'\0') class_name = nullptr;
  if (title[0] == L'\0') title = nullptr;
  return (long)::FindWindowExW((HWND)parent, NULL, class_name, title);
}

bool WinApi::FindWindowByProcess(const wchar_t *class_name,
                                 const wchar_t *title, LONG &rethwnd,
                                 const wchar_t *process_name, DWORD Pid) {
  bool bret = false;
  rethwnd = 0;
  if (process_name) {
    if (wcslen(process_name) < 1) return false;
    memset(npid, 0, MAX_PATH);
    IsEuemprosuccess = 0;
    if (EnumProcessbyName(0, process_name) == false) return false;

    HWND p = ::GetWindow(GetDesktopWindow(), GW_CHILD);  //获取桌面窗口的子窗口
    p = ::GetWindow(p, GW_HWNDFIRST);
    while (p != NULL) {
      if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
        DWORD pid = 0;
        GetWindowThreadProcessId(p, &pid);
        if (EnumProcessbyName(pid, process_name)) {
          if (wcslen(class_name) < 1 && wcslen(title) < 1) {
            rethwnd = (LONG)p;
            bret = true;
            break;
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if ((wcslen(class_name) >= 1 && strfindclass) ||
                  (wcslen(title) >= 1 && strfindtitle)) {
                rethwnd = (LONG)p;
                bret = true;
                break;
              }
            }

            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              const wchar_t *classname = NULL;
              const wchar_t *titles = NULL;
              if (wcslen(class_name) > 0) classname = class_name;
              if (wcslen(title) > 0) titles = titles;
              DWORD dret = FindChildWnd(hchile, titles, classname, NULL, false,
                                        false, process_name);
              if (dret > 0) {
                rethwnd = (LONG)dret;
                bret = true;
                break;
              }
            }
          }
        }
      }
      p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
    }
  } else if (Pid > 0) {
    HWND p = ::GetWindow(GetDesktopWindow(), GW_CHILD);  //获取桌面窗口的子窗口
    p = ::GetWindow(p, GW_HWNDFIRST);
    while (p != NULL) {
      if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
        DWORD npid = 0;
        GetWindowThreadProcessId(p, &npid);
        if (Pid == npid) {
          if (wcslen(class_name) < 1 && wcslen(title) < 1) {
            rethwnd = (LONG)p;
            bret = true;
            break;
          } else {
            wchar_t WindowClassName[MAX_PATH] = {0};
            ::GetClassName(p, WindowClassName, MAX_PATH);
            wchar_t WindowTitle[MAX_PATH] = {0};
            ::GetWindowText(p, WindowTitle, MAX_PATH);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
              wchar_t *strfindclass =
                  wcsstr(WindowClassName, class_name);  //模糊匹配
              wchar_t *strfindtitle = wcsstr(WindowTitle, title);  //模糊匹配
              if ((wcslen(class_name) >= 1 && strfindclass) ||
                  (wcslen(title) >= 1 && strfindtitle)) {
                rethwnd = (LONG)p;
                bret = true;
                break;
              }
            }
            HWND hchile = ::GetWindow(p, GW_CHILD);
            if (hchile != NULL) {
              const wchar_t *classname = NULL;
              const wchar_t *titles = NULL;
              if (wcslen(class_name) > 0) classname = class_name;
              if (wcslen(title) > 0) titles = titles;
              DWORD dret = FindChildWnd(hchile, titles, classname, NULL, false,
                                        false, process_name);
              if (dret > 0) {
                rethwnd = (LONG)dret;
                bret = true;
                break;
              }
            }
          }
        }
      }
      p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
    }
  }

  return bret;
}
bool WinApi::GetClientRect(LONG hwnd, LONG &x, LONG &y, LONG &x1, LONG &y1) {
  bool bret = false;
  RECT clientrect;
  if (IsWindow((HWND)hwnd)) {
    ::GetClientRect((HWND)hwnd, &clientrect);
    POINT point;
    point.x = clientrect.left;
    point.y = clientrect.top;
    ::ClientToScreen((HWND)hwnd, &point);
    x = point.x;
    y = point.y;
    point.x = clientrect.right;
    point.y = clientrect.bottom;
    ::ClientToScreen((HWND)hwnd, &point);
    x1 = point.x;
    y1 = point.y;
    bret = true;
  }

  return bret;
}
bool WinApi::GetClientSize(LONG hwnd, LONG &width, LONG &height) {
  bool bret = false;
  RECT clientrect;
  if (IsWindow((HWND)hwnd)) {
    ::GetClientRect((HWND)hwnd, &clientrect);
    width = clientrect.right - clientrect.left;
    height = clientrect.bottom - clientrect.top;
    bret = true;
  }
  return bret;
}
bool WinApi::GetMousePointWindow(LONG &rethwnd, LONG x, LONG y) {
  bool bret = false;
  rethwnd = 0;
  POINT point;
  if ((x != -1 && y != -1)) {
    point.x = x;
    point.y = y;
  } else {
    ::GetCursorPos(&point);
  }
  rethwnd = (DWORD)::WindowFromPoint(point);
  if (rethwnd == NULL) {
    HWND p = ::GetWindow(GetDesktopWindow(), GW_CHILD);  //获取桌面窗口的子窗口
    p = ::GetWindow(p, GW_HWNDFIRST);
    while (p != NULL) {
      if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
        RECT rc;
        ::GetWindowRect(p, &rc);
        if ((rc.top <= point.y) && (rc.left <= point.x) &&
            (rc.right >= (point.x - rc.left)) &&
            (rc.bottom >= (point.y - rc.top))) {
          wchar_t WindowClass[MAX_PATH] = {0};
          ::GetClassName(p, WindowClass, MAX_PATH);
          // if((windowpoint.x==0||windowpoint.x<rc.left)&&wcscmp(WindowClass,L"CabinetWClass")!=0)
          // //IE框窗体排除在外
          if (wcscmp(WindowClass, L"CabinetWClass") != 0)  // IE框窗体排除在外
          {
            rethwnd = (DWORD)p;
            bret = true;
            break;
          }
        }
      }
      p = ::GetWindow(p, GW_HWNDNEXT);  //获取下一个窗口
    }
  } else
    bret = true;

  return bret;
}

int WinApi::GetProcessNumber()  //获取CPU个数
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return (int)info.dwNumberOfProcessors;
}

// 时间格式转换
__int64 WinApi::FileTimeToInt64(const FILETIME &time) {
  ULARGE_INTEGER tt;
  tt.LowPart = time.dwLowDateTime;
  tt.HighPart = time.dwHighDateTime;
  return (tt.QuadPart);
}

double WinApi::get_cpu_usage(DWORD ProcessID)  //获取指定进程CPU使用率
{
  // cpu数量
  static int processor_count_ = -1;
  //上一次的时间
  static __int64 last_time_ = 0;
  static __int64 last_system_time_ = 0;

  FILETIME now;
  FILETIME creation_time;
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;
  __int64 system_time;
  __int64 time;
  // 	__int64 system_time_delta;
  // 	__int64 time_delta;

  double cpu = -1;

  if (processor_count_ == -1) {
    processor_count_ = GetProcessNumber();
  }

  GetSystemTimeAsFileTime(&now);

  // HANDLE hProcess =
  // OpenProcess(PROCESS_QUERY_INFORMATION/*PROCESS_ALL_ACCESS*/, false,
  // ProcessID);
  HANDLE hProcess = NULL;

  hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                         ProcessID);

  if (!hProcess) {
    return -1;
  }
  if (!GetProcessTimes(hProcess, &creation_time, &exit_time, &kernel_time,
                       &user_time)) {
    return -1;
  }
  system_time = (FileTimeToInt64(kernel_time) + FileTimeToInt64(user_time)) /
                processor_count_;  // CPU使用时间
  time = FileTimeToInt64(now);     //现在的时间

  last_system_time_ = system_time;
  last_time_ = time;
  CloseHandle(hProcess);

  Sleep(1000);

  // hProcess = OpenProcess(PROCESS_QUERY_INFORMATION/*PROCESS_ALL_ACCESS*/,
  // false, ProcessID);

  hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                         ProcessID);

  if (!hProcess) {
    return -1;
  }
  if (!GetProcessTimes(hProcess, &creation_time, &exit_time, &kernel_time,
                       &user_time)) {
    return -1;
  }
  GetSystemTimeAsFileTime(&now);
  system_time = (FileTimeToInt64(kernel_time) + FileTimeToInt64(user_time)) /
                processor_count_;  // CPU使用时间
  time = FileTimeToInt64(now);     //现在的时间

  CloseHandle(hProcess);

  cpu = ((double)(system_time - last_system_time_) /
         (double)(time - last_time_)) *
        100;
  return cpu;
}

//或者指定进程内存使用率
DWORD WinApi::GetMemoryInfo(DWORD ProcessID) {
  PROCESS_MEMORY_COUNTERS pmc;
  DWORD memoryInK = 0;
  HANDLE hProcess = NULL;

  hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                         ProcessID);

  if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
    // memoryInK = pmc.WorkingSetSize/1024;		//单位为k
    memoryInK = pmc.WorkingSetSize;
  }

  CloseHandle(hProcess);
  return memoryInK;
}

bool WinApi::GetProcessInfo(LONG pid, wchar_t *retstring) {
  bool bret = false;
  wchar_t process_name[MAX_PATH] = {0};
  wchar_t process_path[MAX_PATH] = {0};
  DWORD cpu = 0;
  DWORD meminfo = 0;

  int nItem = 0;  // 项计数
  PROCESSENTRY32 pe32 = {sizeof(PROCESSENTRY32)};
  HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap == INVALID_HANDLE_VALUE) return FALSE;
  if (::Process32First(hProcessSnap, &pe32)) {
    do {
      if (pe32.th32ProcessID == pid) {
        wcscpy(process_name, pe32.szExeFile);
        break;
      }

    } while (::Process32Next(hProcessSnap, &pe32));
  }
  ::CloseHandle(hProcessSnap);
  if (wcslen(process_name) < 1) return bret;

  // TSRuntime::GetRemoteModulePath(process_name, pid, process_path);
  cpu = (DWORD)get_cpu_usage(pid);
  meminfo = (DWORD)GetMemoryInfo(pid);

  swprintf(retstring, L"%s|%s|%d|%d", process_name, process_path, cpu, meminfo);

  return bret;
}
bool WinApi::GetProcesspath(DWORD ProcessID, wchar_t *process_path) {
  HANDLE hProcess = NULL;

  hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                         ProcessID);

  HMODULE hMods = NULL;
  DWORD cbNeededModule = 0;
  EnumProcessModules(hProcess, &hMods, sizeof(hMods), &cbNeededModule);
  GetModuleFileNameEx(hProcess, hMods, process_path, MAX_PATH);

  return true;
}
bool WinApi::GetWindow(LONG hwnd, LONG flag, LONG &rethwnd) {
  bool bret = false;
  rethwnd = 0;
  HWND wnd = (HWND)hwnd;
  if (IsWindow(wnd) == false) return bret;
  DWORD type = -1;
  if (flag == 0)  // 0:获取父窗口
    rethwnd = (LONG)::GetParent(wnd);
  else if (flag == 1)  //获取第一个儿子窗口
    type = GW_CHILD;
  else if (flag == 2)  //获取First 窗口
    type = GW_HWNDFIRST;
  else if (flag == 3)  //获取Last窗口
    type = GW_HWNDLAST;
  else if (flag == 4)  //获取下一个窗口
    type = GW_HWNDNEXT;
  else if (flag == 5)  //获取上一个窗口
    type = GW_HWNDPREV;
  else if (flag == 6)  //获取拥有者窗口
    type = GW_OWNER;
  else if (flag == 7)  //获取顶层窗口
  {
    // rethwnd = (LONG)::GetForegroundWindow();
    HWND next = NULL, current = (HWND)hwnd;
    while (next = ::GetParent(current)) current = next;
    rethwnd = (long)current;
    return ::IsWindow(current);
  }

  if (type != -1) rethwnd = (LONG)::GetWindow(wnd, (UINT)type);

  if (rethwnd != 0) bret = true;

  return bret;
}

bool WinApi::GetWindowState(LONG hwnd, LONG flag) {
  bool bret = false;
  HWND wnd = (HWND)hwnd;
  if (flag == 0)  // 0://判断窗口是否存在
    bret = ::IsWindow(wnd);
  else if (flag == 1)  //判断窗口是否处于激活
  {
    if (::GetActiveWindow() == wnd) bret = true;
  } else if (flag == 2)  // 2 : 判断窗口是否可见
    bret = ::IsWindowVisible(wnd);
  else if (flag == 3)  // 3 : 判断窗口是否最小化
    bret = ::IsIconic(wnd);
  else if (flag == 4)  // 4 : 判断窗口是否最大化
    bret = ::IsZoomed(wnd);
  else if (flag == 5)  // 5 : 判断窗口是否置顶
  {
    if (::GetForegroundWindow() == wnd) bret = true;
  } else if (flag == 6)  // 6 : 判断窗口是否无响应
    bret = ::IsHungAppWindow(wnd);
  else if (flag == 7)  //判断窗口是否可用(灰色为不可用)
    bret = ::IsWindowEnabled(wnd);

  return bret;
}

bool WinApi::SendPaste(LONG hwnd) {
  bool bret = true;
  HANDLE hClip;
  char *chBuffer = NULL;
  if (OpenClipboard(NULL)) {
    //从剪贴板中取出一个内存的句柄
    hClip = GetClipboardData(CF_TEXT);
    //定义字符型指针变量用来保存内存块中的数据
    //对内存块进行加锁，将内存句柄值转化为一个指针,并将内存块的引用计数器加一，内存中的数据也返回到指针型变量中
    chBuffer = (char *)GlobalLock(hClip);
    //将数据保存到字符型变量中
    //将内存块的引用计数器减一
    GlobalUnlock(hClip);
    //关闭剪贴板，释放剪贴板资源的占用权
    CloseClipboard();
  }
  // anscii 转 unicode
  DWORD num = MultiByteToWideChar(CP_ACP, 0, chBuffer, -1, NULL, 0);
  wchar_t *wword = new wchar_t[num + 1];          //动态的申请空间存字
  memset(wword, 0, (num + 1) * sizeof(wchar_t));  //初始化动作
  MultiByteToWideChar(CP_ACP, 0, chBuffer, -1, wword, num);

  int len = wcslen(wword);
  // MessageBoxA(NULL,tts,tts,NULL);
  for (int i = 0; i < len; i++) {
    ::SendMessage((HWND)hwnd, WM_CHAR, (WPARAM)wword[i], (LPARAM)1);
    Sleep(10);
  }
  delete[] wword;

  return bret;
}

bool WinApi::SetWindowSize(LONG hwnd, LONG width, LONG hight, int type) {
  bool bret = false;
  if (type == 0)  // SetClientSize
  {
    RECT rectProgram, rectClient;
    HWND hWnd = (HWND)hwnd;
    ::GetWindowRect(hWnd, &rectProgram);  //获得程序窗口位于屏幕坐标
    ::GetClientRect(hWnd, &rectClient);   //获得客户区坐标
    //非客户区宽,高
    int nWidth = rectProgram.right - rectProgram.left -
                 (rectClient.right - rectClient.left);
    int nHeiht = rectProgram.bottom - rectProgram.top -
                 (rectClient.bottom - rectClient.top);
    nWidth += width;
    nHeiht += hight;
    rectProgram.right = nWidth;
    rectProgram.bottom = nHeiht;
    int showToScreenx =
        GetSystemMetrics(SM_CXSCREEN) / 2 - nWidth / 2;  //居中处理
    int showToScreeny = GetSystemMetrics(SM_CYSCREEN) / 2 - nHeiht / 2;
    bret = ::MoveWindow(hWnd, showToScreenx, showToScreeny, rectProgram.right,
                        rectProgram.bottom, false);
  } else  // SetWindowSize
  {
    RECT rectClient;
    HWND hWnd = (HWND)hwnd;
    ::GetWindowRect(hWnd, &rectClient);  //获得程序窗口位于屏幕坐标
    bret = ::MoveWindow(hWnd, rectClient.left, rectClient.top, width, hight,
                        false);
  }
  return bret;
}

bool WinApi::SetWindowState(LONG hwnd, LONG flag, LONG rethwnd) {
  bool bret = false;
  HWND hWnd = (HWND)hwnd;
  if (IsWindow(hWnd) == false) return bret;
  int type = -1;
  type = flag;
  if (flag == 0)  //关闭指定窗口
    ::SendMessage(hWnd, WM_CLOSE, 0, 0);
  else if (flag == 1)  //激活指定窗口
  {
    ::ShowWindow(hWnd, SW_SHOW);
    ::SetForegroundWindow(hWnd);
  } else if (flag == 2)  //最小化指定窗口,但不激活
    ::ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
  else if (flag == 3)  //最小化指定窗口,并释放内存,但同时也会激活窗口
    ::ShowWindow(hWnd, SW_SHOWMINIMIZED);
  else if (flag == 4)  //最大化指定窗口,同时激活窗口.
    ::ShowWindow(hWnd, SW_SHOWMAXIMIZED);
  else if (flag == 5)  //恢复指定窗口 ,但不激活
    ::ShowWindow(hWnd, SW_SHOWNOACTIVATE);
  else if (flag == 6)  //隐藏指定窗口
    ::ShowWindow(hWnd, SW_HIDE);
  else if (flag == 7)  //显示指定窗口
  {
    ::ShowWindow(hWnd, SW_SHOW);
    ::SetForegroundWindow(hWnd);
  } else if (flag == 8)  //置顶指定窗口
    ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE);  //窗口置顶
  else if (flag == 9)                         // 9 : 取消置顶指定窗口
    ::SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  else if (flag == 10)  //禁止指定窗口
    ::EnableWindow(hWnd, false);
  else if (flag == 11)  //取消禁止指定窗口
    ::EnableWindow(hWnd, true);
  else if (flag == 12)  // 12 : 恢复并激活指定窗口
    ::ShowWindow(hWnd, SW_RESTORE);
  else if (flag == 13)  // 13 : 强制结束窗口所在进程.
  {
    DWORD pid = 0;
    ::GetWindowThreadProcessId(hWnd, &pid);
    // TSRuntime::EnablePrivilege(L"SeDebugPrivilege", true);
    HANDLE hprocess = NULL;

    hprocess = ::OpenProcess(PROCESS_ALL_ACCESS, false, pid);

    ::TerminateProcess(hprocess, 0);
  } else if (flag == 14)  // 14 : 闪烁指定的窗口
  {
    FLASHWINFO fInfo;
    fInfo.cbSize = sizeof(FLASHWINFO);
    fInfo.dwFlags =
        FLASHW_ALL |
        FLASHW_TIMERNOFG;  //这里是闪动窗标题和任务栏按钮,直到用户激活窗体
    fInfo.dwTimeout = 0;
    fInfo.hwnd = hWnd;
    fInfo.uCount = 0xffffff;
    FlashWindowEx(&fInfo);
  } else if (flag == 15)  //使指定的窗口获取输入焦点
  {
    ::ShowWindow(hWnd, SW_SHOW);
    ::SetFocus(hWnd);
  }

  if (type >= 0 && type < 16) bret = true;

  return bret;
}

bool WinApi::SetWindowTransparent(LONG hwnd, LONG trans) {
  bool bret = false;

  COLORREF crKey = NULL;
  DWORD dwFlags = 0;
  BYTE bAlpha = 0;
  if (trans < 0) trans = 0;
  if (trans > 255) trans = 255;
  //...
  /*typedef bool(__stdcall  *  mySetLayeredWindowAttributes)(
          HWND hwnd,
          COLORREF pcrKey,
          BYTE pbAlpha,
          DWORD pdwFlags);
  mySetLayeredWindowAttributes obj_SetLayeredWindowAttributes = NULL;
  HINSTANCE hlibrary;
  hlibrary = LoadLibrary(_T("user32.dll"));
  obj_SetLayeredWindowAttributes =
  (mySetLayeredWindowAttributes)GetProcAddress(hlibrary,
  "SetLayeredWindowAttributes");*/

  SetWindowLong((HWND)hwnd, GWL_EXSTYLE, 0x80001);
  bret = SetLayeredWindowAttributes((HWND)hwnd, crKey, trans, 2);

  return bret;
}

bool WinApi::SetClipboard(wchar_t *values) {
  bool bret = false;
  int n = ::WideCharToMultiByte(CP_ACP, 0, values, -1, NULL, 0, NULL, NULL);
  char *chcontent = new char[n + 1];
  memset(chcontent, 0, sizeof(char) * n + 1);
  WideCharToMultiByte(CP_ACP, 0, values, -1, chcontent, n, NULL, NULL);

  if (OpenClipboard(NULL)) {
    //将剪贴板内容清空
    EmptyClipboard();
    //字节长度
    int leng = strlen(chcontent) + 1;
    //在堆上分配可移动的内存块，程序返回一个内存句柄
    HANDLE hClip = GlobalAlloc(GHND | GMEM_SHARE, leng);
    //定义指向字符型的指针变量
    char *buff;
    //对分配的内存块进行加锁，将内存块句柄转化成一个指针,并将相应的引用计数器加一
    buff = (char *)GlobalLock(hClip);
    //将用户输入的数据拷贝到指针变量中，实际上就是拷贝到分配的内存块中
    memcpy(buff, chcontent, leng);
    buff[leng - 1] = 0;
    //数据写入完毕，进行解锁操作，并将引用计数器数字减一
    GlobalUnlock(hClip);
    //将存放有数据的内存块放入剪贴板的资源管理中
    HANDLE help = SetClipboardData(CF_TEXT, hClip);
    //关闭剪贴板，释放剪贴板资源的占用权
    CloseClipboard();
    // MessageBox(0,L"已将数据存入剪贴板",L"剪切扳",0);
    if (help != NULL) {
      bret = true;
    } else {
      bret = false;
    }
  }
  delete[] chcontent;
  return bret;
}

bool WinApi::GetClipboard(wchar_t *retstr) {
  bool bret = false;
  HANDLE hClip;
  char *chBuffer = NULL;
  if (OpenClipboard(NULL)) {
    //从剪贴板中取出一个内存的句柄
    hClip = GetClipboardData(CF_TEXT);
    //定义字符型指针变量用来保存内存块中的数据

    //对内存块进行加锁，将内存句柄值转化为一个指针,并将内存块的引用计数器加一，内存中的数据也返回到指针型变量中
    chBuffer = (char *)GlobalLock(hClip);

    //将数据保存到字符型变量中
    //将内存块的引用计数器减一
    GlobalUnlock(hClip);
    //关闭剪贴板，释放剪贴板资源的占用权
    CloseClipboard();
  }

  DWORD num = MultiByteToWideChar(CP_ACP, 0, chBuffer, -1, NULL, 0);
  wchar_t *wword = new wchar_t[num + 1];          //动态的申请空间存字
  memset(wword, 0, (num + 1) * sizeof(wchar_t));  //初始化动作
  MultiByteToWideChar(CP_ACP, 0, chBuffer, -1, wword, num);

  if (num < MAX_PATH * 4 - 1) wcscpy(retstr, wword);

  delete[] wword;

  return bret;
}

long WinApi::SendString(HWND hwnd, const wstring &s) {
  if (::IsWindow(hwnd)) {
    auto p = s.data();
    for (int i = 0; i < s.length(); ++i) {
      ::PostMessageW(hwnd, WM_CHAR, p[i], 0);
      ::Sleep(5);
    }
    return 1;
  }
  return 0;
}

long WinApi::SendStringIme(HWND hwnd, const wstring &s) {
  if (::IsWindow(hwnd)) {
    auto p = s.data();
    for (int i = 0; i < s.length(); ++i) {
      ::PostMessage(hwnd, WM_IME_CHAR, p[i], 0);
      ::Sleep(5);
    }
    return 1;
  }
  return 0;
}

long WinApi::RunApp(const wstring &cmd, long mode) {
  std::unique_ptr<wchar_t> cmdptr(new wchar_t[cmd.length()]);
  memcpy(cmdptr.get(), cmd.data(), cmd.length() * sizeof(wchar_t));
  /*SECURITY_ATTRIBUTES SA;
  SA.bInheritHandle = NULL;
  SA.*/
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  int bret;
  wstring curr_dir;
  if (mode == 1) {
    // find
    size_t pos;
    pos = cmd.find(L".exe");
    if (pos != wstring::npos && pos != 0) {
      for (int i = pos - 1; i >= 1; --i) {
        if (cmd[i] == L'\\' || cmd[i] == L'/') {
          pos = i;
          break;
        }
      }
      if (pos > 0) {
        curr_dir = cmd.substr(0, pos);
      }
    }
    // setlog(curr_dir.c_str());
  }
  bret = ::CreateProcessW(
      nullptr,       //// 应用程序名称
      cmdptr.get(),  // 命令行字符串
      NULL,          // 进程的安全属性
      NULL,          // 线程的安全属性
      false,         // 是否继承父进程的属性
      0,             // 创建标志
      nullptr,       // 指向新的环境块的指针
      mode == 1 && !curr_dir.empty() ? curr_dir.c_str()
                                     : nullptr,  // 指向当前目录名的指针
      &si,                                       // 传递给新进程的信息
      &pi                                        // 新进程返回的信息
  );
  if (bret) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }

  return bret;
}

HWND WinApi::GetTopWindowSp(HWND hwnd) {
  HWND i = hwnd, temp;

  while (GetWindowLongA(i, GWL_STYLE) >= 0) {
    temp = GetParent(i);
    if (!temp) break;
  }
  return i;
}
