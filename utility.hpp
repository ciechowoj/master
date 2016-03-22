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

class HemisphereSampler {
public:
    vec3 sample();
private:
    UniformSampler uniform;
};

void saveEXR(
    const string& path, 
    const vector<vec3>& data, 
    size_t pitch);

void saveEXR(
    const string& path, 
    const vector<vec4>& data, 
    size_t pitch);

string homePath();
string baseName(string path);
pair<string, string> splitext(string path);

}

