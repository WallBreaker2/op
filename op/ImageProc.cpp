#include "stdafx.h"
#include "ImageProc.h"
#include "Common.h"
#include <fstream>
#include <bitset>
#include <algorithm>
ImageProc::ImageProc()
{
	_curr_dict = _dicts;
}


ImageProc::~ImageProc()
{
}

long ImageProc::FindPic(long x1, long y1, long x2, long y2, const std::wstring& files, double sim, long& x, long &y) {
	return 0;
}

long ImageProc::AddDict(int idx, const wstring& file_name) {
	if (idx < 0 || idx >= _max_dicts)
		return 0;
	_dicts[idx].clear();
	std::wfstream file;
	file.open(file_name, std::ios::in);
	std::wstring str;
	std::vector<std::wstring> vstr;
	auto& _this_dict = _dicts[idx];
	one_words_t words;
	if (file.is_open()) {
		//getline
		while (!file.eof()) {
			//line
			std::getline(file,str);
			split(str, vstr, L"$");
			
			if (vstr.size() == 4) {
				//words
				words.binlines.clear();
				ocrline_t line=0;
				int offset = 0;
				int val = 0;
				
				for (auto c : vstr[0]) {
					val = HEX2INT(c);
					std::bitset<16> bt1(val), bt2(line);
					for (int j = 3; j >= 0; --j) {
						//set bit
						if (bt1.test(j))
							bt2.set(15-offset,1);
						//if (_this_dict.empty())
						//	setlog("%X", line);
						if (offset >=10){//a line full,next line
							offset = 0;
	
								
							words.binlines.push_back(line);
							line = 0;
						}
						else
							++offset;
					}//end for j
						
					
				}//end for c
				if (offset!=0) {//left a line
					words.binlines.push_back(line);
				}
				//put a word
				words.word = vstr[1];
				words.bit_ct = _wtoi(vstr[2].substr(vstr[2].rfind(L'.') + 1).c_str());
				_this_dict.push_back(words);
			}//end if
			else
				break;
		}//end while
		file.close();
		//
		//for (auto it : _this_dict[0])
		//	setlog("dict:%X", it);
		return 1;
	}
	return 0;
	
}

long ImageProc::SetDict(int idx) {
	if (idx < 0 || idx >= _max_dicts)
		return 0;
	_curr_dict = &_dicts[idx];
	return 1;
}

long ImageProc::OCR(const wstring& color, double sim, std::wstring& out_str) {
	out_str.clear();
	std::vector<wstring>vstr;
	split(color,vstr, L"-");
	color_t cr, df;
	if (vstr.size() == 2) {
		if (vstr[0].length() != 6 || vstr[1].length() != 6)
			return 0;
		cr.str2color(vstr[0]);
		df.str2color(vstr[1]);
	}
	else {
		if (vstr[0].length() != 6)
			return 0;
		cr.str2color(vstr[0]);
		df.b = df.g = df.r = 0;
	}
		
	if (sim<0. || sim>1.)
		sim = 1.;
	
	long x, y, s;
	ImageExtend::bgr2binary(cr, df);
	for (auto&it : *_curr_dict) {
		s=ImageExtend::Ocr(it, sim, x, y);
		if (s)
			out_str += it.word;
	}
	return 1;

}

long ImageProc::FindColor(const wstring& color, long&x, long&y) {
	std::vector<wstring>vstr;
	split(color, vstr, L"-");
	color_t cr, df;
	cr.str2color(vstr[0]);
	if (vstr.size() == 2) {
		
		df.str2color(vstr[1]);
	}
	return ImageExtend::FindColor(cr, df, x, y);
}

wstring ImageProc::GetColor(long x, long y) {
	color_t cr;
	if (ImageExtend::GetPixel(x, y, cr)) {
		return cr.tostr();
	}
	else {
		return L"";
	}
}
