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
        if (open(cmd) <= 0)
            return _readed;

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milseconds);
        while (is_open() && std::chrono::steady_clock::now() < deadline) {
            ::Sleep(1);
        }
        close(0);
        return _readed;
    }

  private:
    string _readed;
};

