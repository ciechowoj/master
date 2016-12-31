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
#include <ImfDoubleAttribute.h>
#include <ImfVecAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfArray.h>

using namespace std;
using namespace Imf;

namespace haste {

void saveEXR(
    const std::string& path,
    const metadata_t& metadata,
    const vec3* data) {
    runtime_assert(metadata.resolution.x > 0);
    runtime_assert(metadata.resolution.y > 0);

    int width = metadata.resolution.x;
    int height = metadata.resolution.y;

    Header header (width, height);
    header.channels().insert ("R", Channel (Imf::FLOAT));
    header.channels().insert ("G", Channel (Imf::FLOAT));
    header.channels().insert ("B", Channel (Imf::FLOAT));

    header.insert("technique", StringAttribute(metadata.technique));
    header.insert("num_samples", DoubleAttribute(double(metadata.num_samples)));
    header.insert("num_basic_rays", DoubleAttribute(double(metadata.num_basic_rays)));
    header.insert("num_shadow_rays", DoubleAttribute(double(metadata.num_shadow_rays)));
    header.insert("num_tentative_rays", DoubleAttribute(double(metadata.num_tentative_rays)));
    header.insert("num_photons", DoubleAttribute(double(metadata.num_photons)));
    header.insert("num_threads", DoubleAttribute(double(metadata.num_threads)));

    header.insert("roulette", DoubleAttribute(double(metadata.roulette)));
    header.insert("radius", DoubleAttribute(double(metadata.radius)));
    header.insert("alpha", DoubleAttribute(double(metadata.alpha)));
    header.insert("beta", DoubleAttribute(double(metadata.beta)));
    header.insert("epsilon", DoubleAttribute(double(metadata.epsilon)));
    header.insert("total_time", DoubleAttribute(double(metadata.total_time)));

    Imath::V3f metadata_average(
        metadata.average.x,
        metadata.average.y,
        metadata.average.z);

    header.insert("average", V3fAttribute(metadata_average));

    OutputFile file (path.c_str(), header);

    FrameBuffer framebuffer;

    size_t size = width * height * 3;
    vector<float> data_copy(size);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            data_copy[(y * width + x) * 3 + 0] = data[(height - y - 1) * width + x].x;
            data_copy[(y * width + x) * 3 + 1] = data[(height - y - 1) * width + x].y;
            data_copy[(y * width + x) * 3 + 2] = data[(height - y - 1) * width + x].z;
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
    const std::string& path,
    const metadata_t& metadata,
    const std::vector<vec3>& data) {
    runtime_assert(metadata.resolution.x > 0);
    runtime_assert(metadata.resolution.y > 0);
    runtime_assert(size_t(metadata.resolution.x * metadata.resolution.y) == data.size());
    saveEXR(path, metadata, data.data());
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
    metadata_t& metadata,
    vector<vec3>& data) {
    InputFile file (path.c_str());

    auto comments = file.header().findTypedAttribute<StringAttribute>("p7yh5o0587jqc5qv-metadata");

    auto technique = file.header().findTypedAttribute<StringAttribute>("technique");
    auto num_samples = file.header().findTypedAttribute<DoubleAttribute>("num_samples");
    auto num_basic_rays = file.header().findTypedAttribute<DoubleAttribute>("num_basic_rays");
    auto num_shadow_rays = file.header().findTypedAttribute<DoubleAttribute>("num_shadow_rays");
    auto num_tentative_rays = file.header().findTypedAttribute<DoubleAttribute>("num_tentative_rays");
    auto num_photons = file.header().findTypedAttribute<DoubleAttribute>("num_photons");
    auto num_threads = file.header().findTypedAttribute<DoubleAttribute>("num_threads");

    auto roulette = file.header().findTypedAttribute<DoubleAttribute>("roulette");
    auto radius = file.header().findTypedAttribute<DoubleAttribute>("radius");
    auto alpha = file.header().findTypedAttribute<DoubleAttribute>("alpha");
    auto beta = file.header().findTypedAttribute<DoubleAttribute>("beta");
    auto epsilon = file.header().findTypedAttribute<DoubleAttribute>("epsilon");
    auto total_time = file.header().findTypedAttribute<DoubleAttribute>("total_time");

    auto average = file.header().findTypedAttribute<V3fAttribute>("average");

    metadata.technique = technique ? technique->value() : std::string("N/A");

    if (comments) {
        std::vector<std::string> history = split(comments->value(), ';');
        metadata.num_samples = history.size();
    }
    else {
        metadata.num_samples = num_samples ? num_samples->value() : std::size_t(0);
    }

    metadata.num_basic_rays = num_basic_rays ? num_basic_rays->value() : std::size_t(0);
    metadata.num_shadow_rays = num_shadow_rays ? num_shadow_rays->value() : std::size_t(0);
    metadata.num_tentative_rays = num_tentative_rays ? num_tentative_rays->value() : std::size_t(0);
    metadata.num_photons = num_photons ? num_photons->value() : std::size_t(0);
    metadata.num_threads = num_threads ? num_threads->value() : std::size_t(0);

    metadata.roulette = roulette ? roulette->value() : 0.0;
    metadata.radius = radius ? radius->value() : 0.0;
    metadata.alpha = alpha ? alpha->value() : 0.0;
    metadata.beta = beta ? beta->value() : 0.0;
    metadata.epsilon = epsilon ? epsilon->value() : 0.0;
    metadata.total_time = total_time ? total_time->value() : 0.0;

    Imath::V3f metadata_average = average ? average->value() : Imath::V3f(0.f, 0.f, 0.f);
    metadata.average = vec3(metadata_average.x, metadata_average.y, metadata_average.z);

    auto dw = file.header().dataWindow();
    int width = dw.max.x - dw.min.x + 1;
    int height = dw.max.y - dw.min.y + 1;

    metadata.resolution.x = width;
    metadata.resolution.y = height;

    runtime_assert(width >= 0);
    runtime_assert(height >= 0);

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

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            data[i * width + j] = data_copy[(height - i - 1) * width + j];
        }
    }
}

std::vector<dvec4> vv3f_to_vv4d(const std::vector<vec3>& data) {
    std::vector<dvec4> result(data.size());

    for (std::size_t i = 0; i < data.size(); ++i) {
        result[i] = dvec4(data[i], 1.0f);
    }

    return result;
}

std::vector<vec3> vv4f_to_vv3f(std::size_t size, const vec4* data) {
    std::vector<vec3> result(size);

    for (std::size_t i = 0; i < size; ++i) {
        result[i] = data[i].rgb() / data[i].a;
    }

    return result;
}

std::vector<vec3> vv4d_to_vv3f(std::size_t size, const dvec4* data) {
    std::vector<vec3> result(size);

    for (std::size_t i = 0; i < size; ++i) {
        result[i] = data[i].rgb() / data[i].a;
    }

    return result;
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
    vector<vec3> data;
    metadata_t metadata;

    loadEXR(path, metadata, data);

    dvec3 result = dvec3(0.0f);

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

dvec3 computeRMS(const string& path0, const string& path1) {
    vector<vec3> data0, data1;
    metadata_t metadata0, metadata1;

    loadEXR(path0, metadata0, data0);
    loadEXR(path1, metadata1, data1);

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
    vector<vec3> data0, data1, data2;
    metadata_t metadata0, metadata1;

    loadEXR(path0, metadata0, data0);
    loadEXR(path1, metadata1, data1);

    if (data0.size() != data1.size()) {
        throw std::runtime_error(
            "Sizes of '" + path0 + "' and '" + path1 + "' doesn't match.");
    }

    data2.resize(data0.size());

    for (size_t i = 0; i < data0.size(); ++i) {
        data2[i] = data0[i] - data1[i];
    }

    metadata_t metadata;
    metadata.technique = "difference";
    metadata.resolution.x = metadata0.resolution.x;
    metadata.resolution.y = metadata0.resolution.y;

    saveEXR(result, metadata, data2);
}

void printRMS(const string& path0, const string& path1) {
    printVec(computeRMS(path0, path1));
}

double high_resolution_time() {
    auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}

}
