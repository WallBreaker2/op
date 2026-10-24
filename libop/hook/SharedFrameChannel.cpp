#include "SharedFrameChannel.h"

namespace op::hook {

namespace {

// 全局通道与它的锁。所有 hook 后端（d3d9/10/11/12/gl）写的是同一个命名段，也就共用这一个通道。
SRWLOCK g_channel_lock = SRWLOCK_INIT;
SharedFrameChannel g_channel;

// 请求协商区的只读映射（见头文件里 ReadFrameRequestByName 的说明）。
// 和写通道一样按名字缓存复用：每帧重开映射会让后续读取重新缺页。
SRWLOCK g_request_lock = SRWLOCK_INIT;
op::SharedMemory g_request_mem;
std::wstring g_request_name;
unsigned g_request_reuse = 0;
constexpr unsigned kRequestReopenIntervalFrames = 64;

bool ensure_request_mem(const std::wstring &mem_name) {
    if (!g_request_name.empty() && g_request_name == mem_name && g_request_reuse < kRequestReopenIntervalFrames) {
        ++g_request_reuse;
        return true;
    }
    g_request_reuse = 0;
    g_request_mem.close();
    g_request_name.clear();
    if (!g_request_mem.open(mem_name)) {
        return false;
    }
    g_request_name = mem_name;
    return true;
}

} // namespace

FrameRequestSnapshot ReadFrameRequestByName(const std::wstring &mem_name, std::uint32_t width,
                                            std::uint32_t height) {
    FrameRequestSnapshot out;
    if (mem_name.empty() || width == 0 || height == 0) {
        return out;
    }
    ::AcquireSRWLockExclusive(&g_request_lock);
    if (ensure_request_mem(mem_name)) {
        out = ReadFrameRequest(g_request_mem, width, height);
    }
    ::ReleaseSRWLockExclusive(&g_request_lock);
    return out;
}

bool SharedFrameChannel::ensure(const std::wstring &mem_name, const std::wstring &mutex_name) {
    if (_valid && mem_name == _mem_name && mutex_name == _mutex_name && _reuse < kReopenIntervalFrames) {
        ++_reuse;
        return true;
    }

    _reuse = 0;
    _valid = false;
    _mem.close();
    _mutex.close();
    _mem_name.clear();
    _mutex_name.clear();

    // 只 open 不 create：这两个对象必须由宿主进程创建。在注入进程里 create 出来的同名段
    // 是本进程私有的，宿主读不到，却会让我们以为「打开了」，比直接失败更难查。
    if (!_mem.open(mem_name)) {
        return false;
    }
    if (!_mutex.open(mutex_name)) {
        _mem.close();
        return false;
    }
    _mem_name = mem_name;
    _mutex_name = mutex_name;
    _valid = true;
    return true;
}

void SharedFrameChannel::reset() {
    _valid = false;
    _mem.close();
    _mutex.close();
    _mem_name.clear();
    _mutex_name.clear();
    _reuse = 0;
}

SharedFrameChannelLease::SharedFrameChannelLease(const std::wstring &mem_name, const std::wstring &mutex_name)
    : _channel(&g_channel) {
    ::AcquireSRWLockExclusive(&g_channel_lock);
    _ok = _channel->ensure(mem_name, mutex_name);
    if (!_ok) {
        ::ReleaseSRWLockExclusive(&g_channel_lock);
        _channel = nullptr;
    }
}

SharedFrameChannelLease::~SharedFrameChannelLease() {
    if (_channel) {
        ::ReleaseSRWLockExclusive(&g_channel_lock);
    }
}

} // namespace op::hook
