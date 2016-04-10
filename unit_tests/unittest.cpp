#include <gtest/gtest.h>
#include <xmmintrin.h>
#include <pmmintrin.h>

int main(int argc, char **argv) {
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
