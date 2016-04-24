#include <sstream>
#include <iostream>
#include <Application.hpp>

namespace haste {

Application::Application(const Options& options) {
    _options = options;

    _device = rtcNewDevice(NULL);
    runtime_assert(_device != nullptr);

    _scene = loadScene(options);
    _scene->buildAccelStructs(_device);

    _technique = makeTechnique(options);
    _ui = make_shared<UserInterface>(options.input);

    _startTime = glfwGetTime();

    if (_options.numJobs != 1) {
        std::cerr << "Multiple threads aren't implemented yet (--num-jobs)." << std::endl;
    }
}

Application::~Application() {
    rtcDeleteDevice(_device);
}

void Application::_saveIfRequired(const ImageView& view, double elapsed) {
    size_t numSamples = size_t(view.last().w);

    if (numSamples != 0) {

        if (_options.numSamples != 0 &&
            _options.numSamples <= size_t(view.last().w)) {
            _save(view, numSamples, false);
        }
        else if (_options.numSeconds != 0.0 &&
            _options.numSeconds <= elapsed) {
            _save(view, numSamples, false);
        }
        else if (_options.snapshot != 0 &&
            numSamples % _options.snapshot == 0) {
            _save(view, numSamples, true);
        }
    }
}

void Application::_updateQuitCond(const ImageView& view, double elapsed) {
    if (_options.numSamples != 0 &&
        _options.numSamples <= size_t(view.last().w)) {
        quit();
    }

    if (_options.numSeconds != 0.0 &&
        _options.numSeconds <= elapsed) {
        quit();
    }
}

void Application::_save(const ImageView& view, size_t numSamples, bool snapshot) {
    string path;
    bool hasSamples = false;

    if (_options.output.empty()) {
        auto split = splitext(_options.input);
        std::stringstream stream;
        stream
            << split.first << "."
            << view.width() << "."
            << view.height() << "."
            << numSamples << "."
            << techniqueString(_options) << ".exr";
        path = stream.str();
        hasSamples = true;
    }
    else {
        path = _options.output;
    }

    if (snapshot) {
        auto split = splitext(path);
        std::stringstream stream;

        stream << split.first;

        if (!hasSamples) {
            stream << "." << numSamples;
        }

        stream << ".snapshot" << split.second;

        path = stream.str();
    }

    saveEXR(path, view.width(), view.height(), view.data());

    if (snapshot) {
        std::cout << "Snapshot saved to `" << path << "`." << std::endl;
    }
    else {
        std::cout << "Result saved to `" << path << "`." << std::endl;
    }
}

}
