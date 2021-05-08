#pragma once
#ifndef __DICT_H_
#define __DICT_H_
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include "bitfunc.h"
#include "Image.hpp"
#include "../core/helpfunc.h"
//#define SET_BIT(x, idx) x |= 1u << (idx)

//#define GET_BIT(x, idx) (((x )>> (idx)) & 1u)
const int op_dict_version = 2;


/*
第 0 代字库
*/
struct word_info_t {
	//char of word
	wchar_t _char[4];
	//char height
	__int16 width, height;
	//char bit ct
	__int32 bit_count;
	word_info_t() :width(0), height(0), bit_count(0) { memset(_char, 0, sizeof(_char)); }
	bool operator==(const word_info_t& rhs) {
		return width == rhs.width && height == rhs.height;
	}
	bool operator!=(const word_info_t& rhs) {
		return width != rhs.width && height == rhs.height;
	}

};
struct word_t {

	
	//32 bit a col
	using cline_t = unsigned __int32;
	word_info_t info;
	//char col line
	cline_t clines[32];
	bool operator==(const word_t& rhs) {
		if (info != rhs.info)
			return false;
		for (int i = 0; i < info.width; ++i)
			if (clines[i] != rhs.clines[i])
				return false;
		return true;
	}
	void set_chars(const std::wstring&s) {
		memcpy(info._char, s.c_str(), min(sizeof(info._char), (s.length() + 1) * sizeof(wchar_t)));
	}
	//从 dm 字库中 的一个点阵转化为op的点阵
	void fromDm(const wchar_t* str, int ct,const std::wstring& w) {
		int bin[50] = { 0 };
		constexpr int DM_DICT_HEIGTH = 11;
		ct = min(ct, 88);
		int i = 0;
		auto hex2bin = [](wchar_t c) {
			return c <= L'9' ? c - L'0' : c - L'A'+10;
		};
		while (i < ct) {
			
			bin[i / 2] = (hex2bin(str[i]) << 4) | (hex2bin(str[i+1]));
			i += 2;
		}
		//
		int cols = (ct * 4) / DM_DICT_HEIGTH;
		memset(this, 0x0, sizeof(*this));
		for (int j = 0; j < cols; ++j) {
			for (int i = 0; i < 11; ++i) {
				int idx = j * 11 + i;
				if (GET_BIT(bin[idx>>3],7-(idx&7))) {
					SET_BIT(clines[j], 31-i);
					++info.bit_count;
				}
					
			}
		}
		info.height = DM_DICT_HEIGTH;
		info.width = cols;
		set_chars(w);
	}
};
/*
第 1 代字库
*/
struct word1_info {
	uint8_t w, h;//max is 255 2B
	uint16_t bit_cnt;//max is 255*255=65025<65536 4B
	wchar_t name[8];//name 12B
	word1_info() :w(0), h(0), bit_cnt(0) {}
};
struct word1_t {
	word1_info info;
	vector<uint8_t> data;//size is (w*h+7)/8
	bool operator==(const word1_t& rhs){
		if (info.w!=rhs.info.w|| info.h != rhs.info.h||info.bit_cnt!=rhs.info.bit_cnt)
			return false;
		for (size_t i=0;i<data.size();i++)
			if (data[i] != rhs.data[i])
				return false;
		return true;
	}
	void set_chars(const std::wstring& s) {
		int nlen = s.length() < 8 ? s.length() : 7;
		memcpy(info.name, s.c_str(),nlen*2);
		info.name[nlen] = L'\0';
	}
	void from_word(word_t& wd) {
		info.w = (uint8_t)wd.info.width;
		info.h = wd.info.height;
		init();
		info.bit_cnt = wd.info.bit_count;
		memcpy(info.name, wd.info._char, 4 * sizeof(wchar_t));
		info.name[3] = 0;
		int idx = 0;
		
		for (int x = 0; x < info.w; x++) {
			for (int y = 0; y < info.h; y++) {
				if (GET_BIT(wd.clines[x], 31-y))
					SET_BIT(data[idx / 8], idx & 7);
				idx++;
			}
		}
	}



	void init() {
		data.resize((info.w * info.h + 7) / 8);
		std::fill(data.begin(), data.end(), 0);
	}
};




struct Dict {
	//v0 v1
	struct dict_info_t {
		__int16 _this_ver;//0 1
		__int16 _word_count;
		//check code=_this_ver^_word_count
		__int32 _check_code;
		dict_info_t() :_this_ver(1), _word_count(0) { _check_code = _word_count ^ _this_ver; }
	};
	dict_info_t info;
	Dict() {}
	std::vector<word1_t>words;
	void read_dict(const std::string&s) {
		if (s.empty())
			return;
		if (s.find(".txt") != -1)
			return read_dict_dm(s);
		clear();
		std::fstream file;
		file.open(s, std::ios::in | std::ios::binary);
		if (!file.is_open())
			return;
		//读取头信息
		file.read((char*)&info, sizeof(info));
		
		//校验
		if (info._this_ver==0&&info._check_code == (info._this_ver^info._word_count)) {
			//old dict format
			words.resize(info._word_count);
			info._this_ver = 1;
			word_t tmp;
			for (size_t i = 0; i < words.size(); i++) {
				file.read((char*)&tmp, sizeof(tmp));
				words[i].from_word(tmp);
			}
			//file.read((char*)&words[0], sizeof(word_t)*info._word_count);
		}
		else if (info._this_ver == 1 && info._check_code == (info._this_ver ^ info._word_count)) {
			//new dict format
			words.resize(info._word_count);
			word1_info head;
			for (size_t i = 0; i < words.size(); i++) {
				file.read((char*)&head, sizeof(head));
			
				words[i].info = head;
				int nlen = (head.w * head.h + 7) / 8;
				words[i].data.resize(nlen);
				file.read((char*)words[i].data.data(), nlen);
			}
		}
		file.close();
		sort_dict();
	}
	void read_dict_dm(const std::string&s) {
		clear();
		std::fstream file;
		file.open(s, std::ios::in);
		if (!file.is_open())
			return;
		//读取信息
		std::wstring ss;
		std::string str;
		while (std::getline(file, str)) {
			std::string strLocale = setlocale(LC_ALL, "");
			const char* chSrc = str.c_str();
			size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
			wchar_t* wchDest = new wchar_t[nDestSize];
			wmemset(wchDest, 0, nDestSize);
			mbstowcs(wchDest, chSrc, nDestSize);
			std::wstring wstrResult = wchDest;
			delete[]wchDest;
			setlocale(LC_ALL, strLocale.c_str());
			ss = wstrResult;
			size_t idx1 = ss.find(L'$');
			auto idx2=ss.find(L'$',idx1+1);
			word_t wd;
			word1_t wd1;
			wstring name;
			if (idx1 != -1&&idx2!=-1) {
				ss[idx1] = L'0';
				name = ss.substr(idx1 + 1, idx2 - idx1 - 1);
				wd.fromDm(ss.data(), idx1, name);
				wd1.from_word(wd);
				wd1.set_chars(name);
				add_word(wd1);
				
			}
		}
		file.close();
		sort_dict();
	}

	void read_memory_dict_dm(const char* buf,size_t size) {
		clear();
		std::stringstream file;
		file.write(buf, size);
 
		//读取信息
		std::wstring ss;
		std::string str;
		while (std::getline(file, str)) {
			std::string strLocale = setlocale(LC_ALL, "");
			const char* chSrc = str.c_str();
			size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
			wchar_t* wchDest = new wchar_t[nDestSize];
			wmemset(wchDest, 0, nDestSize);
			mbstowcs(wchDest, chSrc, nDestSize);
			std::wstring wstrResult = wchDest;
			delete[]wchDest;
			setlocale(LC_ALL, strLocale.c_str());
			ss = wstrResult;
			size_t idx1 = ss.find(L'$');
			auto idx2 = ss.find(L'$', idx1 + 1);
			word_t wd;
			word1_t wd1;
			wstring name;
			if (idx1 != -1 && idx2 != -1) {
				ss[idx1] = L'0';
				name = ss.substr(idx1 + 1, idx2 - idx1 - 1);
				wd.fromDm(ss.data(), idx1, name);
				wd1.from_word(wd);
				wd1.set_chars(name);
				add_word(wd1);

			}
		}
		sort_dict();
	}


	void write_dict(const std::string&s) {
		std::fstream file;
		file.open(s, std::ios::out | std::ios::binary);
		if (!file.is_open())
			return;
		//删除所有空的字符
		auto it = words.begin();
		while (it != words.end()) {
			if (it->info.name[0] == L'\0')
				it = words.erase(it);
			else
				++it;
		}
		info._word_count = words.size();
		//设置校验

		info._check_code = info._this_ver^info._word_count;
		//写入信息
		file.write((char*)&info, sizeof(info));
		//写入数据
		for (int i = 0; i < words.size(); i++) {
			file.write((char*)&words[i].info, sizeof(word1_info));
			file.write((char*)words[i].data.data(), words[i].data.size());
		}
		file.close();
	}
	void add_word(const ImageBin& binary, const rect_t& rc) {
		int x2 = min(rc.x1 + 255, rc.x2);
		int y2 = min(rc.y1 + 255, rc.y2);
		word1_t word;
		word.info.w = x2 - rc.x1;
		word.info.h = y2 - rc.y1;
		word.info.bit_cnt = 0;
		word.init();
		//word.data.resize((word.info.w * word.info.h + 7) / 8);
		int idx = 0;
		for (int j = rc.x1; j < x2; ++j) {
			for (int i = rc.y1; i < y2; ++i) {
				auto val = binary.at(i, j);
				if (val == 1) {
					SET_BIT(word.data[idx / 8],idx & 7);
					
					++word.info.bit_cnt;
				}
				++idx;
					
			}
		}
		auto it = find(word);
		if (words.empty()||it==words.end()) {
			word.set_chars(L"");
			
			words.push_back(word);
			info._word_count = words.size();
		}
		else {//only change char
			//word.set_chars(c);
		}

	}

	void sort_dict() {
		//sort dict(size: big --> small ,cnt: small -->big)
		std::stable_sort(words.begin(), words.end(),
			[](const word1_t& lhs, const word1_t& rhs) {
				int dh = lhs.info.h - rhs.info.h;
			int dw = lhs.info.w - rhs.info.w;
			return dh > 0 || (dh == 0 && dw > 0) ||
				(dh == 0 && dw == 0 && lhs.info.bit_cnt < rhs.info.bit_cnt);
		});
	}

	void add_word(const word1_t&word) {
		auto it = find(word);
		if (words.empty()||it==words.end()) {
			words.push_back(word);
		}
		else {
			it->set_chars(word.info.name);
		}
		info._word_count = words.size();
	}
	void clear() {
		info._word_count = 0;
		words.clear();
	}
	std::vector<word1_t>::iterator find(const word1_t&word) {
		for (auto it = words.begin(); it != words.end(); ++it)
			if (*it == word)return it;
		return words.end();
	}

	void erase(const word1_t&word) {
		auto it = find(word);
		if (!words.empty() && it !=words.end())
			words.erase(it);
		info._word_count = words.size();
	}

	int size() const{
		return info._word_count;
	}

	bool empty()const {
		return size() == 0;
	}
};

#endif
