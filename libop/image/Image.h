#pragma once
#ifndef OP_IMAGE_IMAGE_H_
#define OP_IMAGE_IMAGE_H_
#include "../runtime/Types.h"
#include <atlimage.h>
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
    explicit GlobalLockGuard(HGLOBAL handle) noexcept : handle_(handle), data_(handle ? ::GlobalLock(handle) : nullptr) {
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
        width = w, height = h;
        if (!pdata) {
            pdata = (unsigned char *)malloc(w * h * 4);
        } else {
            pdata = (unsigned char *)realloc(pdata, w * h * 4);
        }
        if (pdata == nullptr)
            throw("memory not enough");
    }
    void release() {
        width = height = 0;
        if (pdata)
            free(pdata);
        pdata = nullptr;
    }

    int size() {
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
    bool read(LPCTSTR file) {
        clear();
        ATL::CImage img;
        HRESULT hr = img.Load(file);
        if (hr == S_OK) {
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
        int pitch = img.GetPitch();
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                ((int *)pdst)[j] = ((int *)psrc)[j];
            }
            pdst += pitch;
            psrc += width * 4;
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
        width = w, height = h;
        pixels.resize(w * h);
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
