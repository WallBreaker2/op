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

        wait_for_exit(static_cast<DWORD>(milseconds));
        close(0);
        return _readed;
    }

  private:
    string _readed;
};

