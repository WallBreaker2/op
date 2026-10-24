// #include "stdafx.h"
#include "ICaptureBackend.h"
#include "../base/AutomationModes.h"
#include "../base/Utils.h"
#include <memory>

namespace op::capture {

ICaptureBackend::ICaptureBackend()
    : _hwnd(NULL), _shmem(nullptr), _pmutex(nullptr), _bind_state(0), _width(0), _height(0), _client_x(0),
      _client_y(0) {
}

ICaptureBackend::~ICaptureBackend() {
    bind_release();
    _bind_state = 0;
}

long ICaptureBackend::Bind(HWND hwnd, long flag) {
    // step 1 check window exists
    if (!::IsWindow(hwnd)) {
        // §72.48：OP 自身失败必须写出自己的错误码，否则 GetLastError 返回别的 API 的残留值。
        op_set_last_error(kOpErrWindowNotFound);
        return 0;
    }
    _hwnd = hwnd;
    // step 2. 准备资源
    if (bind_init() != 1) {
        bind_release();
        _bind_state = 0;
        return 0;
    }
    // step 3. 调用特定的绑定函数

    if (BindEx(hwnd, flag) == 1) {
        _bind_state = 1;
        op_set_last_error(0); // 成功：清掉之前可能残留的失败码
    } else {
        bind_release();
        _bind_state = 0;
        op_set_last_error(kOpErrBindFailed);
    }

    return _bind_state;
}

long ICaptureBackend::UnBind() {
    // setlog("UnBind(");
    if (_bind_state) {
        UnBindEx();
    }
    if (DeferBindReleaseAfterUnBind()) {
        // §72.48（原设计如此，补上说明）：Win10 的 WGC worker 线程此时还在写共享段，
        // 立刻 bind_release() 会让 worker 写已经释放的映射 → AV。所以这里故意不清
        // _bind_state、不释放 _shmem/_pmutex，资源留给下一次 Bind() 的 bind_init()
        // 或析构去收。只有 WgcCapture 在 Win10 worker 运行时会返回 true。
        return 1;
    }
    bind_release();
    _bind_state = 0;
    return 1;
}

long ICaptureBackend::bind_init() {
    RECT rc;
    assert(::IsWindow(_hwnd));
    ::GetWindowRect(_hwnd, &rc);
    const long width = rc.right - rc.left;
    const long height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) {
        setlog("bkdisplay::bind_init() invalid window size width=%ld height=%ld", width, height);
        op_set_last_error(kOpErrBadArgument);
        return 0;
    }
    // 只有注入式后端需要像素缓冲；其余后端只写 28 字节帧头。
    const size_t pixel_bytes =
        RequiresSharedPixelBuffer() ? static_cast<size_t>(width) * static_cast<size_t>(height) * 4 : 0;
    const size_t res_size = pixel_bytes + sizeof(FrameInfo);
    _shared_res_name = MakeOpSharedResourceName(_hwnd);
    _mutex_name = MakeOpMutexName(_hwnd);
    // setlog(L"mem=%s mutex=%s", _shared_res_name, _mutex_name);
    // bind_release();
    try {
        auto shmem = std::make_unique<SharedMemory>();
        if (!shmem->open_create(_shared_res_name, res_size)) {
            setlog(L"bkdisplay::bind_init() open shared memory failed %s size=%llu", _shared_res_name.c_str(),
                   static_cast<unsigned long long>(res_size));
            op_set_last_error(kOpErrSharedMemoryFailed);
            return 0;
        }
        auto mutex = std::make_unique<ProcessMutex>();
        if (!mutex->open_create(_mutex_name)) {
            setlog(L"bkdisplay::bind_init() open mutex failed %s", _mutex_name.c_str());
            op_set_last_error(kOpErrSharedMemoryFailed);
            return 0;
        }
        SAFE_DELETE(_shmem);
        SAFE_DELETE(_pmutex);
        _shmem = shmem.release();
        _pmutex = mutex.release();
        return 1;
    } catch (std::exception &e) {
        setlog(L"bkdisplay::bind_init() %s exception:%s", _shared_res_name.c_str(), _s2wstring(e.what()).c_str());
        op_set_last_error(kOpErrSharedMemoryFailed);
        return 0;
    }

    return 0;
}

long ICaptureBackend::bind_release() {
    SAFE_DELETE(_shmem);
    SAFE_DELETE(_pmutex);

    _hwnd = NULL;
    //_image_data = nullptr;
    return 0;
}

void ICaptureBackend::getFrameInfo(FrameInfo &info) {
    info = FrameInfo{};
    // 共享内存/互斥体在绑定失败或被延迟释放（WGC detach 路径）时可能为空；
    // 公开 API GetScreenFrameInfo 只判 IsBind()，这里必须自己兜住，否则就是空指针 AV。
    if (!_pmutex || !_shmem) {
        return;
    }
    if (_shmem->size() < sizeof(FrameInfo)) {
        setlog("getFrameInfo: shared memory too small size=%llu need=%llu",
               static_cast<unsigned long long>(_shmem->size()), static_cast<unsigned long long>(sizeof(FrameInfo)));
        return;
    }
    _pmutex->lock();
    memcpy(&info, _shmem->data<uchar>(), sizeof(FrameInfo));
    _pmutex->unlock();
}

// byte* bkdisplay::get_data() {
//	return _shmem->data<byte>();
// }

} // namespace op::capture
