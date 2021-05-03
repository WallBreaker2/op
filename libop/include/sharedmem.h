#pragma once
#include <windows.h>
#include <string>
using std::wstring;
class sharedmem
{
public:
	explicit sharedmem(const wstring& name_,size_t size_):_hmap(nullptr),_paddress(nullptr),_ismaped(0){
		open_create(name_, size_);

	}
	sharedmem() :_hmap(nullptr),_paddress(nullptr), _ismaped(0) {

	}
	~sharedmem() {
		close();
	}
	/*open or create a shared memory*/
	bool open_create(const wstring& name_, size_t size_) {
		auto temph = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name_.data());
		if (!temph) {
			temph = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size_, name_.data());
			if (!temph)
				return false;
			
		}
		_hmap = temph;
		if (!_ismaped)
			_paddress = MapViewOfFile(_hmap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		_ismaped = 1;
		return true;
		
	}
	/*open only*/
	bool open(const wstring& name_) {
		auto temph = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name_.data());
		if (temph)
			_hmap = temph;
		else
			return false;
		if (!_ismaped)
			_paddress = MapViewOfFile(_hmap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		_ismaped = 1;
		return true;

	}
	/*close the shared memory*/
	void close() {
		if (_ismaped)
			UnmapViewOfFile(_paddress);
		if (_hmap)
			CloseHandle(_hmap);
		_hmap = NULL;
		_ismaped = 0;
		_paddress = nullptr;
	}
	template<typename T>
	T& at(int idx_) {
		//assert(_hmap&&_paddress);
		return (T)_paddress[idx_];
	}
	template<typename T>
	T* data() {
		return (T*)_paddress;
	}
protected:
	/*sharedmem operator=(const sharedmem& rhs) {
		return rhs;
	}*/
private:
	//handle of shared file map
	HANDLE _hmap;
	//address of shared memory
	void* _paddress;
	//state of is maped
	int _ismaped;
};