#pragma once
#include <glm>
#include <vector>
#include <algorithm>
#include <iostream>

namespace haste {

using std::vector;
using std::move;
using std::swap;
using std::pair;
using std::make_pair;

template <size_t N> class BitfieldVector {
private:
    static const size_t flags_per_item = (sizeof(size_t) * 8) / N;
    static const size_t mask = ~(~size_t(0) >> N << N);
public:
    BitfieldVector() { }

    BitfieldVector(size_t size) {
        resize(size);
    }

    void resize(size_t size) {
        data.resize((size + flags_per_item - 1) / flags_per_item);
    }

    void set(size_t index, size_t value) {
        size_t item = index / flags_per_item;
        size_t shift = index % flags_per_item * N;
        data[item] = (data[item] & ~(mask << shift)) | (value << shift);
    }

    size_t get(size_t index) const {
        size_t item = index / flags_per_item;
        size_t shift = index % flags_per_item * N;
        return (data[item] >> shift) & mask;
    }

    void swap(size_t a, size_t b) {
        size_t t = get(a);
        set(a, get(b));
        set(b, t);
    }

private:
    vector<size_t> data;
};

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
public:
    KDTree3D() {

    }

    KDTree3D(vector<T>&& that) {
        _data = move(that);
        flags.resize(_data.size());

        if (!_data.empty()) {
            vec3 lower = position(_data[0]), upper = position(_data[0]);

            for (size_t i = 0; i < _data.size(); ++i) {
                vec3 pos = position(_data[i]);
                lower = min(lower, pos);
                upper = max(upper, pos);
            }

            vector<size_t> X(_data.size());
            vector<size_t> Y(_data.size());
            vector<size_t> Z(_data.size());
            vector<size_t> unique(_data.size());
            vector<size_t> scratch(_data.size());

            iota(X);
            iota(Y);
            iota(Z);
            iota(unique);

            sort<0>(X, unique, _data);
            sort<1>(Y, unique, _data);
            sort<2>(Z, unique, _data);

            size_t* ranges[3] = {
                X.data(),
                Y.data(),
                Z.data()
            };

            build(
                0, 
                _data.size(), 
                make_pair(lower, upper), 
                ranges, 
                unique.data(),
                scratch.data());

            for (size_t i = 0; i < _data.size(); ++i) {
                size_t j = i;
                size_t k = X[j];

                while (k != X[k]) {
                    swap(_data[j], _data[X[j]]);
                    // flags.swap(j, X[j]);
                    X[j] = j;
                    j = k;
                    k = X[k];
                }
            }
        }
    }

    KDTree3D(const vector<T>& that)
        : KDTree3D(vector<T>(that)) { }

    vector<T> query_k(size_t k) const;

    const T* data() const {
        return _data.data();
    }

    size_t size() const {
        return _data.size();
    }

    size_t axis() const {
        return flags.get(size() / 2);
    }

    KDTree3D copy_left() const {
        KDTree3D result;
        size_t median = size() / 2;
        result._data.resize(median);
        result.flags.resize(median);

        for (size_t i = 0; i < median; ++i) {
            result._data[i] = _data[i];
            result.flags.set(i, flags.get(i));
        }

        return result;
    }

    KDTree3D copy_right() const {
        KDTree3D result;
        size_t median = size() / 2;
        size_t rsize = size() - median - 1;
        result._data.resize(rsize);
        result.flags.resize(rsize);

        for (size_t i = 0; i < rsize; ++i) {
            result._data[i] = _data[i + median + 1];
            result.flags.set(i, flags.get(i + median + 1));
        }

        return result;
    }

private:
    vector<T> _data;
    BitfieldVector<2> flags;

    static const size_t leaf = 3;

    static void iota(vector<size_t>& a) {
        for (size_t i = 0; i < a.size(); ++i) {
            a[i] = i;
        }
    }

    template <size_t D> static void sort(
        vector<size_t>& v,
        const vector<size_t>& unique,
        const vector<T>& data) {
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
            return _data[a][axis] == _data[b][axis] 
                ? unique[a] < unique[b] 
                : _data[a][axis] < _data[b][axis];
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
            flags.set(median, axis);

            pair<vec3, vec3> left_aabb = aabb, right_aabb = aabb;
            left_aabb.second[axis] = _data[median][axis];
            right_aabb.first[axis] = _data[median][axis];

            build(begin, median, left_aabb, subranges, unique, scratch);
            build(median + 1, end, right_aabb, subranges, unique, scratch);
        }
        else if (size == 1) {
            flags.set(begin, leaf);
        }
    }

    static vec3 position(const T& x) {
        return vec3(x[0], x[1], x[2]);
    }
};







}
