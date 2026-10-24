#pragma once
#include "../base/WindowsHandle.h"
#include <assert.h>
#include <string>
#include <utility>
#include <windows.h>
namespace op {

class ProcessMutex {
  public:
    explicit ProcessMutex() = default;
    ~ProcessMutex() {
        close();
    }

    ProcessMutex(const ProcessMutex &) = delete;
    ProcessMutex &operator=(const ProcessMutex &) = delete;

    bool open_create(const std::wstring &name_) {
        close();

        op::win32::unique_handle temp(OpenMutexW(MUTEX_ALL_ACCESS, FALSE, name_.c_str()));
        if (!temp) {
            temp.reset(CreateMutexW(NULL, FALSE, name_.c_str()));
        }
        if (temp) {
            _hmutex = std::move(temp);
            return true;
        } else {
            return false;
        }
    }
    bool open(const std::wstring &name_) {
        close();

        op::win32::unique_handle temp(OpenMutexW(MUTEX_ALL_ACCESS, FALSE, name_.c_str()));
        if (temp) {
            _hmutex = std::move(temp);
            return true;
        }
        return false;
    }
    void lock() {
        const DWORD result = ::WaitForSingleObject(_hmutex.get(), INFINITE);
        if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED)
            ++_lock_count;
    }

    // §72.51：INFINITE 等待在产品路径上是隐患。注入侧写一帧要持这把跨进程互斥，
    // 宿主进程如果卡死 / 被挂起 / 被调试器冻住，Present 会永久阻塞；而那时
    // SharedFrameChannelLease 还握着进程内的全局 SRW 锁，同一进程里其他窗口的
    // 捕获也会一起僵住 —— 一个宿主能拖死整片。
    // 注入侧一律走这个有界版本，拿不到就跳过本帧（下一帧再来），不阻塞 Present。
    bool lock_timeout(DWORD timeout_ms) {
        const DWORD result = ::WaitForSingleObject(_hmutex.get(), timeout_ms);
        if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED) {
            ++_lock_count;
            return true;
        }
        return false;
    }

    DWORD try_lock(size_t time_) {
        const DWORD result = ::WaitForSingleObject(_hmutex.get(), static_cast<DWORD>(time_));
        if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED)
            ++_lock_count;
        return result;
    }
    void unlock() {
        assert(_hmutex);
        if (!_hmutex || _lock_count == 0)
            return;
        ::ReleaseMutex(_hmutex.get());
        --_lock_count;
    }

    void close() {
        while (_lock_count > 0)
            unlock();
        _hmutex.reset();
    }

  protected:
  private:
    op::win32::unique_handle _hmutex;
    size_t _lock_count{0};
};

} // namespace op
