#pragma once
#include <glm>

namespace haste {

struct ImageView {
    ImageView() = default;
    ImageView(vec4* data, size_t width, size_t height);

    vec4* _data = nullptr;
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

    vec4& relAt(size_t x, size_t y);
    vec4& absAt(size_t x, size_t y);
    const vec4& relAt(size_t x, size_t y) const;
    const vec4& absAt(size_t x, size_t y) const;
    const vec4& last() const;
    vec4* data() { return _data; }
    void clear();
};

}
