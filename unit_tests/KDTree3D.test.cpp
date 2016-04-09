#include <gtest/gtest.h>
#include <KDTree3D.hpp>

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

    BitfieldVector<3> c(100);
    c.set(99, 5);

    EXPECT_EQ(0, c.get(50));
    EXPECT_EQ(0, c.get(98));
    EXPECT_EQ(5, c.get(99));
}

TEST(KDTree3D, should_create_empty) {
    KDTree3D<vec3> a;
}

TEST(KDTree3D, should_create_singleton) {
    KDTree3D<vec3> a({ vec3(0.0f) });
}

TEST(KDTree3D, should_create_from_bunch_of_points) {
    vector<vec3> points = {
        vec3(2.0f, 1.0f, 1.0f),
        vec3(3.5f, 5.0f, 0.0f),
        vec3(1.5f, 4.0f, 2.0f),
        vec3(5.0f, 3.5f, -1.0f),
        vec3(2.5f, 2.5f, 0.0f),
        vec3(5.0f, 2.0f, 0.0f),
        vec3(2.5f, 3.5f, 0.0f),
    };

    KDTree3D<vec3> a(points);
}



