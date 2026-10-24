#pragma once
#ifndef OP_IMAGE_IMAGE_H_
#define OP_IMAGE_IMAGE_H_
#include "../base/Types.h"
#include <atlimage.h>
#include <limits>
#include <vector>

namespace image_detail {

class GlobalMemory {
  public:
    explicit GlobalMemory(HGLOBAL handle) noexcept : handle_(handle) {
    }

    ~GlobalMemory() {
        if (handle_) {
            ::GlobalFree(handle_);
        }
    }

    GlobalMemory(const GlobalMemory &) = delete;
    GlobalMemory &operator=(const GlobalMemory &) = delete;

    HGLOBAL get() const noexcept {
        return handle_;
    }

    explicit operator bool() const noexcept {
        return handle_ != nullptr;
    }

    HGLOBAL release() noexcept {
        HGLOBAL handle = handle_;
        handle_ = nullptr;
        return handle;
    }

  private:
    HGLOBAL handle_ = nullptr;
};

class GlobalLockGuard {
  public:
    explicit GlobalLockGuard(HGLOBAL handle) noexcept
        : handle_(handle), data_(handle ? ::GlobalLock(handle) : nullptr) {
    }

    ~GlobalLockGuard() {
        if (data_) {
            ::GlobalUnlock(handle_);
        }
    }

    GlobalLockGuard(const GlobalLockGuard &) = delete;
    GlobalLockGuard &operator=(const GlobalLockGuard &) = delete;

    void *data() const noexcept {
        return data_;
    }

    explicit operator bool() const noexcept {
        return data_ != nullptr;
    }

  private:
    HGLOBAL handle_ = nullptr;
    void *data_ = nullptr;
};

class StreamPtr {
  public:
    explicit StreamPtr(IStream *stream) noexcept : stream_(stream) {
    }

    ~StreamPtr() {
        if (stream_) {
            stream_->Release();
        }
    }

    StreamPtr(const StreamPtr &) = delete;
    StreamPtr &operator=(const StreamPtr &) = delete;

    IStream *get() const noexcept {
        return stream_;
    }

  private:
    IStream *stream_ = nullptr;
};

inline size_t CheckedImageBytes(int w, int h, int bytes_per_pixel) {
    // 图像尺寸来自截图和外部文件，先统一校验，避免乘法溢出后分配到错误大小。
    if (w <= 0 || h <= 0 || bytes_per_pixel <= 0)
        throw("invalid image size");

    const auto width = static_cast<size_t>(w);
    const auto height = static_cast<size_t>(h);
    const auto bpp = static_cast<size_t>(bytes_per_pixel);
    const size_t max_size = (std::numeric_limits<size_t>::max)();
    if (width > max_size / height || width * height > max_size / bpp) {
        throw("image size overflow");
    }
    return width * height * bpp;
}

inline bool IsSupportedImageFormat(const ATL::CImage &img) {
    const int pix_size = img.GetBPP() / 8;
    return img.GetWidth() > 0 && img.GetHeight() > 0 && (pix_size == 1 || pix_size == 3 || pix_size == 4);
}

// 仅测试用开关：设置环境变量 OP_TEST_FORCE_IMAGE_THROW=1 时，Image::create 会故意抛一次
// 异常，用来验证 COM 层能把 C++ 异常兜成 HRESULT（否则这条路径没法造出稳定用例）。
// 生产环境不设置该变量即完全无影响；进程内只读取一次（懒初始化）。
inline bool ForceThrowForTest() {
    static const bool forced = [] {
        wchar_t value[8] = {};
        const DWORD len = ::GetEnvironmentVariableW(L"OP_TEST_FORCE_IMAGE_THROW", value, 8);
        return len > 0 && value[0] == L'1';
    }();
    return forced;
}

} // namespace image_detail

namespace op {

struct Image {
    using iterator = unsigned __int32 *;
    Image() : width(0), height(0), pdata(nullptr) {
    }
    Image(int w, int h) : pdata(nullptr) {

        create(w, h);
    }
    // copy ctr
    Image(const Image &rhs) : pdata(nullptr) {

        if (rhs.empty()) {
            this->clear();
        } else {
            this->create(rhs.width, rhs.height);
            memcpy(this->pdata, rhs.pdata, width * height * 4);
        }
    }
    ~Image() {
        release();
    }

    void create(int w, int h) {
        // 测试注入点：见 image_detail::ForceThrowForTest 的注释。
        if (image_detail::ForceThrowForTest()) {
            throw("forced image create failure (OP_TEST_FORCE_IMAGE_THROW)");
        }
        // 尺寸没变就复用已有缓冲：截图主路径每帧都会 create(_src, w, h)，
        // 命中这里可以省掉一次 realloc 调用和潜在的整块搬移。
        if (pdata && width == w && height == h) {
            return;
        }
        const size_t bytes = image_detail::CheckedImageBytes(w, h, 4);
        void *new_data = pdata ? realloc(pdata, bytes) : malloc(bytes);
        if (new_data == nullptr)
            throw("memory not enough");
        pdata = static_cast<unsigned char *>(new_data);
        width = w;
        height = h;
    }
    void release() {
        width = height = 0;
        if (pdata)
            free(pdata);
        pdata = nullptr;
    }

    int size() const {
        return width * height;
    }
    void clear() {
        width = height = 0;
    }

    bool empty() const {
        return width == 0;
    }

    Image &operator=(const Image &rhs) {
        if (rhs.empty()) {
            this->clear();
        } else if (this != &rhs) {
            this->create(rhs.width, rhs.height);
            memcpy(this->pdata, rhs.pdata, width * height * 4);
        }
        return *this;
    }
    // BMP 直读快路径（§72.40）：GDI+ 解一张 3KB 的 BMP 要 ~1.8ms，比一次 HTTP OCR 往返还贵。
    // 这里只处理最常见的 BI_RGB + 24/32 位 + 行对齐；调色板 / RLE / 16 位 / PNG / JPEG 一律
    // 返回 false，交回 GDI+ 走原路，所以行为对“支持哪些格式”没有任何变化。
    bool read_bmp(const unsigned char *data, size_t size) {
        if (!data || size < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
            return false;
        BITMAPFILEHEADER file_header = {};
        memcpy(&file_header, data, sizeof(file_header));
        if (file_header.bfType != 0x4D42) // 'BM'
            return false;
        BITMAPINFOHEADER info = {};
        memcpy(&info, data + sizeof(file_header), sizeof(info));
        if (info.biSize < sizeof(BITMAPINFOHEADER) || info.biPlanes != 1 || info.biCompression != BI_RGB)
            return false;
        if (info.biBitCount != 24 && info.biBitCount != 32)
            return false;
        if (info.biWidth <= 0 || info.biHeight == 0 || info.biHeight == (std::numeric_limits<int>::min)())
            return false;

        const int bmp_width = info.biWidth;
        const int bmp_height = info.biHeight < 0 ? -info.biHeight : info.biHeight;
        const size_t src_pitch =
            ((static_cast<size_t>(bmp_width) * info.biBitCount + 31) / 32) * 4;
        const size_t offset = static_cast<size_t>(file_header.bfOffBits);
        // 行数 × 行跨度的乘法先按 size_t 算，再用“跨度 > size / 行数”判溢出。
        if (bmp_height <= 0 || src_pitch == 0 || src_pitch > size / static_cast<size_t>(bmp_height))
            return false;
        if (offset < sizeof(file_header) + info.biSize || offset + src_pitch * bmp_height > size)
            return false;

        create(bmp_width, bmp_height);
        const unsigned char *src = data + offset;
        const bool top_down = info.biHeight < 0;
        const size_t dst_pitch = static_cast<size_t>(bmp_width) * 4;
        for (int i = 0; i < bmp_height; ++i) {
            const unsigned char *row = src + (top_down ? i : bmp_height - 1 - i) * src_pitch;
            unsigned char *dst = pdata + static_cast<size_t>(i) * dst_pitch;
            if (info.biBitCount == 32) {
                // 与 translate() 的 pixSize==4 分支保持一致：第 4 字节原样搬，不强行置 0xff。
                memcpy(dst, row, dst_pitch);
            } else {
                for (int x = 0; x < bmp_width; ++x) {
                    dst[x * 4 + 0] = row[x * 3 + 0];
                    dst[x * 4 + 1] = row[x * 3 + 1];
                    dst[x * 4 + 2] = row[x * 3 + 2];
                    dst[x * 4 + 3] = 0xff;
                }
            }
        }
        return true;
    }

    bool read_bmp_file(LPCTSTR file) {
        HANDLE handle = ::CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            return false;
        LARGE_INTEGER file_size = {};
        std::vector<unsigned char> buffer;
        bool loaded = false;
        // 超过 64MB 的文件交给 GDI+：这里是一次性整读，不值得为超大图占内存。
        if (::GetFileSizeEx(handle, &file_size) && file_size.QuadPart > 0 &&
            file_size.QuadPart <= (64LL << 20)) {
            buffer.resize(static_cast<size_t>(file_size.QuadPart));
            DWORD read = 0;
            loaded = ::ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr) != 0 &&
                     static_cast<size_t>(read) == buffer.size();
        }
        ::CloseHandle(handle);
        return loaded && read_bmp(buffer.data(), buffer.size());
    }

    bool read(LPCTSTR file) {
        clear();
        if (read_bmp_file(file))
            return true;
        ATL::CImage img;
        HRESULT hr = img.Load(file);
        if (hr == S_OK) {
            if (!image_detail::IsSupportedImageFormat(img))
                return false;
            create(img.GetWidth(), img.GetHeight());
            translate((unsigned char *)img.GetBits(), img.GetBPP() / 8, img.GetPitch());
            return true;
        } else {
            return false;
        }
    }
    bool read(void *pMemData, long len) {
        clear();
        if (!pMemData || len <= 0)
            return false;
        if (read_bmp(static_cast<const unsigned char *>(pMemData), static_cast<size_t>(len)))
            return true;

        ATL::CImage img;
        const auto byte_len = static_cast<SIZE_T>(len);
        image_detail::GlobalMemory memory(::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, byte_len));
        if (!memory)
            return false;

        {
            image_detail::GlobalLockGuard lock(memory.get());
            if (!lock)
                return false;
            if (memcpy_s(lock.data(), byte_len, pMemData, byte_len) != 0)
                return false;
        }

        IStream *raw_stream = nullptr;
        if (CreateStreamOnHGlobal(memory.get(), TRUE, &raw_stream) != S_OK || !raw_stream)
            return false;
        memory.release();

        image_detail::StreamPtr stream(raw_stream);
        HRESULT hr = img.Load(stream.get());
        if (hr == S_OK) {
            create(img.GetWidth(), img.GetHeight());
            translate((unsigned char *)img.GetBits(), img.GetBPP() / 8, img.GetPitch());
            return true;
        } else {
            return false;
        }
    }

    bool read(ATL::CImage *img) {
        if (!img)
            return false;
        if (!image_detail::IsSupportedImageFormat(*img))
            return false;
        create(img->GetWidth(), img->GetHeight());
        translate((unsigned char *)img->GetBits(), img->GetBPP() / 8, img->GetPitch());
        return true;
    }

    bool write(LPCTSTR file) {
        if (empty())
            return false;
        ATL::CImage img;

        img.Create(width, height, 32);
        auto pdst = (unsigned char *)img.GetBits();
        auto psrc = pdata;
        const int pitch = img.GetPitch();
        const size_t row_bytes = static_cast<size_t>(width) * 4;
        for (int i = 0; i < height; ++i) {
            // 逐像素拷贝改成整行 memcpy，保存整屏 BMP/PNG 时差异很明显。
            memcpy(pdst, psrc, row_bytes);
            pdst += pitch;
            psrc += row_bytes;
        }
        return img.Save(file) == S_OK;
    }
    void translate(unsigned char *psrc, int pixSize, int pitch) {
        auto pdst = pdata;
        // gray
        if (pixSize == 1) {
            for (int i = 0; i < height; ++i) {
                for (int j = 0; j < width; ++j) {
                    *pdst++ = psrc[j];
                    *pdst++ = psrc[j];
                    *pdst++ = psrc[j];
                    *pdst++ = 0xff;
                }
                psrc += pitch;
            }
        } // bgr
        else if (pixSize == 3) {
            for (int i = 0; i < height; ++i) {
                for (int j = 0; j < width; ++j) {
                    *pdst++ = psrc[j * 3 + 0];
                    *pdst++ = psrc[j * 3 + 1];
                    *pdst++ = psrc[j * 3 + 2];
                    *pdst++ = 0xff;
                }
                psrc += pitch;
            }
        } else if (pixSize == 4) {
            for (int i = 0; i < height; ++i) {
                for (int j = 0; j < width; ++j) {
                    *pdst++ = psrc[j * 4 + 0];
                    *pdst++ = psrc[j * 4 + 1];
                    *pdst++ = psrc[j * 4 + 2];
                    *pdst++ = psrc[j * 4 + 3];
                }
                psrc += pitch;
            }
        }
    }
    template <typename Tp> Tp at(int y, int x) const {
        return ((Tp *)pdata)[y * width + x];
    }
    template <typename Tp> Tp &at(int y, int x) {
        return ((Tp *)pdata)[y * width + x];
    }
    template <typename Tp> Tp *ptr(int y) {
        return (Tp *)(pdata + y * width * 4);
    }

    template <typename Tp> const Tp *ptr(int y) const {
        return (Tp *)(pdata + y * width * 4);
    }

    iterator begin() {
        return (iterator)pdata;
    }
    iterator end() {
        return (iterator)pdata + width * height;
    }

    iterator begin() const {
        return (iterator)pdata;
    }
    iterator end() const {
        return (iterator)pdata + width * height;
    }

    void fill(unsigned int val) {
        std::fill(begin(), end(), val);
    }
    void fill(int row, int col, int h, int w, unsigned int val) {
        for (int i = 0; i < h; ++i) {
            auto p = ptr<unsigned int>(row + i) + col;
            std::fill(p, p + w, val);
        }
    }
    int width, height;
    unsigned char *pdata;
};
// 单通道图像
struct ImageBin {
    using iterator = unsigned char *;
    ImageBin() : width(0), height(0) {
    }
    ImageBin(const ImageBin &rhs) {
        this->width = rhs.width;
        this->height = rhs.height;
        this->pixels = rhs.pixels;
    }
    void create(int w, int h) {
        image_detail::CheckedImageBytes(w, h, 1);
        width = w;
        height = h;
        pixels.resize(static_cast<size_t>(w) * static_cast<size_t>(h));
    }
    void clear() {
        width = height = 0;
    }
    int size() const {
        return width * height;
    }
    bool empty() const {
        return width == 0;
    }
    unsigned char *data() {
        return pixels.data();
    }
    ImageBin &operator=(const ImageBin &rhs) {
        this->width = rhs.width;
        this->height = rhs.height;
        this->pixels = rhs.pixels;
        return *this;
    }
    unsigned char at(int y, int x) const {
        return pixels[y * width + x];
    }
    unsigned char &at(int y, int x) {
        return pixels[y * width + x];
    }

    unsigned char *ptr(int y) {
        return pixels.data() + y * width;
    }

    unsigned char const *ptr(int y) const {
        return pixels.data() + y * width;
    }

    void fromImage4(const Image &img4) {
        create(img4.width, img4.height);
        auto psrc = img4.pdata;
        for (size_t i = 0; i < pixels.size(); ++i) {
            // pixels[i] = (psrc[0] + psrc[1] + psrc[2]) / 3;
            //  Gray = (R*299 + G*587 + B*114 + 500) / 1000
            pixels[i] = (psrc[2] * 299 + psrc[1] * 587 + psrc[0] * 114 + 500) / 1000;
            psrc += 4;
        }
    }

    bool write(LPCTSTR file) {
        if (empty())
            return false;
        ATL::CImage img;

        img.Create(width, height, 32);
        auto pdst = (unsigned char *)img.GetBits();
        auto psrc = pixels.data();
        int pitch = img.GetPitch();
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                //((int*)pdst)[j] = ((int*)psrc)[j];
                uchar v = psrc[j] == 1 ? 0xff : 0;
                pdst[j * 4] = pdst[j * 4 + 1] = pdst[j * 4 + 2] = v;
                pdst[j * 4 + 3] = 0xff;
            }
            pdst += pitch;
            psrc += width;
        }
        return img.Save(file) == S_OK;
    }

    iterator begin() {
        return pixels.data();
    }
    iterator end() {
        return pixels.data() + pixels.size();
    }
    int width, height;
    std::vector<unsigned char> pixels;
};

using inputimg = const Image &;
using outputimg = Image &;

using inputbin = const ImageBin &;
using outputbin = ImageBin &;

} // namespace op

#endif // OP_IMAGE_IMAGE_H_
