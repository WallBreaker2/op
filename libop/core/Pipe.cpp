// #include "stdafx.h"
#include "Pipe.h"
#include "globalVar.h"
#include "helpfunc.h"
#include <chrono>
#include <iostream>
#include <vector>
Pipe::Pipe() {
    _hread = _hwrite = _hread2 = _hwrite2 = nullptr;
    _hprocess = nullptr;
    _reading = false;
    _pthread = nullptr;
    ZeroMemory(&_pi, sizeof(_pi));
    ZeroMemory(&_si, sizeof(_si));
    ZeroMemory(&_ai, sizeof(_ai));
}

Pipe::~Pipe() {
    close();
}

int Pipe::open(const string &cmd) {
    close();

    _ai.nLength = sizeof(SECURITY_ATTRIBUTES);
    _ai.bInheritHandle = true;
    _ai.lpSecurityDescriptor = nullptr;
    if (!CreatePipe(&_hread, &_hwrite, &_ai, 0))
        return -1;
    if (!CreatePipe(&_hread2, &_hwrite2, &_ai, 0))
        return -2;
    GetStartupInfoA(&_si);
    _si.cb = sizeof(STARTUPINFO);
    _si.hStdError = _hwrite;
    _si.hStdOutput = _hwrite;
    _si.hStdInput = _hread2;
    _si.wShowWindow = SW_HIDE;
    _si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    std::vector<char> cmdline(cmd.begin(), cmd.end());
    cmdline.push_back('\0');
    if (!CreateProcessA(NULL, cmdline.data(), nullptr, nullptr, true, CREATE_NO_WINDOW, nullptr, nullptr, &_si, &_pi))
        return -3;

    SAFE_CLOSE(_hwrite);
    SAFE_CLOSE(_hread2);

    _reading = true;
    _pthread = new std::thread(&Pipe::reader, this);
    return 1;
}

int Pipe::close() {
    const bool wasReading = _reading.load();
    _reading = false;
    if (wasReading) {
        SAFE_CLOSE(_hwrite2);

        if (_pi.hProcess && ::WaitForSingleObject(_pi.hProcess, 1000) == WAIT_TIMEOUT) {
            ::TerminateProcess(_pi.hProcess, 0);
        }
    }

    if (_pthread) {
        if (_pthread->joinable()) {
            ::CancelSynchronousIo(_pthread->native_handle());
            SAFE_CLOSE(_hread);
            _pthread->join();
        }
    }
    SAFE_DELETE(_pthread);
    SAFE_CLOSE(_hread);
    SAFE_CLOSE(_hwrite);
    SAFE_CLOSE(_hwrite2);
    SAFE_CLOSE(_hread2);
    SAFE_CLOSE(_pi.hProcess);
    SAFE_CLOSE(_pi.hThread);
    return 0;
}

void Pipe::on_read(const string &info) {
    std::cout << info << std::endl;
}

int Pipe::on_write(const string &info) {
    if (_reading.load() && _hwrite2) {
        unsigned long wlen = 0;

        return WriteFile(_hwrite2, info.data(), info.length() * sizeof(char), &wlen, nullptr);
    }
    return 0;
}

void Pipe::reader() {
    const static int buf_size = 1 << 10;
    char buf[buf_size];
    unsigned long read_len = 0;
    while (_reading.load()) {
        memset(buf, 0, buf_size * sizeof(char));
        if (ReadFile(_hread, buf, buf_size - 1, &read_len, NULL) && read_len > 0) {
            // setlog("readed:%s", buf);
            on_read(string(buf, read_len));
        } else {
            _reading = false;
            break;
        }
    }
}

bool Pipe::is_open() {
    if (_reading.load() && _pi.hProcess) {
        DWORD code = 0;
        ::GetExitCodeProcess(_pi.hProcess, &code);
        return code == STILL_ACTIVE;
    }
    return false;
}
