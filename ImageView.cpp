#include <cstring>
#include <ImageView.hpp>

namespace haste {

ImageView::ImageView(dvec4* data, size_t width, size_t height)
    : _data(data)
    , _width(width)
    , _height(height)
    , _xOffset(0)
    , _yOffset(0)
    , _xWindow(width)
    , _yWindow(height) {
}

dvec4& ImageView::relAt(size_t x, size_t y) {
    return _data[(y + _yOffset) * _width + x + _xOffset];
}

dvec4& ImageView::absAt(size_t x, size_t y) {
    return _data[y * _width + x];
}

const dvec4& ImageView::relAt(size_t x, size_t y) const {
    return _data[(y + _yOffset) * _width + x + _xOffset];
}

const dvec4& ImageView::absAt(size_t x, size_t y) const {
    return _data[y * _width + x];
}

const dvec4& ImageView::last() const {
    return _data[(_yWindow + _yOffset - 1) * _width + _xWindow + _xOffset - 1];
}

void ImageView::clear() {
    if (_xOffset == 0 && _yOffset == 0 &&
        _xWindow == _width && _yWindow == _height) {
        std::memset(_data, 0, _width * _height * sizeof(dvec4));
    }
    else {
        size_t yBegin = _yOffset;
        size_t yEnd = _yOffset + _yWindow;

        for (size_t y = yBegin; y < yEnd; ++y) {
            std::memset(_data + y * _width + _xOffset, 0, _xWindow * sizeof(dvec4));
        }
    }
}

void ImageView::copyFrom(const std::vector<dvec4>& data, size_t width, size_t height) {
    if (width == this->width() && height == this->height()) {
        for (size_t y = 0; y < height; ++y) {
            std::memcpy(_data + width * y, data.data() + width * y, sizeof(dvec4) * width);
        }
    }
}

}
