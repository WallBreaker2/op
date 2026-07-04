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
    } else {
        bind_release();
        _bind_state = 0;
    }

    return _bind_state;
}

long ICaptureBackend::UnBind() {
    // setlog("UnBind(");
    if (_bind_state) {
        UnBindEx();
    }
    if (DeferBindReleaseAfterUnBind()) {
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
        return 0;
    }
    const size_t res_size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4 + sizeof(FrameInfo);
    _shared_res_name = MakeOpSharedResourceName(_hwnd);
    _mutex_name = MakeOpMutexName(_hwnd);
    // setlog(L"mem=%s mutex=%s", _shared_res_name, _mutex_name);
    // bind_release();
    try {
        auto shmem = std::make_unique<SharedMemory>();
        if (!shmem->open_create(_shared_res_name, res_size)) {
            setlog(L"bkdisplay::bind_init() open shared memory failed %s size=%llu", _shared_res_name.c_str(),
                   static_cast<unsigned long long>(res_size));
            return 0;
        }
        auto mutex = std::make_unique<ProcessMutex>();
        if (!mutex->open_create(_mutex_name)) {
            setlog(L"bkdisplay::bind_init() open mutex failed %s", _mutex_name.c_str());
            return 0;
        }
        SAFE_DELETE(_shmem);
        SAFE_DELETE(_pmutex);
        _shmem = shmem.release();
        _pmutex = mutex.release();
        return 1;
    } catch (std::exception &e) {
        setlog(L"bkdisplay::bind_init() %s exception:%s", _shared_res_name.c_str(), _s2wstring(e.what()).c_str());
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
    _pmutex->lock();
    memcpy(&info, _shmem->data<uchar>(), sizeof(FrameInfo));
    _pmutex->unlock();
}

// byte* bkdisplay::get_data() {
//	return _shmem->data<byte>();
// }

} // namespace op::capture
