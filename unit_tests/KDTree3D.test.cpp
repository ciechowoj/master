#include <gtest/gtest.h>
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
