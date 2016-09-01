#include <iostream>
#include <map>
#include <cstring>
#include <Options.hpp>
#include <loader.hpp>

#include <BPT.hpp>
#include <PT.hpp>
#include <PhotonMapping.hpp>
#include <VCM.hpp>
#include <Viewer.hpp>

namespace haste {

using std::map;
using std::strstr;
using std::make_pair;

static const char help[] =
R"(
    Usage:
      master <input> [options]
      master (-h | --help)
      master --version
      master avg <x>      Compute average value of pixels in <x>.
      master rms <x> <y>  Compute root mean square error between <x> and <y>.

    Options:
      -h --help           Show this screen.
      --version           Show version.
      --BPT               Use bidirectional path tracing (balance heuristics).
      --PT                Use path tracing for rendering (this is default one).
      --PM                Use photon mapping for rendering.
      --VCM               Use vertex connection and merging (not implemented/wip).
      --num-photons=<n>   Use n photons. [default: 1 000 000]
      --num-gather=<n>    Use n as maximal number of gathered photons. [default: 100]
      --max-radius=<n>    Use n as maximum gather radius. [default: 0.1]
      --min-subpath=<n>   Do not use Russian roulette for sub-paths shorter than n. [default: 5]
      --roulette=<n>      Russian roulette coefficient. [default: 0.5]
      --batch             Run in batch mode (interactive otherwise).
      --quiet             Do not output anything to console.
      --no-reload         Disable autoreload (input file is reloaded on modification in interactive mode).
      --num-samples=<n>   Terminate after n samples.
      --num-seconds=<n>   Terminate after n seconds.
      --num-minutes=<n>   Terminate after n minutes.
      --parallel          Use multithreading.
      --snapshot=<n>      Save output every n samples (adds number of samples to output file).
      --output=<path>     Output file. <input>.<width>.<height>.<samples>.<technique>.exr if not specified.
      --reference=<path>  Reference file for comparison.
      --camera=<id>       Use camera with given id. [default: 0]
      --resolution=<WxH>  Resolution of output image. [default: 512x512]

)";

bool startsWith(const string& a, const string& b);

map<string, string> extractOptions(int argc, char const* const* argv) {
    map<string, string> result;

    for (int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "-h") == 0) {
            result.insert(make_pair<string, string>("--help", ""));
        }
        else if (strstr(argv[i], "-") == argv[i]) {
            const char* eq = strstr(argv[i], "=");

            if (eq == nullptr) {
                result.insert(make_pair<string, string>(argv[i], ""));
            }
            else {
                result.insert(make_pair(string((const char*)argv[i], eq), string(eq + 1)));
            }
        }
        else {
            result.insert(make_pair<string, string>("--input", argv[i]));
        }
    }

    return result;
}

bool isUnsigned(const string& s) {
    if (!s.empty()) {
        for (size_t i = 0; i < s.size(); ++i) {
            if (!std::isdigit(s[i])) {
                return false;
            }
        }
    }
    else {
        return false;
    }

    return true;
}

bool isReal(const string& s) {
    if (!s.empty()) {
        size_t i = 0;

        while (std::isdigit(s[i])) {
            ++i;
        }

        if (i == s.size()) {
            return true;
        }

        if (s[i] == '.') {
            ++i;
        }
        else {
            return false;
        }

        while (i < s.size()) {
            if (!std::isdigit(s[i])) {
                return false;
            }

            ++i;
        }
    }
    else {
        return false;
    }

    return true;
}

bool isResolution(const string& s) {
    if (s.empty()) {
        return false;
    }
    else {
        size_t i = 0;

        while (std::isdigit(s[i])) {
            ++i;
        }

        if (s[i] != 'x') {
            return false;
        }
        else {
            ++i;
        }

        if (i == s.size()) {
            return false;
        }

        while (i < s.size()) {
            if (!std::isdigit(s[i])) {
                return false;
            }

            ++i;
        }
    }

    return true;
}

Options parseAvgArgs(int argc, char const* const* argv) {
    Options options;

    if (argc != 3) {
        options.displayHelp = true;
        options.displayMessage = "Input file is required.";
    }
    else {
        options.action = Options::AVG;
        options.input0 = argv[2];
    }

    return options;
}

Options parseRmsArgs(int argc, char const* const* argv) {
    Options options;

    if (argc != 4) {
        options.displayHelp = true;
        options.displayMessage = "Input files are required.";
    }
    else {
        options.action = Options::RMS;
        options.input0 = argv[2];
        options.input1 = argv[3];
    }

    return options;
}

Options parseArgs(int argc, char const* const* argv) {
    if (1 < argc) {
        if (1 < argc && argv[1] == string("avg")) {
            return parseAvgArgs(argc, argv);
        }
        else if (1 < argc && argv[1] == string("rms")) {
            return parseRmsArgs(argc, argv);
        }
    }

    auto dict = extractOptions(argc, argv);
    Options options;

    if (argc < 2) {
        options.displayHelp = true;
        options.displayMessage = "Input file is required.";
        return options;
    }
    if (dict.count("--help")) {
        options.displayHelp = true;
        return options;
    }
    else if (dict.count("--version")) {
        options.displayVersion = true;
        return options;
    }
    else if (dict.count("--input") == 0) {
        options.displayHelp = true;
        options.displayMessage = "Input file is required.";
        return options;
    }
    else {
        options.input0 = dict["--input"];
        dict.erase("--input");

        size_t numTechniqes =
            dict.count("--BPT") +
            dict.count("--PT") +
            dict.count("--PM") +
            dict.count("--VCM");

        if (numTechniqes > 1) {
            options.displayHelp = true;
            options.displayMessage = "Only one technique can be specified.";
            return options;
        }
        else if (dict.count("--BPT")) {
            options.technique = Options::BPT;
            dict.erase("--BPT");
        }
        else if (dict.count("--PT")) {
            options.technique = Options::PT;
            dict.erase("--PT");
        }
        else if (dict.count("--PM")) {
            options.technique = Options::PM;
            dict.erase("--PM");
        }
        else if (dict.count("--VCM")) {
            options.technique = Options::VCM;
            dict.erase("--VCM");
        }
        else {
            options.technique = Options::Viewer;
        }

        if (dict.count("--num-photons")) {
            if (options.technique != Options::PM &&
                options.technique != Options::VCM) {
                options.displayHelp = true;
                options.displayMessage = "Number of photons can be specified for PM and VCM only.";
                return options;
            }
            else if (!isUnsigned(dict["--num-photons"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-photons.";
                return options;
            }
            else {
                options.numPhotons = atoi(dict["--num-photons"].c_str());
                dict.erase("--num-photons");
            }
        }

        if (dict.count("--num-gather")) {
            if (options.technique != Options::PM &&
                options.technique != Options::VCM) {
                options.displayHelp = true;
                options.displayMessage = "--num-gather can be specified for PM and VCM only.";
                return options;
            }
            else if (!isUnsigned(dict["--num-gather"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-gather.";
                return options;
            }
            else {
                options.numGather = atoi(dict["--num-gather"].c_str());
                dict.erase("--num-gather");
            }
        }

        if (dict.count("--max-radius")) {
            if (options.technique != Options::PM &&
                options.technique != Options::VCM) {
                options.displayHelp = true;
                options.displayMessage = "--max-radius can be specified for PM and VCM only.";
                return options;
            }
            else if (!isReal(dict["--max-radius"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --max-radius.";
                return options;
            }
            else {
                options.maxRadius = atof(dict["--max-radius"].c_str());
                dict.erase("--max-radius");
            }
        }

        if (dict.count("--min-subpath")) {
            if (options.technique != Options::BPT &&
                options.technique != Options::PT &&
                options.technique != Options::VCM) {
                options.displayHelp = true;
                options.displayMessage = "--min-subpath in not available for specified technique.";
                return options;
            }
            else if (!isUnsigned(dict["--min-subpath"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --min-subpath.";
                return options;
            }
            else {
                options.minSubpath = atoi(dict["--min-subpath"].c_str());
                dict.erase("--min-subpath");
            }
        }

        if (dict.count("--beta")) {
            if (options.technique != Options::BPT &&
                options.technique != Options::PT) {
                options.displayHelp = true;
                options.displayMessage = "--beta in not available for specified technique.";
                return options;
            }
            else if (!isReal(dict["--beta"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --beta.";
                return options;
            }
            else {
                options.beta = atof(dict["--beta"].c_str());
                dict.erase("--beta");
            }
        }

        if (dict.count("--roulette")) {
            if (options.technique != Options::BPT &&
                options.technique != Options::PT &&
                options.technique != Options::VCM) {
                options.displayHelp = true;
                options.displayMessage = "--roulette in not available for specified technique.";
                return options;
            }
            else if (!isReal(dict["--roulette"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --roulette.";
                return options;
            }
            else {
                options.roulette = atof(dict["--roulette"].c_str());

                if (options.roulette <= 0.0 || 1.0 < options.roulette)
                {
                    options.displayHelp = true;
                    options.displayMessage = "A value for --roulette must be in range (0, 1].";
                }

                dict.erase("--roulette");
            }
        }

        if (dict.count("--batch")) {
            options.batch = true;
            dict.erase("--batch");
        }

        if (dict.count("--quiet")) {
            options.quiet = true;
            dict.erase("--quiet");
        }

        if (dict.count("--no-reload")) {
            options.reload = false;
            dict.erase("--no-reload");
        }

        if (dict.count("--num-samples")) {
            if (!isUnsigned(dict["--num-samples"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-samples.";
                return options;
            }
            else {
                options.numSamples = atoi(dict["--num-samples"].c_str());
                dict.erase("--num-samples");
            }
        }

        if (dict.count("--num-minutes") && dict.count("--num-seconds")) {
            options.displayHelp = true;
            options.displayMessage = "--num-seconds and --num-minutes cannot be specified at the same time.";
            return options;
        }
        else if (dict.count("--num-minutes")) {
            if (!isReal(dict["--num-minutes"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-minutes.";
                return options;
            }
            else {
                options.numSeconds = atof(dict["--num-minutes"].c_str()) * 60.0;
                dict.erase("--num-minutes");
            }
        }
        else if (dict.count("--num-seconds")) {
            if (!isReal(dict["--num-seconds"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-seconds.";
                return options;
            }
            else {
                options.numSeconds = atof(dict["--num-seconds"].c_str());
                dict.erase("--num-seconds");
            }
        }

        if (dict.count("--parallel")) {
            options.parallel = true;
            dict.erase("--parallel");
        }

        if (dict.count("--snapshot")) {
            if (!isUnsigned(dict["--snapshot"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --snapshot.";
                return options;
            }
            else {
                options.snapshot = atoi(dict["--snapshot"].c_str());
                dict.erase("--snapshot");
            }
        }

        if (dict.count("--output")) {
            if (!options.output.empty()) {
                options.displayHelp = true;
                options.displayMessage = "Output cannot be specified twice.";
                return options;
            }
            else if (dict["--output"].empty()) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --output.";
                return options;
            }
            else {
                options.output = dict["--output"];
                dict.erase("--output");
            }
        }

        if (dict.count("--reference")) {
            if (!options.output.empty()) {
                options.displayHelp = true;
                options.displayMessage = "Output cannot be specified twice.";
                return options;
            }
            else if (dict["--reference"].empty()) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --reference.";
                return options;
            }
            else {
                options.reference = dict["--reference"];
                dict.erase("--reference");
            }
        }

        if (dict.count("--camera")) {
            if (!isUnsigned(dict["--camera"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --camera.";
                return options;
            }
            else {
                options.cameraId = atoi(dict["--camera"].c_str());
                dict.erase("--camera");
            }
        }

        if (dict.count("--resolution")) {
            if (!isResolution(dict["--resolution"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --resolution.";
                return options;
            }
            else {
                const char* w = dict["--resolution"].c_str();
                options.width = atoi(w);
                const char* h = strstr(w, "x") + 1;
                options.height = atoi(h);
                dict.erase("--resolution");
            }
        }

        if (dict.empty()) {
            return options;
        }
        else {
            options.displayHelp = true;
            options.displayMessage = "Unsupported option: " + dict.begin()->first + ".";
            return options;
        }
    }
}

pair<bool, int> displayHelpIfNecessary(
    const Options& options,
    const char* version)
{
    if (options.displayHelp) {

        if (options.displayMessage.empty()) {
            std::cout << "Master's project.\n";
            std::cout << help;
            return std::make_pair(true, 0);
        }
        else {
            std::cout << options.displayMessage << std::endl;
            std::cout << help;
            return std::make_pair(true, 1);
        }

    }
    else if (options.displayVersion) {
        if (version) {
            std::cout << version << std::endl;
            return std::make_pair(true, 0);
        }
        else {
            std::cout << "0.0.0" << std::endl;
            return std::make_pair(true, 1);
        }
    }

    return std::make_pair(false, 0);
}

shared<Technique> makeViewer(Options& options) {
    auto image = vector<vec4>();
    loadEXR(options.input0, options.width, options.height, image);
    return std::make_shared<Viewer>(image, options.width, options.height);
}

shared<Technique> makeTechnique(Options& options) {
    switch (options.technique) {
        case Options::BPT:
            if (options.beta == 0.0f) {
                return std::make_shared<BPT0>(options.minSubpath, options.roulette);
            }
            else if (options.beta == 1.0f) {
                return std::make_shared<BPT1>(options.minSubpath, options.roulette);
            }
            else if (options.beta == 2.0f) {
                return std::make_shared<BPT2>(options.minSubpath, options.roulette);
            }
            else {
                return std::make_shared<BPTb>(
                    options.minSubpath,
                    options.roulette,
                    options.beta);
            }

        case Options::PT:
            return std::make_shared<PathTracing>(
                options.minSubpath,
                options.roulette);

        case Options::PM:
            return std::make_shared<PhotonMapping>(
                options.numPhotons,
                options.numGather,
                options.maxRadius);

        case Options::VCM:
            return std::make_shared<VCM>(
                options.numPhotons,
                options.numGather,
                options.maxRadius,
                options.minSubpath,
                options.roulette);

        case Options::Viewer:
            return makeViewer(options);
    }
}

shared<Scene> loadScene(const Options& options) {
    return loadScene(options.input0);
}

string techniqueString(const Options& options) {
    switch (options.technique) {
        case Options::BPT: return "BPT";
        case Options::PT: return "PT";
        case Options::PM: return "PM";
        case Options::VCM: return "VCM";
        default: return "UNKNOWN";
    }
}

}
