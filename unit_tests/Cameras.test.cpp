#include <gtest>
#include <Cameras.hpp>
#include <iostream>

using namespace glm;
using namespace haste;

TEST(CamerasTest, basic_tests) {
    Cameras cameras;

    size_t id = cameras.addCameraFovX(
        "dummy",
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, -1.0f),
        vec3(0.0f, 1.0f, 0.0f),
        half_pi<float>());

    ASSERT_EQ(1, cameras.numCameras());
    EXPECT_EQ("dummy", cameras.name(id));

    EXPECT_FLOAT_EQ(half_pi<float>(), cameras.fovx(id, 1.0f));
    EXPECT_FLOAT_EQ(half_pi<float>(), cameras.fovx(id, 4.0f / 3.0f));
    EXPECT_FLOAT_EQ(half_pi<float>(), cameras.fovy(id, 1.0f));
    EXPECT_FLOAT_EQ(1.2870022f, cameras.fovy(id, 4.0f / 3.0f));

    const float w = 800;
    const float h = 600;
    const float wInv = 1.0f / w;
    const float hInv = 1.0f / h;
    const float a = w / h;

    auto r0 = cameras.shoot(id, vec2(0.0f, 0.0f), wInv, hInv, a, 400.0f, 300.0f);
    auto r1 = cameras.shoot(id, vec2(0.0f, 0.0f), wInv, hInv, a, 0.0f, 0.0f);
    auto r2 = cameras.shoot(id, vec2(0.0f, 0.0f), wInv, hInv, a, 800.0f, 600.0f);
    auto r3 = cameras.shoot(id, vec2(0.0f, 0.0f), 0.01f, 0.01f, 1, 0.0f, 0.0f);

    EXPECT_VEC3_EQ(vec3(0.0f, 0.0f, 0.0f), r0.origin, 0.00001f);
    EXPECT_VEC3_EQ(vec3(0.0f, 0.0f, -1.0f), r0.direction, 0.00001f);

    EXPECT_VEC3_EQ(vec3(0.0f), r1.origin, 0.000001f);
    EXPECT_VEC3_EQ(vec3(-0.685994f, -0.514496f, -0.514496f), r1.direction, 0.000001f);

    EXPECT_VEC3_EQ(vec3(0.0f), r2.origin, 0.000001f);
    EXPECT_VEC3_EQ(vec3(0.685994f, 0.514496f, -0.514496f), r2.direction, 0.0001f);

    EXPECT_VEC3_EQ(vec3(0.0f), r3.origin, 0.00001f);
    EXPECT_VEC3_EQ(vec3(-0.577f, -0.577f, -0.577f), r3.direction, 0.001f);
}

TEST(CamerasTest, rotated) {
    Cameras cameras;

    size_t id = cameras.addCameraFovX(
        "dummy",
        vec3(0.0f, 0.0f, 0.0f),
        vec3(1.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        half_pi<float>());

    ASSERT_EQ(1, cameras.numCameras());

    auto r0 = cameras.shoot(id, vec2(0.0f, 0.0f), 0.01f, 0.01f, 1, 0.0f, 0.0f);
    EXPECT_VEC3_EQ(vec3(0.577f, -0.577f, -0.577f), r0.direction, 0.001f);
}
