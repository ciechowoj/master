#pragma once
#include <glm>
#include <unordered_map>
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
            size_t x = std::hash<float>()(key.x);
            size_t y = std::hash<float>()(key.y);
            size_t z = std::hash<float>()(key.z);

            return x ^ (y << 1) ^ (z << 2);
        }
    };
}

namespace haste {

using std::unordered_map;

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
                for (float x = -1.f; x < 2.f; x += 1.f) {
                    vec3 key = center + vec3(x, y, z);
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

    void aabb(vec3& first, vec3& second, const vector<T>& data) {
        first = data[0].position();
        second = data[0].position();

        for (uint32_t i = 1; i < data.size(); ++i) {
            first = min(first, data[i].position());
            second = max(second, data[i].position());
        }
    }

    void build(const vector<T>& data, float radius) {
        unordered_map<vec3, uint32_t> counts;

        for (uint32_t i = 0; i < data.size(); ++i) {
            vec3 key = floor(data[i].position() / radius);
            ++counts[key];
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

        _radius = radius;
    }
};

}
