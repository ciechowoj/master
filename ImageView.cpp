#include <cstring>
#include <stdexcept>
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

void rms_abs_errors(
    float& rms,
    float& abs,
    image_view_t<dvec4> a,
    image_view_t<vec3> b)
{
  if (a.width != b.width || a.height != b.height) {
    throw std::invalid_argument("Image view dimensions must match.");
  }

  rms = 0;
  abs = 0;

  for (size_t y = 0; y < a.height; ++y) {
    for (size_t x = 0; x < a.width; ++x) {
      vec3 d = glm::abs(vec3(a.at(x, y).xyz() / a.at(x, y).w) - b.at(x, y));
      abs += d.x + d.y + d.z;
      rms += glm::dot(d, d);
    }
  }

  float num = float(a.width * a.height * 3);

  rms = sqrt(rms / num);
  abs = abs / num;
}

void rms_abs_errors_windowed(
  float& rms,
  float& abs,
  image_view_t<dvec4> a,
  image_view_t<vec3> b,
  const ivec2& center,
  int radius)
{
  if (a.width != b.width || a.height != b.height) {
    throw std::invalid_argument("Image view dimensions must match.");
  }

  rms = 0;
  abs = 0;

  float num = 0.0f;

  for (int y = glm::max(0, center.y - radius + 1); y < glm::min(center.y + radius, int(a.height)); ++y) {
    for (int x = glm::max(0, center.x - radius + 1); x < glm::min(center.x + radius, int(a.width)); ++x) {
      vec3 d = glm::abs(vec3(a.at(x, y).xyz() / a.at(x, y).w) - b.at(x, y));
      abs += d.x + d.y + d.z;
      rms += glm::dot(d, d);
      num += 1.0f;
    }
  }

  rms = sqrt(rms / num);
  abs = abs / num;
}

}
