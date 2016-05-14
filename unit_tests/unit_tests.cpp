#include <gtest/gtest.h>
#include <Scene.hpp>
#include <cmath>

#if !defined GLM_FORCE_RADIANS
#error "GLM_FORCE_RADIANS is not defined"
#endif

using namespace haste;
using namespace glm;

TEST(NANTest, main) {
    EXPECT_TRUE(std::isinf(1.0f / 0.0f));
    EXPECT_FALSE(std::isnan(1.0f / 1.0f));
    EXPECT_TRUE(std::isnan(0.0f / 0.0f));
    EXPECT_TRUE(std::isnan(-0.0f / 0.0f));
}

