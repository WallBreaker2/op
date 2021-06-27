//#include "stdafx.h"
#include "ImageLoc.h"

#include <assert.h>
#include <time.h>

#include <numeric>

#include "../core/helpfunc.h"
#include "imageView.hpp"
using std::to_wstring;
//检查是否为透明图，返回透明像素个数, 四角颜色相同且透明颜色数量在50%-99%范围内
int check_transparent(Image *img) {
  if (img->width < 2 || img->height < 2) return 0;
  uint c0 = *img->begin();
  bool x = c0 == img->at<uint>(0, img->width - 1) &&
           c0 == img->at<uint>(img->height - 1, 0) &&
           c0 == img->at<uint>(img->height - 1, img->width - 1);
  if (!x) return 0;

  int ct = 0;
  for (auto it : *img)
    if (it == c0) ++ct;
  int total = img->height * img->width;
  return total * 0.5 <= ct && ct < total ? ct : 0;
}

void get_match_points(const Image &img, vector<uint> &points) {
  points.clear();
  uint cbk = *img.begin();
  for (int i = 0; i < img.height; ++i) {
    for (int j = 0; j < img.width; ++j)
      if (cbk != img.at<uint>(i, j)) points.push_back((i << 16) | j);
  }
}

void gen_next(const Image &img, vector<int> &next) {
  next.resize(img.width * img.height);

  auto t = img.ptr<int>(0);
  auto p = next.data();
  p[0] = -1;
  int k = -1, j = 0;
  while (j < next.size() - 1) {
    if (k == -1 || t[k] == t[j]) {
      k++;
      j++;
      p[j] = k;
    } else {
      k = p[k];
    }
  }
}

void Connectivity(const ImageBin &bin, ImageBin &rec) {}

void extractConnectivity(const ImageBin &src, int threshold,
                         std::vector<ImageBin> &out) {
  ImageBin bin = src;
  for (auto &it : bin) {
    it = it > threshold ? 0xffu : 0;
  }
  ImageBin rec;
  rec.create(bin.width, bin.height);
  for (auto &it : rec) {
    it = 0;
  }
}
struct MatchContext {
  imageView view;
  Image *pic;
  ImageBin *gray;
  color_t dfcolor;
  int tnorm;
  vector<uint> &points;
  int use_ts_match;
};

enum PicMatchType { PicMatchRGB = 0, PicMatchGray = 1, PicMatchTrans = 2 };

// template <int picMatchType>
// point_t PicViewMatch(MatchContext context, double sim, bool *stop) {
//   return point_t();
// }
// template <>
// point_t PicViewMatch<PicMatchGray>(MatchContext context, double sim,
//                                    bool *stop) {
//   // 计算最大误差
//   int max_err_ct =
//       (context.pic->height * context.pic->width - context.use_ts_match) *
//       (1.0 - sim);
//   rect_t &block = context.view._block;
//   for (int i = block.y1; i < block.y2; ++i) {
//     for (int j = block.x1; j < block.x2; ++j) {
//       if (*stop) return point_t(-1, -1);
//       // 开始匹配
//       // quick check
//       if ((double)abs(context.tnorm -
//                       region_sum(x, y, x + timg->width, y + timg->height)) /
//               (double)context.tnorm >
//           1.0 - sim)
//         return 0;
//       int err = 0;
//       int maxErr = (1.0 - sim) * tnorm;
//       for (int i = 0; i < context.gray->height; i++) {
//         auto ptr = context.view._src.ptr(y + i) + x;
//         auto ptr2 = timg->ptr(i);
//         for (int j = 0; j < timg->width; j++) {
//           err += abs(*ptr - *ptr2);
//           ptr++;
//           ptr2++;
//         }
//         if (err > maxErr) return 0;
//       }

//       return 1;
//       // simple_match<false>(j, i, pic, dfcolor,tnorm, sim));
//       if (match_ret) {
//         *stop = true;
//         return point_t(j + _x1 + _dx, i + _y1 + _dy);
//       }

//     }  // end for j
//   }    // end for i
//   return point_t(-1, -1);
//}

ImageBase::ImageBase() : m_threadPool(std::thread::hardware_concurrency()) {
  _x1 = _y1 = 0;
  _dx = _dy = 0;
}

ImageBase::~ImageBase() {}

void ImageBase::set_offset(int x1, int y1) {
  _x1 = x1;
  _y1 = y1;
}

long ImageBase::GetPixel(long x, long y, color_t &cr) {
  auto p = _src.ptr<color_t>(0);
  // setlog("%d", _src.width);
  // static_assert(sizeof(color_t) == 4);
  cr = *p;
  return 1;
}

long ImageBase::CmpColor(color_t color, std::vector<color_df_t> &colors,
                         double sim) {
  for (auto &it : colors) {
    if (IN_RANGE(color, it.color, it.df)) return 1;
  }

  return 0;
}

long ImageBase::FindColor(vector<color_df_t> &colors, int dir, long &x,
                          long &y) {
  for (auto &it : colors) {  //对每个颜色描述

    for (int i = 0; i < _src.height; ++i) {
      auto p = _src.ptr<color_t>(i);
      for (int j = 0; j < _src.width; ++j) {
        if (IN_RANGE(*(p + j), it.color, it.df)) {
          x = j + _x1 + _dx;
          y = i + _y1 + _dy;
          return 1;
        }
        p++;
      }
    }
  }

  x = y = -1;
  return 0;
}

long ImageBase::FindColorEx(vector<color_df_t> &colors, std::wstring &retstr) {
  retstr.clear();
  int find_ct = 0;
  for (int i = 0; i < _src.height; ++i) {
    auto p = _src.ptr<color_t>(i);
    for (int j = 0; j < _src.width; ++j) {
      for (auto &it : colors) {  //对每个颜色描述
        if (IN_RANGE(*p, it.color, it.df)) {
          retstr += std::to_wstring(j + _x1 + _dx) + L"," +
                    std::to_wstring(i + _y1 + _dy);
          retstr += L"|";
          ++find_ct;
          // return 1;
          if (find_ct > _max_return_obj_ct) goto _quick_break;
          break;
        }
      }
      p++;
    }
  }
_quick_break:
  if (!retstr.empty() && retstr.back() == L'|') retstr.pop_back();
  return find_ct;
}

long ImageBase::FindMultiColor(std::vector<color_df_t> &first_color,
                               std::vector<pt_cr_df_t> &offset_color,
                               double sim, long dir, long &x, long &y) {
  int max_err_ct = offset_color.size() * (1. - sim);
  int err_ct;
  for (int i = 0; i < _src.height; ++i) {
    auto p = _src.ptr<color_t>(i);
    for (int j = 0; j < _src.width; ++j) {
      // step 1. find first color
      for (auto &it : first_color) {  //对每个颜色描述
        if (IN_RANGE(*p, it.color, it.df)) {
          //匹配其他坐标
          err_ct = 0;
          for (auto &off_cr : offset_color) {
              int ptX = j + off_cr.x;
              int ptY = i + off_cr.y;
              if (ptX >= 0 && ptX < _src.width && ptY >= 0 && ptY < _src.height) {
                  color_t currentColor = _src.at<color_t>(ptY, ptX);
                  if (!CmpColor(currentColor, off_cr.crdfs, sim))
                      ++err_ct;
              }
              else {
                  ++err_ct;
              }
              if (err_ct > max_err_ct) goto _quick_break;
           
          }
          // ok
          x = j + _x1 + _dx, y = i + _y1 + _dy;
          return 1;
        }
      }
    _quick_break:
      p++;
    }
  }
  x = y = -1;
  return 0;
}

long ImageBase::FindMultiColorEx(std::vector<color_df_t> &first_color,
                                 std::vector<pt_cr_df_t> &offset_color,
                                 double sim, long dir, std::wstring &retstr) {
  int max_err_ct = offset_color.size() * (1. - sim);
  int err_ct;
  int find_ct = 0;
  for (int i = 0; i < _src.height; ++i) {
    auto p = _src.ptr<color_t>(i);
    for (int j = 0; j < _src.width; ++j) {
      // step 1. find first color
      for (auto &it : first_color) {  //对每个颜色描述
        if (IN_RANGE(*p, it.color, it.df)) {
          //匹配其他坐标
          err_ct = 0;
          for (auto &off_cr : offset_color) {
           /* color_t currentColor = _src.at<color_t>(j + off_cr.x, i + off_cr.y);
            if (!CmpColor(currentColor, off_cr.crdfs, sim)) ++err_ct;
            if (err_ct > max_err_ct) goto _quick_break;*/

            //
            int ptX = j + off_cr.x;
            int ptY = i + off_cr.y;
            if (ptX >= 0 && ptX < _src.width && ptY >= 0 && ptY < _src.height) {
                color_t currentColor = _src.at<color_t>(ptY, ptX);
                if (!CmpColor(currentColor, off_cr.crdfs, sim))
                    ++err_ct;
            }
            else {
                ++err_ct;
            }
            if (err_ct > max_err_ct) goto _quick_break;
          }

          // ok
          retstr +=
              to_wstring(j + _x1 + _dx) + L"," + to_wstring(i + _y1 + _dy);
          retstr += L"|";
          ++find_ct;
          if (find_ct > _max_return_obj_ct)
            goto _quick_return;
          else
            goto _quick_break;
        }
      }
    _quick_break:
      p++;
    }
  }
_quick_return:
  if (!retstr.empty() && retstr.back() == L'|') retstr.pop_back();
  return find_ct;
  // x = y = -1;
}

long ImageBase::FindPic(std::vector<Image *> &pics, color_t dfcolor, double sim,
                        long &x, long &y) {
  x = y = -1;
  vector<uint> points;
  int match_ret = 0;
  ImageBin gimg;
  _gray.fromImage4(_src);
  record_sum(_gray);
  int tnorm;
  //将小循环放在最外面，提高处理速度
  for (int pic_id = 0; pic_id < pics.size(); ++pic_id) {
    auto pic = pics[pic_id];
    int use_ts_match = check_transparent(pic);
    if (use_ts_match)
      get_match_points(*pic, points);
    else {
      gimg.fromImage4(*pic);
      tnorm = sum(gimg.begin(), gimg.end());
    }

    for (int i = 0; i < _src.height; ++i) {
      for (int j = 0; j < _src.width; ++j) {
        // step 1. 边界检查
        if (i + pic->height > _src.height || j + pic->width > _src.width)
          continue;
        // step 2. 计算最大误差
        int max_err_ct =
            (pic->height * pic->width - use_ts_match) * (1.0 - sim);
        // step 3. 开始匹配

        /*match_ret = (use_ts_match ? trans_match<false>(j, i, pic, dfcolor,
           points, max_err_ct) : simple_match<false>(j, i, pic, dfcolor,
           max_err_ct));*/
        match_ret = (use_ts_match ? trans_match<false>(j, i, pic, dfcolor,
                                                       points, max_err_ct)
                                  : real_match(j, i, &gimg, tnorm, sim));
        // simple_match<false>(j, i, pic, dfcolor,tnorm, sim));
        if (match_ret) {
          x = j + _x1 + _dx;
          y = i + _y1 + _dy;
          return pic_id;
        }

      }  // end for j
    }    // end for i
  }      // end for pics
  return -1;
}

long ImageBase::FindPicTh(std::vector<Image *> &pics, color_t dfcolor,
                          double sim, long &x, long &y) {
  x = y = -1;
  vector<uint> points;
  int match_ret = 0;
  ImageBin gimg;
  _gray.fromImage4(_src);
  record_sum(_gray);
  int tnorm;
  std::vector<rect_t> blocks;
  //将小循环放在最外面，提高处理速度
  for (int pic_id = 0; pic_id < pics.size(); ++pic_id) {
    auto pic = pics[pic_id];
    int use_ts_match = check_transparent(pic);
    if (use_ts_match)
      get_match_points(*pic, points);
    else {
      gimg.fromImage4(*pic);
      tnorm = sum(gimg.begin(), gimg.end());
    }
    auto pgimg = &gimg;
    rect_t matchRect(0, 0, _src.width, _src.height);
    matchRect.shrinkRect(pic->width, pic->height);
    if (!matchRect.valid()) continue;
    matchRect.divideBlock(m_threadPool.getThreadNum(),
                          matchRect.width() > matchRect.height(), blocks);
    std::vector<std::future<point_t>> results;
    bool stop = false;
    for (size_t i = 0; i < m_threadPool.getThreadNum(); ++i) {
      results.push_back(m_threadPool.enqueue(
          [this, dfcolor, points, pgimg, tnorm](rect_t &block, Image *pic,
                                                int use_ts_match, double sim,
                                                bool *stop) {
            // 计算最大误差
            int max_err_ct =
                (pic->height * pic->width - use_ts_match) * (1.0 - sim);
            for (int i = block.y1; i < block.y2; ++i) {
              for (int j = block.x1; j < block.x2; ++j) {
                if (*stop) return point_t(-1, -1);
                // 开始匹配
                int match_ret =
                    (use_ts_match ? trans_match<false>(j, i, pic, dfcolor,
                                                       points, max_err_ct)
                                  : real_match(j, i, pgimg, tnorm, sim));
                // simple_match<false>(j, i, pic, dfcolor,tnorm, sim));
                if (match_ret) {
                  *stop = true;
                  return point_t(j + _x1 + _dx, i + _y1 + _dy);
                }

              }  // end for j
            }    // end for i
            return point_t(-1, -1);
          },
          blocks[i], pic, use_ts_match, sim, &stop));
      // results.push_back(r);
    }
    // wait all
    for (auto &&f : results) {
      point_t p = f.get();
      if (p.x != -1) {
        x = p.x;
        y = p.y;
        // return pic_id;
      }
    }
    if (x != -1) {
      return pic_id;
    }

  }  // end for pics
  return -1;
}

long ImageBase::FindPicEx(std::vector<Image *> &pics, color_t dfcolor,
                          double sim, vpoint_desc_t &vpd) {
  int obj_ct = 0;
  vpd.clear();
  vector<uint> points;
  bool nodfcolor = color2uint(dfcolor) == 0;
  int match_ret = 0;
  ImageBin gimg;
  _gray.fromImage4(_src);
  record_sum(_gray);
  int tnorm;
  for (int pic_id = 0; pic_id < pics.size(); ++pic_id) {
    auto pic = pics[pic_id];
    int use_ts_match = check_transparent(pic);

    if (use_ts_match)
      get_match_points(*pic, points);
    else {
      gimg.fromImage4(*pic);
      tnorm = sum(gimg.begin(), gimg.end());
    }
    for (int i = 0; i < _src.height; ++i) {
      for (int j = 0; j < _src.width; ++j) {
        // step 1. 边界检查
        if (i + pic->height > _src.height || j + pic->width > _src.width)
          continue;
        // step 2. 计算最大误差
        int max_err_ct =
            (pic->height * pic->width - use_ts_match) * (1.0 - sim);
        // step 3. 开始匹配
      
        match_ret = (use_ts_match ? trans_match<false>(j, i, pic, dfcolor,
                                                       points, max_err_ct)
                                  : real_match(j, i, &gimg, tnorm, sim));
        if (match_ret) {
          point_desc_t pd = {pic_id, point_t(j + _x1 + _dx, i + _y1 + _dy)};

          vpd.push_back(pd);
          ++obj_ct;
          if (obj_ct > _max_return_obj_ct) goto _quick_return;
        }

      }  // end for j
    }    // end for i
  }      // end for pics
_quick_return:

  return obj_ct;
}

long ImageBase::FindPicExTh(std::vector<Image*>& pics, color_t dfcolor,
    double sim, vpoint_desc_t& vpd) {
    vpd.clear();
    int obj_ct = 0;
    vector<uint> points;
    int match_ret = 0;
    ImageBin gimg;
    _gray.fromImage4(_src);
    record_sum(_gray);
    int tnorm;
    std::vector<rect_t> blocks;
    //将小循环放在最外面，提高处理速度
    for (int pic_id = 0; pic_id < pics.size(); ++pic_id) {
        auto pic = pics[pic_id];
        int use_ts_match = check_transparent(pic);
        if (use_ts_match)
            get_match_points(*pic, points);
        else {
            gimg.fromImage4(*pic);
            tnorm = sum(gimg.begin(), gimg.end());
        }
        auto pgimg = &gimg;
        rect_t matchRect(0, 0, _src.width, _src.height);
        matchRect.shrinkRect(pic->width, pic->height);
        if (!matchRect.valid()) continue;
        matchRect.divideBlock(m_threadPool.getThreadNum(),
            matchRect.width() > matchRect.height(), blocks);
        std::vector<std::future<vpoint_t>> results;
        for (size_t i = 0; i < m_threadPool.getThreadNum(); ++i) {
            results.push_back(m_threadPool.enqueue(
                [this, dfcolor, points, pgimg, tnorm](rect_t& block, Image* pic,
                    int use_ts_match, double sim)->vpoint_t {
                        vpoint_t vp;
                        // 计算最大误差
                        int max_err_ct =
                            (pic->height * pic->width - use_ts_match) * (1.0 - sim);
                        for (int i = block.y1; i < block.y2; ++i) {
                            for (int j = block.x1; j < block.x2; ++j) {
                                // 开始匹配
                                int match_ret =
                                    (use_ts_match ? trans_match<false>(j, i, pic, dfcolor,
                                        points, max_err_ct)
                                        : real_match(j, i, pgimg, tnorm, sim));
                                // simple_match<false>(j, i, pic, dfcolor,tnorm, sim));
                                if (match_ret) {
                                    vp.push_back(point_t(j + _x1 + _dx, i + _y1 + _dy));
                                }

                            }  // end for j
                        }    // end for i
                        return vp;
                },
                blocks[i], pic, use_ts_match, sim));
            // results.push_back(r);
        }
        // wait all
        for (auto&& f : results) {
            vpoint_t vp = f.get();
            if (vp.size()>0) {
               
                for (auto& p : vp) {
                    point_desc_t pd = { pic_id,p };

                    vpd.push_back(pd);
                    ++obj_ct;
                    if (obj_ct > _max_return_obj_ct) goto _quick_return;
                }

                // return pic_id;
            }
        }

    }  
_quick_return:

    return obj_ct;
}

long ImageBase::FindColorBlock(double sim, long count, long height, long width,
                               long &x, long &y) {
  x = y = -1;
  record_sum(_binary);
  for (int i = 0; i <= _binary.height - height; ++i) {
    for (int j = 0; j < _binary.width - width; ++j) {
      if (region_sum(j, i, j + width, i + height) >= count) {
        x = j + _x1 + _dx;
        y = i + _y1 + _dy;
        return 1;
      }
    }
  }
  return -1;
}

long ImageBase::FindColorBlockEx(double sim, long count, long height,
                                 long width, std::wstring &retstr) {
  record_sum(_binary);
  int cnt = 0;
  for (int i = 0; i <= _binary.height - height; ++i) {
    for (int j = 0; j < _binary.width - width; ++j) {
      if (region_sum(j, i, j + width, i + height) >= count) {
        wchar_t buff[20];
        wsprintfW(buff, L"%d,%d|", j + _x1 + _dx, i + _y1 + _dy);
        retstr += buff;
        ++cnt;
        if (cnt > _max_return_obj_ct) goto _quick_return;
      }
    }
  }
_quick_return:
  if (cnt) {
    retstr.pop_back();
  }
  return cnt;
}

long ImageBase::Ocr(Dict &dict, double sim, wstring &retstr) {
  retstr.clear();
  std::map<point_t, wstring> ps;
  bin_ocr(dict, sim, ps);
  for (auto &it : ps) {
    retstr += it.second;
  }
  return 1;
}

long ImageBase::OcrEx(Dict &dict, double sim, std::wstring &retstr) {
  retstr.clear();
  std::map<point_t, wstring> ps;
  bin_ocr(dict, sim, ps);
  // x1,y1,str....|x2,y2,str2...|...
  int find_ct = 0;
  for (auto &it : ps) {
    retstr += std::to_wstring(it.first.x + _x1 + _dx);
    retstr += L",";
    retstr += std::to_wstring(it.first.y + _y1 + _dy);
    retstr += L",";
    retstr += it.second;
    retstr += L"|";
    ++find_ct;
    if (find_ct > _max_return_obj_ct) break;
  }
  if (!retstr.empty() && retstr.back() == L'|') retstr.pop_back();
  return find_ct;
}

long ImageBase::FindStr(Dict &dict, const vector<wstring> &vstr, double sim,
                        long &retx, long &rety) {
  retx = rety = -1;

  //查找字符 返回坐标
  // step 1. 找出频幕中所有字符及其坐标信息
  std::map<point_t, wstring> ps;
  bin_ocr(dict, sim, ps);
  // step 2. 拼接字符 形成完整字符串
  wstring str;
  for (auto &it : ps) str.append(it.second);
  // step 3. 在完整字符中查找目标字符串，并记录 索引
  int idx = -1;
  int oneIndex = -1;
  for (int i = 0; i < vstr.size(); ++i) {
    idx = str.find(vstr[i]);
    if (idx != -1) {
      oneIndex = i;
      break;
    }
  }
  // step 4.根据索引给出对应 坐标 并返回
  if (idx != -1) {  // locate it
    int curr_len = 0;
    for (auto &it : ps) {
      curr_len += it.second.length();
      if (curr_len < idx + 1) continue;
      if (it.second.find(str[idx]) != -1) {
        retx = it.first.x + _x1 + _dx;
        rety = it.first.y + _y1 + _dy;
        return oneIndex;
      }
    }
    //这里进行断言，表示不会走到这一步
    assert(0);
  }

  return -1;
}

long ImageBase::FindStrEx(Dict &dict, const vector<wstring> &vstr, double sim,
                          std::wstring &retstr) {
  //描述：查找屏幕指定位置的字符（或者字符串）位置，返回所有出现的坐标（注意与FindStr接口的区别）！！！
  //----------------------步骤-----------------
  // step 1. 获取指定位置的字符及坐标信息
  // step 2. 拼接字符，形成完整字符串 str
  // step 2.对每个目标字符 ti , 查找其在str中的位置，并记录 index(如果存在）
  // step 4.根据index ,获取其坐标，并将坐标转化为字符串，拼接到返回值
  // step 5. 回到第3步

  retstr.clear();
  std::map<point_t, wstring> ps;
  // step 1.
  bin_ocr(dict, sim, ps);
  // setp 2.
  wstring str;
  for (auto &it : ps) {
    str.append(it.second);
  }
  // step 3.
  int find_ct = 0;
  for (int i = 0; i < vstr.size(); ++i) {
    int index = -1, old = -1;
    do {
      index = str.find(vstr[i], old + 1);
      if (index == -1) {  // failed!!
        break;
      }
      // step 4
      int current_len = 0;
      for (auto &it : ps) {
        //注意 字符长度要大于index 才记录坐标
        current_len += it.second.length();
        if (current_len < index + 1) continue;
        if (it.second.find(str[index]) != -1) {
          //记录坐标
          wchar_t buff[20] = {0};
          //注意加偏移
          wsprintf(buff, L"%d,%d,%d|", i, it.first.x + _x1 + _dx,
                   it.first.y + _y1 + _dy);
          retstr.append(buff);
          ++find_ct;
          if (find_ct > _max_return_obj_ct)
            goto _quick_return;
          else
            break;
          // to do 这里还需要修改
        }
      }  // end for ps
      old = index;
    } while (1);
  }
_quick_return:
  if (!retstr.empty() && retstr.back() == L'|') retstr.pop_back();
  return find_ct;
}

long ImageBase::FindLine(double sim, std::wstring &outStr) {
  outStr.clear();
  int h =
      sqrt(_binary.width * _binary.width + _binary.height * _binary.height) + 2;
  _sum.create(360, h);
  //行：距离，列：角度
  _sum.fill(0);
  for (int i = 0; i < _binary.height; i++) {
    for (int j = 0; j < _binary.width; j++) {
      if (_binary.at(i, j) == WORD_COLOR) {
        for (int t = 0; t < 360; t++) {
          int d =
              j * cos(t * 0.0174532925) + i * sin(t * 0.0174532925);  //可以优化
          assert(d <= h);
          if (d >= 0) _sum.at<int>(d, t)++;
        }
      }
    }
  }
  int maxRow = 0, maxCol = 0;
  int maxval = -1;
  for (int i = 0; i < _sum.height; i++) {
    for (int j = 0; j < _sum.width; j++) {
      if (_sum.at<int>(i, j) > maxval) {
        maxRow = i;
        maxCol = j;
        maxval = _sum.at<int>(i, j);
      }
    }
  }
  // setlog("degree=%d,dis=%d,val=%d", maxCol, maxRow, maxval);
  wchar_t buffer[256];
  wsprintf(buffer, L"%d,%d", maxCol, maxRow);
  outStr = buffer;
  return maxval;
}

template <bool nodfcolor>
long ImageBase::simple_match(long x, long y, Image *timg, color_t dfcolor,
                             int tnorm, double sim) {
  int err_ct = 0;
  // quick check
  if ((double)abs(tnorm - region_sum(x, y, x + timg->width, y + timg->height)) >
      (double)tnorm * (1.0 - sim))
    return 0;
  int max_error = (1.0 - sim) * timg->size();
  uint *pscreen_top, *pscreen_bottom, *pimg_top, *pimg_bottom;
  pscreen_top = _src.ptr<uint>(y) + x;
  pscreen_bottom = _src.ptr<uint>(y + timg->height - 1) + x;
  pimg_top = timg->ptr<uint>(0);
  pimg_bottom = timg->ptr<uint>(timg->height - 1);
  while (pscreen_top <= pscreen_bottom) {
    auto ps1 = pscreen_top, ps2 = pscreen_top + timg->width - 1;
    auto ps3 = pscreen_bottom, ps4 = pscreen_bottom + timg->width - 1;
    auto pt1 = pimg_top, pt2 = pimg_top + timg->width - 1;
    auto pt3 = pimg_bottom, pt4 = pimg_bottom + timg->width - 1;
    while (ps1 <= ps2) {
      if (nodfcolor) {
        if (*ps1++ != *pt1++) ++err_ct;  // top left
        if (*ps2-- != *pt2--) ++err_ct;  // top right
        if (*ps3++ != *pt3++) ++err_ct;  // bottom left
        if (*ps4-- != *pt4--) ++err_ct;  // bottom right
      } else {
        if (!IN_RANGE(*(color_t *)ps1++, *(color_t *)pt1++, dfcolor)) ++err_ct;
        if (!IN_RANGE(*(color_t *)ps2--, *(color_t *)pt2--, dfcolor)) ++err_ct;
        if (!IN_RANGE(*(color_t *)ps3++, *(color_t *)pt3++, dfcolor)) ++err_ct;
        if (!IN_RANGE(*(color_t *)ps4--, *(color_t *)pt4--, dfcolor)) ++err_ct;
      }

      if (err_ct > max_error) return 0;
    }
    pscreen_top += _src.width;
    pscreen_bottom -= _src.width;
  }

  return 1;
}
template <bool nodfcolor>
long ImageBase::trans_match(long x, long y, Image *timg, color_t dfcolor,
                            vector<uint> pts, int max_error) {
  int err_ct = 0, k, dx, dy;
  int left, right;
  left = 0;
  right = pts.size() - 1;
  while (left <= right) {
    auto it = pts[left];
    if (nodfcolor) {
      if (_src.at<uint>(y + PTY(pts[left]), x + PTX(pts[left])) !=
          timg->at<uint>(PTY(pts[left]), PTX(pts[left])))
        ++err_ct;
      if (_src.at<uint>(y + PTY(pts[right]), x + PTX(pts[right])) !=
          timg->at<uint>(PTY(pts[right]), PTX(pts[right])))
        ++err_ct;
    } else {
      color_t cr1, cr2;
      cr1 = _src.at<color_t>(y + PTY(pts[left]), x + PTX(pts[left]));
      cr2 = timg->at<color_t>(PTY(pts[left]), PTX(pts[left]));
      if (!IN_RANGE(cr1, cr2, dfcolor)) ++err_ct;
      cr1 = _src.at<color_t>(y + PTY(pts[right]), x + PTX(pts[right]));
      cr2 = timg->at<color_t>(PTY(pts[right]), PTX(pts[right]));
      if (!IN_RANGE(cr1, cr2, dfcolor)) ++err_ct;
    }

    ++left;
    --right;
    if (err_ct > max_error) return 0;
  }
  return 1;
}

long ImageBase::real_match(long x, long y, ImageBin *timg, int tnorm,
                           double sim) {
  // quick check
  if ((double)abs(tnorm - region_sum(x, y, x + timg->width, y + timg->height)) /
          (double)tnorm >
      1.0 - sim)
    return 0;
  int err = 0;
  int maxErr = (1.0 - sim) * tnorm;
  for (int i = 0; i < timg->height; i++) {
    auto ptr = _gray.ptr(y + i) + x;
    auto ptr2 = timg->ptr(i);
    for (int j = 0; j < timg->width; j++) {
      err += abs(*ptr - *ptr2);
      ptr++;
      ptr2++;
    }
    if (err > maxErr) return 0;
  }

  return 1;
}

void ImageBase::record_sum(const ImageBin &gray) {
  //为了减少边界判断，这里多多加一行一列
  _sum.create(gray.width + 1, gray.height + 1);
  _sum.fill(0);
  int m = _sum.height;
  int n = _sum.width;
  for (int i = 1; i < m; i++) {
    for (int j = 1; j < n; j++) {
      int s = 0;

      s += _sum.at<int>(i - 1, j);

      s += _sum.at<int>(i, j - 1);

      s -= _sum.at<int>(i - 1, j - 1);
      s += (int)gray.at(i - 1, j - 1);
      _sum.at<int>(i, j) = s;
    }
  }
}

int ImageBase::region_sum(int x1, int y1, int x2, int y2) {
  int ans = _sum.at<int>(y2, x2) - _sum.at<int>(y2, x1) - _sum.at<int>(y1, x2) +
            _sum.at<int>(y1, x1);
  return ans;
}

constexpr int MIN_CUT_W = 5;
constexpr int MIN_CUT_H = 2;

int ImageBase::get_bk_color(inputbin bin) {
  int y[256] = {0};
  auto ptr = bin.pixels.data();
  int n = bin.pixels.size();
  for (int i = 0; i < n; ++i) y[ptr[i]]++;
  // scan max
  int m = 0;
  for (int i = 1; i < 256; ++i) {
    if (y[i] > y[m]) m = i;
  }
  return m;
}

void ImageBase::bgr2binary(vector<color_df_t> &colors) {
  if (_src.empty()) return;
  int ncols = _src.width, nrows = _src.height;
  _binary.fromImage4(_src);
  for (int i = 0; i < nrows; ++i) {
    auto psrc = _src.ptr<color_t>(i);

    auto pbin = _binary.ptr(i);
    for (int j = 0; j < ncols; ++j) {
      uchar g1 = psrc->toGray();
      *pbin = WORD_BKCOLOR;
      for (auto &it : colors) {  //对每个颜色描述
        if (abs(g1 - it.color.toGray()) <= it.df.toGray()) {
          *pbin = WORD_COLOR;
          break;
        }
      }
      ++pbin;
      ++psrc;
    }
  }
  //_binary.create(ncols, nrows);
  // for (int i = 0; i < nrows; ++i) {
  //	auto psrc = _src.ptr<color_t>(i);
  //	auto pbin = _binary.ptr(i);
  //	for (int j = 0; j < ncols; ++j) {
  //		*pbin = WORD_BKCOLOR;
  //		for (auto& it : colors) {//对每个颜色描述
  //			if (IN_RANGE(*psrc, it.color, it.df)) {
  //				*pbin = WORD_COLOR;
  //				break;
  //			}
  //		}
  //		++pbin; ++psrc;
  //	}
  //}
  // test
  // cv::imwrite("src.png", _src);
  // cv::imwrite("binary.png", _binary);
}

//二值化
void ImageBase::bgr2binarybk(const vector<color_df_t> &bk_colors) {
  //创建二值图
  _binary.create(_src.width, _src.height);
  memset(_binary.pixels.data(), WORD_BKCOLOR, _binary.size());
  int n = _binary.size();
  auto pdst = _binary.data();
  if (bk_colors.size() == 0) {  // auto
    //转为灰度图
    _gray.fromImage4(_src);

    //获取背景颜色
    int bkcolor = get_bk_color(_gray);

    auto pgray = _gray.data();
    for (int i = 0; i < n; ++i) {
      pdst[i] =
          (std::abs((int)pgray[i] - bkcolor) < 20 ? WORD_BKCOLOR : WORD_COLOR);
    }
  } else {
    for (auto bk : bk_colors) {
      for (int i = 0; i < n; ++i) {
        auto c = (color_t *)(_src.pdata + i * 4);
        if (!IN_RANGE(*c, bk.color, bk.df)) pdst[i] = WORD_COLOR;
      }
    }
  }
}

//垂直方向投影到x轴
void ImageBase::binshadowx(const rect_t &rc, std::vector<rect_t> &out_put) {
  // qDebug("in x rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
  out_put.clear();
  // ys.clear();
  // Mat paintx(binary.size(), CV_8UC1, cv::Scalar(255));
  // //创建一个全白图片，用作显示

  // int* blackcout = new int[binary.cols];
  std::vector<int> vx;
  vx.resize(_binary.width);
  memset(&vx[0], 0, _binary.width * 4);
  for (int j = rc.x1; j < rc.x2; j++) {
    for (int i = rc.y1; i < rc.y2; i++) {
      if (_binary.at(i, j) == WORD_COLOR) {
        vx[j]++;  //垂直投影按列在x轴进行投影
      }
    }
  }

  int startindex = 0;
  int endindex = 0;
  bool inblock = false;  //是否遍历到字符位置
  rect_t roi;
  for (int j = rc.x1; j < rc.x2; j++) {
    if (!inblock && vx[j] != 0)  //进入有字符区域
    {
      inblock = true;
      startindex = j;
      // std::cout << "startindex:" << startindex << std::endl;
    }
    // if (inblock&&vx[j] == 0) //进入空白区
    else if (inblock && vx[j] == 0 &&
             j - startindex >= MIN_CUT_W)  //进入空白区域，且宽度不小于1
    {
      endindex = j;
      inblock = false;
      // Mat roi = binary.colRange(startindex, endindex + 1);

      roi.x1 = startindex;
      roi.y1 = rc.y1;
      roi.x2 = endindex;
      roi.y2 = rc.y2;
      // qDebug("out xrc:%d,%d,%d,%d", roi.x1, roi.y1, roi.x2, roi.y2);
      out_put.push_back(roi);
    }
  }
  // special case
  if (inblock) {
    roi.x1 = startindex;
    roi.y1 = rc.y1;
    roi.x2 = rc.x2;
    roi.y2 = rc.y2;
    out_put.push_back(roi);
  }
}
//投影到y轴
void ImageBase::binshadowy(const rect_t &rc, std::vector<rect_t> &out_put) {
  // qDebug("in y rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
  out_put.clear();
  //是否为白色或者黑色根据二值图像的处理得来
  // Mat painty(binary.size(), CV_8UC1, cv::Scalar(255)); //初始化为全白
  //水平投影
  // int* pointcount = new int[binary.rows]; //在二值图片中记录行中特征点的个数
  std::vector<int> vy;
  vy.resize(_binary.height);
  memset(&vy[0], 0, _binary.height * 4);  //注意这里需要进行初始化

  for (int i = rc.y1; i < rc.y2; i++) {
    for (int j = rc.x1; j < rc.x2; j++) {
      if (_binary.at(i, j) == WORD_COLOR) {
        vy[i]++;  //记录每行中黑色点的个数 //水平投影按行在y轴上的投影
      }
    }
  }

  // std::vector<Mat> result;
  int startindex = 0;
  int endindex = 0;
  bool inblock = false;  //是否遍历到字符位置
  rect_t roi;
  for (int i = rc.y1; i < rc.y2; i++) {
    if (!inblock && vy[i] != 0)  //进入有字符区域
    {
      inblock = true;
      startindex = i;
      // std::cout << "startindex:" << startindex << std::endl;
    }
    // if (inblock&&vy[i] == 0) //进入空白区
    if (inblock && vy[i] == 0 &&
        i - startindex >= MIN_CUT_H)  //进入空白区,且高度不小于1
    {
      endindex = i;
      inblock = false;

      roi.x1 = rc.x1;
      roi.y1 = startindex;
      roi.x2 = rc.x2;
      roi.y2 = endindex;
      out_put.push_back(roi);
    }
  }

  if (inblock) {
    roi.x1 = rc.x1;
    roi.y1 = startindex;
    roi.x2 = rc.x2;
    roi.y2 = rc.y2;
    out_put.push_back(roi);
  }
}

void ImageBase::bin_image_cut(int min_word_h, const rect_t &inrc,
                              rect_t &outrc) {
  //水平裁剪，缩小高度
  std::vector<int> v;
  outrc = inrc;
  int i, j;
  v.resize(_binary.height);
  for (auto &it : v) it = 0;
  for (i = inrc.y1; i < inrc.y2; ++i) {
    for (j = inrc.x1; j < inrc.x2; ++j)
      v[i] += (_binary.at(i, j) == WORD_COLOR ? 1 : 0);
  }
  i = inrc.y1;
  while (v[i] == 0) i++;
  outrc.y1 = i;
  i = inrc.y2 - 1;
  while (v[i] == 0) i--;
  if (i + 1 - outrc.y1 > min_word_h) outrc.y2 = i + 1;
  //垂直裁剪.缩小宽度
  v.resize(_binary.width);
  for (auto &it : v) it = 0;

  for (i = inrc.y1; i < inrc.y2; ++i) {
    for (j = inrc.x1; j < inrc.x2; ++j)
      v[j] += _binary.at(i, j) == WORD_COLOR ? 1 : 0;
  }
  i = inrc.x1;
  while (v[i] == 0) i++;
  outrc.x1 = i;
  i = inrc.x2 - 1;
  while (v[i] == 0) i--;
  outrc.x2 = i + 1;
}

void ImageBase::get_rois(int min_word_h, std::vector<rect_t> &vroi) {
  vroi.clear();
  std::vector<rect_t> vrcx, vrcy;
  rect_t rc;
  rc.x1 = rc.y1 = 0;
  rc.x2 = _binary.width;
  rc.y2 = _binary.height;
  binshadowy(rc, vrcy);
  for (int i = 0; i < vrcy.size(); ++i) {
    binshadowx(vrcy[i], vrcx);
    for (int j = 0; j < vrcx.size(); j++) {
      if (vrcx[j].width() >= min_word_h)
        bin_image_cut(min_word_h, vrcx[j], vrcx[j]);
      vroi.push_back(vrcx[j]);
    }
  }
}

inline int full_match(const ImageBin &binary, rect_t &rc, const uint8_t *data) {
  //匹配
  int idx = 0;
  for (int x = rc.x1; x < rc.x2; ++x) {
    for (int y = rc.y1; y < rc.y2; ++y) {
      int val = GET_BIT(data[idx / 8], idx & 7);
      if (binary.at(y, x) != val) return 0;
      idx++;
    }
  }
  return 1;
}

inline int part_match(const ImageBin &binary, rect_t &rc, int max_error,
                      const uint8_t *data) {
  //匹配
  //匹配
  int err_ct = 0;
  int idx = 0;
  for (int x = rc.x1; x < rc.x2; ++x) {
    for (int y = rc.y1; y < rc.y2; ++y) {
      int val = GET_BIT(data[idx / 8], idx & 7);
      if (binary.at(y, x) != val) {
        ++err_ct;
        if (err_ct > max_error) return 0;
      }
      idx++;
    }
  }
  return 1;
}

inline void fill_rect(ImageBin &record, const rect_t &rc) {
  int w = rc.width();
  for (int y = rc.y1; y < rc.y2; ++y) {
    uchar *p = record.ptr(y) + rc.x1;
    memset(p, 1, sizeof(uchar) * w);
  }
}

int binarySearch(const word1_t a[], int bidx, int eidx, int target)  //循环实现
{
  int low = bidx, high = eidx, middle;
  while (low < high) {
    middle = (low + high) / 2;
    if (target == a[middle].info.bit_cnt)
      return middle;
    else if (target > a[middle].info.bit_cnt)
      low = middle + 1;
    else if (target < a[middle].info.bit_cnt)
      high = middle;
  }
  return -1;
};

//完全匹配 待识别文字不能含有任何噪声
/*
算法
f(p,size) 为从点p开始，大小为size的矩形块像素之和,这个函数使用查表法快速计算
字库:D 字的像素范围:m-M 字的大小范围:size_min-size_max
识别图像:src
for each point in src:
        if f(p,size_max)<m //像素太少
                continue;
        if f(p,size_min)>M //像素太多
                continue;
        //像素合适
        for each w in D
                if f(p,w_size)==w_cnt
                        ok=match(...) //作最后的匹配
                        if ok
                                add w to result;
                                delete rect(p,w_size);
                        else
                                continue;//to next w
                        endif
                end
        end
end
*/
void ImageBase::_bin_ocr(const Dict &dict,
                         std::map<point_t, std::wstring> &ps) {
  int px, py;
  if (_binary.empty()) return;
  //
  record_sum(_binary);
  // find cnt range
  // find width and height range;
  int cnt_min = 255 * 255, cnt_max = 0;
  int w_min = 255, h_min = 255;
  int w_max = 0, h_max = 0;
  for (auto &it : dict.words) {
    cnt_min = min(cnt_min, it.info.bit_cnt);
    cnt_max = max(cnt_max, it.info.bit_cnt);
    w_min = min(w_min, it.info.w);
    h_min = min(h_min, it.info.h);
    w_max = max(w_max, it.info.w);
    h_max = max(h_max, it.info.h);
  }

  //将所有字库按照大小分成几类，对于每个大小根据像素密度查找对应的符合字库
  auto makeinfo = [](int begin, int end, int szh, int szw) {
    return std::make_pair(begin << 16 | end, szh << 8 | szw);
  };
  vector<std::pair<int, int>> dict_sz;
  auto &vword = dict.words;
  // 32 begin(8)
  dict_sz.push_back(makeinfo(0, 0, dict.words[0].info.h, dict.words[0].info.w));
  for (int i = 1; i < vword.size(); i++) {
    int sz = vword[i].info.h << 8 | vword[i].info.w;
    if (dict_sz.back().second != sz) {
      dict_sz.back().first |= i;                       // fix old end
      dict_sz.push_back(std::make_pair(i << 16, sz));  // add new begin
    }
  }
  dict_sz.back().first |= vword.size();

  //遍历行
  for (py = 0; py < _binary.height - h_min + 1; ++py) {
    //遍历列
    for (px = 0; px < _binary.width - w_min + 1; ++px) {
      if (_record.at(py, px)) continue;
      //检测像素密度
      if (region_sum(px, py, min(px + w_max, _binary.width),
                     min(py + h_max, _binary.height)) < cnt_min)  // too less
        continue;
      if (region_sum(px, py, px + w_min, py + h_min) > cnt_max)  // too much
        continue;
      point_t pt;
      pt.x = px;
      pt.y = py;
      int k = 0;
      for (int k = 0; k < dict_sz.size(); k++) {
        int h = dict_sz[k].second >> 8, w = dict_sz[k].second & 0xff;
        rect_t crc;
        crc.x1 = px;
        crc.y1 = py;
        crc.x2 = px + w;
        crc.y2 = py + h;
        //边界检查
        if (crc.y2 > _binary.height || crc.x2 > _binary.width) continue;

        int fidx = dict_sz[k].first >> 16, eidx = dict_sz[k].first & 0xffff;

        // quick check
        int cnt_src = region_sum(crc.x1, crc.y1, crc.x2, crc.y2);
        if (cnt_src < vword[fidx].info.bit_cnt ||
            cnt_src > vword[eidx - 1].info.bit_cnt)
          continue;
        int tidx = binarySearch(&vword[0], fidx, eidx, cnt_src);
        if (tidx == -1) continue;
        int tleft = tidx, tright = tidx;
        while (tleft > 0 && vword[tleft - 1].info.bit_cnt == cnt_src) --tleft;
        while (tright < eidx && vword[tright].info.bit_cnt == cnt_src) ++tright;
        int matched = 0;
        // match
        tidx = tleft;
        while (tidx < tright) {
          auto &it = vword[tidx++];
          matched = full_match(_binary, crc, it.data.data());
          if (matched) {
            // final check
            // check right col is empty/background
            int rs = 0;
            if (crc.x2 < _binary.width) {
              for (int k = crc.y1; k < crc.y2; k++) rs += _binary.at(k, crc.x2);
            }
            if (rs < it.info.h / 2) {
              ps[pt] = it.info.name;
              fill_rect(_record, crc);
              // break;//words
              break;
            } else {
              // not matched
              matched = 0;
            }
          }
        }
        if (matched) break;
      }  // end for words
    }    // end for j
  }      // end for i
}
//模糊匹配 待识别区域可以含有噪声
void ImageBase::_bin_ocr(const Dict &dict, double sim,
                         std::map<point_t, std::wstring> &ps) {
  int px, py, y;
  if (_binary.empty()) return;
  //
  record_sum(_binary);
  // find cnt range
  // find width and height range;
  int cnt_min = 255 * 255, cnt_max = 0;
  int w_min = 255, h_min = 255;
  int w_max = 0, h_max = 0;
  for (auto &it : dict.words) {
    cnt_min = min(cnt_min, it.info.bit_cnt);
    cnt_max = max(cnt_max, it.info.bit_cnt);
    w_min = min(w_min, it.info.w);
    h_min = min(h_min, it.info.h);
    w_max = max(w_max, it.info.w);
    h_max = max(h_max, it.info.h);
  }
  int matched = 0;
  //遍历行
  for (py = 0; py < _binary.height - h_min + 1; ++py) {
    //遍历列
    for (px = 0; px < _binary.width - w_min + 1; ++px) {
      if (_record.at(py, px)) continue;
      //检测像素密度
      if (region_sum(px, py, min(px + w_max, _binary.width),
                     min(py + h_max, _binary.height)) <
          cnt_min * sim)  // too less
        continue;
      if (region_sum(px, py, px + w_min, py + h_min) > cnt_max * (2 - sim))
        continue;
      point_t pt;
      pt.x = px;
      pt.y = py;
      //遍历字库
      // assert(i != 4 || j != 3);
      int k = 0;
      for (auto &it : dict.words) {
        rect_t crc;
        crc.x1 = px;
        crc.y1 = py;
        crc.x2 = px + it.info.w;
        crc.y2 = py + it.info.h;
        //边界检查
        if (crc.y2 > _binary.height || crc.x2 > _binary.width) continue;
        // quick check

        int error = (1 - sim) * it.info.w * it.info.h;
        if (abs(region_sum(crc.x1, crc.y1, crc.x2, crc.y2) - it.info.bit_cnt) >
            error)
          continue;
        // match
        matched = part_match(_binary, crc, error, it.data.data());

        if (matched) {
          // final check
          // check right col is empty/background
          int rs = 0;
          if (crc.x2 < _binary.width) {
            for (int k = crc.y1; k < crc.y2; k++) rs += _binary.at(k, crc.x2);
          }
          if (rs <= it.info.h / 2) {
            ps[pt] = it.info.name;
            fill_rect(_record, crc);
            // break;//words
            break;
          } else {
            // not matched
            matched = 0;
          }
        }
      }  // end for words
         // if (matched)break;
    }    // end for j
  }      // end for i
}

void ImageBase::bin_ocr(const Dict &dict, double sim,
                        std::map<point_t, std::wstring> &ps) {
  ps.clear();
  if (dict.words.empty()) return;
  if (_binary.empty()) return;
  _record.create(_binary.width, _binary.height);
  memset(_record.data(), 0, sizeof(uchar) * _record.width * _record.height);

  if (sim > 1.0 - 1e-5) {
    _bin_ocr(dict, ps);
  } else {
    sim = 0.5 + sim / 2;

    _bin_ocr(dict, sim, ps);
  }
}
