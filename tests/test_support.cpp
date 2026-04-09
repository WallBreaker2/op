#include "test_support.h"

#include <algorithm>
#include <chrono>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace test_support {
namespace {

bool GetConfiguredOcrHostPort(std::wstring &host, INTERNET_PORT &port) {
    const std::wstring endpoint = GetConfiguredOcrEndpoint();
    URL_COMPONENTS parts = {};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(endpoint.c_str(), 0, 0, &parts))
        return false;

    host.assign(parts.lpszHostName, parts.dwHostNameLength);
    port = parts.nPort;
    return !host.empty();
}

bool WriteBinaryFile(const std::wstring &path, const std::vector<uchar> &buffer) {
    std::ofstream out(std::filesystem::path(path), std::ios::binary);
    if (!out)
        return false;
    out.write(reinterpret_cast<const char *>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    return static_cast<bool>(out);
}

} // namespace

void OpEnvironment::SetUp() {
    libop op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
}

std::wstring TrimCopy(const std::wstring &value) {
    const auto start = value.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos)
        return L"";
    const auto end = value.find_last_not_of(L" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::wstring GetEnvString(const wchar_t *name) {
    wchar_t *value = nullptr;
    size_t len = 0;
    if (_wdupenv_s(&value, &len, name) != 0 || value == nullptr)
        return L"";

    std::wstring out(value);
    free(value);
    return TrimCopy(out);
}

std::wstring GetConfiguredOcrEndpoint() {
    const std::wstring explicit_url = GetEnvString(L"OP_OCR_URL");
    if (!explicit_url.empty()) {
        URL_COMPONENTS parts = {};
        parts.dwStructSize = sizeof(parts);
        parts.dwSchemeLength = static_cast<DWORD>(-1);
        parts.dwHostNameLength = static_cast<DWORD>(-1);
        parts.dwUrlPathLength = static_cast<DWORD>(-1);
        parts.dwExtraInfoLength = static_cast<DWORD>(-1);
        if (!WinHttpCrackUrl(explicit_url.c_str(), 0, 0, &parts))
            return L"";

        std::wstring path;
        if (parts.dwUrlPathLength > 0)
            path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
        if (parts.dwExtraInfoLength > 0)
            path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
        if (path.empty() || path == L"/") {
            if (!explicit_url.empty() && explicit_url.back() == L'/')
                return explicit_url + L"api/v1/ocr";
            return explicit_url + L"/api/v1/ocr";
        }
        return explicit_url;
    }

    const std::wstring backend = GetEnvString(L"OP_OCR_BACKEND");
    if (!_wcsicmp(backend.c_str(), L"paddle") || !_wcsicmp(backend.c_str(), L"paddleocr") ||
        !_wcsicmp(backend.c_str(), L"paddle_ocr")) {
        return L"http://127.0.0.1:8081/api/v1/ocr";
    }

    return L"http://127.0.0.1:8080/api/v1/ocr";
}

bool IsOcrServerHealthy() {
    std::wstring host;
    INTERNET_PORT port = 0;
    if (!GetConfiguredOcrHostPort(host, port))
        return false;

    HINTERNET hSession = WinHttpOpen(L"op-test-health/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/health", nullptr, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    WinHttpSetTimeouts(hRequest, 1000, 1000, 1000, 1000);

    bool ok = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {
        DWORD status_code = 0;
        DWORD size = sizeof(status_code);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, WINHTTP_NO_HEADER_INDEX)) {
            ok = (status_code == 200);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

std::wstring PtrToWString(const void *ptr, bool hex) {
    std::wstringstream ss;
    if (hex) {
        ss << L"0x" << std::hex << reinterpret_cast<uintptr_t>(ptr);
    } else {
        ss << reinterpret_cast<uintptr_t>(ptr);
    }
    return ss.str();
}

std::vector<uchar> BuildBmp32TopDown(int width, int height, const std::vector<uchar> &bgra_pixels) {
    const size_t pixel_bytes = static_cast<size_t>(width) * height * 4;
    const size_t header_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    std::vector<uchar> buffer(header_size + pixel_bytes, 0);

    auto *bfh = reinterpret_cast<BITMAPFILEHEADER *>(buffer.data());
    auto *bih = reinterpret_cast<BITMAPINFOHEADER *>(buffer.data() + sizeof(BITMAPFILEHEADER));
    bfh->bfType = static_cast<WORD>(0x4d42);
    bfh->bfOffBits = static_cast<DWORD>(header_size);
    bfh->bfSize = static_cast<DWORD>(buffer.size());

    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biWidth = width;
    bih->biHeight = -height;
    bih->biPlanes = 1;
    bih->biBitCount = 32;
    bih->biCompression = BI_RGB;
    bih->biSizeImage = static_cast<DWORD>(pixel_bytes);

    memcpy(buffer.data() + header_size, bgra_pixels.data(), pixel_bytes);
    return buffer;
}

std::wstring ToLowerCopy(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) { return towlower(ch); });
    return value;
}

bool ContainsInsensitive(const std::wstring &haystack, const std::wstring &needle) {
    return ToLowerCopy(haystack).find(ToLowerCopy(needle)) != std::wstring::npos;
}

std::wstring NormalizeOcrLetters(const std::wstring &text) {
    std::wstring normalized;
    normalized.reserve(text.size());
    for (wchar_t ch : text) {
        wchar_t lower = towlower(ch);
        if (lower == L'1')
            lower = L'l';
        else if (lower == L'0')
            lower = L'o';

        if (lower >= L'a' && lower <= L'z')
            normalized.push_back(lower);
    }
    return normalized;
}

std::wstring ExtractDigits(const std::wstring &text) {
    std::wstring digits;
    digits.reserve(text.size());
    for (wchar_t ch : text) {
        if (ch >= L'0' && ch <= L'9')
            digits.push_back(ch);
    }
    return digits;
}

size_t EditDistance(const std::wstring &lhs, const std::wstring &rhs) {
    std::vector<size_t> prev(rhs.size() + 1);
    std::vector<size_t> curr(rhs.size() + 1);
    for (size_t j = 0; j <= rhs.size(); ++j)
        prev[j] = j;

    for (size_t i = 0; i < lhs.size(); ++i) {
        curr[0] = i + 1;
        for (size_t j = 0; j < rhs.size(); ++j) {
            const size_t cost = lhs[i] == rhs[j] ? 0 : 1;
            curr[j + 1] = std::min(std::min(curr[j] + 1, prev[j + 1] + 1), prev[j] + cost);
        }
        prev.swap(curr);
    }
    return prev[rhs.size()];
}

std::wstring GetTempBmpPath(const wchar_t *file_name) {
    wchar_t temp_path[MAX_PATH] = {0};
    DWORD len = GetTempPathW(MAX_PATH, temp_path);
    if (len == 0 || len >= MAX_PATH)
        return file_name;
    return std::wstring(temp_path) + file_name;
}

bool CreateConsoleLikeBmp(const std::wstring &path, const std::wstring &text) {
    const int width = 1400;
    const int height = 280;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screen_dc = GetDC(nullptr);
    if (!screen_dc)
        return false;

    HDC mem_dc = CreateCompatibleDC(screen_dc);
    void *bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(screen_dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, screen_dc);
    if (!mem_dc || !bitmap || !bits) {
        if (bitmap)
            DeleteObject(bitmap);
        if (mem_dc)
            DeleteDC(mem_dc);
        return false;
    }

    HGDIOBJ old_bitmap = SelectObject(mem_dc, bitmap);
    RECT rect = {0, 0, width, height};
    HBRUSH background = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(mem_dc, &rect, background);
    DeleteObject(background);

    SetBkMode(mem_dc, TRANSPARENT);
    SetTextColor(mem_dc, RGB(255, 255, 255));

    HFONT font = CreateFontW(-110, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                             CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    HGDIOBJ old_font = nullptr;
    if (font)
        old_font = SelectObject(mem_dc, font);

    DrawTextW(mem_dc, text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    GdiFlush();

    std::vector<uchar> pixels(static_cast<uchar *>(bits), static_cast<uchar *>(bits) + width * height * 4);
    std::vector<uchar> bmp = BuildBmp32TopDown(width, height, pixels);

    if (old_font)
        SelectObject(mem_dc, old_font);
    if (font)
        DeleteObject(font);
    SelectObject(mem_dc, old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(mem_dc);

    return WriteBinaryFile(path, bmp);
}

LRESULT CALLBACK SendStringWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

bool SendStringWindow::Create() {
    static const wchar_t *kClassName = L"OpSendStringTestWindow";
    static bool class_registered = false;
    HINSTANCE hinst = GetModuleHandleW(nullptr);

    if (!class_registered) {
        WNDCLASSW wc = {0};
        wc.lpfnWndProc = SendStringWindow::WndProc;
        wc.hInstance = hinst;
        wc.lpszClassName = kClassName;
        if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;
        class_registered = true;
    }

    parent = CreateWindowExW(0, kClassName, L"op-sendstring-test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             320, 120, nullptr, nullptr, hinst, nullptr);
    if (!parent)
        return false;

    edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | WS_TABSTOP, 8, 8, 280,
                           24, parent, nullptr, hinst, nullptr);
    if (!edit) {
        DestroyWindow(parent);
        parent = nullptr;
        return false;
    }

    ShowWindow(parent, SW_SHOW);
    UpdateWindow(parent);
    SetActiveWindow(parent);
    SetFocus(edit);
    return IsWindow(parent) && IsWindow(edit);
}

std::wstring SendStringWindow::GetEditText() const {
    wchar_t buffer[256] = {0};
    GetWindowTextW(edit, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
    return buffer;
}

SendStringWindow::~SendStringWindow() {
    if (parent)
        DestroyWindow(parent);
}

LRESULT CALLBACK MouseEventWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    MouseEventWindow *self = reinterpret_cast<MouseEventWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto *cs = reinterpret_cast<CREATESTRUCTW *>(lparam);
        self = reinterpret_cast<MouseEventWindow *>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    if (self) {
        switch (msg) {
        case WM_LBUTTONDOWN:
            self->left_down++;
            self->last_x = GET_X_LPARAM(lparam);
            self->last_y = GET_Y_LPARAM(lparam);
            return 0;
        case WM_LBUTTONUP:
            self->left_up++;
            self->last_x = GET_X_LPARAM(lparam);
            self->last_y = GET_Y_LPARAM(lparam);
            return 0;
        case WM_RBUTTONDOWN:
            self->right_down++;
            self->last_x = GET_X_LPARAM(lparam);
            self->last_y = GET_Y_LPARAM(lparam);
            return 0;
        case WM_RBUTTONUP:
            self->right_up++;
            self->last_x = GET_X_LPARAM(lparam);
            self->last_y = GET_Y_LPARAM(lparam);
            return 0;
        case WM_MOUSEWHEEL:
            self->wheel_count++;
            self->wheel_delta_sum += GET_WHEEL_DELTA_WPARAM(wparam);
            return 0;
        case OP_WM_LBUTTONDOWN:
            self->op_left_down++;
            return 0;
        case OP_WM_LBUTTONUP:
            self->op_left_up++;
            return 0;
        case OP_WM_RBUTTONDOWN:
            self->op_right_down++;
            return 0;
        case OP_WM_RBUTTONUP:
            self->op_right_up++;
            return 0;
        case OP_WM_MOUSEWHEEL:
            self->op_wheel_count++;
            self->op_wheel_delta_sum += static_cast<short>(HIWORD(wparam));
            return 0;
        default:
            break;
        }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

bool MouseEventWindow::Create() {
    static const wchar_t *kClassName = L"OpMouseEventTestWindow";
    static bool class_registered = false;
    HINSTANCE hinst = GetModuleHandleW(nullptr);

    if (!class_registered) {
        WNDCLASSW wc = {0};
        wc.lpfnWndProc = MouseEventWindow::WndProc;
        wc.hInstance = hinst;
        wc.lpszClassName = kClassName;
        if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;
        class_registered = true;
    }

    hwnd = CreateWindowExW(0, kClassName, L"op-mouseevent-test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           400, 260, nullptr, nullptr, hinst, this);
    if (!hwnd)
        return false;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return IsWindow(hwnd);
}

MouseEventWindow::~MouseEventWindow() {
    if (hwnd)
        DestroyWindow(hwnd);
}

void OcrTest::SetUp() {
    ASSERT_TRUE(IsOcrServerHealthy())
        << "OCR service is required for OcrTest. Configure OP_OCR_URL/OP_OCR_BACKEND and start the service before running the suite.";
}

void OcrTest::TearDown() {
    const std::wstring endpoint = GetConfiguredOcrEndpoint();
    if (!endpoint.empty()) {
        op.SetOcrEngine(endpoint.c_str(), L"", L"--timeout=3000");
    }
}

} // namespace test_support