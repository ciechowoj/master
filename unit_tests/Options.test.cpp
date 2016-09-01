#include <gtest>
#include <Options.hpp>
#include <iostream>

using namespace glm;
using namespace haste;

template <class... T> Options parseArgs2(const T&... argv) {
    char const* const table[] = { argv... };
    return parseArgs(sizeof...(T), table);
}

TEST(OptionsTest, basic_tests) {
    Options options = parseArgs2("", "--help", "--version");
    ASSERT_TRUE(options.displayHelp);
    ASSERT_FALSE(options.displayVersion);

    Options x0 = parseArgs2("", "input.blend", "output.exr");
    ASSERT_FALSE(x0.displayHelp);
    EXPECT_EQ(x0.input0, "input.blend");
    EXPECT_EQ(x0.output, "output.exr");

    Options x1 = parseArgs2("", "input.blend");
    ASSERT_FALSE(x0.displayHelp);
    EXPECT_EQ(x1.output, "");
    EXPECT_FALSE(x1.batch);

    Options x2 = parseArgs2("");
    EXPECT_TRUE(x2.displayHelp);
    EXPECT_TRUE(!x2.displayMessage.empty());

    Options x3 = parseArgs2("", "input.blend", "--output=x.exr", "--num-samples=42", "--batch");
    EXPECT_FALSE(x3.displayHelp);
    EXPECT_EQ("input.blend", x3.input0);
    EXPECT_EQ("x.exr", x3.output);
    EXPECT_EQ(42, x3.numSamples);
    EXPECT_TRUE(x3.batch);

    Options x4 = parseArgs2(
        "",
        "foo.bar",
        "foo.baz",
        "--PM",
        "--num-photons=100",
        "--max-gather=10",
        "--max-radius=0.01",
        "--batch",
        "--num-samples=31415",
        "--num-minutes=60",
        "--parallel",
        "--snapshot=100",
        "--camera=1");

    EXPECT_FALSE(x4.displayHelp);
    EXPECT_EQ("foo.bar", x4.input0);
    EXPECT_EQ("foo.baz", x4.output);
    EXPECT_EQ(Options::PM, x4.technique);
    EXPECT_EQ(100, x4.numPhotons);
    EXPECT_EQ(10, x4.numGather);
    EXPECT_EQ(0.01, x4.maxRadius);
    EXPECT_TRUE(x4.batch);
    EXPECT_EQ(31415, x4.numSamples);
    EXPECT_EQ(3600.0, x4.numSeconds);
    EXPECT_TRUE(x4.parallel);
    EXPECT_EQ(100, x4.snapshot);
    EXPECT_EQ(1, x4.cameraId);

    Options x5 = parseArgs2(
        "",
        "foo.bar",
        "--master");

    EXPECT_TRUE(x5.displayHelp);
    EXPECT_FALSE(x5.displayMessage.empty());

    Options x6 = parseArgs2(
        "",
        "foo",
        "--resolution=100x200");

    EXPECT_FALSE(x6.displayHelp);
    EXPECT_EQ(100, x6.width);
    EXPECT_EQ(200, x6.height);
}
