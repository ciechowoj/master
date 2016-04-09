#pragma once
#include <glm>
#include <vector>
#include <algorithm>

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
        data = move(that);
        flags.resize(data.size());

        if (!data.empty()) {
            vec3 lower = position(data[0]), upper = position(data[0]);

            for (size_t i = 0; i < data.size(); ++i) {
                vec3 pos = position(data[i]);
                lower = min(lower, pos);
                upper = max(upper, pos);
            }

            vector<size_t> X(data.size());
            vector<size_t> Y(data.size());
            vector<size_t> Z(data.size());
            vector<size_t> unique(data.size());

            iota(X);
            iota(Y);
            iota(Z);
            iota(unique);

            sort<0>(X, unique, data);
            sort<1>(Y, unique, data);
            sort<2>(Z, unique, data);

            size_t* ranges[3] = {
                X.data(),
                Y.data(),
                Z.data()
            };

            build(0, data.size(), make_pair(lower, upper), ranges, unique.data());
        }
    }

    KDTree3D(const vector<T>& that)
        : KDTree3D(vector<T>(that)) { }

    vector<T> query_k(size_t k) const;

private:
    vector<T> data;
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
        size_t* unique) 
    {
        size_t size = end - begin;
        size_t adjoint = subranges[axis][median];

        if (adjoint != median) {
            for (size_t j = 0; j < 3; ++j) {
                for (size_t i = begin; i < end; ++i) {
                    if (subranges[j][i] == median) {
                        subranges[j][i] = adjoint;
                        break;
                    }
                }

                subranges[j][median] = median;
            }

            swap(data[median], data[adjoint]);
            swap(unique[median], unique[adjoint]);
        }

        auto less = [&](size_t a, size_t b) -> bool {
            return data[a][axis] == data[b][axis] 
                ? unique[a] < unique[b] 
                : data[a][axis] < data[b][axis];
        };

        for (size_t j = 0; j < 3; ++j) {
            size_t* subrange = subranges[j];
            size_t l = begin;
            size_t g = median + 1;

            while (g < end) {
                if (less(subrange[g], median)) {
                    while (less(subrange[l], median)) {
                        ++l;
                    }

                    swap(subrange[l], subrange[g]);
                }

                ++g;
            }
        }
    }

    void build(
        size_t begin, 
        size_t end,
        const pair<vec3, vec3>& aabb,
        size_t* subranges[3],
        size_t* unique)
    {
        size_t size = end - begin;

        if (size > 1) {
            size_t axis = max_axis(aabb);
            size_t median = begin + size / 2;
            rearrange(axis, begin, end, median, subranges, unique);
            flags.set(median, axis);

            pair<vec3, vec3> left_aabb = aabb, right_aabb = aabb;
            left_aabb.second[axis] = data[median][axis];
            right_aabb.first[axis] = data[median][axis];

            build(begin, median, left_aabb, subranges, unique);
            build(median + 1, end, left_aabb, subranges, unique);
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
