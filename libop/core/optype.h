#pragma once
#ifndef __optype_h_
#define __optype_h_
#include <Windows.h>
#include <assert.h>

#include <map>
#include <string>
#include <vector>
using uint = unsigned int;
using uchar = unsigned char;

using std::map;
using std::string;
using std::vector;
using std::wstring;

using bytearray = std::vector<uchar>;

struct point_t {
  int x, y;
  point_t() : x(0), y(0) {}
  point_t(int x_, int y_) : x(x_), y(y_) {}
  bool operator<(const point_t& rhs) const {
    if (std::abs(y - rhs.y) < 9)
      return x < rhs.x;
    else
      return y < rhs.y;
  }
  bool operator==(const point_t& rhs) const { return x == rhs.x && y == rhs.y; }
};

using vpoint_t = std::vector<point_t>;
//(5,3) --> (2, 2, 1)
class NumberGen {
  int _q, _r;

 public:
  NumberGen(int n, int cnt) : _q(n / cnt), _r(n % cnt) {}
  int operator[](int idx) const { return idx < _r ? _q + 1 : _q; }
};

struct rect_t {
  rect_t() : x1(0), y1(0), x2(0), y2(0) {}
  rect_t(int x1_, int y1_, int x2_, int y2_)
      : x1(x1_), y1(y1_), x2(x2_), y2(y2_) {}
  int x1, y1;
  int x2, y2;
  int width() const { return x2 - x1; }
  int height() const { return y2 - y1; }
  rect_t& shrinkRect(int w, int h) {
    x2 -= w;
    y2 -= h;
    x2 += 1;
    y2 += 1;
    return *this;
  }
  bool valid() const { return 0 <= x1 && x1 < x2 && 0 <= y1 && y1 < y2; }

  void divideBlock(int count, bool vertical, std::vector<rect_t>& blocks) {
    assert(valid());

    assert(count > 0);
    blocks.resize(count);
    if (vertical) {
      NumberGen gen(height(), count);
      int basey = y1;
      for (int i = 0; i < count; ++i) {
        blocks[i] = rect_t(x1, basey, x2, basey + gen[i]);
        basey += gen[i];
      }

    } else {
      NumberGen gen(width(), count);
      int basex = x1;
      for (int i = 0; i < count; ++i) {
        blocks[i] = rect_t(basex, y1, basex + gen[i], y2);
        basex += gen[i];
      }
    }
    assert(blocks.back().x2 == x2);
    assert(blocks.back().y2 == y2);
  }
};

using vrect_t = std::vector<rect_t>;

struct point_desc_t {
  int id;
  point_t pos;
};

using vpoint_desc_t = std::vector<point_desc_t>;

#endif