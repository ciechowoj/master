#include <runtime_assert>
#include <utility.hpp>

namespace haste {

UniformSampler::UniformSampler() {
    std::random_device device;
    engine.seed(device());
}

float UniformSampler::sample() {
    return std::uniform_real_distribution<float>()(engine);
}

PiecewiseSampler::PiecewiseSampler() { }

PiecewiseSampler::PiecewiseSampler(const float* weightsBegin, const float* weightsEnd) {
    std::random_device device;
    engine.seed(device());

    size_t numWeights = weightsEnd - weightsBegin;

    distribution = std::piecewise_constant_distribution<float>(
        numWeights,
        0.f,
        1.f,
        [&](float x) { return weightsBegin[size_t(x * (numWeights + 1))]; }
        );
}

float PiecewiseSampler::sample() {
    return distribution(engine);
}

vec3 BarycentricSampler::sample() {
    float u = uniform.sample();
    float v = uniform.sample();

    if (u + v <= 1) {
        return vec3(u, v, 1 - u - v);
    }
    else {
        return vec3(1 - u, 1 - v, u + v - 1);
    }
}

vec3 HemisphereSampler::sample() {
    float a = uniform.sample();
    float b = uniform.sample() * pi<float>() * 2.0f;
    float c = sqrt(1 - a * a);

    return vec3(cos(b) * c, a, sin(b) * c);
}

}

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfArray.h>

using namespace std;
using namespace Imf;

namespace haste {

void saveEXR(
    const string& path,
    const vector<vec4>& data,
    size_t pitch)
{
    auto data3 = vector<vec3>(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        data3[i] = data[i].xyz() / data[i].w;
    }

    saveEXR(path, data3, pitch);
}

void saveEXR(
    const string& path,
    const vector<vec3>& data,
    size_t pitch)
{
    runtime_assert(pitch < INT_MAX);
    runtime_assert(data.size() / pitch < INT_MAX);

    int width = int(pitch);
    int height = int(data.size() / width);

    Header header (width, height);
    header.channels().insert ("R", Channel (Imf::FLOAT));
    header.channels().insert ("G", Channel (Imf::FLOAT));
    header.channels().insert ("B", Channel (Imf::FLOAT));

    OutputFile file (path.c_str(), header);

    FrameBuffer framebuffer;
            
    size_t size = data.size() * 3;
    vector<float> data_copy(size);

    for (size_t y = 0; y < height; ++y) {
        ::memcpy(
            data_copy.data() + y * width * 3, 
            data.data() + (height - y - 1) * width, 
            width * sizeof(vec3));
    }

    auto R = Slice(Imf::FLOAT, (char*)(data_copy.data() + 0), sizeof(float) * 3, sizeof(float) * width * 3);
    auto G = Slice(Imf::FLOAT, (char*)(data_copy.data() + 1), sizeof(float) * 3, sizeof(float) * width * 3);
    auto B = Slice(Imf::FLOAT, (char*)(data_copy.data() + 2), sizeof(float) * 3, sizeof(float) * width * 3);

    framebuffer.insert("R", R);
    framebuffer.insert("G", G);
    framebuffer.insert("B", B);

    file.setFrameBuffer(framebuffer);
    file.writePixels(height);
}

}

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace haste {

string homePath() {
    const char* home = getenv("HOME");

    if (home == nullptr) {
        home = getpwuid(getuid())->pw_dir;
    }

    return home;
}

string baseName(string path) {
    size_t index = path.find_last_of("/");

    if (index != string::npos) {
        return path.substr(index + 1, path.size());
    }
    else {
        return path;
    }
}

pair<string, string> splitext(string path) {
    size_t index = path.find_last_of(".");

    if (index == string::npos || index == 0) {
        return make_pair(path, string());
    }
    else {
        return make_pair(path.substr(0, index), path.substr(index, path.size()));
    }
}

}
