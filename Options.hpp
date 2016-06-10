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
    enum Technique { BPT, PT, PM, VCM };

    string input;
    string output;
    string reference;
    Technique technique = PT;
    size_t numPhotons = 100000;
    size_t numGather = 100;
    double maxRadius = 0.1;
    size_t minSubpath = 5;
    double beta = 1.0f;
    double roulette = 0.5;
    bool batch = false;
    size_t numSamples = 0;
    double numSeconds = 0.0;
    bool parallel = false;
    bool reload = true;
    size_t snapshot = 0;
    size_t cameraId = 0;
    size_t width = 512;
    size_t height = 512;

    bool displayHelp = false;
    bool displayVersion = false;
    string displayMessage;
};

Options parseArgs(int argc, char const* const* argv);
pair<bool, int> displayHelpIfNecessary(
    const Options& options,
    const char* version = nullptr);

class Technique;
class Scene;

shared<Technique> makeTechnique(const Options& options);
shared<Scene> loadScene(const Options& options);
string techniqueString(const Options& options);


}
