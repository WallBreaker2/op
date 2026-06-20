#pragma once
#ifndef OP_RUNTIME_WINDOWS_HANDLE_H_
#define OP_RUNTIME_WINDOWS_HANDLE_H_

#include <Windows.h>

namespace op::win32 {

class unique_handle {
  public:
    unique_handle() noexcept = default;

    explicit unique_handle(HANDLE handle) noexcept : handle_(handle) {
    }

    ~unique_handle() {
        reset();
    }

    unique_handle(const unique_handle &) = delete;
    unique_handle &operator=(const unique_handle &) = delete;

    unique_handle(unique_handle &&other) noexcept : handle_(other.release()) {
    }

    unique_handle &operator=(unique_handle &&other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    HANDLE get() const noexcept {
        return handle_;
    }

    explicit operator bool() const noexcept {
        return is_valid(handle_);
    }

    HANDLE release() noexcept {
        HANDLE handle = handle_;
        handle_ = nullptr;
        return handle;
    }

    void reset(HANDLE handle = nullptr) noexcept {
        if (is_valid(handle_)) {
            ::CloseHandle(handle_);
        }
        handle_ = handle;
    }

  private:
    static bool is_valid(HANDLE handle) noexcept {
        return handle != nullptr && handle != INVALID_HANDLE_VALUE;
    }

    HANDLE handle_ = nullptr;
};

} // namespace op::win32

#endif // OP_RUNTIME_WINDOWS_HANDLE_H_
