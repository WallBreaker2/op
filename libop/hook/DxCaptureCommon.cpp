#include "DxCaptureCommon.h"

#include "DisplayHook.h"
#include "../base/AutomationModes.h"
#include <emmintrin.h>

namespace op::hook {

namespace {

// 小于这个字节数不值得走非临时存储：movntdq 需要目的地 16 字节对齐、末尾要 sfence，
// 小块反而比 memcpy（走 rep movsb / SSE 循环）慢。
constexpr size_t kStreamCopyThreshold = 256 * 1024;

// 非临时存储整块写：目的地对齐后走 movntdq，末尾 sfence 保证可见性。
// 共享段是「只写一遍、之后由另一个进程读」的大块内存，绕开 read-for-ownership 能明显省时间。
void streaming_copy(char *dst_, const char *src_, size_t bytes) {
    auto dst = reinterpret_cast<unsigned char *>(dst_);
    auto src = reinterpret_cast<const unsigned char *>(src_);
    size_t i = 0;
    // 共享段的像素区起点是「段基址 + sizeof(FrameInfo)」（28 字节），不保证 16 字节对齐，
    // 先把头部几个字节用普通写补齐。
    while (i < bytes && (reinterpret_cast<uintptr_t>(dst + i) & 15) != 0) {
        dst[i] = src[i];
        ++i;
    }
    for (; i + 64 <= bytes; i += 64) {
        const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
        const __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i + 16));
        const __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i + 32));
        const __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i + 48));
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i), a);
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i + 16), b);
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i + 32), c);
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i + 48), d);
    }
    for (; i + 16 <= bytes; i += 16) {
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i),
                         _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i)));
    }
    _mm_sfence();
    for (; i < bytes; ++i) {
        dst[i] = src[i];
    }
}

// B8G8R8A8 且没有行尾填充：整帧就是一段连续内存，一次写完。
inline void copy_packed_bgra(char *dst_, const char *src_, size_t bytes) {
    if (bytes >= kStreamCopyThreshold) {
        streaming_copy(dst_, src_, bytes);
    } else {
        ::memcpy(dst_, src_, bytes);
    }
}

// R8G8B8A8 → B8G8R8A8：只交换每个像素的首尾两个字节（R 与 B），G 和 A 原地不动。
// 逐像素版本在 1080p 上要跑 200 多万次（实测约 0.75ms，占整帧四成），这里按 4 像素一组做。
void swap_rb_sse(char *dst_, const char *src_, size_t bytes) {
    auto dst = reinterpret_cast<unsigned char *>(dst_);
    auto src = reinterpret_cast<const unsigned char *>(src_);
    // 小端下 32 位像素是 A<<24 | R<<16 | G<<8 | B，所以：
    //   B 在 bit0-7  → 左移 16 位到 bit16-23
    //   R 在 bit16-23 → 右移 16 位到 bit0-7
    //   G/A（bit8-15、bit24-31）用掩码原样保留
    const __m128i mask_b = _mm_set1_epi32(0x000000FF);
    const __m128i mask_r = _mm_set1_epi32(0x000000FF << 16);
    const __m128i mask_keep = _mm_set1_epi32(static_cast<int>(0xFF00FF00u));
    size_t i = 0;
    for (; i + 16 <= bytes; i += 16) {
        const __m128i x = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
        const __m128i b = _mm_slli_epi32(_mm_and_si128(x, mask_b), 16);
        const __m128i r = _mm_srli_epi32(_mm_and_si128(x, mask_r), 16);
        const __m128i y = _mm_or_si128(_mm_and_si128(x, mask_keep), _mm_or_si128(b, r));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i), y);
    }
    for (; i + 4 <= bytes; i += 4) {
        dst[i + 0] = src[i + 2]; // b
        dst[i + 1] = src[i + 1]; // g
        dst[i + 2] = src[i + 0]; // r
        dst[i + 3] = src[i + 3]; // a
    }
}

// BGRA 源（其实就是 B8G8R8A8）按行搬到紧凑 32bpp 目标：行内直接整段拷贝。
inline void copy_rows_bgra(char *dst_, const char *src_, int rows, int dstPitch, int rowPitch) {
    for (int i = 0; i < rows; ++i) {
        ::memcpy(dst_, src_, static_cast<size_t>(dstPitch));
        dst_ += dstPitch;
        src_ += rowPitch;
    }
}

// R8G8B8A8 源按行交换：目标紧凑、源有行尾填充，所以逐行处理。
inline void swap_rows_rb(char *dst_, const char *src_, int rows, int dstPitch, int rowPitch) {
    for (int i = 0; i < rows; ++i) {
        swap_rb_sse(dst_, src_, static_cast<size_t>(dstPitch));
        dst_ += dstPitch;
        src_ += rowPitch;
    }
}

} // namespace

void CopyImageData(char *dst_, const char *src_, int rows_, int cols_, int rowPitch, int fmt_) {
    // assert(rowsPitch >= cols_ * 4);
    if (rowPitch == cols_ * (fmt_ == IBF_R8G8B8 ? 3 : 4)) {
        if (fmt_ == IBF_B8G8R8A8) {
            copy_packed_bgra(dst_, src_, static_cast<size_t>(rows_) * static_cast<size_t>(cols_) * 4);
        } else if (fmt_ == IBF_R8G8B8A8) {
            swap_rb_sse(dst_, src_, static_cast<size_t>(rows_) * static_cast<size_t>(cols_) * 4);
        } else {
            // pixels count
            int n = rows_ * cols_;
            for (int i = 0; i < n; ++i) {
                *dst_++ = *src_++;
                *dst_++ = *src_++;
                *dst_++ = *src_++;
                *dst_++ = (char)0xff; // dst is 4 B
            }
        }
    } else {
        const int dstPitch = cols_ * 4;
        if (fmt_ == IBF_B8G8R8A8) {
            copy_rows_bgra(dst_, src_, rows_, dstPitch, rowPitch);
        } else if (fmt_ == IBF_R8G8B8A8) {
            swap_rows_rb(dst_, src_, rows_, dstPitch, rowPitch);
        } else {
            for (int i = 0; i < rows_; ++i) {
                for (int j = 0; j < cols_; ++j) {
                    const char *p = src_ + j * 3; // offset
                    dst_[0] = p[0];               // b
                    dst_[1] = p[1];               // g
                    dst_[2] = p[2];               // r
                    dst_[3] = (char)0xff;         // a
                    dst_ += 4;                    // notice that dst ptr is increasing
                }
                src_ += rowPitch; // row increase
            }
        }
    }
}

DXGI_FORMAT NormalizeDxgiFormat(DXGI_FORMAT format) {
    // 读回到 CPU 前统一转成非 SRGB 格式，避免 staging/resolve 阶段因格式不匹配失败。
    if (format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) {
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    }
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    return format;
}

int GetImageBufferFormat(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT_B8G8R8A8_UNORM || format == DXGI_FORMAT_B8G8R8X8_UNORM ||
        format == DXGI_FORMAT_B8G8R8A8_TYPELESS || format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ||
        format == DXGI_FORMAT_B8G8R8X8_TYPELESS || format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB) {
        return IBF_B8G8R8A8;
    }
    return IBF_R8G8B8A8;
}

} // namespace op::hook
