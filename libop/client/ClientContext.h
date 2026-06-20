#pragma once
#ifndef OP_CLIENT_CONTEXT_H_
#define OP_CLIENT_CONTEXT_H_

#include "binding/BindingSession.h"
#include "image/ImageSearchService.h"
#include "window/WindowService.h"

#include <map>
#include <string>
#include <vector>

namespace op::internal {

struct ClientContext {
    explicit ClientContext(int client_id);

    // Windows API helpers
    WindowService window_service;
    // Window binding and capture/input state
    op::binding::BindingSession bkproc;
    // Image processing module
    op::image::ImageSearchService image_proc;
    // work path
    std::wstring curr_path;

    std::map<std::wstring, long> vkmap;
    std::vector<unsigned char> screenData;
    std::vector<unsigned char> screenDataBmp;
    std::wstring opPath;
    long screen_data_mode = 0;
    int id = 0;
};

} // namespace op::internal

#endif // OP_CLIENT_CONTEXT_H_
