#pragma once
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "SharedFrameExchange.h"
#include <cstdint>
#include <string>
#include <windows.h>

namespace op::hook {

// §72.51：注入侧等宿主释放共享段互斥的上限。宿主是另一个进程，可能被挂起、
// 被调试器冻住、或者干脆崩在临界区里；无限等会连带把 Present 和我们自己
// 进程里的全局 SRW 锁一起卡住。50ms 足够覆盖正常的一帧搬运（1080p 整帧约
// 0.5~1ms），超时就当这一帧没抢到，直接跳过。
constexpr DWORD kSharedFrameLockTimeoutMs = 50;

// 后台截图要把整帧写进「命名共享段」交给宿主进程（op_x64.dll）读取，
// 每帧都需要那块的映射视图和配套的跨进程互斥体。
//
// 原来的写法是每帧临时构造 SharedMemory / ProcessMutex：
//     SharedMemory mem; mem.open(name);        // OpenFileMappingW + MapViewOfFile
//     ProcessMutex mutex; mutex.open(name);    // OpenMutexW
//     ... 写完出作用域，析构里 UnmapViewOfFile + CloseHandle
// 这两个调用本身是微秒级（Open/Map 都是惰性的，不碰页面就不真把页连进来），
// 所以按「单帧总耗时」量的时候几乎看不见它们。真正的代价藏在后面：
// 每帧换一块新的映射视图，紧接着的整帧 memcpy 就每帧都要重新缺页。
// 实测 1080p（1920x1080x4 = 7.91 MB，本机 RTX 3060 Ti）：
//     每帧新映射 + memcpy        1.87 ms  (≈4.4 GB/s)
//     复用同一映射 + memcpy      0.63 ms  (≈12.2 GB/s)
//     复用同一映射 + 非临时写    0.47 ms  (≈16.4 GB/s)
// 「每帧重开映射」这一项就占了整帧搬运约三分之二的时间。
//
// 所以这里把映射和互斥体按名字缓存住复用：
//   - 名字不变、句柄有效 → 直接复用，零系统调用；
//   - 名字变化 / 首次 / 上次打开失败 → 重开一次；
//   - 每 kReopenIntervalFrames 帧强制重开：宿主在窗口重绑、退出重进时可能销毁旧段建新段，
//     我们手里的旧映射依然「有效」（句柄把旧内核对象吊住了），只是写的已经不是宿主在读的那块。
//     定期重开让这种漂移在约两秒内自愈。
class SharedFrameChannel {
  public:
    SharedFrameChannel() = default;
    ~SharedFrameChannel() = default;

    SharedFrameChannel(const SharedFrameChannel &) = delete;
    SharedFrameChannel &operator=(const SharedFrameChannel &) = delete;

    // 调用方必须已持有全局通道锁。
    bool ensure(const std::wstring &mem_name, const std::wstring &mutex_name);
    void reset();

    SharedMemory &mem() { return _mem; }
    ProcessMutex &mutex() { return _mutex; }
    bool valid() const { return _valid; }

    static constexpr unsigned kReopenIntervalFrames = 64;

  private:
    SharedMemory _mem;
    ProcessMutex _mutex;
    std::wstring _mem_name;
    std::wstring _mutex_name;
    unsigned _reuse{0};
    bool _valid{false};
};

// 一次「拿到通道并使用」的独占租约。
//
// 之所以要租约而不是让调用方自己 ensure：映射一旦被别的线程重开就会 UnmapViewOfFile，
// 正在往里写的那条线程会踩空指针。租约把「打开 + 使用」整段串起来，期间不会重开。
// 各 swapchain 的 Present 线程本来就该被跨进程互斥体串行化，这里不让它们并行并不额外损失。
class SharedFrameChannelLease {
  public:
    SharedFrameChannelLease(const std::wstring &mem_name, const std::wstring &mutex_name);
    ~SharedFrameChannelLease();

    SharedFrameChannelLease(const SharedFrameChannelLease &) = delete;
    SharedFrameChannelLease &operator=(const SharedFrameChannelLease &) = delete;

    bool ok() const { return _ok; }
    SharedMemory &mem() { return _channel->mem(); }
    ProcessMutex &mutex() { return _channel->mutex(); }

  private:
    SharedFrameChannel *_channel{nullptr};
    bool _ok{false};
};

// 注入侧专用：不拿写通道租约、按名字读一次「宿主请求的矩形」。
//
// 为什么要单独一条路径：D3D 后端的区域拷贝发生在 GPU 拷贝之前（CopySubresourceRegion 的
// 源矩形得先知道），而写通道的租约要一直持有到整段 Map+memcpy 结束 —— 在租约里读请求等于
// 把多个窗口的 Present 串行化。这里用独立的只读映射 + 自己的锁，读写不互相阻塞。
//
// 读不到（名字没设、段不存在、对面是旧宿主）一律返回 valid=false，调用方按整窗搬运。
op::hook::FrameRequestSnapshot ReadFrameRequestByName(const std::wstring &mem_name, std::uint32_t width,
                                                     std::uint32_t height);

} // namespace op::hook
