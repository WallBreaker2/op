#pragma once
#include "Pipe.h"
class Cmder : public Pipe {
  public:
    void on_read(const string &ss) override {
        _readed += ss;
    }
    string GetCmdStr(const std::wstring &cmd, DWORD milliseconds) {
        _readed.clear();
        if (open(cmd) <= 0)
            return _readed;

        close(milliseconds);
        return _readed;
    }

  private:
    string _readed;
};

