#include <gtest/gtest.h>
#include <glm/gtc/constants.hpp>
#include <raytrace.hpp>
#include <streamops.hpp>

using namespace std;
using namespace glm;

bool eq(float fa, float fb, int ulps = 2) {
    int a = *(int*)&fa;
    
    if (a < 0) {
        a = 0x80000000 - a;
    }

    int b = *(int*)&fb;

    if (b < 0) {
        b = 0x80000000 - b;
    }

    return abs(a - b) <= ulps;
}

bool eq(const vec3& a, const vec3& b) {
	return eq(a.x, b.x) && eq(a.y, b.y) && eq(a.z, b.z);
}

vec3 shoot(int width, int height, int x, int y, float fovy) {
    float znear = 1.f / tan(fovy * 0.5f);
    float aspect = float(width) / float(height);
    float fx = ((float(x) + 0.5f) / float(width) * 2.f - 1.f) * aspect;
    float fy = (float(y) + 0.5f) / float(height) * 2.f - 1.f;
    return normalize(vec3(fx, fy, -znear));
}

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


	cout << shoot(640, 480, 320, 240, pi<float>() / 3.f) << endl;
}

void raytrace(std::vector<vec3>& image, int width, int height, const camera_t& camera, const scene_t& scene) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {

            ray_t ray;
            ray.pos = vec3(0.f, 0.f, 0.f);
            ray.dir = shoot(width, height, x, y, camera.fovy);

            // std::cout << ray.dir << std::endl;

            bool hit = false;
            for (int i = 0; i < scene.shapes[0].mesh.positions.size(); i += 9) {
                if (intersect((vec3*)(scene.shapes[0].mesh.positions.data() + i), ray)) {
                	hit = true;
                }
            }

            if (hit) {
                image[y * width + x] = vec3(1.f);
            }
            else {
                image[y * width + x] = vec3(1.f, 0.f, 0.f);
            }
        }
    }
}

TEST(raytrace, ray_triangle_intersection1) {
	vec3 triangle[] = {
		vec3(0.0f, 0.0f, -2.0f),
		vec3(1.0f, 0.0f, -2.0f),
		vec3(0.0f, 1.0f, -2.0f),
	};

	ASSERT_TRUE(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.f, 0.f, -1.f)));
	ASSERT_TRUE(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.1f, 0.1f, -1.f)));
	ASSERT_FALSE(intersect(triangle, ray_t(0.f, 0.f, 0.f, -0.1f, -0.1f, -1.f)));
}

TEST(raytrace, ray_triangle_intersection2) {
	vec3 triangle[] = {
		vec3(0.0f, 1.0f, -2.0f),
		vec3(-1.0f, -1.0f, -2.0f),
		vec3(1.0f, -1.0f, -2.0f),
	};

	ASSERT_TRUE(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.f, 0.f, -1.f)));
	ASSERT_TRUE(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.1f, 0.1f, -1.f)));
	ASSERT_TRUE(intersect(triangle, ray_t(0.f, 0.f, 0.f, -0.1f, -0.1f, -1.f)));
	ASSERT_TRUE(intersect(triangle, ray_t(0.f, 0.f, 0.f, 0.00120279, 0.00120279, -0.999999)));
}

