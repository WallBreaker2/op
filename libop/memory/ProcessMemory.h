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

    blackbone::Process _proc;
    HWND _hwnd;
};

} // namespace op

#endif // OP_MEMORY_PROCESS_MEMORY_H_
