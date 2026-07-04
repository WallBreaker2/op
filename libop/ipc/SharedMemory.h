#pragma once
#include "../base/WindowsHandle.h"
#include <cstdint>
#include <string>
#include <utility>
#include <windows.h>
namespace op {

class SharedMemory {
  public:
    explicit SharedMemory(const std::wstring &name_, size_t size_) {
        open_create(name_, size_);
    }
    SharedMemory() = default;
    ~SharedMemory() {
        close();
    }

    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;

    /*open or create a shared memory*/
    bool open_create(const std::wstring &name_, size_t size_) {
        close();

        op::win32::unique_handle temph(OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name_.c_str()));
        if (!temph) {
            ULARGE_INTEGER mapping_size{};
            mapping_size.QuadPart = static_cast<ULONGLONG>(size_);
            temph.reset(CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, mapping_size.HighPart,
                                           mapping_size.LowPart, name_.c_str()));
            if (!temph)
                return false;
        }
        void *address = MapViewOfFile(temph.get(), FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!address)
            return false;
        _hmap = std::move(temph);
        _paddress = address;
        return true;
    }
    /*open only*/
    bool open(const std::wstring &name_) {
        close();

        op::win32::unique_handle temph(OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name_.c_str()));
        if (!temph)
            return false;
        void *address = MapViewOfFile(temph.get(), FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!address)
            return false;
        _hmap = std::move(temph);
        _paddress = address;
        return true;
    }
    /*close the shared memory*/
    void close() {
        if (_paddress)
            UnmapViewOfFile(_paddress);
        _paddress = nullptr;
        _hmap.reset();
    }
    template <typename T> T &at(int idx_) {
        // assert(_hmap&&_paddress);
        return (T)_paddress[idx_];
    }
    template <typename T> T *data() {
        return (T *)_paddress;
    }

  protected:
    /*SharedMemory operator=(const SharedMemory& rhs) {
        return rhs;
    }*/
  private:
    // handle of shared file map
    op::win32::unique_handle _hmap;
    // address of shared memory
    void *_paddress{nullptr};
};

} // namespace op
