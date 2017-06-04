#include <iostream>
#include <map>
#include <cstring>
#include <cctype>
#include <Options.hpp>
#include <loader.hpp>

#include <BPT.hpp>
#include <PT.hpp>
#include <UPG.hpp>
#include <Viewer.hpp>

#include <exr.hpp>

namespace haste {

using std::map;
using std::strstr;
using std::make_pair;
using namespace std::__cxx11;

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
      --radius=<n>       Use n as maximum gather radius. [default: 0.1]
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
      --output=<path>        Output file. <input>.<width>.<height>.<time>.<technique>.exr if not specified.
      --reference=<path>     Reference file for comparison.
      --seed=<n>             Seed random number generator (for single threaded BPT only).
      --snapshot=<n>         Save output every n seconds (tags output file with time).
      --camera=<id>          Use camera with given id. [default: 0]
      --resolution=<WxH>     Resolution of output image. [default: 512x512]

)";

string Options::caption() const {
    return input0 + " [" + to_string(this->technique) + "]";
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
                options.num_photons = atoi(dict["--num-photons"].c_str());
                dict.erase("--num-photons");
            }
        }

        if (dict.count("--radius")) {
            if (options.technique != Options::VCM &&
                options.technique != Options::UPG) {
                options.displayHelp = true;
                options.displayMessage = "--radius can be specified for PM, VCM and UPG.";
                return options;
            }
            else if (!isReal(dict["--radius"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --radius.";
                return options;
            }
            else {
                options.radius = atof(dict["--radius"].c_str());
                dict.erase("--radius");
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
                options.max_path = atoi(dict["--max-path"].c_str());
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

        if (dict.count("--batch") + dict.count("--interactive") + dict.count("--fast") > 1) {
            options.displayHelp = true;
            options.displayMessage = "Only one of --batch, --interactive and --fast can be used.";
            return options;
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
                options.num_samples = atoi(dict["--num-samples"].c_str());
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
                options.num_seconds = atof(dict["--num-minutes"].c_str()) * 60.0;
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
                options.num_seconds = atof(dict["--num-seconds"].c_str());
                dict.erase("--num-seconds");
            }
        }

        if (dict.count("--parallel")) {
            options.num_threads = 0;
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
            if (!options.reference.empty()) {
                options.displayHelp = true;
                options.displayMessage = "Reference cannot be specified twice.";
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

        if (dict.count("--seed")) {
            if (options.technique != Options::BPT &&
                options.technique != Options::UPG &&
                options.technique != Options::VCM) {
                options.displayHelp = true;
                options.displayMessage = "--seed is only valid with --BPT or --UPG.";
                return options;
            }
            else if (options.num_threads != 1) {
                options.displayHelp = true;
                options.displayMessage = "--seed is invalid with --parallel.";
                return options;
            }
            else if (!isUnsigned(dict["--seed"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --seed.";
                return options;
            }
            else {
                options.enable_seed = true;
                options.seed = atoi(dict["--seed"].c_str());
                dict.erase("--seed");
            }
        }

        if (dict.count("--camera")) {
            if (!isUnsigned(dict["--camera"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --camera.";
                return options;
            }
            else {
                options.camera_id = atoi(dict["--camera"].c_str());
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

        if (options.num_photons == 0) {
            options.num_photons = options.width * options.height;
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
    auto metadata = map<string, string>();

    size_t width = 0;
    size_t height = 0;

    load_exr(options.input0, metadata, width, height, data);

    return std::make_shared<Viewer>(vv3f_to_vv4d(data), width, height);
}

template <class T>
shared<Technique> make_bpt_technique(const shared<const Scene>& scene, const Options& options) {
    return std::make_shared<T>(
        scene,
        options.lights,
        options.roulette,
        options.beta,
        options.num_threads);
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
        options.num_photons,
        options.radius,
        options.alpha,
        options.beta,
        options.num_threads);
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
                options.max_path,
                options.num_threads);

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

string to_string(const Options::Technique& technique) {
    switch (technique) {
        case Options::BPT: return "BPT";
        case Options::PT: return "PT";
        case Options::VCM: return "VCM";
        case Options::UPG: return "UPG";
        case Options::Viewer: return "Viewer";
        default: return "UNKNOWN";
    }
}


Options::Technique technique(string technique) {
    if (technique == "BPT")
        return Options::BPT;
    else if (technique == "PT")
        return Options::PT;
    else if (technique == "VCM")
        return Options::VCM;
    else if (technique == "UPG")
        return Options::UPG;
    else if (technique == "Viewer")
        return Options::Viewer;
    else
        throw std::invalid_argument("technique");
}

string to_string(const Options::Action& action) {
    switch (action) {
        case Options::Render: return "Render";
        case Options::AVG: return "AVG";
        case Options::SUB: return "SUB";
        case Options::Errors: return "Errors";
        case Options::Merge: return "Merge";
        case Options::Filter: return "Filter";
        case Options::Time: return "Time";
        default: return "UNKNOWN";
    }
}

Options::Action action(string action) {
    if (action == "Render")
        return Options::Render;
    else if (action == "AVG")
        return Options::AVG;
    else if (action == "SUB")
        return Options::SUB;
    else if (action == "Errors")
        return Options::Errors;
    else if (action == "Merge")
        return Options::Merge;
    else if (action == "Filter")
        return Options::Filter;
    else if (action == "Time")
        return Options::Time;
    else
        throw std::invalid_argument("action");
}

map<string, string> Options::to_dict()
{
    using std::to_string;

    map<string, string> result;
    result["input0"] = input0;
    result["input1"] = input1;
    result["output"] = output;
    result["reference"] = reference;
    result["technique"] = to_string(technique);
    result["action"] = to_string(action);
    result["num_photons"] = to_string(num_photons);
    result["radius"] = to_string(radius);
    result["max_path"] = to_string(max_path);
    result["alpha"] = to_string(alpha);
    result["beta"] = to_string(beta);
    result["roulette"] = to_string(roulette);
    result["batch"] = to_string(batch);
    result["quiet"] = to_string(quiet);
    result["enable_vc"] = to_string(enable_vc);
    result["enable_vm"] = to_string(enable_vm);
    result["lights"] = to_string(lights);
    result["num_samples"] = to_string(num_samples);
    result["num_seconds"] = to_string(num_seconds);
    result["num_threads"] = to_string(num_threads);
    result["reload"] = to_string(reload);
    result["enable_seed"] = to_string(enable_seed);
    result["seed"] = to_string(seed);
    result["snapshot"] = to_string(snapshot);
    result["camera_id"] = to_string(camera_id);
    result["width"] = to_string(width);
    result["height"] = to_string(height);
    return result;
}

string Options::get_output() const {
    return !output.empty()
        ? output
        : make_output_path(
            input0,
            width,
            height,
            num_samples,
            snapshot,
            to_string(technique));
}


Options dict_to_config(const map<string, string>& dict)
{
    Options result;

    result.input0 = dict.find("input0")->second;
    result.input1 = dict.find("input1")->second;
    result.output = dict.find("output")->second;
    result.reference = dict.find("reference")->second;
    result.technique = haste::technique(dict.find("technique")->second);
    result.action = haste::action(dict.find("action")->second);
    result.num_photons = stoll(dict.find("num_photons")->second);
    result.radius = stod(dict.find("radius")->second);
    result.max_path = stoll(dict.find("max_path")->second);
    result.alpha = stod(dict.find("alpha")->second);
    result.beta = stod(dict.find("beta")->second);
    result.roulette = stod(dict.find("roulette")->second);
    result.batch = stoi(dict.find("batch")->second);
    result.quiet = stoi(dict.find("quiet")->second);
    result.enable_vc = stoi(dict.find("enable_vc")->second);
    result.enable_vm = stoi(dict.find("enable_vm")->second);
    result.lights = stod(dict.find("lights")->second);
    result.num_samples = stoll(dict.find("num_samples")->second);
    result.num_seconds = stod(dict.find("num_seconds")->second);
    result.num_threads = stoll(dict.find("num_threads")->second);
    result.reload = stoi(dict.find("reload")->second);
    result.enable_seed = stoi(dict.find("enable_seed")->second);
    result.seed = stoll(dict.find("seed")->second);
    result.snapshot = stoll(dict.find("snapshot")->second);
    result.camera_id = stoll(dict.find("camera_id")->second);
    result.width = stoll(dict.find("width")->second);
    result.height = stoll(dict.find("height")->second);

    return result;
}

void save_exr(Options options, statistics_t statistics, const vec3* data) {
  auto metadata = statistics.to_dict();
  auto local_options = options.to_dict();
  metadata.insert(local_options.begin(), local_options.end());

  save_exr(options.get_output(), metadata, options.width, options.height, data);
}

void save_exr(Options options, statistics_t statistics, const vec4* data) {
  auto metadata = statistics.to_dict();
  auto local_options = options.to_dict();
  metadata.insert(local_options.begin(), local_options.end());

  save_exr(options.get_output(), metadata, options.width, options.height, data);
}

void save_exr(Options options, statistics_t statistics, const dvec3* data) {
  size_t size = options.width * options.height;
  vector<vec3> fdata(size);

  std::transform(
    data,
    data + size,
    fdata.begin(),
    [](dvec3 x) { return vec3(x); });

  save_exr(options, statistics, fdata.data());
}

void save_exr(Options options, statistics_t statistics, const dvec4* data) {
  size_t size = options.width * options.height;
  vector<vec4> fdata(size);

  std::transform(
    data,
    data + size,
    fdata.begin(),
    [](dvec4 x) { return vec4(x); });

  save_exr(options, statistics, fdata.data());
}

}
