#pragma once
#include <glm>
#include <vector>
#include <algorithm>

namespace haste {

using std::vector;
using std::move;
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

            iota(X);
            iota(Y);
            iota(Z);

            sort<0>(X, data);
            sort<1>(Y, data);
            sort<2>(Z, data);

            pair<size_t*, size_t*> ranges[3] = {
                make_pair(X.data(), X.data() + X.size()),
                make_pair(Y.data(), Y.data() + Y.size()),
                make_pair(Z.data(), Z.data() + Z.size())
            };

            build(
                make_pair(data.data(), data.data() + data.size()),
                ranges,
                make_pair(lower, upper));
        }
    }

    KDTree3D(const vector<T>& that)
        : KDTree3D(vector<T>(that)) { }

    vector<T> query_k(size_t k) const;

private:
    vector<T> data;
    BitfieldVector<2> flags;

    static void iota(vector<size_t>& a) {
        for (size_t i = 0; i < a.size(); ++i) {
            a[i] = i;
        }
    }

    template <size_t D> static void sort(
        vector<size_t>& v,
        const vector<T>& data) {
        std::sort(v.begin(), v.end(), [&](size_t a, size_t b) -> bool {
            return data[a][D] < data[b][D];
        });
    }

    void build(
        const pair<T*, T*>& subtree,
        const pair<size_t*, size_t*> subranges[3],
        const pair<vec3, vec3>& aabb) {


        size_t axis = max_axis(aabb);






    }

    static vec3 position(const T& x) {
        return vec3(x[0], x[1], x[2]);
    }
};







}
