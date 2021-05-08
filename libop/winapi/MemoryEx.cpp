//#include "stdafx.h"
#include "MemoryEx.h"
#include "./core/helpfunc.h"
#define push(s,x)s.push_back(x)
#define pop(s) s.back();s.pop_back()

int get_op_prior(wchar_t op) {
	if (op == L'+' || op == L'-')
		return 0;
	if (op == L'*' || op == L'/')
		return 1;
	return 2;
}

bool is_op(wchar_t op) {
	return op == L'+' || op == L'-' || op == L'*' || op == L'/';
}

int do_op(int a, int b, wchar_t op) {
	int ans;
	if (op == L'+')
		ans = a + b;
	else if (op == L'-')
		ans = a - b;
	else if (op == L'*')
		ans = a * b;
	else if (op == L'/')
		ans = a / b;
	else
		ans = 0;
	return ans;
}
//like AA+BB+DD-CC*cc
int stringcompute(const wchar_t* s) {
	int ans = 0;
	if (!s || !*s)
		return 0;
	wstring num;
	int op;
	vector<int> ns;
	vector<int> os;
	while (*s || !os.empty()) {
		if (is_op(*s) || *s == 0) {
			int x;
			swscanf(num.data(), L"%X", &x);

			if (!num.empty()) {
				push(ns, x); num.clear();
			}

			if (ns.empty()) { /// 数字栈空,直接压入数据
				push(os, *s);
				s++;
			}
			else if (!os.empty()) {
				op = os.back();
				if (*s == 0 || get_op_prior(op) >= get_op_prior(*s)) { // 进行运算
					int num2 = pop(ns);
					int num1 = pop(ns);
					op = pop(os);
					ans = do_op(num1, num2, op);
					push(ns, ans);
					//push(os, c);
				}
				else {   // 将运算压入栈中
					push(os, *s);
					s++;
				}
			}
			else {
				push(os, *s);
				s++;
			}


		}
		else {
			num.push_back(*s);
			s++;
		}

	}

	return ns.back();
}

MemoryEx::MemoryEx()
{
}


MemoryEx::~MemoryEx()
{
}

long MemoryEx::WriteData(HWND hwnd, const wstring& address, const wstring& data, LONG size) {
	_hwnd = hwnd;
	if (!checkaddress(address))
		return 0;
	vector<uchar> bin;
	hex2bins(bin, data,size);
	if (_hwnd) {
		if (!::IsWindow(hwnd))
			return 0;
		DWORD pid;
		::GetWindowThreadProcessId(hwnd, &pid);
		auto hr = _proc.Attach(pid);
		
		if (hr >= 0) {
			size_t addr = str2address(address);
			if (addr == 0)return 0;
			return mem_write(addr, bin.data(), size);
			
		}
		return 0;
	}
	else {
		size_t addr = str2address(address);
		if (addr == 0)return 0;
		return mem_write(addr,bin.data(), size);
	}
	

}

wstring MemoryEx::ReadData(HWND hwnd, const wstring& address, LONG size) {
	_hwnd = hwnd;
	if (!checkaddress(address))
		return L"";
	vector<uchar> bin;
	bin.resize(size);
	wstring hex;
	if (_hwnd) {
		if (!::IsWindow(hwnd))
			return L"";
		DWORD pid;
		::GetWindowThreadProcessId(hwnd, &pid);
		auto hr = _proc.Attach(pid);

		if (hr >= 0) {
			size_t addr = str2address(address);
			if (addr == 0)return L"";
			 mem_read(bin.data(), addr, size);
		}
	}
	else {
		size_t addr = str2address(address);
		if (addr == 0)return L"";
		mem_read(bin.data(), addr, size);
	}

	bin2hexs(bin, hex);
	return hex;

}

bool MemoryEx::mem_read(void* dst, size_t src, size_t size) {
	if (_hwnd) {
		return _proc.memory().Read(src, size, dst) >= 0;
	}
	else {
		::memcpy(dst, (void*)src, size);
		return true;
	}

}

bool MemoryEx::mem_write(size_t dst, void* src, size_t size) {
	if (_hwnd) {
		return _proc.memory().Write(dst, size, src) >= 0;
	}
	else {
		::memcpy((void*)dst, src, size);
		return true;
	}
}

bool MemoryEx::checkaddress(const wstring& address) {
	vector<wchar_t> sk;
	auto p = address.data();
	while (*p) {
		if (*p == L'[' || *p == L']') {
			if (sk.empty())
				sk.push_back(*p);
			else if (sk.back() == *p)
				sk.push_back(*p);
			else
				sk.pop_back();
		}
		p++;
	}
	return sk.empty();
}

size_t MemoryEx::str2address(const wstring& caddress) {
	wstring address = caddress;
	if (!checkaddress(address))
		return 0;

	vector<int> sk;
	int idx1 = 0, idx2 = 0;
	size_t re = 0;
	idx1 = address.find(L'<');
	idx2 = address.find(L'>');
	if (idx1 != -1 && idx2 != -1 && idx1 < idx2) {
		auto mod_name = address.substr(idx1 + 1, idx2 - idx1 - 1);
		HMODULE hmod = NULL;
		if (_hwnd == 0)
			hmod = ::GetModuleHandleW(mod_name.data());
		else {
			auto mptr = _proc.modules().GetModule(mod_name);
			if (mptr)hmod = (HMODULE)mptr->baseAddress;
		}
		if (hmod == NULL)return 0;
		wchar_t buff[128];
		wsprintf(buff, L"%X", hmod);
		address.replace(idx1, idx2 - idx1 + 1, buff);
	}
	for (int i = 0; i < address.size();) {
		if (address[i] == L'[')push(sk, i);
		if (address[i] == L']') {
			idx1 = pop(sk);
			idx2 = i;
			auto sad = address.substr(idx1 + 1, idx2 - idx1 - 1);
			size_t src = stringcompute(sad.data());
			size_t next;
			if (!mem_read(&next, src, sizeof(size_t)) || next == 0)
				return 0;
			wchar_t buff[128];
			wsprintf(buff, L"%X", next);
			address.replace(idx1, idx2 - idx1 + 1, buff);
			i = idx1;
		}
		++i;
	}
	return stringcompute(address.data());
}

void MemoryEx::hex2bins(vector<uchar>&bin, const wstring& hex,size_t size) {
	bin.resize(size);
	ZeroMemory(bin.data(), bin.size());
	int low = 1;
	for (int i = hex.size() - 1; i >= 0; --i) {
		
		bin[size - i / 2-1] |= low & 1 ? hex2bin(hex[i]) : hex2bin(hex[i]) << 4;
		low ^= 1;
	}
}

void MemoryEx::bin2hexs(const vector<uchar>&bin, wstring& hex) {
	//hex.resize(bin.size() * 2);
	hex.reserve(bin.size() * 2);
	hex.clear();
	for (int i = 0; i < bin.size(); ++i) {
		int ans = bin2hex(bin[i]);
		hex.push_back(ans >> 8);
		hex.push_back(ans & 0xff);
	}
}
