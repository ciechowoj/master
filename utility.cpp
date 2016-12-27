#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <runtime_assert>
#include <utility.hpp>
#include <chrono>

namespace haste {

PiecewiseSampler::PiecewiseSampler() { }

PiecewiseSampler::PiecewiseSampler(const float* weightsBegin, const float* weightsEnd) {
    std::random_device device;
    engine.seed(device());

    size_t numWeights = weightsEnd - weightsBegin;

    auto lambda = [&](float x) {
        return weightsBegin[min(size_t(x * numWeights), numWeights - 1)];
    };

    distribution = std::piecewise_constant_distribution<float>(
        numWeights,
        0.f,
        1.f,
        lambda);
}

float PiecewiseSampler::sample() {
    return distribution(engine);
}

}

#include <ImfInputFile.h>
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
    size_t widthll,
    size_t heightll,
    const dvec3* data,
    const std::vector<std::string>& metadata) {
    runtime_assert(widthll < INT_MAX);
    runtime_assert(heightll < INT_MAX);

    const int width = int(widthll);
    const int height = int(heightll);

    Header header (width, height);
    header.channels().insert ("R", Channel (Imf::FLOAT));
    header.channels().insert ("G", Channel (Imf::FLOAT));
    header.channels().insert ("B", Channel (Imf::FLOAT));

    std::stringstream stream;

    for (auto&& entry : metadata) {
        stream << entry << ";";
    }

    header.insert("p7yh5o0587jqc5qv-metadata", StringAttribute(stream.str()));

    OutputFile file (path.c_str(), header);

    FrameBuffer framebuffer;

    size_t size = width * height * 3;
    vector<float> data_copy(size);

    for (size_t y = 0; y < heightll; ++y) {
        for (size_t x = 0; x < widthll; ++x) {
            data_copy[(y * width + x) * 3 + 0] = float(data[(heightll - y - 1) * width + x].x);
            data_copy[(y * width + x) * 3 + 1] = float(data[(heightll - y - 1) * width + x].y);
            data_copy[(y * width + x) * 3 + 2] = float(data[(heightll - y - 1) * width + x].z);
        }
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

void saveEXR(
    const string& path,
    size_t width,
    size_t height,
    const vec4* data,
    const std::vector<std::string>& metadata) {
    auto data3 = vector<dvec3>(width * height);

    for (size_t i = 0; i < data3.size(); ++i) {
        data3[i] = data[i].xyz() / data[i].w;
    }

    saveEXR(path, width, height, data3.data(), metadata);
}

void saveEXR(
    const string& path,
    size_t width,
    size_t height,
    const dvec4* data,
    const std::vector<std::string>& metadata)
{
    auto data3 = vector<dvec3>(width * height);

    for (size_t i = 0; i < data3.size(); ++i) {
        data3[i] = data[i].xyz() / data[i].w;
    }

    saveEXR(path, width, height, data3.data(), metadata);
}

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

void loadEXR(
    const string& path,
    size_t& width,
    size_t& height,
    vector<dvec3>& data,
    std::vector<std::string>* metadata)
{
    InputFile file (path.c_str());

    auto comments = file.header().findTypedAttribute<StringAttribute>("p7yh5o0587jqc5qv-metadata");

    if (comments && metadata) {
        *metadata = split(comments->value(), ';');
    }

    auto dw = file.header().dataWindow();
    int ihegith = dw.max.x - dw.min.x + 1;
    int iwidth = dw.max.y - dw.min.y + 1;

    runtime_assert(iwidth >= 0);
    runtime_assert(ihegith >= 0);

    width = size_t(iwidth);
    height = size_t(ihegith);

    FrameBuffer framebuffer;

    vector<vec3> data_copy;
    data_copy.resize(width * height);

    float* fdata = (float*)data_copy.data();

    auto R = Slice(Imf::FLOAT, (char*)(fdata + 0), sizeof(float) * 3, sizeof(float) * width * 3);
    auto G = Slice(Imf::FLOAT, (char*)(fdata + 1), sizeof(float) * 3, sizeof(float) * width * 3);
    auto B = Slice(Imf::FLOAT, (char*)(fdata + 2), sizeof(float) * 3, sizeof(float) * width * 3);

    framebuffer.insert("R", R);
    framebuffer.insert("G", G);
    framebuffer.insert("B", B);

    file.setFrameBuffer(framebuffer);
    file.readPixels(dw.min.y, dw.max.y);

    data.resize(width * height);

    for (size_t i = 0; i < height; ++i) {
        for (size_t j = 0; j < width; ++j) {
            data[i * width + j] = data_copy[(height - i - 1) * width + j];
        }
    }
}

void loadEXR(
    const string& path,
    size_t& width,
    size_t& height,
    vector<dvec4>& data,
    std::vector<std::string>* metadata)
{
    auto data3 = vector<dvec3>();
    loadEXR(path, width, height, data3, metadata);

    data.resize(data3.size());

    for (size_t i = 0; i < data3.size(); ++i) {
        data[i] = vec4(data3[i], 1.0f);
    }
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

string fixedPath(string base, string scene, int samples) {
    string ext;
    tie(base, ext) = splitext(base);

    if (ext.empty()) {
        ext = ".exr";
    }

    string sceneBase, sceneExt;
    tie(sceneBase, sceneExt) = splitext(scene);

    stringstream result;

    if (!base.empty() && base[base.size() - 1] != '/') {
        result << base << "." << baseName(sceneBase) << "." << samples << ext;
    }
    else {
        result << base << baseName(sceneBase) << "." << samples << ext;
    }

    return result.str();
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

#include <sys/stat.h>

namespace haste {

size_t getmtime(const string& path) {
    struct stat buf;
    stat(path.c_str(), &buf);
    return buf.st_mtime;
}

dvec3 computeAVG(const string& path) {
    vector<dvec3> data;
    size_t width, height;

    loadEXR(path, width, height, data);

    dvec3 result = vec3(0.0f);

    for (size_t i = 0; i < data.size(); ++i) {
        result += data[i];
    }

    return result / double(data.size());
}

static void printVec(const vec3& v) {
    std::cout
        << std::setprecision(7)
        << std::fixed
        << std::setw(12)
        << v.x
        << std::setw(12)
        << v.y
        << std::setw(12)
        << v.z
        << std::endl;
}

void printAVG(const string& path) {
    printVec(computeAVG(path));
}

void printHistory(const string& path) {
    std::vector<glm::dvec3> data;
    std::vector<std::string> metadata;
    size_t width, height;

    loadEXR(path, width, height, data, &metadata);

    for (auto&& entry : metadata) {
        std::cout << entry << "\n";
    }

    std::cout.flush();
}

dvec3 computeRMS(const string& path0, const string& path1) {
    vector<dvec3> data0, data1;
    size_t width0, height0, width1, height1;

    loadEXR(path0, width0, height0, data0);
    loadEXR(path1, width1, height1, data1);

    dvec3 result = dvec3(0.0f);

    if (data0.size() != data1.size()) {
        throw std::runtime_error(
            "Sizes of '" + path0 + "' and '" + path1 + "' doesn't match.");
    }

    for (size_t i = 0; i < data0.size(); ++i) {
        auto diff = data0[i] - data1[i];
        result += diff * diff;
    }

    return sqrt(result / double(data0.size()));
}

void subtract(const string& result, const string& path0, const string& path1) {
    vector<dvec3> data0, data1, data2;
    size_t width0, height0, width1, height1;

    loadEXR(path0, width0, height0, data0);
    loadEXR(path1, width1, height1, data1);

    if (data0.size() != data1.size()) {
        throw std::runtime_error(
            "Sizes of '" + path0 + "' and '" + path1 + "' doesn't match.");
    }

    data2.resize(data0.size());

    for (size_t i = 0; i < data0.size(); ++i) {
        data2[i] = data0[i] - data1[i];
    }

    saveEXR(result, width0, height0, data2.data());
}

void printRMS(const string& path0, const string& path1) {
    printVec(computeRMS(path0, path1));
}

double high_resolution_time() {
    auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}

}
