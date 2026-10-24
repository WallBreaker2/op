#pragma once
#ifndef OP_MEMORY_PROCESS_MEMORY_H_
#define OP_MEMORY_PROCESS_MEMORY_H_
#include "../base/Utils.h"
#include "BlackBone/Process/Process.h"
#include <Windows.h>
#include <cstdint>

namespace op {

class ProcessMemory {
  public:
    ProcessMemory();
    ~ProcessMemory();
    // 兼容老接口：按十六进制字符串写入指定字节数。
    long WriteData(HWND hwnd, const wstring &address, const wstring &data, LONG size);
    // 兼容老接口：读取后返回连续的大写十六进制字符串。
    wstring ReadData(HWND hwnd, const wstring &address, LONG size);
    // 直接读写原始字节，给上层类型接口共用。
    bool ReadRaw(HWND hwnd, const wstring &address, void *buf, size_t size);
    bool WriteRaw(HWND hwnd, const wstring &address, const void *buf, size_t size);

    // 整数类型沿用大漠的编号：0=i32 1=i16 2=i8 3=i64 4=u32 5=u16 6=u8。
    static size_t IntTypeSize(long type);
    // C API 需要知道读没读成功，所以这里额外给一个带 out 参数的版本。
    bool ReadInt(HWND hwnd, const wstring &address, long type, int64_t *value);
    int64_t ReadInt(HWND hwnd, const wstring &address, long type);
    long WriteInt(HWND hwnd, const wstring &address, long type, int64_t value);

    // 批量形状：一次 prepare_process、一次遍历，把“每调用一次就要付的固定开销”
    // （进程上下文准备、字符串转换、异常帧）摊到整批上。
    // addresses/count 按顺序一一对应；values[i] 读失败时写 0，ok[i]（可空）标记成功与否。
    // 返回成功的条数。
    long ReadIntBatch(HWND hwnd, const wstring *addresses, long count, long type, int64_t *values,
                      bool *ok = nullptr);
    long WriteIntBatch(HWND hwnd, const wstring *addresses, long count, long type, const int64_t *values,
                       bool *ok = nullptr);
    // 每项读 item_size 字节，连续写进 buffer（第 i 项落在 buffer + i*item_size）。
    long ReadRawBatch(HWND hwnd, const wstring *addresses, long count, size_t item_size, void *buffer,
                      bool *ok = nullptr);

    // 浮点数按目标进程内存里的原始 float/double 读取。
    bool ReadFloat(HWND hwnd, const wstring &address, float *value);
    float ReadFloat(HWND hwnd, const wstring &address);
    long WriteFloat(HWND hwnd, const wstring &address, float value);
    bool ReadDouble(HWND hwnd, const wstring &address, double *value);
    double ReadDouble(HWND hwnd, const wstring &address);
    long WriteDouble(HWND hwnd, const wstring &address, double value);

    // 字符串类型：0=本机 ANSI/GBK，1=UTF-16，2=UTF-8。len<=0 时读到第一个 0 结束。
    wstring ReadString(HWND hwnd, const wstring &address, long type, long len);
    long WriteString(HWND hwnd, const wstring &address, long type, const wstring &value);

    // 只检查中括号配对，真正的地址有效性在读取时判断。
    bool checkaddress(const wstring &address);
    // 地址表达式支持十六进制、四则运算、模块基址 <module.dll> 和多级指针 [addr+offset]。
    size_t str2address(const wstring &caddress);

    // hwnd 为空时读写当前进程，否则通过 BlackBone 读写目标窗口所在进程。
    bool mem_read(void *dst, size_t src, size_t size);
    bool mem_write(size_t dst, void *src, size_t size);

    // 十六进制字符串和字节数组之间的小工具。
    void hex2bins(vector<uchar> &bin, const wstring &hex, size_t size);
    void bin2hexs(const vector<uchar> &bin, wstring &hex);

  private:
    // 每次读写前准备进程上下文，当前进程场景不用 attach。
    bool prepare_process(HWND hwnd);
    // 目标变了或跨进程读写失败时丢掉缓存的 attach，下一次重新 Open。
    void invalidate_attach();
    // 地址表达式解析很贵（含 <module> 时还要查模块基址），按“目标 + 表达式原文”缓存。
    size_t resolve_address(const wstring &address);

    blackbone::Process _proc;
    HWND _hwnd = nullptr;
    // 已 attach 的目标 pid，0 表示当前没有跨进程上下文。
    // BlackBone 的 Attach() 是 Detach()+OpenProcess()，每次调用都做等于白付一次句柄 churn。
    DWORD _attached_pid = 0;
    // 小容量解析缓存：脚本里同一地址会被反复读写，命中就省掉整段解析。
    struct AddressCacheEntry {
        wstring address;
        size_t resolved = 0;
    };
    vector<AddressCacheEntry> _addr_cache;
    DWORD _addr_cache_pid = 0;
};

} // namespace op

#endif // OP_MEMORY_PROCESS_MEMORY_H_
