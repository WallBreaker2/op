#pragma once
#include <atomic>
#include <string>
#include <thread>
#include <windows.h>
class Pipe {
  public:
    using handle_t = HANDLE;
    using string = std::string;
    Pipe();
    virtual ~Pipe();
    int open(const string &cmd);
    int close(DWORD process_wait_ms = 1000);
    virtual void on_read(const string &info);
    virtual int on_write(const string &info);
    bool is_open();
    bool wait_for_exit(DWORD timeout_ms);

  private:
    handle_t _hread, _hwrite;
    handle_t _hread2, _hwrite2;
    handle_t _hprocess;
    SECURITY_ATTRIBUTES _ai;
    PROCESS_INFORMATION _pi;
    STARTUPINFOA _si;
    std::atomic<bool> _reading;
    std::thread *_pthread;
    void reader();
};


