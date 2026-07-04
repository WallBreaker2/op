#ifndef OP_IMAGE_IMAGE_VIEW_H_
#define OP_IMAGE_IMAGE_VIEW_H_
#include "../base/Types.h"
#include "../image/Image.h"

namespace op {

class ImageView {
  private:
  public:
    ImageView(ImageBin const &src, rect_t const &block);
    ImageView(ImageView const &) = delete;
    ~ImageView();

    /* data */
    const ImageBin &_src;
    rect_t _block;
};

inline ImageView::ImageView(ImageBin const &src, rect_t const &block) : _src(src), _block(block) {
}

inline ImageView::~ImageView() {
}

} // namespace op

#endif // OP_IMAGE_IMAGE_VIEW_H_
