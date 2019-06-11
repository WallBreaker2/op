#pragma once
#ifndef __DICT_H_
#define __DICT_H_
#include <vector>
#include <string>
#include <math.h>
#include "../op/include/bitfunc.h"
//#define SET_BIT(x, idx) x |= 1u << (idx)

//#define GET_BIT(x, idx) (((x )>> (idx)) & 1u)


struct rect_t {
	int x1, y1;
	int x2, y2;
	int width() const { return x2 - x1; }
	int height() const { return y2 - y1; }
};
struct word_t {

	struct word_info_t {
		//char of word
		wchar_t _char[4];
		//char height
		__int16 width, height;
		//char bit ct
		__int32 bit_count;
		word_info_t() :width(0), height(0), bit_count(0) { memset(_char, 0, sizeof(_char)); }
		bool operator==(const word_info_t& rhs) {
			return width == rhs.width&&height == rhs.height;
		}
		bool operator!=(const word_info_t& rhs) {
			return width != rhs.width&&height == rhs.height;
		}

	};
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
		memcpy(info._char, s.c_str(), std::min(sizeof(info._char), (s.length() + 1) * sizeof(wchar_t)));
	}
	/*从 dm 字库中 的一个点阵转化为op的点阵*/
	void fromDm(const wchar_t* str, int ct,const std::wstring& w) {
		int bin[50] = { 0 };
		ct = std::min<int>(ct, 88);
		int i = 0;
		auto hex2bin = [](wchar_t c) {
			return c <= L'9' ? c - L'0' : c - L'A'+10;
		};
		while (i < ct) {
			
			bin[i / 2] = (hex2bin(str[i]) << 4) | (hex2bin(str[i+1]));
			i += 2;
		}
		int cols = (ct * 4) / 11;
		memset(this, 0, sizeof(*this));
		for (int j = 0; j < cols; ++j) {
			for (int i = 0; i < 11; ++i) {
				int idx = j * 11 + i;
				if (GET_BIT(bin[idx>>3],7-(idx&7))) {
					SET_BIT(clines[j], 31-i);
					++info.bit_count;
				}
					
			}
		}
		info.height = 11;
		info.width = cols;
		set_chars(w);
	}
};

struct Dict {
	struct dict_info_t {
		__int16 _this_ver;
		__int16 _word_count;
		//check code=_this_ver^_word_count
		__int32 _check_code;
		dict_info_t() :_this_ver(0), _word_count(0) { _check_code = _word_count ^ _this_ver; }
	};
	dict_info_t info;
	Dict() {}
	std::vector<word_t>words;
	void read_dict(const std::string&s) {
		clear();
		std::fstream file;
		file.open(s, std::ios::in | std::ios::binary);
		if (!file.is_open())
			return;
		//读取信息
		file.read((char*)&info, sizeof(info));
		//校验
		if (info._check_code == (info._this_ver^info._word_count)) {
			words.resize(info._word_count);
			file.read((char*)&words[0], sizeof(word_t)*info._word_count);
		}
		file.close();
	}
	void read_dict_dm(const std::string&s) {
		clear();
		std::wfstream file;
		file.open(s, std::ios::in);
		if (!file.is_open())
			return;
		//读取信息
		std::wstring ss;
		while (std::getline(file, ss)) {
			size_t idx1 = ss.find(L'$');
			auto idx2=ss.find(L'$',idx1+1);
			word_t wd;
			if (idx1 != -1&&idx2!=-1) {
				wd.fromDm(ss.data(), idx1 + 1, ss.substr(idx1 + 1, idx2 - idx1-1));
				add_word(wd);
			}
		}
		file.close();
	}

	void write_dict(const std::string&s) {
		std::fstream file;
		file.open(s, std::ios::out | std::ios::binary);
		if (!file.is_open())
			return;
		//删除所有空的字符
		auto it = words.begin();
		while (it != words.end()) {
			if (it->info._char[0] == L'\0')
				it = words.erase(it);
			else
				++it;
		}
		//设置校验
		info._check_code = info._this_ver^info._word_count;
		//写入信息
		file.write((char*)&info, sizeof(info));
		//写入数据
		file.write((char*)&words[0], sizeof(word_t)*info._word_count);
		file.close();
	}
	void add_word(const cv::Mat& binary, const rect_t& rc) {
		int x2 = std::min(rc.x1 + 32, rc.x2);
		int y2 = std::min(rc.y1 + 32, rc.y2);
		word_t word;
		for (int j = rc.x1; j < x2; ++j) {
			unsigned __int32 x = 0, val;
			for (int i = rc.y1, id = 0; i < y2; ++i, ++id) {
				val = binary.at<uchar>(i, j);
				if (val == 0) {
					SET_BIT(x, 31 - id);
					++word.info.bit_count;
				}
					
			}
			word.clines[word.info.width++] = x;
			//t = QString::asprintf("%08X", x);
			//tp += t;

		}
		word.info.height = y2 - rc.y1;
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
	void add_word(const word_t&word) {
		auto it = find(word);
		if (words.empty()||it==words.end()) {
			words.push_back(word);
		}
		else {
			it->set_chars(word.info._char);
		}
		info._word_count = words.size();
	}
	void clear() {
		info._word_count = 0;
		words.clear();
	}
	std::vector<word_t>::iterator find(const word_t&word) {
		for (auto it = words.begin(); it != words.end(); ++it)
			if (*it == word)return it;
		return words.end();
	}

	void erase(const word_t&word) {
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
