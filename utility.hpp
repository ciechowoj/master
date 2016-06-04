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

class UniformSampler {
public:
    UniformSampler();
    float sample();
private:
    std::minstd_rand engine;
};

class PiecewiseSampler {
public:
    PiecewiseSampler();
    PiecewiseSampler(const float* weightsBegin, const float* weightsEnd);
    float sample();
private:
    std::minstd_rand engine;
    std::piecewise_constant_distribution<float> distribution;
};

class BarycentricSampler {
public:
    vec3 sample();
private:
    UniformSampler uniform;
};

void saveEXR(
    const string& path,
    size_t width,
    size_t height,
    const vec3* data);

void saveEXR(
    const string& path,
    size_t width,
    size_t height,
    const vec4* data);

void loadEXR(
    const string& path,
    size_t& width,
    size_t& height,
    vector<vec3>& data);

void loadEXR(
    const string& path,
    size_t& width,
    size_t& height,
    vector<vec4>& data);

string homePath();
string baseName(string path);
string fixedPath(string base, string scene, int samples);
pair<string, string> splitext(string path);
size_t getmtime(const string& path);

void renderPoints(
    vector<vec4>& image,
    size_t width,
    const vector<vec3>& points,
    const vec3& color,
    const mat4& proj);

void renderPoints(
    vector<vec4>& image,
    size_t width,
    const vector<vec3>& points,
    const vec3& color,
    const vec3& origin);

void renderPoints(
    vector<vec4>& image,
    size_t width,
    const vector<vec3>& points,
    const vec3& color,
    float theta,
    float phi,
    float radius);

}

