#include <map>
#include <cstring>
#include <Options.hpp>

namespace haste {

using std::map;
using std::strstr;
using std::make_pair;

static const char help[] =
R"(Master's project.

    Usage:
      master <input> [<output>] [options]
      master (-h | --help)
      master --version

    Options:
      -h --help             Show this screen.
      --version             Show version.
      --PT                  Use path tracing for rendering (this is default one).
      --PM                  Use photon mapping for rendering.
      --num-photons=<n>     Use n photons. [default: 1 000 000]
      --max-gather=<n>      Use n as maximal number of gathered photons. [default: 100]
      --max-radius=<n>      Use n as maximum gather radius. [default: 0.1]
      --batch               Run in batch mode (interactive otherwise).
      --num-samples=<n>     Terminate after n samples.
      --num-seconds=<n>     Terminate after n seconds.
      --num-minutes=<n>     Terminate after n minutes.
      --num-jobs=<n>        Use n threads. [default: 1]
      --snapshot=<n>        Save output every n samples (adds number of samples to output file).
      --output=<path>       Output file. <input>.<samples>.exr if not specified.
      --camera=<id>         Use camera with given id. [default: 0]
)";

bool startsWith(const string& a, const string& b);

map<string, string> extractOptions(int argc, char const* const* argv) {
    map<string, string> result;

    for (int i = 1; i < argc; ++i) {
        if (strstr(argv[i], "--") == argv[i]) {
            const char* eq = strstr(argv[i], "=");

            if (eq == nullptr) {
                result.insert(make_pair<string, string>(argv[i], ""));
            }
            else {
                result.insert(make_pair(string((const char*)argv[i], eq), string(eq + 1)));
            }
        }
        else if(strcmp(argv[i], "-h") == 0) {
            result.insert(make_pair<string, string>("--help", ""));
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

    return true;
}

Options parseArgs(int argc, char const* const* argv) {
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
    else if (strstr(argv[1], "-") == argv[1]) {
        options.displayHelp = true;
        options.displayMessage = "Input file is required.";
        return options;
    }
    else {
        options.input = argv[1];

        if (argc >= 3 && strstr(argv[2], "-") != argv[2]) {
            options.output = argv[2];
        }

        if (dict.count("--PT") && dict.count("--PM")) {
            options.displayHelp = true;
            options.displayMessage = "Only one technique can be specified.";
            return options;
        }
        else if (dict.count("--PT")) {
            options.technique = Options::PT;
            dict.erase("--PT");
        }
        else if (dict.count("--PM")) {
            options.technique = Options::PM;
            dict.erase("--PM");
        }

        if (dict.count("--num-photons")) {
            if (options.technique != Options::PM) {
                options.displayHelp = true;
                options.displayMessage = "Number of photons can be specified for photon mapping only.";
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

        if (dict.count("--max-gather")) {
            if (options.technique != Options::PM) {
                options.displayHelp = true;
                options.displayMessage = "--max-gather can be specified for PM only.";
                return options;
            }
            else if (!isUnsigned(dict["--max-gather"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --max-gather.";
                return options;
            }
            else {
                options.maxGather = atoi(dict["--max-gather"].c_str());
                dict.erase("--max-gather");
            }
        }

        if (dict.count("--max-radius")) {
            if (options.technique != Options::PM) {
                options.displayHelp = true;
                options.displayMessage = "--max-radius can be specified for PM only.";
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

        if (dict.count("--batch")) {
            options.batch = true;
            dict.erase("--batch");
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

        if (dict.count("--num-jobs")) {
            if (!isUnsigned(dict["--num-jobs"])) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-jobs.";
                return options;
            }
            else {
                options.numJobs = atoi(dict["--num-jobs"].c_str());
                dict.erase("--num-jobs");
            }
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

}
