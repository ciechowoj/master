#pragma once
#include <glm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "flat_hash_map.hpp"

namespace std
{
    template<> struct hash<glm::vec3>
    {
        typedef glm::vec3 argument_type;
        typedef std::size_t result_type;

        result_type operator()(const argument_type& key) const
        {
            // size_t x = std::hash<float>()(key.x);
            // size_t y = std::hash<float>()(key.y);
            // size_t z = std::hash<float>()(key.z);
            // return x ^ (y << 1) ^ (z << 2);

            #if defined _MSC_VER
            return std::_Hash_seq((const unsigned char*)(&key), sizeof(key));
            #else
            return std::_Hash_impl::hash(key);
            #endif
        }
    };
}

namespace haste {

using std::map;
using std::vector;
using std::unordered_map;
using std::unordered_set;
using std::make_pair;

namespace v2 {

template <class T> class HashGrid3D {
public:
    HashGrid3D() { }

    HashGrid3D(const vector<T>* that, float radius)
        : _data(that) {
        build(*that, radius);
    }

    template <class Callback> void rQuery(
        Callback callback,
        const vec3& query,
        const float radius) const
    {
        vec3 center = floor(query * _radius_inv);

        const float radiusSq = radius * radius;

        for (float z = -1.f; z < 2.f; z += 1.f) {
            for (float y = -1.f; y < 2.f; y += 1.f) {
                for (float x = -1.f; x < 2.f; x += 1.f) {

                    vec3 key = vec3(center.x + x, center.y + y, center.z + z);
                    auto cell = _ranges.find(key);

                    if (cell != _ranges.end()) {
                        for (uint32_t i = cell->second.begin; i < cell->second.end; ++i) {
                            if (distance2(query, _points[i].cell) < radiusSq) {
                                callback(_points[i].index);
                            }
                        }
                    }
                }
            }
        }
    }

    size_t rQuery(T* result, const vec3& query, const float radius) const {
        size_t itr = 0;

        auto callback = [&](uint32_t index) {
            result[itr] = _data->operator[](index);
            ++itr;
        };

        rQuery(callback, query, radius);

        return itr;
    }

private:
    struct Point {
        vec3 cell;
        uint32_t index;
    };

    const vector<T>* _data = nullptr;
    vector<Point> _points;
    float _radius;
    float _radius_inv;

    struct Range {
        uint32_t begin;
        uint32_t end;
    };

    std::unordered_map<vec3, Range> _ranges;

    void build(const vector<T>& data, float radius) {
        struct Comparator {
            bool operator()(const vec3& a, const vec3& b) const {
                return a.z != b.z ? a.z < b.z : (a.y != b.y ? a.y < b.y : a.x < b.x);
            }
        };

        if (data.empty()) {
            return;
        }

        const float radius_inv = 1.0f / radius;

        _points.reserve(data.size());

        for (size_t i = 0; i < data.size(); ++i) {
            if (!data[i].is_light()) {
                _points.push_back({ floor(data[i].position() * radius_inv) + vec3(0.f, 0.f, 0.f), uint32_t(i) });
            }
        }

        sort(_points.begin(), _points.end(), [](const Point& a, const Point& b) {
            const Comparator comparator;
            return comparator(a.cell, b.cell);
        });

        vector<uint32_t> cells(1, 0);

        for (uint32_t i = 1; i < _points.size(); ++i) {
            if (_points[i - 1].cell.y != _points[i].cell.y ||
                _points[i - 1].cell.z != _points[i].cell.z) {
                cells.push_back(i);
            }
            else if(_points[i - 1]. cell.x != _points[i].cell.x) {
                cells.push_back(i);
            }
        }

        cells.push_back(_points.size());

        for (uint32_t j = 1; j < cells.size(); ++j) {
            _ranges[_points[cells[j - 1]].cell] = { cells[j - 1], cells[j] };
        }


        for (uint32_t i = 0; i < _points.size(); ++i) {
            _points[i].cell = _data->operator[](_points[i].index).position();
        }

        _radius = radius;
        _radius_inv = radius_inv;
    }
};

}


namespace v3 {

template <class T> class HashGrid3D {
public:
    HashGrid3D() { }

    HashGrid3D(const vector<T>* that, float radius)
        : _data(that) {
        build(*that, radius);
    }

    template <class Callback> void rQuery(
        Callback callback,
        const vec3& query,
        const float radius) const
    {
        vec3 center = floor(query * _radius_inv);

        const float radiusSq = radius * radius;

        for (float z = -1.f; z < 2.f; z += 1.f) {
            for (float y = -1.f; y < 2.f; y += 1.f) {
                vec3 key = vec3(center.x + 0.f, center.y + y, center.z + z);
                auto cell = _ranges.find(key);

                if (cell != _ranges.end()) {
                    for (uint32_t i = cell->second.begin; i < cell->second.end; ++i) {
                        if (distance2(query, _points[i].cell) < radiusSq) {
                            callback(_points[i].index);
                        }
                    }
                }
            }
        }
    }

    size_t rQuery(T* result, const vec3& query, const float radius) const {
        size_t itr = 0;

        auto callback = [&](uint32_t index) {
            result[itr] = _data->operator[](index);
            ++itr;
        };

        rQuery(callback, query, radius);

        return itr;
    }

private:
    struct Point {
        vec3 cell;
        uint32_t index;
    };

    const vector<T>* _data = nullptr;
    vector<Point> _points;
    float _radius;
    float _radius_inv;

    struct Range {
        uint32_t begin;
        uint32_t end;
    };

    ska::flat_hash_map<vec3, Range> _ranges;

    void build(const vector<T>& data, float radius) {
        struct Comparator {
            bool operator()(const vec3& a, const vec3& b) const {
                return a.z != b.z ? a.z < b.z : (a.y != b.y ? a.y < b.y : a.x < b.x);
            }
        };

        if (data.empty()) {
            return;
        }

        const float radius_inv = 1.0f / radius;

        _points.reserve(data.size());

        for (size_t i = 0; i < data.size(); ++i) {
            if (!data[i].is_light()) {
                _points.push_back({ floor(data[i].position() * radius_inv) + vec3(0.f, 0.f, 0.f), uint32_t(i) });
            }
        }

        sort(_points.begin(), _points.end(), [](const Point& a, const Point& b) {
            const Comparator comparator;
            return comparator(a.cell, b.cell);
        });


        vector<uint32_t> slabs(1, 0);
        vector<uint32_t> cells(1, 0);

        for (uint32_t i = 1; i < _points.size(); ++i) {
            if (_points[i - 1].cell.y != _points[i].cell.y ||
                _points[i - 1].cell.z != _points[i].cell.z) {
                slabs.push_back(cells.size());
                cells.push_back(i);
            }
            else if(_points[i - 1]. cell.x != _points[i].cell.x) {
                cells.push_back(i);
            }
        }

        slabs.push_back(cells.size());
        cells.push_back(_points.size());


        for (uint32_t j = 1; j < slabs.size(); ++j) {
            uint32_t slab_begin = slabs[j - 1];
            uint32_t slab_end = slabs[j];

            vec3 first = _points[cells[slab_begin]].cell;

            _ranges[first - vec3(1, 0, 0)] = { cells[slab_begin], cells[slab_begin + 1] };
            _ranges[first] = { cells[slab_begin], cells[slab_begin + 1] };

            for (uint32_t i = slab_begin + 1; i < slab_end; ++i) {
                vec3 prev = _points[cells[i - 1]].cell;
                vec3 curr = _points[cells[i]].cell;
                float diff = curr.x - prev.x;

                if (diff == 1.0f) {
                    // adjacent
                    _ranges[prev].end = cells[i + 1];
                    _ranges[curr] = { cells[i - 1], cells[i + 1] };
                }
                else if (diff == 2.0f) {
                    // single space
                    _ranges[curr - vec3(1, 0, 0)] = { cells[i - 1], cells[i + 1] };
                    _ranges[curr] = { cells[i], cells[i + 1] };
                }
                else {
                    _ranges[prev + vec3(1, 0, 0)] = { cells[i - 1], cells[i] };
                    _ranges[curr - vec3(1, 0, 0)] = { cells[i], cells[i + 1] };
                    _ranges[curr] = { cells[i], cells[i + 1] };
                }
            }

            vec3 last = _points[cells[slab_end - 1]].cell;
            _ranges[last + vec3(1, 0, 0)] = { cells[slab_end - 1], cells[slab_end] };
        }


        for (uint32_t i = 0; i < _points.size(); ++i) {
            _points[i].cell = _data->operator[](_points[i].index).position();
        }

        _radius = radius;
        _radius_inv = radius_inv;
    }
};

}

}
