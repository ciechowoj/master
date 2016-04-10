#include <gtest>
#include <KDTree3D.hpp>
#include <iostream>

using namespace glm;
using namespace haste;

TEST(BitfieldVector, basic_tests) {
    BitfieldVector<2> a;
    BitfieldVector<2> b(4);

    b.set(0, 2);
    b.set(1, 1);

    EXPECT_EQ(2, b.get(0));
    EXPECT_EQ(1, b.get(1));
    EXPECT_EQ(0, b.get(2));

    b.swap(0, 1);

    EXPECT_EQ(1, b.get(0));
    EXPECT_EQ(2, b.get(1));

    BitfieldVector<3> c(100);
    c.set(99, 5);

    EXPECT_EQ(0, c.get(50));
    EXPECT_EQ(0, c.get(98));
    EXPECT_EQ(5, c.get(99));
}

TEST(KDTree3D, max_axis) {
    EXPECT_EQ(0, max_axis(make_pair(vec3(1, 2, 3), vec3(-4, 0, 0))));
    EXPECT_EQ(1, max_axis(make_pair(vec3(1, -20, 3), vec3(-4, 0, 0))));
    EXPECT_EQ(2, max_axis(make_pair(vec3(1, 2, 30), vec3(-4, 0, 0))));
}

TEST(KDTree3D, should_create_empty) {
    KDTree3D<vec3> a;
}

TEST(KDTree3D, should_create_singleton) {
    KDTree3D<vec3> a({ vec3(0.0f) });
    EXPECT_EQ(3, a.axis());
}

TEST(KDTree3D, should_create_from_bunch_of_points) {
    vector<vec3> points = {
        vec3(5.0f, 3.5f, -1.0f),
        vec3(2.0f, 1.0f, 1.0f),
        vec3(3.5f, 5.0f, 0.0f),
        vec3(1.5f, 4.0f, 2.0f),
        vec3(2.5f, 2.5f, 0.0f),
        vec3(5.0f, 2.0f, 0.0f),
        vec3(2.5f, 3.5f, 0.0f),
    };

    KDTree3D<vec3> a(points);

    EXPECT_EQ(1, a.axis());

    auto bottom = a.copy_left();
    auto top = a.copy_right();

    EXPECT_EQ(3, bottom.size());
    EXPECT_EQ(0, bottom.axis());
    EXPECT_EQ(3, top.size());
    EXPECT_EQ(0, top.axis());

    auto left = top.copy_left();
    auto right = top.copy_right();

    EXPECT_EQ(1, left.size());
    EXPECT_EQ(3, left.axis());
    EXPECT_EQ(1, right.size());
    EXPECT_EQ(3, right.axis());
}

TEST(KDTree3D, slightly_unbalanced) {
    vector<vec3> points = {
        vec3(5.0f, 3.5f, -1.0f),
        vec3(2.0f, 1.0f, 1.0f),
    };

    KDTree3D<vec3> a(points);

    EXPECT_EQ(2, a.size());
    EXPECT_EQ(0, a.axis());

    EXPECT_EQ(1, a.copy_left().size());
    EXPECT_EQ(0, a.copy_right().size());
}

TEST(KDTree3D, test_y_split) {
    vector<vec3> points = {
        vec3(5.0f, 3.5f, -1.0f),
        vec3(2.0f, -3.5f, 1.0f),
    };

    KDTree3D<vec3> a(points);

    EXPECT_EQ(1, a.axis());
}

TEST(KDTree3D, test_z_split) {
    vector<vec3> points = {
        vec3(5.0f, 3.5f, -10.0f),
        vec3(2.0f, -3.5f, 1.0f),
    };

    KDTree3D<vec3> a(points);

    EXPECT_EQ(2, a.axis());
}

TEST(KDTree3D, test_string_alike) {
    vector<vec3> points = {
        vec3(0, 1, 0),
        vec3(0, 2, 0),
        vec3(0, 3, 0),
        vec3(0, 4, 0),
        vec3(0, 5, 0),
        vec3(0, 6, 0),
        vec3(0, 7, 0),
        vec3(0, 8, 0),
    };

    KDTree3D<vec3> a(points);

    EXPECT_EQ(1, a.axis());

    auto b = a.copy_left();
    auto c = a.copy_right();

    EXPECT_EQ(1, b.axis());
    EXPECT_EQ(4, b.size());
    EXPECT_EQ(1, c.axis());
    EXPECT_EQ(3, c.size());

    EXPECT_TRUE(vec3(0, 6, 0) == c.data()[0]);
    EXPECT_TRUE(vec3(0, 7, 0) == c.data()[1]);
    EXPECT_TRUE(vec3(0, 8, 0) == c.data()[2]);
}

TEST(KDTree3D, test_same_points) {
    vector<vec3> points = {
        vec3(0, 0, 0),
        vec3(0, 0, 0),
        vec3(0, 0, 0),
        vec3(0, 0, 0),
        vec3(0, 0, 0),
        vec3(0, 0, 0),
        vec3(0, 0, 0),
    };

    KDTree3D<vec3> a(points);

    EXPECT_EQ(7, a.size());
    EXPECT_NE(3, a.axis());

    auto b = a.copy_left();
    auto c = a.copy_right();

    EXPECT_EQ(3, b.size());
    EXPECT_EQ(3, c.size());

    EXPECT_TRUE(vec3(0, 0, 0) == c.data()[0]);
    EXPECT_TRUE(vec3(0, 0, 0) == c.data()[1]);
    EXPECT_TRUE(vec3(0, 0, 0) == c.data()[2]);
}

template <class T> ::testing::AssertionResult AssertKDTreeQueryK(
    const char* expected_expr,
    const char* kdtree_expr,
    const char* query_expr,
    const char* k_expr,
    const char* q_expr,
    const vector<T>& expected,
    const KDTree3D<T>& kdtree,
    const vec3& query,
    size_t k,
    float d) {

    vector<T> actual(k);

    size_t n = kdtree.query_k(actual.data(), query, k, d);

    actual.resize(n);

    bool same = true;

    if (n != expected.size()) {
        same = false;
    }
    else {
        auto less = [](const vec3& a, const vec3& b) -> bool {

            return a.x == b.x
                ? a.y == b.y
                    ? (a.z == b.z ? false : a.z < b.z)
                    : a.y < b.y
                : a.x < b.x;



        };

        auto expected_copy = expected;
        std::sort(actual.begin(), actual.end(), less);
        std::sort(expected_copy.begin(), expected_copy.end(), less);

        for (size_t i = 0; i < expected_copy.size(); ++i) {
            if (expected_copy[i] != actual[i]) {
                same = false;
                break;
            }
        }
    }

    if (same) {
        return ::testing::AssertionSuccess();
    }
    else {
        auto result = ::testing::AssertionFailure();
        result
            << "Expected "
            << ::testing::PrintToString(expected)
            << "\nActual "
            << ::testing::PrintToString(actual);

        return result;
    }
}

#define EXPECT_QUERY_K(expected, kdtree, query, k, d) \
    EXPECT_PRED_FORMAT5(AssertKDTreeQueryK, expected, kdtree, query, k, d)

TEST(KDTree3D, test_query_k_single_point) {
    vector<vec3> points = {
        vec3(5.0f, 3.5f, -1.0f),
        vec3(2.0f, 1.0f, 1.0f),
        vec3(3.5f, 5.0f, 0.0f),
        vec3(1.5f, 4.0f, 2.0f),
        vec3(2.5f, 2.5f, 0.0f),
        vec3(5.0f, 2.0f, 0.0f),
        vec3(2.5f, 3.5f, 0.0f),
    };

    KDTree3D<vec3> tree(points);

    EXPECT_QUERY_K({ vec3(5.0f, 3.5f, -1.0f) }, tree, vec3(5.0f, 3.5f, -1.0f), 1, 1);
    EXPECT_QUERY_K({ vec3(2.0f, 1.0f, 1.0f) }, tree, vec3(2.0f, 1.0f, 1.0f), 1, 1);
    EXPECT_QUERY_K({ vec3(3.5f, 5.0f, 0.0f) }, tree, vec3(3.5f, 5.0f, 0.0f), 1, 1);
    EXPECT_QUERY_K({ vec3(1.5f, 4.0f, 2.0f) }, tree, vec3(1.5f, 4.0f, 2.0f), 1, 1);
    EXPECT_QUERY_K({ vec3(2.5f, 2.5f, 0.0f) }, tree, vec3(2.5f, 2.5f, 0.0f), 1, 1);
    EXPECT_QUERY_K({ vec3(5.0f, 2.0f, 0.0f) }, tree, vec3(5.0f, 2.0f, 0.0f), 1, 1);
    EXPECT_QUERY_K({ vec3(2.5f, 3.5f, 0.0f) }, tree, vec3(2.5f, 3.5f, 0.0f), 1, 1);
}

TEST(KDTree3D, test_query_k_multiple_points) {
    vector<vec3> points = {
        vec3(5.0f, 3.5f, -1.0f),
        vec3(2.0f, 1.0f, 1.0f),
        vec3(3.5f, 5.0f, 0.0f),
        vec3(1.5f, 4.0f, 2.0f),
        vec3(2.5f, 2.5f, 0.0f),
        vec3(5.0f, 2.0f, 0.0f),
        vec3(2.5f, 3.5f, 0.0f),
    };

    KDTree3D<vec3> tree(points);

    auto query = vec3(5.0f, 3.5f, -1.0f);

    EXPECT_QUERY_K(
        { vec3(5.0f, 3.5f, -1.0f) },
        tree, query, 1, 100);

    EXPECT_QUERY_K(
        (vector<vec3>{ vec3(5, 2, 0), vec3(5.0f, 3.5f, -1.0f) }),
        tree, query, 2, 100);

    EXPECT_QUERY_K(
        (vector<vec3>{
            vec3(3.5f, 5.0f, 0.0f),
            vec3(5, 2, 0),
            vec3(5.0f, 3.5f, -1.0f) }),
        tree, query, 3, 100);

    EXPECT_QUERY_K(
        (vector<vec3>{
            vec3(2.5f, 3.5f, 0.0f),
            vec3(3.5f, 5.0f, 0.0f),
            vec3(5, 2, 0),
            vec3(5.0f, 3.5f, -1.0f) }),
        tree, query, 4, 100);

    EXPECT_QUERY_K(
        (vector<vec3>{
            vec3(2.5f, 3.5f, 0.0f),
            vec3(3.5f, 5.0f, 0.0f),
            vec3(2.5f, 2.5f, 0.0f),
            vec3(5, 2, 0),
            vec3(5.0f, 3.5f, -1.0f) }),
        tree, query, 5, 100);

    EXPECT_QUERY_K(
        (vector<vec3>{
            vec3(2.5f, 3.5f, 0.0f),
            vec3(3.5f, 5.0f, 0.0f),
            vec3(2.5f, 2.5f, 0.0f),
            vec3(5, 2, 0),
            vec3(2, 1, 1),
            vec3(5.0f, 3.5f, -1.0f) }),
        tree, query, 6, 100);

    EXPECT_QUERY_K(
        (vector<vec3>{
            vec3(2, 1, 1),
            vec3(2.5f, 3.5f, 0.0f),
            vec3(3.5f, 5.0f, 0.0f),
            vec3(2.5f, 2.5f, 0.0f),
            vec3(5, 2, 0),
            vec3(1.5f, 4.0f, 2.0f),
            vec3(5.0f, 3.5f, -1.0f) }),
        tree, query, 7, 100);
}

TEST(KDTree3D, test_query_k_limited_range) {
    vector<vec3> points = {
        vec3(5.0f, 3.5f, -1.0f),
        vec3(2.0f, 1.0f, 1.0f),
        vec3(3.5f, 5.0f, -1.0f),
        vec3(1.5f, 4.0f, 2.0f),
        vec3(3.5f, 2.0f, -1.0f),
        vec3(5.0f, 2.0f, 0.0f),
        vec3(2.5f, 3.5f, 0.0f),
    };

    KDTree3D<vec3> tree(points);

    auto query = vec3(5.0f, 3.5f, -1.0f);

    EXPECT_QUERY_K(
        (vector<vec3>{
            vec3(3.5f, 5.0f, -1.0f),
            vec3(3.5f, 2.0f, -1.0f),
            vec3(5, 2, 0),
            vec3(5.0f, 3.5f, -1.0f) }),
        tree, query, 7, 2.4);
}
