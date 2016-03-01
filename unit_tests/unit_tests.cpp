#include <gtest/gtest.h>
#include <glm/gtc/constants.hpp>
#include <raytrace.hpp>
#include <cmath>

#if !defined GLM_FORCE_RADIANS
#error "GLM_FORCE_RADIANS is not defined"
#endif

TEST(math, float_eq) {
	ASSERT_TRUE(eq(1.f, 1.f));
	ASSERT_FALSE(eq(1.f, 1.001f));
	ASSERT_FALSE(eq(1.f, 1.0001f));
	ASSERT_FALSE(eq(1.f, 1.00001f));
	ASSERT_FALSE(eq(1.f, 1.000001f));
	ASSERT_TRUE(eq(1.f, 1.0000001f));
	ASSERT_FALSE(eq(-1.0000001f, 1.0000001f));
	ASSERT_FALSE(eq(-0.0000001f, 0.0000000f));
}

TEST(math, trigonometric_functions_should_use_radians) {
	ASSERT_TRUE(eq(1.f, tan(quarter_pi<float>())));
	ASSERT_TRUE(eq(1.f, sin(half_pi<float>())));
	ASSERT_TRUE(eq(-1.f, cos(pi<float>())));
}

TEST(math, shoot) {
	ASSERT_TRUE(eq(vec3(0, 0, -1), shoot(3, 3, 1, 1, half_pi<float>())));
}

TEST(raytrace, ray_triangle_intersection1) {
	vec3 triangle[] = {
		vec3(0.0f, 0.0f, -2.0f),
		vec3(1.0f, 0.0f, -2.0f),
		vec3(0.0f, 1.0f, -2.0f),
	};

	ASSERT_TRUE(!isnan(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.f, 0.f, -1.f))));
	ASSERT_TRUE(!isnan(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.1f, 0.1f, -1.f))));
	ASSERT_FALSE(isnan(intersect(triangle, ray_t(0.f, 0.f, 0.f, -0.1f, -0.1f, -1.f))));
}

TEST(raytrace, ray_triangle_intersection2) {
	vec3 triangle[] = {
		vec3(0.0f, 1.0f, -2.0f),
		vec3(-1.0f, -1.0f, -2.0f),
		vec3(1.0f, -1.0f, -2.0f),
	};

	ASSERT_TRUE(!isnan(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.f, 0.f, -1.f))));
	ASSERT_TRUE(!isnan(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.1f, 0.1f, -1.f))));
	ASSERT_TRUE(!isnan(intersect(triangle, ray_t(0.f, 0.f, 0.f, -0.1f, -0.1f, -1.f))));
	ASSERT_TRUE(!isnan(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.00120279, 0.00120279, -0.999999))));
}

