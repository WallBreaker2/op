#pragma once
#include "Pipe.h"
#include <chrono>
class Cmder : public Pipe {
  public:
    void on_read(const string &ss) override {
        _readed += ss;
    }
    string GetCmdStr(const string &cmd, size_t milseconds) {
        _readed.clear();
        open(cmd);
        if (is_open()) {
            const ULONGLONG deadline = ::GetTickCount64() + static_cast<ULONGLONG>(milseconds);
            while (is_open() && ::GetTickCount64() < deadline) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            close();
        }
        return _readed;
    }

  private:
    string _readed;
};
