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
#include <system_utils.hpp>

namespace haste {

using std::map;
using std::strstr;
using std::make_pair;

#if !defined _MSC_VER
using namespace std::__cxx11;
#endif

static const char help[] =
R"(
    Usage:
      master <input> [options]
      master (-h | --help)
      master --version
      master avg <x>              Compute average value of pixels in <x>.
      master errors <x> <y>       Compute abs and rms (in this order) error between <x> and <y>.
      master sub <x> <y>          Compute difference between <x> and <y>.
      master strip <o> <i>        Remove metadata from <o> and save the result to <i>.
      master merge <o> <a> <b>    Merge <a> and <b> into <o>.
      master gnuplot <inputs>...  Create convergence charts.
      master bake <input>         Remove the channel with number of samples from the image <input>.

    Options (master):
      -h --help                   Show this screen.
      --version                   Show version.
      --PT                        Use path tracing for rendering (this is default one).
      --BPT                       Use bidirectional path tracing (balance heuristics).
      --VCM                       Use vertex connection and merging.
      --UPG                       Use unbiased photon gathering.
      --num-photons=<n>           Use n photons. [default: 1 000 000]
      --radius=<n>                Use n as maximum gather radius. [default: 0.1]
      --roulette=<n>              Russian roulette coefficient. [default: 0.5]
      --beta=<n>                  MIS beta. [default: 1]
      --alpha=<n>                 VCM alpha. [default: 0.75]
      --batch                     Run in batch mode (interactive otherwise).
      --quiet                     Do not output anything to console.
      --no-vc                     Disable vertex connection.
      --no-vm                     Disable vertex merging.
      --from-camera               Merge from camera perspective.
      --from-light                Merge from light perspective.
      --no-lights                 Do not draw the lights.
      --no-reload                 Disable auto-reload (input file is reloaded on modification in interactive mode).
      --num-samples=<n>           Terminate after n samples.
      --num-seconds=<n>           Terminate after n seconds.
      --num-minutes=<n>           Terminate after n minutes.
      --parallel                  Use multi-threading.
      --output=<path>             Output file. <input>.<width>.<height>.<time>.<technique>.exr if not specified.
      --reference=<path>          Reference file for comparison.
      --seed=<n>                  Seed random number generator (for single threaded BPT only).
      --snapshot=<n>              Save output every n seconds (tags output file with time).
      --camera=<id>               Use camera with given id. [default: 0]
      --resolution=<WxH>          Resolution of output image. [default: 512x512]
      --trace=<XxY[xW]>           Trace errors in window of radius W and at the center at XxY. [default: XxYx2]
      --sky-horizon=<RxGxB>       Color of sky horizon. [default: 0x0x0]
      --sky-zenith=<RxGxB>        Color of sky zenith. [default: 0x0x0]
      --blue-sky=<B>              Alias to --sky-horizon=<0x0x0> --sky-zenith<0x0xB>. [default: 0]

    Options (gnuplot):
      --input=<path>              An exr file containing errors data.
      --output                    A output image with chart.
      --traces                    Generate a matrix of charts with data from window traces.
      --select=<wildcard>         Consider only series which name matches <wildcard>.
      --error=<type>              Specify the type of error rms/abs.
)";

string Options::caption() const {
    return input0 + " [" + to_string(this->technique) + "]";
}

ivec2 parse_xnotation2(const string& s) {
  ivec2 result;
  const char* x = s.c_str();
  result.x = atoi(x);
  const char* y = strstr(x, "x") + 1;
  result.y = atoi(y);
  return result;
}

ivec3 parse_xnotation3(const string& s, int default_z) {
  ivec3 result;
  const char* x = s.c_str();
  result.x = atoi(x);
  const char* y = strstr(x, "x") + 1;
  result.y = atoi(y);
  const char* z = strstr(y, "x");

  if (z != nullptr) {
    result.z = atoi(z + 1);
  }
  else {
    result.z = default_z;
  }

  return result;
}

vec3 parse_xnotation3f(const string& s) {
  ivec3 result;
  const char* x = s.c_str();
  result.x = atof(x);
  const char* y = strstr(x, "x") + 1;
  result.y = atof(y);
  const char* z = strstr(y, "x");
  result.z = atof(z + 1);

  return result;
}

std::multimap<string, string> extractOptions(int argc, char const* const* argv) {
    std::multimap<string, string> result;

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

void displayHelp() {
  std::cout << "Master's project.\n";
  std::cout << help;
}

bool displayHelp(const std::multimap<string, string>& options) {
  auto itr = options.find("--help");

  if (itr != options.end()) {
    displayHelp();
    return true;
  }

  itr = options.find("--version");

  if (itr != options.end()) {
    std::cout << "0.9.0" << std::endl;
    return true;
  }

  return false;
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
        options.action = Options::Average;
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
        options.action = Options::Subtract;
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

Options parseSingleInputFile(int argc, char const* const* argv, Options::Action action) {
  Options options;

  if (argc < 3) {
    options.displayHelp = true;
    options.displayMessage = "Input file is required.";
  }
  else {
    options.action = action;
    options.input0 = fullpath(argv[2]);
  }

  return options;
}

Options parseInputOutput(int argc, char const* const* argv, Options::Action action) {
    Options options;

    if (argc != 4) {
        options.displayHelp = true;
        options.displayMessage = "Input and output files are required.";
    }
    else {
        options.action = action;
        options.output = argv[2];
        options.input0 = argv[3];
    }

    return options;
}

Options::Action parseAction(const char* argument) {
  static const std::map<const string, Options::Action> map = {
    { "average", Options::Action::Average },
    { "subtract", Options::Action::Subtract },
    { "errors", Options::Action::Errors },
    { "strip", Options::Action::Strip },
    { "merge", Options::Action::Merge },
    { "time", Options::Action::Time },
    { "continue", Options::Action::Continue },
    { "statistics", Options::Action::Statistics },
    { "measurements", Options::Action::Measurements },
    { "gnuplot", Options::Action::Gnuplot },
    { "bake", Options::Action::Bake }
  };

  auto itr = map.find(argument);

  if (itr != map.end()) {
    return itr->second;
  }
  else {
    return Options::Action::Render;
  }
}


Options parseArgs(int argc, char const* const* argv) {
  if (1 < argc) {
    auto action = parseAction(argv[1]);

    switch (action) {
      case Options::Action::Average:
        return parseAvgArgs(argc, argv);
      case Options::Action::Subtract:
        return parseSubArgs(argc, argv);
      case Options::Action::Errors:
        return parseErrorsArgs(argc, argv);
      case Options::Action::Strip:
        return parseInputOutput(argc, argv, Options::Strip);
      case Options::Action::Merge:
        return parseMergeArgs(argc, argv);
      case Options::Action::Time:
        return parseSingleInputFile(argc, argv, Options::Time);
      case Options::Action::Continue:
        return parseSingleInputFile (argc, argv, Options::Continue);
      case Options::Action::Statistics:
        return parseSingleInputFile(argc, argv, Options::Statistics);
      case Options::Action::Measurements:
        return parseSingleInputFile(argc, argv, Options::Measurements);
      case Options::Action::Gnuplot:
        return Options(Options::Action::Gnuplot);
      case Options::Action::Bake:
        return Options(Options::Action::Bake);
      default:
        break;
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
        options.input0 = fullpath(dict.find("--input")->second);
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
            options.input1 = options.input0;
        }

        if (dict.count("--num-photons")) {
            if (options.technique != Options::VCM &&
                options.technique != Options::UPG) {
                options.displayHelp = true;
                options.displayMessage = "Number of photons can be specified for PM, VCM and UPG.";
                return options;
            }
            else if (!isUnsigned(dict.find("--num-photons")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-photons.";
                return options;
            }
            else {
                options.num_photons = atoi(dict.find("--num-photons")->second.c_str());
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
            else if (!isReal(dict.find("--radius")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --radius.";
                return options;
            }
            else {
                options.radius = atof(dict.find("--radius")->second.c_str());
                dict.erase("--radius");
            }
        }

        if (dict.count("--max-path")) {
            if (options.technique != Options::PT) {
                options.displayHelp = true;
                options.displayMessage = "--max-path in not available for specified technique.";
                return options;
            }
            else if (!isUnsigned(dict.find("--max-path")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --max-path.";
                return options;
            }
            else {
                options.max_path = atoi(dict.find("--max-path")->second.c_str());
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
            else if (!isReal(dict.find("--beta")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --beta.";
                return options;
            }
            else {
                options.beta = atof(dict.find("--beta")->second.c_str());
                dict.erase("--beta");
            }
        }

        if (dict.count("--alpha")) {
            if (options.technique != Options::VCM) {
                options.displayHelp = true;
                options.displayMessage = "--alpha is valid only for VCM.";
                return options;
            }
            else if (!isReal(dict.find("--alpha")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --alpha.";
                return options;
            }
            else {
                options.alpha = atof(dict.find("--alpha")->second.c_str());
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
            else if (!isReal(dict.find("--roulette")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --roulette.";
                return options;
            }
            else {
                options.roulette = atof(dict.find("--roulette")->second.c_str());

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

        size_t num_from_light =
          dict.count("--from-light") +
          dict.count("--from-camera");

        if (num_from_light > 1) {
          options.displayHelp = true;
          options.displayMessage = "--from-light and --from-camera cannot be specified at the same time.";
          return options;
        }

        if (dict.count("--from-light")) {
          if (options.technique != Options::VCM &&
            options.technique != Options::UPG) {
            options.displayHelp = true;
            options.displayMessage = "--from-light is only valid with --VCM and --UPG.";
            return options;
          }

          options.from_light = true;
          dict.erase("--from-light");
        }

        if (dict.count("--from-camera")) {
          if (options.technique != Options::VCM &&
            options.technique != Options::UPG) {
            options.displayHelp = true;
            options.displayMessage = "--from-camera is only valid with --VCM and --UPG.";
            return options;
          }

          options.from_light = false;
          dict.erase("--from-camera");
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
            if (!isUnsigned(dict.find("--num-samples")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-samples.";
                return options;
            }
            else {
                options.num_samples = atoi(dict.find("--num-samples")->second.c_str());
                dict.erase("--num-samples");
            }
        }

        if (dict.count("--num-minutes") && dict.count("--num-seconds")) {
            options.displayHelp = true;
            options.displayMessage = "--num-seconds and --num-minutes cannot be specified at the same time.";
            return options;
        }
        else if (dict.count("--num-minutes")) {
            if (!isReal(dict.find("--num-minutes")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-minutes.";
                return options;
            }
            else {
                options.num_seconds = atof(dict.find("--num-minutes")->second.c_str()) * 60.0;
                dict.erase("--num-minutes");
            }
        }
        else if (dict.count("--num-seconds")) {
            if (!isReal(dict.find("--num-seconds")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --num-seconds.";
                return options;
            }
            else {
                options.num_seconds = atof(dict.find("--num-seconds")->second.c_str());
                dict.erase("--num-seconds");
            }
        }

        if (dict.count("--parallel")) {
            options.num_threads = 0;
            dict.erase("--parallel");
        }

        if (dict.count("--snapshot")) {
            if (!isUnsigned(dict.find("--snapshot")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --snapshot.";
                return options;
            }
            else {
                options.snapshot = atoi(dict.find("--snapshot")->second.c_str());
                dict.erase("--snapshot");
            }
        }

        if (dict.count("--output")) {
            if (!options.output.empty()) {
                options.displayHelp = true;
                options.displayMessage = "Output cannot be specified twice.";
                return options;
            }
            else if (dict.find("--output")->second.empty()) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --output.";
                return options;
            }
            else {
                options.output = fullpath(dict.find("--output")->second);
                dict.erase("--output");
            }
        }

        while (dict.count("--trace") != 0) {
          ivec2 trace;

          if (dict.count("--reference") == 0) {
            options.displayHelp = true;
            options.displayMessage = "The --trace switch can only be used together with --reference.";
            return options;
          }

          auto itr = dict.find("--trace");
          options.trace.push_back(parse_xnotation3(itr->second, 1));
          dict.erase(itr);
        }

        if (dict.count("--reference")) {
            if (!options.reference.empty()) {
                options.displayHelp = true;
                options.displayMessage = "Reference cannot be specified twice.";
                return options;
            }
            else if (dict.find("--reference")->second.empty()) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --reference.";
                return options;
            }
            else {
                options.reference = fullpath(dict.find("--reference")->second);
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
            else if (!isUnsigned(dict.find("--seed")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --seed.";
                return options;
            }
            else {
                options.enable_seed = true;
                options.seed = atoi(dict.find("--seed")->second.c_str());
                dict.erase("--seed");
            }
        }

        if (dict.count("--camera")) {
            if (!isUnsigned(dict.find("--camera")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --camera.";
                return options;
            }
            else {
                options.camera_id = atoi(dict.find("--camera")->second.c_str());
                dict.erase("--camera");
            }
        }

        if (dict.count("--resolution")) {
            if (!isResolution(dict.find("--resolution")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --resolution.";
                return options;
            }
            else {
                const char* w = dict.find("--resolution")->second.c_str();
                options.width = atoi(w);
                const char* h = strstr(w, "x") + 1;
                options.height = atoi(h);
                dict.erase("--resolution");
            }
        }

        if (dict.count("--blue-sky")) {
            if (dict.count("--sky-horizon") || dict.count("--sky-zenith")) {
                options.displayHelp = true;
                options.displayMessage = "--blue-sky cannot be specified together with --sky-horizon or --sky-zenith.";
                return options;
            }
            if (!isReal(dict.find("--blue-sky")->second)) {
                options.displayHelp = true;
                options.displayMessage = "Invalid value for --blue-sky.";
                return options;
            }
            else {
                float intensity = atof(dict.find("--blue-sky")->second.c_str());

                if (intensity < 0.0)
                {
                  options.displayHelp = true;
                  options.displayMessage = "A value for --blue-sky must not be nagative.";
                }

                options.sky_horizon = vec3(intensity, intensity, intensity);
                options.sky_zenith = vec3(0, 0, intensity);
                dict.erase("--blue-sky");
            }
        }

        if (dict.count("--sky-horizon")) {
            options.sky_horizon = parse_xnotation3f(dict.find("--sky-horizon")->second);
            dict.erase("--sky-horizon");
        }

        if (dict.count("--sky-zenith")) {
            options.sky_zenith = parse_xnotation3f(dict.find("--sky-zenith")->second);
            dict.erase("--sky-zenith");
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

void overrideArgs(Options& options, int argc, const char* const* argv)
{
    auto dict = extractOptions(argc, argv);

    dict.erase("--input");

    if (dict.count("--snapshot")) {
        if (!isUnsigned(dict.find("--snapshot")->second)) {
            options.displayHelp = true;
            options.displayMessage = "Invalid value for --snapshot.";
            return;
        }
        else {
            options.snapshot = atoi(dict.find("--snapshot")->second.c_str());
            dict.erase("--snapshot");
        }
    }

    if (dict.count("--num-samples")) {
        if (!isUnsigned(dict.find("--num-samples")->second)) {
            options.displayHelp = true;
            options.displayMessage = "Invalid value for --num-samples.";
            return;
        }
        else {
            options.num_samples += atoi(dict.find("--num-samples")->second.c_str());
            dict.erase("--num-samples");
        }
    }
    else {
        options.num_samples = 0;
    }

    if (dict.count("--num-minutes") && dict.count("--num-seconds")) {
        options.displayHelp = true;
        options.displayMessage = "--num-seconds and --num-minutes cannot be specified at the same time.";
        return;
    }
    else if (dict.count("--num-minutes")) {
        if (!isReal(dict.find("--num-minutes")->second)) {
            options.displayHelp = true;
            options.displayMessage = "Invalid value for --num-minutes.";
            return;
        }
        else {
            options.num_seconds += atof(dict.find("--num-minutes")->second.c_str()) * 60.0;
            dict.erase("--num-minutes");
        }
    }
    else if (dict.count("--num-seconds")) {
        if (!isReal(dict.find("--num-seconds")->second)) {
            options.displayHelp = true;
            options.displayMessage = "Invalid value for --num-seconds.";
            return;
        }
        else {
            options.num_seconds += atof(dict.find("--num-seconds")->second.c_str());
            dict.erase("--num-seconds");
        }
    }
    else {
        options.num_seconds = 0;
    }

    if (dict.empty()) {
        return;
    }
    else {
        options.displayHelp = true;
        options.displayMessage = "Unsupported override: " + dict.begin()->first + ".";
        return;
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
        case Options::Average: return "AVG";
        case Options::Subtract: return "SUB";
        case Options::Errors: return "Errors";
        case Options::Merge: return "Merge";
        case Options::Time: return "Time";
        case Options::Continue: return "Continue";
        case Options::Statistics: return "Statistics";
        case Options::Measurements: return "Measurements";
        case Options::Gnuplot: return "Gnuplot";
        default: return "UNKNOWN";
    }
}

Options::Action action(string action) {
    if (action == "Render")
        return Options::Render;
    else if (action == "Average")
        return Options::Average;
    else if (action == "Subtract")
        return Options::Subtract;
    else if (action == "Errors")
        return Options::Errors;
    else if (action == "Merge")
        return Options::Merge;
    else if (action == "Time")
        return Options::Time;
    else if (action == "Continue")
      return Options::Continue;
    else if (action == "Statistics")
      return Options::Statistics;
    else if (action == "Measurements")
      return Options::Measurements;
    else if (action == "Gnuplot")
      return Options::Gnuplot;
    else
        throw std::invalid_argument("action");
}

bool safe_bool(const map<string, string>& dict, string key) {
    auto itr = dict.find(key);
    return itr == dict.end() ? false : (bool)stoi(itr->second);
}

Options::Options(const map<string, string>& dict) {
    input0 = dict.find("options.input0")->second;
    input1 = dict.find("options.input1")->second;
    output = dict.find("options.output")->second;
    reference = dict.find("options.reference")->second;
    technique = haste::technique(dict.find("options.technique")->second);
    action = haste::action(dict.find("options.action")->second);
    num_photons = stoll(dict.find("options.num_photons")->second);
    radius = stod(dict.find("options.radius")->second);
    max_path = stoll(dict.find("options.max_path")->second);
    alpha = stod(dict.find("options.alpha")->second);
    beta = stod(dict.find("options.beta")->second);
    roulette = stod(dict.find("options.roulette")->second);
    batch = stoi(dict.find("options.batch")->second);
    quiet = stoi(dict.find("options.quiet")->second);
    enable_vc = stoi(dict.find("options.enable_vc")->second);
    enable_vm = stoi(dict.find("options.enable_vm")->second);
    from_light = safe_bool(dict, "options.from_light");
    lights = (float)stod(dict.find("options.lights")->second);
    num_samples = stoll(dict.find("options.num_samples")->second);
    num_seconds = stod(dict.find("options.num_seconds")->second);
    num_threads = stoll(dict.find("options.num_threads")->second);
    reload = stoi(dict.find("options.reload")->second);
    enable_seed = stoi(dict.find("options.enable_seed")->second);
    seed = stoll(dict.find("options.seed")->second);
    snapshot = stoll(dict.find("options.snapshot")->second);
    camera_id = stoll(dict.find("options.camera_id")->second);
    width = stoll(dict.find("options.width")->second);
    height = stoll(dict.find("options.height")->second);

    const string prefix = "options.trace[";

    for (auto&& item : dict) {
      unsigned long long i = 0;

      if (startswith(item.first, prefix) &&
        sscanf(item.first.c_str() + prefix.size(), "%llu", &i) == 1) {

        if (trace.size() <= i) {
          trace.resize(i + 1);
        }

        int read = sscanf(
          item.second.c_str(),
          "[%d, %d, %d]",
          &trace[i].x,
          &trace[i].y,
          &trace[i].z);

        if (read == 2) {
          trace[i].z = 2;
        }
      }
    }
}

map<string, string> Options::to_dict() const
{
    using std::to_string;

    map<string, string> result;
    result["options.input0"] = input0;
    result["options.input1"] = input1;
    result["options.output"] = output;
    result["options.reference"] = reference;
    result["options.technique"] = to_string(technique);
    result["options.action"] = to_string(action);
    result["options.num_photons"] = to_string(num_photons);
    result["options.radius"] = to_string(radius);
    result["options.max_path"] = to_string(max_path);
    result["options.alpha"] = to_string(alpha);
    result["options.beta"] = to_string(beta);
    result["options.roulette"] = to_string(roulette);
    result["options.batch"] = to_string(batch);
    result["options.quiet"] = to_string(quiet);
    result["options.enable_vc"] = to_string(enable_vc);
    result["options.enable_vm"] = to_string(enable_vm);
    result["options.from_light"] = to_string(from_light);
    result["options.lights"] = to_string(lights);
    result["options.num_samples"] = to_string(num_samples);
    result["options.num_seconds"] = to_string(num_seconds);
    result["options.num_threads"] = to_string(num_threads);
    result["options.reload"] = to_string(reload);
    result["options.enable_seed"] = to_string(enable_seed);
    result["options.seed"] = to_string(seed);
    result["options.snapshot"] = to_string(snapshot);
    result["options.camera_id"] = to_string(camera_id);
    result["options.width"] = to_string(width);
    result["options.height"] = to_string(height);

    for (size_t i = 0; i < trace.size(); ++i) {
      auto x = std::to_string(trace[i].x);
      auto y = std::to_string(trace[i].y);
      auto z = std::to_string(trace[i].z);
      auto value = "[" + x + ", " + y + ", " + z + "]";
      result["options.trace[" + std::to_string(i) + "]"] = value;
    }

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

void save_exr(Options options, statistics_t statistics, const vec3* data) {
  auto metadata = statistics.to_dict();
  auto local_options = options.to_dict();
  metadata.insert(local_options.begin(), local_options.end());

  if (isfile(options.output)) {
    string temp = temppath(".exr");
    save_exr(temp, metadata, options.width, options.height, data);
    move_file(temp, options.get_output());
  }
  else {
    save_exr(options.get_output(), metadata, options.width, options.height, data);
  }
}

map<string, string> fuse_metadata(const Options& options, const statistics_t& statistics) {
  auto metadata = statistics.to_dict();
  auto local_options = options.to_dict();
  metadata.insert(local_options.begin(), local_options.end());
  return metadata;
}

void save_exr(Options options, statistics_t statistics, const vec4* data) {
  auto metadata = fuse_metadata(options, statistics);

  if (isfile(options.output)) {
    string temp = temppath(".exr");
    save_exr(temp, metadata, options.width, options.height, data);
    move_file(temp, options.get_output());
  }
  else {
    save_exr(options.get_output(), metadata, options.width, options.height, data);
  }
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

void strip_exr(string dst, string src) {
  vector<vec4> data;
  map<string, string> metadata;

  size_t width = 0, height = 0;

  load_exr(src, metadata, width, height, data);

  auto options = Options(metadata);
  auto statistics = statistics_t(metadata);

  options.output = fullpath(dst);
  options.input1 = fullpath(src);

  auto& records = statistics.records;

  double rendering_duration = 0;

  for (size_t i = 0; i < records.size(); ++i) {
    rendering_duration += records[i].frame_duration;
  }

  if (records.size() != 0) {
    records.erase(records.begin(), records.end() - 1);
  }

  statistics.records.back().frame_duration = rendering_duration;

  statistics.measurements.clear();

  save_exr(options, statistics, data.data());
}

void merge_exr(string dst, string fst, string snd) {
  vector<vec4> dst_data, fst_data, snd_data;
  map<string, string> fst_metadata, snd_metadata;

  size_t fst_width = 0, snd_width = 0, fst_height = 0, snd_height = 0;

  load_exr(fst, fst_metadata, fst_width, fst_height, fst_data);
  load_exr(snd, snd_metadata, snd_width, snd_height, snd_data);

  if (fst_width != snd_width || fst_height != snd_height) {
    throw std::runtime_error("Sizes of '" + fst + "' and '" + snd +
      "' doesn't match.");
  }

  dst_data.resize(fst_data.size());

  for (size_t i = 0; i < fst_data.size(); ++i) {
    dst_data[i] = fst_data[i] + snd_data[i];
  }

  auto fst_options = Options(fst_metadata);
  auto snd_options = Options(snd_metadata);
  auto fst_statistics = statistics_t(fst_metadata);
  auto snd_statistics = statistics_t(snd_metadata);

  if (fst_options.technique != snd_options.technique) {
    std::cerr << "Cannot merge images rendered using different techniques." << std::endl;
    return;
  }

  fst_options.output = dst;

  auto& fst_records = fst_statistics.records;
  auto& snd_records = snd_statistics.records;

  double rendering_duration = 0;

  for (size_t i = 0; i < fst_records.size(); ++i) {
    rendering_duration += fst_records[i].frame_duration;
  }

  for (size_t i = 0; i < snd_records.size(); ++i) {
    rendering_duration += snd_records[i].frame_duration;
  }

  if (fst_records.size() != 0) {
    fst_records.erase(fst_records.begin(), fst_records.end() - 1);
  }

  fst_statistics.num_samples += snd_statistics.num_samples;
  fst_statistics.num_basic_rays += snd_statistics.num_basic_rays;
  fst_statistics.num_shadow_rays += snd_statistics.num_shadow_rays;
  fst_statistics.num_tentative_rays += snd_statistics.num_tentative_rays;
  fst_statistics.num_photons += snd_statistics.num_photons;
  fst_statistics.num_scattered += snd_statistics.num_scattered;
  fst_statistics.total_time += snd_statistics.total_time;
  fst_statistics.scatter_time += snd_statistics.scatter_time;
  fst_statistics.build_time += snd_statistics.build_time;
  fst_statistics.gather_time += snd_statistics.gather_time;
  fst_statistics.merge_time += snd_statistics.merge_time;
  fst_statistics.density_time += snd_statistics.density_time;
  fst_statistics.intersect_time += snd_statistics.intersect_time;
  fst_statistics.trace_eye_time += snd_statistics.trace_eye_time;
  fst_statistics.trace_light_time += snd_statistics.trace_light_time;

  fst_statistics.records.back().frame_duration = rendering_duration;
  fst_statistics.measurements.clear();

  save_exr(fst_options, fst_statistics, dst_data.data());
}

}
