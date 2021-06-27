#ifndef __IMAGE_VIEW_H
#define __IMAGE_VIEW_H
#include "../core/optype.h"
#include "../include/Image.hpp"
class imageView {
 private:
  

 public:
  imageView(ImageBin const& src, rect_t const& block);
  imageView(imageView const&) = delete;
  ~imageView();

  /* data */
  const ImageBin& _src;
  rect_t _block;
};

imageView::imageView(ImageBin const& src, rect_t const& block)
    : _src(src), _block(block) {}

imageView::~imageView() {}

#endif