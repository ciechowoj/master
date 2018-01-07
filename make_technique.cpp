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

void print_traces_tabular(std::ostream& stream, const map<string, string>& metadata) {
    struct trace_t {
        int index;
        int x, y, r;
    };

    vector<trace_t> traces;

    for (auto&& itr : metadata) {
      if (startswith(itr.first, "options.trace")) {
        trace_t trace;

        sscanf(
            itr.first.c_str(),
            "options.trace[%d]",
            &trace.index);

        sscanf(
            itr.second.c_str(),
            "[%d, %d, %d]",
            &trace.x,
            &trace.y,
            &trace.r);

        traces.push_back(trace);
       }
    }

    std::sort(traces.begin(), traces.end(), [](auto a, auto b) { return a.index < b.index; });

    for (auto&& trace : traces) {
        std::cout << trace.x << " " << trace.y << " " << trace.r << "\n";
    }
}

shared<Technique> makeViewer(Options& options) {
    auto data = vector<vec3>();
    auto metadata = map<string, string>();

    size_t width = 0;
    size_t height = 0;

    string input1 = options.input1;
    string reference = options.reference;
    load_exr(input1, metadata, width, height, data);

    const string records = "records";
    const string measurements = "measurements";

    for (auto&& itr : metadata) {
      if (!startswith(itr.first, records) && !startswith(itr.first, measurements)) {
        std::cout << itr.first << ": " << itr.second << "\n";
       }
    }

    std::cout.flush();

    options = Options(metadata);
    options.input1 = input1;
    options.reference = reference;
    options.technique = Options::Viewer;
    options.batch = false;
    options.reload = true;

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
        options.from_light,
        options.lights,
        options.roulette,
        options.num_photons,
        options.radius,
        options.alpha,
        options.beta,
        options.num_threads);
}

shared<Technique> makeTechnique(const shared<const Scene>& scene, Options& options) {
    shared<Technique> result;

    switch (options.technique) {
        case Options::BPT:
            if (options.beta == 0.0f) {
                result = make_bpt_technique<BPT0>(scene, options);
            }
            else if (options.beta == 1.0f) {
                result = make_bpt_technique<BPT1>(scene, options);
            }
            else if (options.beta == 2.0f) {
                result = make_bpt_technique<BPT2>(scene, options);
            }
            else {
                result = make_bpt_technique<BPTb>(scene, options);
            }

            break;

        case Options::PT:
            result = std::make_shared<PathTracing>(
                scene,
                options.lights,
                options.roulette,
                options.beta,
                options.max_path,
                options.num_threads);

            break;

        case Options::VCM:
        case Options::UPG:
            if (options.beta == 0.0f) {
                result = make_upg_technique<UPG0>(scene, options);
            }
            else if (options.beta == 1.0f) {
                result = make_upg_technique<UPG1>(scene, options);
            }
            else if (options.beta == 2.0f) {
                result = make_upg_technique<UPG2>(scene, options);
            }
            else {
                result = make_upg_technique<UPGb>(scene, options);
            }

            break;

        default:
            result = makeViewer(options);
            break;
    }

    result->set_sky_gradient(
      options.sky_horizon,
      options.sky_zenith);

    return result;
}

shared<Scene> loadScene(const Options& options) {
    return loadScene(options.input0);
}

}
