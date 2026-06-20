#pragma once
#include "../runtime/WindowsHandle.h"
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
