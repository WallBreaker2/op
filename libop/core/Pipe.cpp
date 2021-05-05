//#include "stdafx.h"
#include "Pipe.h"
#include <iostream>
#include<chrono>
#include  "globalVar.h"
#include "helpfunc.h"
Pipe::Pipe()
{
	_hread = _hwrite = _hread2 = _hwrite2 = nullptr;
	_hprocess = nullptr;
	_reading = 0;
	_pthread = nullptr;
}




Pipe::~Pipe()
{
	close();
}

int Pipe::open(const string& cmd) {
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
	char buf[0xff];
	memcpy(buf, cmd.data(), sizeof(char) * (1 + cmd.length()));
	if (!CreateProcessA(NULL, buf, nullptr, nullptr, true, NULL, nullptr, nullptr, &_si, &_pi))
		return -3;
	_reading = 1;
	_pthread = new std::thread(&Pipe::reader, this);
	return 1;
}

int Pipe::close() {
	if (_reading) {
		_reading = 0;
		on_write("exit");

		if (::WaitForSingleObject(_pi.hProcess, 1000) == WAIT_TIMEOUT) {
			::TerminateProcess(_pi.hProcess, 0);
			TerminateThread(_pthread->native_handle(), -1);
		}
		else {

		}

		_pthread->join();


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

void Pipe::on_read(const string& info) {
	std::cout << info << std::endl;
}

int Pipe::on_write(const string& info) {
	if (_reading) {
		unsigned long wlen = 0;

		return WriteFile(_hwrite2, info.data(), info.length() * sizeof(char), &wlen, nullptr);
	}
	return 0;
}

void Pipe::reader() {
	const static int buf_size = 1 << 10;
	char buf[buf_size];
	unsigned long read_len = 0;
	while (_reading) {
		memset(buf, 0, buf_size * sizeof(char));
		if (ReadFile(_hread, buf, buf_size - 1, &read_len, NULL)) {
			//setlog("readed:%s", buf);
			on_read(buf);
		}
		else {
			_reading = 0;
			break;
		}
	}
}

bool Pipe::is_open() {
	if (_reading) {
		DWORD code = 0;
		::GetExitCodeProcess(_pi.hProcess, &code);
		return code == STILL_ACTIVE;
	}
	return false;
}

