#pragma once
#include "Pipe.h"
class Cmder : public Pipe {
public:
	void on_read(const string& ss) {
		_readed += ss;

	}
	string ExcuteCmd(const string&cmd,size_t milseconds) {
		open(cmd);
		if (is_open()) {
			auto deadline = clock()+milseconds;
			while (is_open()&& clock()<deadline) {
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
			close();
		}
		return _readed;
	}
private:
	string _readed;

};