#pragma once
#include <initializer_list>
#include <memory>
#include <string>

namespace haste {

using std::initializer_list;
using std::pair;
using std::string;
template <class T> using shared = std::shared_ptr<T>;

struct Options {
    enum Technique { PT, BPT, VCM, UPG, Viewer };
    enum Action { Render, AVG, SUB, Errors, Merge, Filter, Time };

    string input0;
    string input1;
    string output;
    string reference;
    Technique technique = PT;
    Action action = Render;
    size_t numPhotons = 0;
    double maxRadius = 0.01;
    size_t maxPath = SIZE_MAX;
    double alpha = 0.75f;
    double beta = 1.0f;
    double roulette = 0.9;
    bool batch = false;
    bool quiet = false;
    bool enable_vc = true;
    bool enable_vm = true;
    float lights = 1.0f;
    size_t numSamples = 0;
    double numSeconds = 0.0;
    size_t numThreads = 1;
    bool reload = true;
    size_t snapshot = 0;
    size_t cameraId = 0;
    size_t width = 512;
    size_t height = 512;

    bool displayHelp = false;
    bool displayVersion = false;
    string displayMessage;

    string caption() const;
};

Options parseArgs(int argc, char const* const* argv);
pair<bool, int> displayHelpIfNecessary(
    const Options& options,
    const char* version = nullptr);

class Technique;
class Scene;

shared<Technique> makeTechnique(const shared<const Scene>& scene, Options& options);
shared<Scene> loadScene(const Options& options);
string techniqueString(const Options& options);

}
