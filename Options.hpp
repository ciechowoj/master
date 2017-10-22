#pragma once
#include <initializer_list>
#include <memory>
#include <string>
#include <map>
#include <glm>

#include <statistics.hpp>

namespace haste {

using std::initializer_list;
using std::pair;
using std::string;
using std::map;

template <class T> using shared = std::shared_ptr<T>;

struct Options {
    enum Technique { PT, BPT, VCM, UPG, Viewer };
    enum Action {
        Render, AVG, SUB, Errors, Strip, Merge,
        Filter, Time, Continue, Statistics, Measurements,
        Gnuplot };

    string input0;
    string input1;
    string output;
    string reference;
    Technique technique = PT;
    Action action = Render;
    size_t num_photons = 0;
    double radius = 0.01;
    size_t max_path = PTRDIFF_MAX;
    double alpha = 0.75f;
    double beta = 1.0f;
    double roulette = 0.9;
    bool batch = false;
    bool quiet = false;
    bool enable_vc = true;
    bool enable_vm = true;
    bool from_light = true;
    float lights = 1.0f;
    size_t num_samples = 0;
    double num_seconds = 0.0;
    size_t num_threads = 1;
    bool reload = true;
    bool enable_seed = false;
    size_t seed = 0;
    size_t snapshot = 0;
    size_t camera_id = 0;
    size_t width = 512;
    size_t height = 512;
	vector<ivec3> trace;
    vec3 sky_horizon = vec3(0);
    vec3 sky_zenith = vec3(0);

    bool displayHelp = false;
    bool displayVersion = false;
    string displayMessage;

    string caption() const;

    Options() = default;
    Options(const map<string, string>& dict);

    map<string, string> to_dict() const;
    string get_output() const;
};

Options parseArgs(int argc, char const* const* argv);
void overrideArgs(Options& options, int argc, const char* const* argv);
pair<bool, int> displayHelpIfNecessary(
    const Options& options,
    const char* version = nullptr);

class Technique;
class Scene;

shared<Technique> makeTechnique(const shared<const Scene>& scene, Options& options);
shared<Scene> loadScene(const Options& options);

string to_string(const Options::Technique& technique);

void save_exr(Options options, statistics_t statistics, const vec3* data);
void save_exr(Options options, statistics_t statistics, const vec4* data);
void save_exr(Options options, statistics_t statistics, const dvec3* data);
void save_exr(Options options, statistics_t statistics, const dvec4* data);
void strip_exr(string dst, string src);
void merge_exr(string dst, string fst, string snd);

}
