#pragma once
#include <glm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cstdint>

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

            return std::_Hash_impl::hash(key);
        }
    };
}

namespace haste {

using std::map;
using std::unordered_map;
using std::unordered_set;
using std::make_pair;

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
                vec3 key = vec3(center.x, center.y + y, center.z + z);
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

    unordered_map<vec3, Range> _ranges;

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
                _points.push_back({ floor(data[i].position() * radius_inv), uint32_t(i) });
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

template <class T> class HashGrid3D {
public:
    HashGrid3D() { }

    HashGrid3D(vector<T>&& that, float radius) {
        build(that, radius);
    }

    HashGrid3D(const vector<T>& that, float radius) {
        build(that, radius);
    }

    template <class Callback> void rQuery(
        Callback callback,
        const vec3& query,
        const float radius) const
    {
        vec3 center = floor(query / _radius);

        const float radiusSq = radius * radius;

        for (float z = -1.f; z < 2.f; z += 1.f) {
            for (float y = -1.f; y < 2.f; y += 1.f) {
                vec3 key = vec3(center.x, center.y + y, center.z + z);
                auto cell = _ranges.find(key);

                if (cell != _ranges.end()) {
                    for (uint32_t i = cell->second.begin; i < cell->second.end; ++i) {
                        if (distance2(query, _points[i]) < radiusSq) {
                            callback(_data[i]);
                        }
                    }
                }
            }
        }
    }

    size_t rQuery(T* result, const vec3& query, const float radius) const {
        size_t itr = 0;

        auto callback = [&](const T& value) {
            result[itr] = value;
            ++itr;
        };

        rQuery(callback, query, radius);

        return itr;
    }

private:
    vector<T> _data;
    vector<vec3> _points;
    float _radius;

    struct Range {
        uint32_t begin;
        uint32_t end;
    };

    unordered_map<vec3, Range> _ranges;

    void build(const vector<T>& data, float radius) {
        struct Comparator {
            bool operator()(const vec3& a, const vec3& b) const {
                return a.z != b.z ? a.z < b.z : (a.y != b.y ? a.y < b.y : a.x < b.x);
            }
        };

        map<vec3, uint32_t, Comparator> counts;

        for (uint32_t i = 0; i < data.size(); ++i) {
            vec3 key = floor(data[i].position() / radius);
            ++counts[key];

            counts.insert(make_pair(key + vec3(1.f, 0.f, 0.f), uint32_t(0)));
            counts.insert(make_pair(key - vec3(1.f, 0.f, 0.f), uint32_t(0)));
        }

        uint32_t offset = 0;
        for (auto&& itr : counts) {
            _ranges[itr.first] = { offset, offset };
            offset += itr.second;
        }

        _data.resize(offset);
        _points.resize(offset);

        for (uint32_t i = 0; i < data.size(); ++i) {
            vec3 key = floor(data[i].position() / radius);

            auto cell = _ranges.find(key);

            _data[cell->second.end] = data[i];
            _points[cell->second.end] = data[i].position();

            ++cell->second.end;
        }


        for (auto&& itr : counts) {
            auto prev = counts.find(itr.first - vec3(1.f, 0.f, 0.f));
            auto next = counts.find(itr.first + vec3(1.f, 0.f, 0.f));

            if (prev != counts.end()) {
                _ranges[itr.first].begin -= prev->second;
            }

            if (next != counts.end()) {
                _ranges[itr.first].end += next->second;
            }
        }

        _radius = radius;
    }
};

namespace v2 {

template <class T> class HashGrid3D : private std::hash<vec3> {
public:
    HashGrid3D() { }

    HashGrid3D(vector<T>&& that, float radius) {
        build(that, radius);
    }

    HashGrid3D(const vector<T>& that, float radius) {
        build(that, radius);
    }

    template <class Callback> void rQuery(
        Callback callback,
        const vec3& query,
        const float radius) const
    {
        vec3 center = floor(query / _cellSize);
        vec3 offset = query / _cellSize - center;

        const float radiusSq = radius * radius;

        float zBegin = 0.f, zEnd = 2.f;

        if (offset.z < 0.5f) {
            zBegin -= 1.f;
            zEnd -= 1.f;
        }

        float yBegin = 0.f, yEnd = 2.f;

        if (offset.y < 0.5f) {
            yBegin -= 1.f;
            yEnd -= 1.f;
        }

        float xBegin = 0.f, xEnd = 2.f;

        if (offset.x < 0.5f) {
            xBegin -= 1.f;
            xEnd -= 1.f;
        }

        uint32_t hashes[9];
        hashes[8] = ~uint32_t(0);
        uint32_t numHashes = 0;

        for (float z = zBegin; z < zEnd; z += 1.f) {
            for (float y = yBegin; y < yEnd; y += 1.f) {
                for (float x = xBegin; x < xEnd; x += 1.f) {
                    hashes[numHashes++] = fhash(center + vec3(x, y, z));
                }
            }
        }

        std::sort(hashes, hashes + numHashes);

        for (uint32_t i = 0; i < 8; ++i) {
            if (hashes[i] != hashes[i + 1]) {
                auto hash = hashes[i];

                for (uint32_t i = _ranges[hash]; i < _ranges[hash + 1]; ++i) {
                    if (distance2(query, _points[i]) < radiusSq) {
                        callback(_data[i]);
                    }
                }
            }
        }
    }

    size_t rQuery(T* result, const vec3& query, const float radius) const {
        size_t itr = 0;

        auto callback = [&](const T& value) {
            result[itr] = value;
            ++itr;
        };

        rQuery(callback, query, radius);

        return itr;
    }

private:
    vector<T> _data;
    vector<vec3> _points;
    vector<uint32_t> _ranges;
    float _cellSize;
    uint32_t _mask;

    uint32_t fhash(const vec3& value) const {
        return uint32_t(this->operator()(floor(value))) & _mask;
    }

    void build(const vector<T>& data, float radius) {
        _cellSize = radius * 2.f;

        unordered_set<size_t> hashes;

        for (uint32_t i = 0; i < data.size(); ++i) {
            hashes.insert(this->operator()(floor(data[i].position() / _cellSize)));
        }

        uint32_t sizeExp = uint32_t(log2(double(hashes.size()) * 1.618));
        uint32_t size = 1u << sizeExp;

        hashes.clear();

        _mask = ~(~uint32_t(0) >> (sizeExp - 1) << (sizeExp - 1));
        _ranges.resize(size + 2, 0u);

        for (uint32_t i = 0; i < data.size(); ++i) {
            ++_ranges[fhash(data[i].position() / _cellSize) + 2];
        }

        uint32_t sum = 0;
        for (uint32_t i = 2; i < _ranges.size(); ++i) {
            _ranges[i - 1] = sum;
            sum += _ranges[i];
        }

        _data.resize(data.size());
        _points.resize(data.size());

        for (uint32_t i = 0; i < data.size(); ++i) {
            uint32_t hash = fhash(data[i].position() / _cellSize);
            uint32_t offset = _ranges[hash + 1];
            ++_ranges[hash + 1];

            _data[offset] = data[i];
            _points[offset] = data[i].position();
        }
    }
};

}

}
