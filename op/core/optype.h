#pragma once
#ifndef __optype_h_
#define __optype_h_
#include <string>
#include <vector>
#include <map>
using uint = unsigned int;
using uchar = unsigned char;

using std::wstring;
using std::string;
using std::vector;
using std::map;

using bytearray = std::vector<uchar>;

struct point_t {
	int x, y;
	bool operator<(const point_t& rhs) const {
		if (std::abs(y - rhs.y) < 9)
			return x < rhs.x;
		else
			return y < rhs.y;
	}
	bool operator==(const point_t& rhs) const {
		return x == rhs.x && y == rhs.y;
	}
};

using vpoint_t = std::vector<point_t>;

struct rect_t {
	int x1, y1;
	int x2, y2;
	int width() const { return x2 - x1; }
	int height() const { return y2 - y1; }
};

using vrect_t = std::vector<rect_t>;

struct point_desc_t {
	int id;
	point_t pos;
};

using vpoint_desc_t = std::vector<point_desc_t>;



#endif