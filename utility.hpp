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

struct metadata_t {
    std::string technique;
    size_t num_samples = 0;
    size_t num_basic_rays = 0;
    size_t num_shadow_rays = 0;
    size_t num_tentative_rays = 0;
    size_t num_photons = 0;
    size_t num_threads = 0;
    glm::ivec2 resolution = glm::ivec2(0, 0);
    double roulette = 0.0;
    double radius = 0.0;
    double alpha = 0.0;
    double beta = 0.0;
    double epsilon = 0.0;
    double total_time = 0.0;
    glm::vec3 average = glm::vec3(0.0f, 0.0f, 0.0f);
};

void saveEXR(
    const std::string& path,
    const metadata_t& metadata,
    const vec3* data);

void saveEXR(
    const std::string& path,
    const metadata_t& metadata,
    const std::vector<vec3>& data);

void loadEXR(
    const std::string& path,
    metadata_t& metadata,
    std::vector<vec3>& data);

std::vector<dvec4> vv3f_to_vv4d(const std::vector<vec3>& data);
std::vector<vec3> vv4f_to_vv3f(std::size_t size, const vec4* data);
std::vector<vec3> vv4d_to_vv3f(std::size_t size, const dvec4* data);

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

