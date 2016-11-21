#include <sstream>
#include <iostream>
#include <iomanip>
#include <Application.hpp>

namespace haste {

Application::Application(const Options& options) {
    _options = options;

    _device = rtcNewDevice(NULL);
    runtime_assert(_device != nullptr);

    _technique = makeTechnique(_options);
    _ui = make_shared<UserInterface>(_options.input0, _scale);

    _modificationTime = 0;

    bool reload = _options.reload;
    _options.reload = true;
    updateScene();
    _options.reload = reload;

    if (!_options.reference.empty()) {
        loadEXR(
            _options.reference,
            _options.width,
            _options.height,
            _reference);
    }

    if (!_options.quiet) {
        std::cout << "Using: " << _technique->name() << std::endl;
    }

    _startTime = high_resolution_time();
}

Application::~Application() {
    rtcDeleteDevice(_device);
}

void Application::render(size_t width, size_t height, glm::vec4* data) {
    if (_preprocessed) {
        auto view = ImageView(data, width, height);

        render_context_t context;
        context.engine = &_engine;

        _technique->render(view, context, _options.cameraId, _options.parallel);

        double elapsed = high_resolution_time() - _startTime;
        _saveIfRequired(view, elapsed);
        _updateQuitCond(view, elapsed);
        _printStatistics(view, elapsed, false);
    }
    else {
        _technique->preprocess(_scene, _engine, [](string, float) {});
        _printStatistics(ImageView(), 0.0f, true);
        _preprocessed = true;
    }
}

void Application::updateUI(
    size_t width,
    size_t height,
    const glm::vec4* data,
    double elapsed)
{
    _ui->update(
        *_technique,
        width,
        height,
        data,
        elapsed);
}

void Application::postproc(glm::vec4* dst, const glm::vec4* src, size_t width, size_t height) {
    size_t size = width * height;

    if (_ui->computeAverage)
    {
        _ui->averageValue = vec3(0.f, 0.f, 0.f);

        for (size_t i = 0; i < size; ++i) {
            _ui->averageValue += src[i].rgb() / src[i].a;
        }

        _ui->averageValue /= float(size);
    }

    if (_reference.size() == 0) {
        Framework::postproc(dst, src, width, height);
    }
    else {
        _ui->maxError = 0.0f;

        switch (_ui->displayMode)
        {
            case DisplayModeUnsignedRelative:
                for (size_t i = 0; i < _reference.size(); ++i) {
                    float current = length(src[i].rgb() / src[i].a);
                    float reference = length(_reference[i].rgb());
                    float error
                        = current == reference
                        ? 0.0f
                        : abs(current - reference) / reference;

                    _ui->maxError = std::max(_ui->maxError, error);

                    dst[i] = vec4(vec3(error), 1.0f);
                }

                break;

            case DisplayModeUnsignedAbsolute:
                _ui->avgAbsError = 0.0f;

                for (size_t i = 0; i < _reference.size(); ++i) {
                    float current = length(src[i].rgb() / src[i].a);
                    float reference = length(_reference[i].rgb());
                    float error = abs(current - reference);

                    _ui->maxError = std::max(_ui->maxError, error);

                    dst[i] = vec4(vec3(error), 1.0f);
                }

                break;

            case DisplayModeRelative:
                for (size_t i = 0; i < _reference.size(); ++i) {
                    float current = length(src[i].rgb() / src[i].a);
                    float reference = length(_reference[i].rgb());
                    float error
                        = current == reference
                        ? 0.0f
                        : abs(current - reference) / reference;

                    _ui->maxError = std::max(_ui->maxError, error);

                    dst[i] = current < reference
                        ? vec4(0.0f, 0.0f, error, 1.0f)
                        : vec4(error, 0.0f, 0.0f, 1.0f);
                }

                break;

            case DisplayModeAbsolute:
                for (size_t i = 0; i < _reference.size(); ++i) {
                    float current = length(src[i].rgb() / src[i].a);
                    float reference = length(_reference[i].rgb());
                    float error = abs(current - reference);

                    _ui->maxError = std::max(_ui->maxError, error);

                    dst[i] = current < reference
                        ? vec4(0.0f, 0.0f, error, 1.0f)
                        : vec4(error, 0.0f, 0.0f, 1.0f);
                }

                break;

            case DisplayModeCurrent:
                Framework::postproc(dst, src, width, height);
                break;

            case DisplayModeReference:
                Framework::postproc(dst, _reference.data(), width, height);
                break;
        }

        _ui->maxErrors.push_back(_ui->maxError);
    }
}

bool Application::updateScene() {
    if (_options.reload && _options.technique != Options::Viewer) {
        auto modificationTime = getmtime(_options.input0);

        if (_modificationTime < modificationTime) {
            _scene = loadScene(_options);
            _scene->buildAccelStructs(_device);
            _preprocessed = false;
            _modificationTime = modificationTime;
            return true;
        }
    }

    return false;
}

void Application::_printStatistics(const ImageView& view, double elapsed, bool preprocessed) {
    if (!_options.quiet && _options.technique != Options::Viewer) {
        if (preprocessed) {
            std::cout << "Preprocessing finished..." << std::endl;
        }
        else {
            size_t numSamples = size_t(view.last().w);
            std::cout
                << "Sample #"
                << std::setw(6)
                << std::left
                << numSamples
                << " "
                << std::right
                << std::fixed
                << std::setw(8)
                << std::setprecision(3)
                << elapsed
                << "s"
                << std::setw(8)
                << elapsed / numSamples
                << "s/sample"
                << std::endl;
        }
    }
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
        auto split = splitext(_options.input0);
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
