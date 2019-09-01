#include "stdafx.h"
#include "MemoryEx.h"
#include "helpfunc.h"
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
int stringcompute(const wstring& str) {
	int ans = 0;

	wstring numstr;
	vector<int> num_stack;
	vector<int> op_stack;
	wchar_t c;
	auto itr = str.begin();
	wchar_t op;
	while (*itr) {
		c = *itr;
		if (is_op(c)) {
			int num;
			swscanf(numstr.data(), L"%X", &num);
			numstr.clear();
			push(num_stack, num);
			if (num_stack.empty())/*数字栈空,直接压入数据*/ {
				push(op_stack, c);
				++itr;
			}
			else if (!op_stack.empty()) {
				op = op_stack.back();
				if (get_op_prior(op) >= get_op_prior(c))/*进行运算*/ {
					int num2 = pop(num_stack);
					int num1 = pop(num_stack);
					op = pop(op_stack);
					int ans = do_op(num1, num2, op);
					push(num_stack, ans);
					push(op_stack, c);
				}
				else {/*将运算压入栈中*/
					push(op_stack, c);
					++itr;
				}
			}
			else {
				push(op_stack, c);
				++itr;
			}


		}
		else {
			numstr.push_back(c);
			++itr;
		}

	}
	int num;
	swscanf(numstr.data(), L"%X", &num);
	numstr.clear();
	push(num_stack, num);
	if (num_stack.size() == 3) {
		ans = do_op(num_stack[0], do_op(num_stack[1], num_stack[2], op_stack[1]), op_stack[0]);
	}
	else if (num_stack.size() == 2)
		ans = do_op(num_stack[0], num_stack[1], op_stack[0]);
	else if (num_stack.size() == 1)
		ans = num_stack[0];
	else
		ans = 0;
	return ans;
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
			return memcpy((void*)addr, (size_t)bin.data(), size);
			
		}
		return 0;
	}
	else {
		size_t addr = str2address(address);
		if (addr == 0)return 0;
		return memcpy((void*)addr, (size_t)bin.data(), size);
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
			 memcpy(bin.data(), addr, size);
		}
	}
	else {
		size_t addr = str2address(address);
		if (addr == 0)return L"";
		memcpy(bin.data(), addr, size);
	}

	bin2hexs(bin, hex);
	return hex;

}

bool MemoryEx::memcpy(void* dst, size_t src, size_t size) {
	if (_hwnd) {
		return _proc.memory().Read(src, size, dst) >= 0;
	}
	else {
		::memcpy(dst, (void*)src, size);
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
			size_t src = stringcompute(sad);
			size_t next;
			if (!memcpy(&next, src, sizeof(size_t)) || next == 0)
				return 0;
			wchar_t buff[128];
			wsprintf(buff, L"%X", next);
			address.replace(idx1, idx2 - idx1 + 1, buff);
			i = idx1;
		}
		++i;
	}
	return stringcompute(address);
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
	hex.resize(bin.size() * 2);
	for (int i = 0; i < bin.size(); ++i) {
		int ans = bin2hex(bin[i]);
		hex[i * 2] = ans >> 4;
		hex[i * 2 + 1] = ans & 0xf;
	}
}

