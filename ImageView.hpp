#pragma once
#include <glm>
#include <vector>
#include <runtime_assert>

namespace haste {

using std::vector;

struct subimage_view_t {
  subimage_view_t() = default;
  subimage_view_t(dvec4* data, size_t width, size_t height);

  dvec4* _data = nullptr;
  size_t _width = 0;
  size_t _height = 0;
  size_t _xOffset = 0;
  size_t _yOffset = 0;
  size_t _xWindow = 0;
  size_t _yWindow = 0;

  const size_t width() const { return _width; }
  const size_t height() const { return _height; }
  const size_t xOffset() const { return _xOffset; }
  const size_t yOffset() const { return _yOffset; }
  const size_t xWindow() const { return _xWindow; }
  const size_t yWindow() const { return _yWindow; }

  const size_t xBegin() const { return _xOffset; }
  const size_t xEnd() const { return _xOffset + _xWindow; }
  const size_t yBegin() const { return _yOffset; }
  const size_t yEnd() const { return _yOffset + _yWindow; }

  const bool inXWindow(size_t x) const { return xBegin() <= x && x < xEnd(); }
  const bool inYWindow(size_t y) const { return yBegin() <= y && y < yEnd(); }
  const bool inWindow(size_t x, size_t y) const { return inXWindow(x) && inYWindow(y); }

  dvec4& relAt(size_t x, size_t y);
  dvec4& absAt(size_t x, size_t y);
  const dvec4& relAt(size_t x, size_t y) const;
  const dvec4& absAt(size_t x, size_t y) const;
  const dvec4& last() const;
  dvec4* data() { return _data; }
  const dvec4* data() const { return _data; }
  void clear();

  void copyFrom(const std::vector<dvec4>& data, size_t width, size_t height);

  subimage_view_t crop(size_t x0, size_t y0, size_t x1, size_t y1)
  {
    subimage_view_t result = *this;
    
    runtime_assert(x0 < _xWindow);
    runtime_assert(x1 <= _xWindow);
    runtime_assert(y0 < _yWindow);
    runtime_assert(y1 <= _yWindow);
    runtime_assert(x0 <= x1);
    runtime_assert(y0 <= y1);

    result._xOffset += x0;
    result._xWindow = x1 - x0;
    result._yOffset += y0;
    result._yWindow = y1 - y0;
     
    return result;
  }
};

template <class T> struct image_view_t {
  image_view_t(
    T* data,
    size_t pitch,
    size_t width,
    size_t height)
    : data(data)
    , pitch(pitch)
    , width(width)
    , height(height)
  { }

  image_view_t(const subimage_view_t& view)
    : data(view._data + view._yOffset * view._width + view._xOffset)
    , pitch(view._width)
    , width(view._xWindow)
    , height(view._yWindow)
  { }

  image_view_t(
    const vector<T>& data,
    size_t width,
    size_t height)
    : data(data.data())
    , pitch(width)
    , width(width)
    , height(height)
  { }

  const T *const data;
  const size_t pitch;
  const size_t width;
  const size_t height;

  T at(size_t x, size_t y) const {
    return data[y * pitch + x];
  }
};

void rms_abs_errors(
  float& rms,
  float& abs,
  image_view_t<dvec4> a,
  image_view_t<vec3> b);

void rms_abs_errors_windowed(
  float& rms,
  float& abs,
  image_view_t<dvec4> a,
  image_view_t<vec3> b,
  const ivec2& center,
  int radius);

}
