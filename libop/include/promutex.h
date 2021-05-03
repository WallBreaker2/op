#pragma once
#include <windows.h>
#include <string>
#include <assert.h>
using std::wstring;
class promutex
{
public:
	explicit promutex():_hmutex(NULL) {
	}
	~promutex() {
		if (_hmutex) {
			unlock();
			::CloseHandle(_hmutex);
			_hmutex = NULL;
		}
	}
	bool open_create(const wstring& name_) {
		HANDLE temp;
		temp = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, name_.data());
		if (!temp) {
			temp = CreateMutexW(NULL, FALSE, name_.data());
		}
		if (temp) {
			_hmutex = temp;
			return true;
		}
		else {
			return false;
		}
		
	}
	bool open(const wstring& name_) {
		HANDLE temp;
		temp = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, name_.data());
		if (temp) {
			_hmutex = temp;
			return true;
		}
		return false;

	}
	void lock() {
		::WaitForSingleObject(_hmutex, INFINITE);
	}

	DWORD try_lock(size_t time_) {
		return ::WaitForSingleObject(_hmutex, time_);
	}
	void unlock() {
		assert(_hmutex);
		::ReleaseMutex(_hmutex);
	}
protected:
private:
	HANDLE _hmutex;
};
