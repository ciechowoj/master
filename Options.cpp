#include <iostream>
#include <map>
#include <cstring>
#include <Options.hpp>
#include <loader.hpp>

#include <BPT.hpp>
#include <PT.hpp>
#include <UPG.hpp>
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
      master avg <x>         Compute average value of pixels in <x>.
      master errors <x> <y>  Compute abs and rms (in this order) error between <x> and <y>.
      master sub <x> <y>     Compute difference between <x> and <y>.

    Options:
      -h --help              Show this screen.
      --version              Show version.
      --PT                   Use path tracing for rendering (this is default one).
      --BPT                  Use bidirectional path tracing (balance heuristics).
      --VCM                  Use vertex connection and merging.
      --UPG                  Use unbiased photon gathering.
      --num-photons=<n>      Use n photons. [default: 1 000 000]
      --max-radius=<n>       Use n as maximum gather radius. [default: 0.1]
      --roulette=<n>         Russian roulette coefficient. [default: 0.5]
      --beta=<n>             MIS beta. [default: 1]
      --alpha=<n>            VCM alpha. [default: 0.75]
      --batch                Run in batch mode (interactive otherwise).
      --quiet                Do not output anything to console.
      --no-vc                Disable vertex connection.
      --no-vm                Disable vertex merging.
      --no-lights            Do not draw the lights.
      --no-reload            Disable auto-reload (input file is reloaded on modification in interactive mode).
      --num-samples=<n>      Terminate after n samples.
      --num-seconds=<n>      Terminate after n seconds.
      --num-minutes=<n>      Terminate after n minutes.
      --parallel             Use multi-threading.
      --snapshot=<n>         Save output every n seconds (tags output file with time).
      --output=<path>        Output file. <input>.<width>.<height>.<samples>.<technique>.exr if not specified.
      --reference=<path>     Reference file for comparison.
      --camera=<id>          Use camera with given id. [default: 0]
      --resolution=<WxH>     Resolution of output image. [default: 512x512]

)";

string Options::caption() const {
    return input0 + " [" + techniqueString(*this) + "]";
}

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

Options parseErrorsArgs(int argc, char const* const* argv) {
    Options options;

    if (argc != 4) {
        options.displayHelp = true;
        options.displayMessage = "Input files are required.";
    }
    else {
        options.action = Options::Errors;
        options.input0 = argv[2];
        options.input1 = argv[3];
    }

    return options;
}

Options parseSubArgs(int argc, char const* const* argv) {
    Options options;

    if (argc != 5) {
        options.displayHelp = true;
        options.displayMessage = "Input files are required.";
    }
    else {
        options.action = Options::SUB;
        options.output = argv[2];
        options.input0 = argv[3];
        options.input1 = argv[4];
    }

    return options;
}

Options parseMergeArgs(int argc, char const* const* argv) {
    Options options;

    if (argc != 5) {
        options.displayHelp = true;
        options.displayMessage = "Input files are required.";
    }
    else {
        options.action = Options::Merge;
        options.output = argv[2];
        options.input0 = argv[3];
        options.input1 = argv[4];
    }

    return options;
}

Options parseFilterArgs(int argc, char const* const* argv) {
    Options options;

    if (argc != 4) {
        options.displayHelp = true;
        options.displayMessage = "Input file is required.";
    }
    else {
        options.action = Options::Filter;
        options.output = argv[2];
        options.input0 = argv[3];
    }

    return options;
}

Options parseTimeArgs(int argc, char const* const* argv) {
    Options options;

    if (argc != 3) {
        options.displayHelp = true;
        options.displayMessage = "Input file is required.";
    }
    else {
        options.action = Options::Time;
        options.input0 = argv[2];
    }

    return options;
}

Options parseArgs(int argc, char const* const* argv) {
    if (1 < argc) {
        if (1 < argc && argv[1] == string("avg")) {
            return parseAvgArgs(argc, argv);
        }
        else if (1 < argc && argv[1] == string("sub")) {
            return parseSubArgs(argc, argv);
        }
        else if (1 < argc && argv[1] == string("errors")) {
            return parseErrorsArgs(argc, argv);
        }
        else if (1 < argc && argv[1] == string("merge")) {
            return parseMergeArgs(argc, argv);
        }
        else if (1 < argc && argv[1] == string("filter")) {
            return parseFilterArgs(argc, argv);
        }
        else if (1 < argc && argv[1] == string("time")) {
            return parseTimeArgs(argc, argv);
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
            dict.count("--VCM") +
            dict.count("--UPG");

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
        else if (dict.count("--VCM")) {
            options.technique = Options::VCM;
            dict.erase("--VCM");
        }
        else if (dict.count("--UPG")) {
            options.technique = Options::UPG;
            dict.erase("--UPG");
        }
        else {
            options.technique = Options::Viewer;
        }

        if (dict.count("--num-photons")) {
            if (options.technique != Options::VCM &&
                options.technique != Options::UPG) {
                options.displayHelp = true;
                options.displayMessage = "Number of photons can be specified for PM, VCM and UPG.";
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

        if (dict.count("--max-radius")) {
            if (options.technique != Options::VCM &&
                options.technique != Options::UPG) {
                options.displayHelp = true;
                options.displayMessage = "--max-radius can be specified for PM, VCM and UPG.";
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

        if (dict.count("--max-path")) {
            if (options.technique != Options::PT) {
                options.displayHelp = true;
                options.displayMessage = "--max-path in not available for specified technique.";
                return options;
            }
            else if (!isUnsigned(dict["--max-path"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --max-path.";
                return options;
            }
            else {
                options.maxPath = atoi(dict["--max-path"].c_str());
                dict.erase("--max-path");
            }
        }

        if (dict.count("--beta")) {
            if (options.technique != Options::BPT &&
                options.technique != Options::PT &&
                options.technique != Options::VCM &&
                options.technique != Options::UPG) {
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

        if (dict.count("--alpha")) {
            if (options.technique != Options::VCM) {
                options.displayHelp = true;
                options.displayMessage = "--alpha is valid only for VCM.";
                return options;
            }
            else if (!isReal(dict["--alpha"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --alpha.";
                return options;
            }
            else {
                options.alpha = atof(dict["--alpha"].c_str());
                dict.erase("--alpha");
            }
        }

        if (dict.count("--roulette")) {
            if (options.technique != Options::BPT &&
                options.technique != Options::PT &&
                options.technique != Options::VCM &&
                options.technique != Options::UPG) {
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

        if (dict.count("--no-vc")) {
            if (options.technique != Options::VCM &&
                options.technique != Options::UPG) {
                options.displayHelp = true;
                options.displayMessage = "--no-vc is only valid with --VCM and --UPG.";
                return options;
            }

            options.enable_vc = false;
            dict.erase("--no-vc");
        }

        if (dict.count("--no-vm")) {
            if (options.technique != Options::VCM &&
                options.technique != Options::UPG) {
                options.displayHelp = true;
                options.displayMessage = "--no-vm is only valid with --VCM and --UPG.";
                return options;
            }

            options.enable_vm = false;
            dict.erase("--no-vm");
        }

        if (dict.count("--no-lights")) {
            options.lights = 0.0f;
            dict.erase("--no-lights");
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
            options.numThreads = 0;
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

        if (options.numPhotons == 0) {
            options.numPhotons = options.width * options.height;
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
    auto data = vector<vec3>();
    auto meta = metadata_t();
    loadEXR(options.input0, meta, data);

    std::cout << meta << std::endl;

    return std::make_shared<Viewer>(vv3f_to_vv4d(data), meta.resolution.x, meta.resolution.y);
}

template <class T>
shared<Technique> make_bpt_technique(const shared<const Scene>& scene, const Options& options) {
    return std::make_shared<T>(
        scene,
        options.lights,
        options.roulette,
        options.beta,
        options.numThreads);
}

template <class T>
shared<Technique> make_upg_technique(const shared<const Scene>& scene, const Options& options) {
    return std::make_shared<T>(
        scene,
        options.technique == Options::UPG,
        options.enable_vc,
        options.enable_vm,
        options.lights,
        options.roulette,
        options.numPhotons,
        options.maxRadius,
        options.alpha,
        options.beta,
        options.numThreads);
}

shared<Technique> makeTechnique(const shared<const Scene>& scene, Options& options) {
    switch (options.technique) {
        case Options::BPT:
            if (options.beta == 0.0f) {
                return make_bpt_technique<BPT0>(scene, options);
            }
            else if (options.beta == 1.0f) {
                return make_bpt_technique<BPT1>(scene, options);
            }
            else if (options.beta == 2.0f) {
                return make_bpt_technique<BPT2>(scene, options);
            }
            else {
                return make_bpt_technique<BPTb>(scene, options);
            }

        case Options::PT:
            return std::make_shared<PathTracing>(
                scene,
                options.lights,
                options.roulette,
                options.beta,
                options.maxPath,
                options.numThreads);

        case Options::VCM:
        case Options::UPG:
            if (options.beta == 0.0f) {
                return make_upg_technique<UPG0>(scene, options);
            }
            else if (options.beta == 1.0f) {
                return make_upg_technique<UPG1>(scene, options);
            }
            else if (options.beta == 2.0f) {
                return make_upg_technique<UPG2>(scene, options);
            }
            else {
                return make_upg_technique<UPGb>(scene, options);
            }

        default:
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
        case Options::VCM: return "VCM";
        case Options::UPG: return "UPG";
        case Options::Viewer: return "Viewer";
        default: return "UNKNOWN";
    }
}

}
