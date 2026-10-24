#pragma once
#include "../base/WindowsHandle.h"
#include <cstdint>
#include <cstddef>
#include <string>
#include <utility>
#include <windows.h>
namespace op {

// §72.51：VirtualQuery 给的 RegionSize 是页粒度的，对文件映射视图它可能比真实
// 映射大最多一个页对齐的零头。拿它当"可安全使用的字节数"就会高估容量：
// 遇到同名段其实更小的场景，容量判断会误通过，后面的整帧搬运直接写越界。
// NtQuerySection(SectionBasicInformation) 能拿到精确大小；拿不到（老系统、
// ntdll 被 hook）就返回 0，调用方退回原来的 VirtualQuery 行为。
inline size_t query_section_size(HANDLE hmap) {
    if (!hmap)
        return 0;
    using nt_query_section_t = LONG(NTAPI *)(HANDLE, int, void *, ULONG, ULONG *);
    static const nt_query_section_t pfn = reinterpret_cast<nt_query_section_t>(
        ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtQuerySection"));
    if (!pfn)
        return 0;
    struct section_basic_information {
        void *base_address;
        ULONG attributes;
        LARGE_INTEGER size;
    } info{};
    ULONG returned = 0;
    const LONG status = pfn(hmap, 0 /* SectionBasicInformation */, &info, static_cast<ULONG>(sizeof(info)), &returned);
    if (status < 0)
        return 0;
    return static_cast<size_t>(info.size.QuadPart);
}

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
        // 注意：mapped_size() 给的是 VirtualQuery 的页粒度 RegionSize，可能比真实映射大，
        // 也不代表调用方申请过这么大。这里再按申请尺寸夹一次，size() 才算"可安全使用的字节数"：
        // 若同名映射是别的进程建的且更小，size() 会小于申请尺寸，容量判断据此拒绝而不是越界写。
        size_t usable = mapped_size(address);
        const size_t section = query_section_size(temph.get());
        if (section != 0 && section < usable)
            usable = section;
        if (usable == 0 || usable > size_)
            usable = size_;
        _max_size = size_;
        _size = usable;
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
        // §72.51：原来直接拿 VirtualQuery 的 RegionSize 当容量（页粒度，偏大）。
        // 先看能不能拿到精确的 section 大小，拿不到才退回 RegionSize。
        size_t usable = mapped_size(address);
        const size_t section = query_section_size(_hmap.get());
        if (section != 0 && section < usable)
            usable = section;
        _size = usable;
        _max_size = _size;
        return true;
    }
    /*close the shared memory*/
    void close() {
        if (_paddress)
            UnmapViewOfFile(_paddress);
        _paddress = nullptr;
        _size = 0;
        _max_size = 0;
        _hmap.reset();
    }
    template <typename T> T &at(int idx_) {
        // assert(_hmap&&_paddress);
        return (T)_paddress[idx_];
    }
    template <typename T> T *data() {
        return (T *)_paddress;
    }
    size_t size() const {
        return _size;
    }

  protected:
    /*SharedMemory operator=(const SharedMemory& rhs) {
        return rhs;
    }*/
  private:
    static size_t mapped_size(void *address) {
        MEMORY_BASIC_INFORMATION info{};
        if (!address || ::VirtualQuery(address, &info, sizeof(info)) == 0)
            return 0;
        return info.RegionSize;
    }

    // handle of shared file map
    op::win32::unique_handle _hmap;
    // address of shared memory
    void *_paddress{nullptr};
    // 可安全使用的字节数（已按申请尺寸夹过，见 open_create 的注释）
    size_t _size{0};
    // 申请/映射上限，仅用于把页粒度的 RegionSize 夹回真实可写范围
    size_t _max_size{0};
};

} // namespace op
