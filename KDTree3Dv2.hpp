#pragma once
#include <glm>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <KDTree3D.hpp>

namespace haste {

namespace v2 {

using std::vector;
using std::move;
using std::swap;
using std::pair;
using std::make_pair;
using std::uint32_t;

inline size_t max_axis(const pair<vec3, vec3>& aabb) {
    vec3 diff = abs(aabb.first - aabb.second);

    if (diff.x < diff.y) {
        return diff.y < diff.z ? 2 : 1;
    }
    else {
        return diff.x < diff.z ? 2 : 0;
    }
}

template <class T> class KDTree3D {
private:
    struct QueryKState {
        T* heap;
        size_t capacity;
        size_t size;
        float limit;
        vec3 query;
    };

    struct Point {
        vec3 position;

        float operator[](size_t index) const {
            return position[index];
        }
    };

    static const size_t leaf = 3;

public:
    KDTree3D() { }

    KDTree3D(vector<T>&& that) {
        _data = move(that);
        _points.resize(_data.size());
        _axes.resize(_data.size());

        if (!_points.empty()) {
            vec3 lower = _points[0], upper = _points[0];

            for (size_t i = 0; i < _data.size(); ++i) {
                _points[i] = _data[i].position();
                lower = min(lower, _data[i].position());
                upper = max(upper, _data[i].position());
            }


            vector<size_t> X(_points.size());
            vector<size_t> Y(_points.size());
            vector<size_t> Z(_points.size());
            vector<size_t> unique(_points.size());
            vector<size_t> scratch(_points.size());

            iota(X);
            iota(Y);
            iota(Z);
            iota(unique);

            sort<0>(X, unique, _points);
            sort<1>(Y, unique, _points);
            sort<2>(Z, unique, _points);

            size_t* ranges[3] = {
                X.data(),
                Y.data(),
                Z.data()
            };

            build(
                0,
                _points.size(),
                make_pair(lower, upper),
                ranges,
                unique.data(),
                scratch.data());

            for (size_t i = 0; i < _points.size(); ++i) {
                size_t j = i;
                size_t k = X[j];

                while (k != X[k]) {
                    std::swap(_points[j], _points[X[j]]);
                    swap(_data[j], _data[X[j]]);

                    // _flags.swap(j, X[j]);
                    X[j] = j;
                    j = k;
                    k = X[k];
                }
            }
        }
    }

    KDTree3D(const vector<T>& that)
        : KDTree3D(vector<T>(that)) { }

    size_t query_k(T* dst, const vec3& q, size_t k, float d) const {
        QueryKState state;
        state.heap = dst;
        state.capacity = k;
        state.size = 0;
        state.limit = d * d;
        state.query = q;

        query_k(state, 0, _data.size());

        return state.size;
    }

    template <class Callback> void rQuery(
        Callback callback,
        const vec3& query,
        const float radius) const
    {
        // As the tree is balanced, assuming no more than 2^32 elements
        // the stack will be no bigger than 32.

        struct Stack {
            size_t begin;
            size_t end;
        };

        Stack stack[32];
        stack[0].begin = 0;
        stack[0].end = _points.size();
        size_t stackSize = 1;

        const float radiusSq = radius * radius;

        while (stackSize) {
            size_t begin = stack[stackSize - 1].begin;
            size_t end = stack[stackSize - 1].end;

            if (end == begin) {
                --stackSize;
                continue;
            }

            size_t median = begin + (end - begin) / 2;
            size_t axis = _axes.get(median);

            vec3 point = _points[median];
            float distanceSq = distance2(query, point);

            if (distanceSq < radiusSq) {
                callback(_data[median]);

                if (axis != leaf) {
                    ++stackSize;
                    stack[stackSize - 2].begin = begin;
                    stack[stackSize - 2].end = median;
                    stack[stackSize - 1].begin = median + 1;
                    stack[stackSize - 1].end = end;
                }
                else {
                    --stackSize;
                }
            }
            else if (axis != leaf) {
                float axisDistance = query[axis] - point[axis];
                float axisDistanceSq = axisDistance * axisDistance;

                if (axisDistance < 0.0) {
                    stack[stackSize - 1].begin = begin;
                    stack[stackSize - 1].end = median;

                    if (axisDistanceSq < radiusSq) {
                        stack[stackSize].begin = median + 1;
                        stack[stackSize].end = end;
                        ++stackSize;
                    }
                }
                else {
                    stack[stackSize - 1].begin = median + 1;
                    stack[stackSize - 1].end = end;

                    if (axisDistanceSq < radiusSq) {
                        stack[stackSize].begin = begin;
                        stack[stackSize].end = median;
                        ++stackSize;
                    }
                }
            }
            else {
                --stackSize;
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

    const T* data() const {
        return _data.data();
    }

    size_t size() const {
        return _data.size();
    }

private:
    vector<T> _data;
    vector<vec3> _points;
    BitfieldVector<2> _axes;

    size_t query_k(
        QueryKState& state,
        size_t begin,
        size_t end) const
    {
        if (end != begin) {
            size_t median = begin + (end - begin) / 2;
            size_t axis = _axes.get(median);
            vec3 point = position(_data[median]);
            float query_dist = distance2(point, state.query);

            auto less = [&](const T& a, const T& b) -> bool {
                return distance2(position(a), state.query) < distance2(position(b), state.query);
            };

            if (query_dist < state.limit) {
                if (state.size < state.capacity) {
                    state.heap[state.size] = _data[median];
                    ++state.size;
                    std::push_heap(state.heap, state.heap + state.size, less);

                    if (state.size == state.capacity) {
                        state.limit = min(
                            state.limit,
                            distance2(position(state.heap[0]), state.query));
                    }
                }
                else {
                    std::pop_heap(state.heap, state.heap + state.size, less);
                    state.heap[state.size - 1] = _data[median];
                    std::push_heap(state.heap, state.heap + state.size, less);
                    state.limit = min(
                        state.limit,
                        distance2(position(state.heap[0]), state.query));
                }
            }

            if (axis != 3) {
                float split_dist = state.query[axis] - _data[median][axis];

                if (split_dist < 0.0) {
                    query_k(state, begin, median);

                    if (split_dist * split_dist < state.limit) {
                        query_k(state, median + 1, end);
                    }
                }
                else {
                    query_k(state, median + 1, end);

                    if (split_dist * split_dist < state.limit) {
                        query_k(state, begin, median);
                    }
                }
            }
        }
    }

    static void iota(vector<size_t>& a) {
        for (size_t i = 0; i < a.size(); ++i) {
            a[i] = i;
        }
    }

    template <size_t D> static void sort(
        vector<size_t>& v,
        const vector<size_t>& unique,
        const vector<vec3>& data) {
        std::sort(v.begin(), v.end(), [&](size_t a, size_t b) -> bool {
            return data[a][D] == data[b][D] ? unique[a] < unique[b] : data[a][D] < data[b][D];
        });
    }

    void rearrange(
        size_t axis,
        size_t begin,
        size_t end,
        size_t median,
        size_t* subranges[3],
        size_t* unique,
        size_t* scratch)
    {
        size_t median_index = subranges[axis][median];

        for (size_t j = 0; j < 3; ++j) {
            if (axis != j) {
                size_t* subrange = subranges[j];
                size_t itr = begin;

                while (subrange[itr] != median_index) {
                    ++itr;
                }

                while (itr < median) {
                    swap(subrange[itr], subrange[itr + 1]);
                    ++itr;
                }

                while (median < itr) {
                    swap(subrange[itr - 1], subrange[itr]);
                    --itr;
                }
            }
        }

        auto less = [&](size_t a, size_t b) -> bool {
            return _points[a][axis] == _points[b][axis]
                ? unique[a] < unique[b]
                : _points[a][axis] < _points[b][axis];
        };

        for (size_t j = 0; j < 3; ++j) {
            size_t* subrange = subranges[j];

            for (size_t i = begin; i < end; ++i) {
                scratch[i] = subrange[i];
            }

            size_t lst_dst = begin;
            size_t geq_dst = median + 1;

            size_t lst_src = begin;
            size_t geq_src = median + 1;

            while (lst_src < median) {
                if (less(scratch[lst_src], median_index)) {
                    subrange[lst_dst] = scratch[lst_src];
                    ++lst_dst;
                }
                else {
                    subrange[geq_dst] = scratch[lst_src];
                    ++geq_dst;
                }

                ++lst_src;
            }

            while (geq_src < end) {
                if (less(scratch[geq_src], median_index)) {
                    subrange[lst_dst] = scratch[geq_src];
                    ++lst_dst;
                }
                else {
                    subrange[geq_dst] = scratch[geq_src];
                    ++geq_dst;
                }

                ++geq_src;
            }
        }
    }

    void build(
        size_t begin,
        size_t end,
        const pair<vec3, vec3>& aabb,
        size_t* subranges[3],
        size_t* unique,
        size_t* scratch)
    {
        size_t size = end - begin;

        if (size > 1) {
            size_t axis = max_axis(aabb);
            size_t median = begin + size / 2;

            rearrange(axis, begin, end, median, subranges, unique, scratch);
            _axes.set(median, axis);

            pair<vec3, vec3> left_aabb = aabb, right_aabb = aabb;
            left_aabb.second[axis] = _points[median][axis];
            right_aabb.first[axis] = _points[median][axis];

            build(begin, median, left_aabb, subranges, unique, scratch);
            build(median + 1, end, right_aabb, subranges, unique, scratch);
        }
        else if (size == 1) {
            _axes.set(begin, leaf);
        }
    }

    static vec3 position(const T& x) {
        return vec3(x[0], x[1], x[2]);
    }
};

}

}
