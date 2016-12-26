#pragma once
#include <glm>
#include <vector>

namespace haste {

struct ImageView {
    ImageView() = default;
    ImageView(dvec4* data, size_t width, size_t height);

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
};

}
