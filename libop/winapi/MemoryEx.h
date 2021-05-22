#pragma once
#ifndef __MEMORYEX_H_
#define __MEMORYEX_H_
#include "./core/helpfunc.h"
#include "BlackBone/Process/Process.h"
class MemoryEx
{
public:
	MemoryEx();
	~MemoryEx();
	long WriteData(HWND hwnd, const wstring& address, const wstring& data, LONG size);
	wstring ReadData(HWND hwnd, const wstring& address, LONG size);
	bool checkaddress(const wstring& address);
	size_t str2address(const wstring& caddress);
	bool mem_read(void* dst, size_t src, size_t size);
	bool mem_write(size_t dst, void* src, size_t size);
	void hex2bins(vector<uchar>&bin, const wstring& hex,size_t size);
	void bin2hexs(const vector<uchar>&bin, wstring& hex);
private:
	blackbone::Process _proc;
	HWND _hwnd;
};


#endif
