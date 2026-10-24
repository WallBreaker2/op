#pragma once

#include "../capture/FrameInfo.h"
#include "../ipc/SharedMemory.h"
#include "SharedFrame.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <windows.h>

// 「只搬请求区域」的跨进程协商区（doc/defect_fixes.md §72.18）。
//
// 背景：注入侧每帧把**整窗**搬进共享段（1080p 7.91 MB），而宿主 `Capture(x, y, w, h)`
// 经常只要一小块（比如 320×180）。整窗搬运里既有 GPU 侧的 CopyResource，也有 CPU 侧的
// 整帧 memcpy，两者都跟窗口大小成正比。
//
// 线格式约束（§34 定死、不许单方面改）：`FrameInfo` 就是 28 字节，一个字段都不能动。
// 所以协商区**不插在帧头和像素之间**（那会让旧宿主把协商区当像素读），而是放到
// 像素区之后：
//      [FrameInfo 28B][像素区 width*height*4][FrameExchange]
// 偏移只跟窗口尺寸有关（帧头里的 width/height 永远记整窗），跟请求矩形无关。
//
// 三种混搭都必须成立：
//   旧宿主 + 新注入：旧宿主按 RequiredSharedFrameBytes 分配，协商区不存在（页取整读出来是垃圾，
//                    magic 对不上）→ 注入侧照旧整窗搬运，行为与旧版一致。
//   新宿主 + 旧注入：旧注入不认协商区，也从不写响应 → 宿主读到的响应 magic 为 0
//                    （CreateFileMapping 清零）→ 走整窗路径。
//   新宿主 + 新注入：宿主发布请求，注入侧按矩形搬运并回写响应，宿主据此按矩形读像素。
//
// 一致性：请求由宿主在持锁状态下写、注入侧在**不持锁**的状态下读（它要在 GPU 拷贝之前
// 就决定搬哪块，那时还拿不到写通道的租约），所以用 seqlock：seq 为奇数表示正在写。
namespace op::hook {

constexpr std::uint32_t kFrameExchangeMagic = 0x4F505252u; // 'OPRR'
constexpr std::uint32_t kFrameExchangeVersion = 1u;

// 响应里的搬运模式。
constexpr std::uint32_t kFrameModeFull = 0u;   // 像素区是整窗
constexpr std::uint32_t kFrameModeRegion = 1u; // 像素区只有请求的那块（行方向与该后端的整窗一致）

#pragma pack(push, 1)
struct FrameRequest {
    std::atomic<std::uint32_t> seq; // 奇数=正在写
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t ttl_ms; // 超过这么久没有新请求，注入侧就退回整窗
    std::int32_t x;
    std::int32_t y;
    std::int32_t w;
    std::int32_t h;
    std::uint64_t tick_ms; // 发布时刻（GetTickCount64）
};

struct FrameResponse {
    std::atomic<std::uint32_t> seq; // 奇数=正在写
    std::uint32_t mode;
    std::int32_t x;
    std::int32_t y;
    std::int32_t w;
    std::int32_t h;
    std::uint32_t magic;
    std::uint32_t version;
};

struct FrameExchange {
    FrameRequest request;
    FrameResponse response;
};
#pragma pack(pop)

static_assert(sizeof(FrameExchange) == 72, "FrameExchange 是跨进程协商区，布局不许变");

// 请求在共享段里的偏移：帧头 + 整窗像素区。
inline std::size_t FrameExchangeOffset(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
        return 0;
    }
    const std::uint64_t pixels = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    const std::uint64_t offset = sizeof(op::capture::FrameInfo) + pixels * 4u;
    if (offset > (std::numeric_limits<std::size_t>::max)()) {
        return 0;
    }
    return static_cast<std::size_t>(offset);
}

// 共享段是否容得下协商区。注意 SharedMemory::open 的 size() 是页粒度，可能比宿主真正
// 申请的字节数大，所以这里只是必要条件，真正的判据是请求里的 magic/version。
inline bool FrameExchangeFits(op::SharedMemory &mem, std::uint32_t width, std::uint32_t height) {
    const std::size_t offset = FrameExchangeOffset(width, height);
    return offset != 0 && mem.size() >= offset + sizeof(FrameExchange);
}

inline FrameExchange *FrameExchangeAt(op::SharedMemory &mem, std::uint32_t width, std::uint32_t height) {
    if (!FrameExchangeFits(mem, width, height)) {
        return nullptr;
    }
    return reinterpret_cast<FrameExchange *>(mem.data<unsigned char>() + FrameExchangeOffset(width, height));
}

// 宿主侧：发布请求（调用方已持跨进程互斥体）。
inline void PublishFrameRequest(op::SharedMemory &mem, std::uint32_t width, std::uint32_t height, int x, int y,
                                int w, int h, std::uint32_t ttl_ms) {
    FrameExchange *exchange = FrameExchangeAt(mem, width, height);
    if (!exchange) {
        return;
    }
    FrameRequest &request = exchange->request;
    const std::uint32_t seq = request.seq.load(std::memory_order_relaxed);
    request.seq.store(seq + 1, std::memory_order_relaxed); // 奇数：注入侧看到就跳过
    std::atomic_thread_fence(std::memory_order_release);
    request.magic = kFrameExchangeMagic;
    request.version = kFrameExchangeVersion;
    request.ttl_ms = ttl_ms;
    request.x = x;
    request.y = y;
    request.w = w;
    request.h = h;
    request.tick_ms = static_cast<std::uint64_t>(::GetTickCount64());
    request.seq.store(seq + 2, std::memory_order_release); // 偶数：稳定
}

struct FrameRequestSnapshot {
    bool valid{false};
    std::uint32_t seq{0};
    int x{0};
    int y{0};
    int w{0};
    int h{0};
};

// 注入侧：读当前请求。不持锁，靠 seqlock 判完整；过期（ttl）视为没有请求。
inline FrameRequestSnapshot ReadFrameRequest(op::SharedMemory &mem, std::uint32_t width, std::uint32_t height) {
    FrameRequestSnapshot out;
    FrameExchange *exchange = FrameExchangeAt(mem, width, height);
    if (!exchange) {
        return out;
    }
    const FrameRequest &request = exchange->request;
    for (int attempt = 0; attempt < 2; ++attempt) {
        const std::uint32_t before = request.seq.load(std::memory_order_acquire);
        if (before == 0 || (before & 1u) != 0) {
            return out;
        }
        const std::uint32_t magic = request.magic;
        const std::uint32_t version = request.version;
        const std::uint32_t ttl = request.ttl_ms;
        const int x = request.x;
        const int y = request.y;
        const int w = request.w;
        const int h = request.h;
        const std::uint64_t tick = request.tick_ms;
        std::atomic_thread_fence(std::memory_order_acquire);
        if (request.seq.load(std::memory_order_relaxed) != before) {
            continue;
        }
        if (magic != kFrameExchangeMagic || version != kFrameExchangeVersion) {
            return out;
        }
        if (w <= 0 || h <= 0) {
            return out;
        }
        if (ttl != 0) {
            const std::uint64_t now = static_cast<std::uint64_t>(::GetTickCount64());
            if (now > tick && now - tick > static_cast<std::uint64_t>(ttl)) {
                return out;
            }
        }
        out.valid = true;
        out.seq = before;
        out.x = x;
        out.y = y;
        out.w = w;
        out.h = h;
        return out;
    }
    return out;
}

// 注入侧：写完像素后登记本帧实际搬了什么。`request_seq` 取本帧采纳的请求 seq（0 表示没有请求）。
inline void WriteFrameResponse(op::SharedMemory &mem, std::uint32_t width, std::uint32_t height,
                               std::uint32_t mode, std::uint32_t request_seq, int x, int y, int w, int h) {
    (void)request_seq;
    FrameExchange *exchange = FrameExchangeAt(mem, width, height);
    if (!exchange) {
        return;
    }
    FrameResponse &response = exchange->response;
    const std::uint32_t seq = response.seq.load(std::memory_order_relaxed);
    response.seq.store(seq + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    response.mode = mode;
    response.x = x;
    response.y = y;
    response.w = w;
    response.h = h;
    response.magic = kFrameExchangeMagic;
    response.version = kFrameExchangeVersion;
    response.seq.store(seq + 2, std::memory_order_release);
}

struct FrameResponseSnapshot {
    bool valid{false};
    std::uint32_t mode{0};
    std::uint32_t seq{0};
    int x{0};
    int y{0};
    int w{0};
    int h{0};
};

// 宿主侧：读注入侧登记的搬运模式（持锁调用）。
inline FrameResponseSnapshot ReadFrameResponse(op::SharedMemory &mem, std::uint32_t width, std::uint32_t height) {
    FrameResponseSnapshot out;
    FrameExchange *exchange = FrameExchangeAt(mem, width, height);
    if (!exchange) {
        return out;
    }
    const FrameResponse &response = exchange->response;
    for (int attempt = 0; attempt < 2; ++attempt) {
        const std::uint32_t before = response.seq.load(std::memory_order_acquire);
        if (before == 0 || (before & 1u) != 0) {
            return out;
        }
        const std::uint32_t mode = response.mode;
        const int x = response.x;
        const int y = response.y;
        const int w = response.w;
        const int h = response.h;
        const std::uint32_t magic = response.magic;
        const std::uint32_t version = response.version;
        std::atomic_thread_fence(std::memory_order_acquire);
        if (response.seq.load(std::memory_order_relaxed) != before) {
            continue;
        }
        if (magic != kFrameExchangeMagic || version != kFrameExchangeVersion) {
            return out;
        }
        out.valid = true;
        out.mode = mode;
        out.seq = before;
        out.x = x;
        out.y = y;
        out.w = w;
        out.h = h;
        return out;
    }
    return out;
}

// 把宿主请求归一化成"要不要按矩形搬"：越界、整窗、过期都算没有请求。
// 四个后端共用，避免每个后端各写一遍边界判断（D3D11 是最早的一版，逻辑等价）。
inline FrameRequestSnapshot NormalizeFrameRequest(const FrameRequestSnapshot &request, std::uint32_t width,
                                                  std::uint32_t height) {
    if (!request.valid || request.w <= 0 || request.h <= 0 || request.x < 0 || request.y < 0) {
        return FrameRequestSnapshot{};
    }
    if (static_cast<std::uint32_t>(request.x) + static_cast<std::uint32_t>(request.w) > width ||
        static_cast<std::uint32_t>(request.y) + static_cast<std::uint32_t>(request.h) > height) {
        return FrameRequestSnapshot{};
    }
    if (request.x == 0 && request.y == 0 && static_cast<std::uint32_t>(request.w) == width &&
        static_cast<std::uint32_t>(request.h) == height) {
        return FrameRequestSnapshot{};
    }
    return request;
}

// 宿主侧分配共享段时要多留一个协商区（旧宿主不会，注入侧据此判断对面版本）。
inline std::size_t RequiredSharedFrameBytesWithExchange(std::uint64_t width, std::uint64_t height) {
    const std::size_t base = RequiredSharedFrameBytes(width, height);
    if (base == 0) {
        return 0;
    }
    return base + sizeof(FrameExchange);
}

} // namespace op::hook
