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
    enum Technique { BPT, PT, PM, VCM, UPG, Viewer };
    enum Action { Render, AVG, RMS };

    string input0;
    string input1;
    string output;
    string reference;
    Technique technique = PT;
    Action action = Render;
    size_t numPhotons = 1000000;
    size_t numGather = 100;
    double maxRadius = 0.001;
    size_t minSubpath = 5;
    double beta = 1.0f;
    double roulette = 0.6;
    bool batch = false;
    bool quiet = false;
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

shared<Technique> makeTechnique(Options& options);
shared<Scene> loadScene(const Options& options);
string techniqueString(const Options& options);

}
