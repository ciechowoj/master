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
    enum Technique { BDPT, PT, PM, VCM };

    string input;
    string output;
    Technique technique = PT;
    size_t numPhotons = 1000000;
    size_t maxGather = 100;
    double maxRadius = 0.1;
    bool batch = false;
    size_t numSamples = 0;
    double numSeconds = 0.0;
    bool parallel = false;
    size_t snapshot = 0;
    size_t cameraId = 0;
    size_t width = 800;
    size_t height = 600;

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
