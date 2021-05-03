#pragma once
#include <windows.h>
#include <string>
#include <thread>
class Pipe
{
public:
	using handle_t = HANDLE;
	using string=std::string;
	Pipe();
	virtual ~Pipe();
	int open(const string& cmd);
	int close();
	virtual void on_read(const string& info);
	virtual int on_write(const string& info);
	bool is_open();
private:
	handle_t _hread, _hwrite;
	handle_t _hread2, _hwrite2;
	handle_t _hprocess;
	SECURITY_ATTRIBUTES _ai;
	PROCESS_INFORMATION _pi;
	STARTUPINFOA _si;
	int _reading;
	std::thread* _pthread;
	void reader();
};

