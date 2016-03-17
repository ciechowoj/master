#include <functional>
#include <gtest/gtest.h>

using namespace std;

size_t lower_bound(const vector<int>& v, int x) {

    size_t a = 0;
    size_t b = v.size();

    while (a != b) {
        size_t h = a + (b - a) / 2;

        if (v[h] < x) {
            a = h + 1;
        }
        else /* v[h] >= x */ {
            b = h;
        }
    }

    return a;
}

TEST(bounds, upper) {
    ASSERT_EQ(0, lower_bound({}, 0));
    ASSERT_EQ(1, lower_bound({ 1 }, 2));
    ASSERT_EQ(0, lower_bound({ 2 }, 2));
    ASSERT_EQ(0, lower_bound({ 3 }, 2));
    ASSERT_EQ(1, lower_bound({ 0, 2 }, 2));
    ASSERT_EQ(0, lower_bound({ 2, 3 }, 2));
    ASSERT_EQ(0, lower_bound({ 3, 4 }, 2));
    ASSERT_EQ(0, lower_bound({ 0, 3, 4 }, 0));
    ASSERT_EQ(1, lower_bound({ 0, 3, 4 }, 1));
    ASSERT_EQ(1, lower_bound({ 0, 3, 4 }, 2));
    ASSERT_EQ(1, lower_bound({ 0, 3, 4 }, 3));
    ASSERT_EQ(2, lower_bound({ 0, 3, 4 }, 4));
    ASSERT_EQ(3, lower_bound({ 0, 3, 4 }, 5));
    ASSERT_EQ(0, lower_bound({ 0, 3, 3, 4 }, 0));
    ASSERT_EQ(1, lower_bound({ 0, 3, 3, 4 }, 1));
    ASSERT_EQ(1, lower_bound({ 0, 3, 3, 4 }, 2));
    ASSERT_EQ(1, lower_bound({ 0, 3, 3, 4 }, 3));
    ASSERT_EQ(3, lower_bound({ 0, 3, 3, 4 }, 4));
    ASSERT_EQ(4, lower_bound({ 0, 3, 3, 4 }, 5));
    ASSERT_EQ(0, lower_bound({ 0, 1, 1, 2, 4, 4, 4, 4 }, 0));
    ASSERT_EQ(1, lower_bound({ 0, 1, 1, 2, 4, 4, 4, 4 }, 1));
    ASSERT_EQ(3, lower_bound({ 0, 1, 1, 2, 4, 4, 4, 4 }, 2));
    ASSERT_EQ(4, lower_bound({ 0, 1, 1, 2, 4, 4, 4, 4 }, 3));
    ASSERT_EQ(4, lower_bound({ 0, 1, 1, 2, 4, 4, 4, 4 }, 4));
    ASSERT_EQ(8, lower_bound({ 0, 1, 1, 2, 4, 4, 4, 4 }, 5));

    ASSERT_EQ(0, lower_bound({ 2, 2, 2, 2 }, 2));
    ASSERT_EQ(7, lower_bound({ 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2 }, 2));
}
