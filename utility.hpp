#pragma once
#include <random>
#include <functional>
#include <string>
#include <vector>
#include <glm>

namespace haste {

using std::function;
using std::vector;
using std::string;
using std::pair;
using namespace glm;

class PiecewiseSampler {
public:
    PiecewiseSampler();
    PiecewiseSampler(const float* weightsBegin, const float* weightsEnd);
    float sample();
private:
    std::minstd_rand engine;
    std::piecewise_constant_distribution<float> distribution;
};

void saveEXR(
    const string& path,
    size_t width,
    size_t height,
    const dvec3* data,
    const std::vector<std::string>& metadata = std::vector<std::string>());

void saveEXR(
    const string& path,
    size_t width,
    size_t height,
    const vec4* data,
    const std::vector<std::string>& metadata = std::vector<std::string>());

void saveEXR(
    const string& path,
    size_t width,
    size_t height,
    const dvec4* data,
    const std::vector<std::string>& metadata = std::vector<std::string>());

void loadEXR(
    const string& path,
    size_t& width,
    size_t& height,
    vector<dvec3>& data,
    std::vector<std::string>* metadata = nullptr);

void loadEXR(
    const string& path,
    size_t& width,
    size_t& height,
    vector<dvec4>& data,
    std::vector<std::string>* metadata = nullptr);

string homePath();
string baseName(string path);
string fixedPath(string base, string scene, int samples);
pair<string, string> splitext(string path);
size_t getmtime(const string& path);

dvec3 computeAVG(const string& path);
void printAVG(const string& path);
void printHistory(const string& path);

dvec3 computeRMS(const string& path0, const string& path1);
void printRMS(const string& path0, const string& path1);
void subtract(const string& result, const string& path0, const string& path1);

double high_resolution_time();

}

