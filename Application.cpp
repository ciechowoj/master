#include <Application.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace haste {

Application::Application(Options& options) {
  _device = rtcNewDevice(NULL);
  runtime_assert(_device != nullptr);

  _options = options;
  _ui = make_shared<UserInterface>(_options.input0, _scale);

  _modificationTime = 0;

  bool reload = _options.reload;
  _options.reload = true;
  updateScene();
  _options.reload = reload;

  if (!_options.reference.empty()) {
    loadEXR(_options.reference, _options.width, _options.height, _reference);
  }
}

Application::~Application() { rtcDeleteDevice(_device); }

void Application::render(size_t width, size_t height, glm::dvec4* data) {
  if (!std::isfinite(_rendering_start_time)) {
    _rendering_start_time = high_resolution_time();
  }

  auto view = ImageView(data, width, height);

  _technique->render(view, _engine, _options.cameraId);

  double elapsed = high_resolution_time() - _rendering_start_time;

  if (_options.technique != Options::Viewer) {
    _update_rms_history(width, height, data);
    _printStatistics(view, elapsed, false);
    _saveIfRequired(view, elapsed);
  }

  _updateQuitCond(view, elapsed);
}

void Application::updateUI(size_t width, size_t height, const glm::vec4* data,
                           double elapsed) {
  _ui->update(*_technique, _num_samples(), width, height, data, elapsed);
}

void Application::postproc(glm::vec4* dst, const glm::dvec4* src, size_t width,
                           size_t height) {
  size_t size = width * height;

  if (_ui->computeAverage) {
    _ui->averageValue = vec3(0.f, 0.f, 0.f);

    for (size_t i = 0; i < size; ++i) {
      _ui->averageValue += src[i].rgb() / src[i].a;
    }

    _ui->averageValue /= float(size);
    size_t center = height / 2 * width + width / 2;
    _ui->centerValue = src[center].rgb() / src[center].a;
  }

  if (_reference.size() == 0) {
    Framework::postproc(dst, src, width, height);
  } else {
    _ui->maxError = 0.0f;

    switch (_ui->displayMode) {
      case DisplayModeUnsignedRelative:
        for (size_t i = 0; i < _reference.size(); ++i) {
          float current = length(src[i].rgb() / src[i].a);
          float reference = length(_reference[i].rgb());
          float error = current == reference ? 0.0f : abs(current - reference) /
                                                          reference;

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
          float error = current == reference ? 0.0f : abs(current - reference) /
                                                          reference;

          _ui->maxError = std::max(_ui->maxError, error);

          dst[i] = current < reference ? vec4(0.0f, 0.0f, error, 1.0f)
                                       : vec4(error, 0.0f, 0.0f, 1.0f);
        }

        break;

      case DisplayModeAbsolute:
        for (size_t i = 0; i < _reference.size(); ++i) {
          float current = length(src[i].rgb() / src[i].a);
          float reference = length(_reference[i].rgb());
          float error = abs(current - reference);

          _ui->maxError = std::max(_ui->maxError, error);

          dst[i] = current < reference ? vec4(0.0f, 0.0f, error, 1.0f)
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
  if (_options.reload) {
    auto modificationTime = getmtime(_options.input0);

    if (_modificationTime < modificationTime) {
      if (_options.technique != Options::Viewer) {
        _scene = loadScene(_options);
        _scene->buildAccelStructs(_device);
      }

      _technique = makeTechnique(_scene, _options);

      if (!_options.quiet && !std::isfinite(_rendering_start_time)) {
        std::cout << "Using: " << _technique->name() << std::endl;
      }

      _reset_rms_history();
      _rendering_start_time = NAN;
      _modificationTime = modificationTime;
      return true;
    }
  }

  return false;
}

double Application::_compute_rms(std::size_t width, std::size_t height,
                                 const glm::dvec4* left,
                                 const glm::dvec4* right) {
  double sum = 0.0f;
  std::size_t num = width * height;

  for (std::size_t i = 0; i < num; ++i) {
    dvec3 d = left[i].rgb() / left[i].a - right[i].rgb() / right[i].a;
    sum += glm::l1Norm(d * d);
  }

  return glm::sqrt(sum / double(num));
}

void Application::_update_rms_history(std::size_t width, std::size_t height,
                                      const glm::dvec4* data) {
  double current_time = high_resolution_time();

  _rms_history.push_back(
      (_reference.empty() || width * height != _reference.size())
          ? std::make_pair(current_time - _rendering_start_time,
                           double(NAN))
          : std::make_pair(
                current_time - _rendering_start_time,
                _compute_rms(width, height, data, _reference.data())));
}

void Application::_reset_rms_history() {
  _rms_history.clear();
}

void Application::_printStatistics(const ImageView& view, double elapsed,
                                   bool preprocessed) {
  if (!_options.quiet && _options.technique != Options::Viewer) {
    if (preprocessed) {
      std::cout << "Preprocessing finished..." << std::endl;
    } else {
      size_t numSamples = _num_samples();
      std::cout << "Sample #" << std::setw(6) << std::left << numSamples << " "
                << std::right << std::fixed << std::setw(8)
                << std::setprecision(3) << elapsed << "s" << std::setw(8)
                << elapsed / numSamples << "s/sample" << std::endl;
    }
  }
}

void Application::_saveIfRequired(const ImageView& view, double elapsed) {
  size_t num_samples = _num_samples();

  if (num_samples != 0) {
    if (_options.numSamples != 0 && _options.numSamples == num_samples) {
      _save(view, num_samples, false);
    } else if (_options.numSeconds != 0.0 && _options.numSeconds <= elapsed) {
      _save(view, num_samples, false);
    } else if (_options.snapshot > 1 && num_samples % _options.snapshot == 0) {
      _save(view, num_samples, true);
    }
  }
}

void Application::_updateQuitCond(const ImageView& view, double elapsed) {
  if ((_options.numSamples != 0 && _options.numSamples == _num_samples()) ||
      (_options.numSeconds != 0.0 && _options.numSeconds <= elapsed)) {
    quit();
  }
}

void Application::_save(const ImageView& view, size_t numSamples,
                        bool snapshot) {
  string path;
  bool hasSamples = false;

  if (_options.output.empty()) {
    auto split = splitext(_options.input0);
    std::stringstream stream;
    stream << split.first << "." << view.width() << "." << view.height() << "."
           << numSamples << "." << techniqueString(_options) << ".exr";
    path = stream.str();
    hasSamples = true;
  } else {
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

  saveEXR(path, view.width(), view.height(), view.data(),
          _serialize_rms_history());

  if (snapshot) {
    std::cout << "Snapshot saved to `" << path << "`." << std::endl;
  } else {
    std::cout << "Result saved to `" << path << "`." << std::endl;
  }
}

std::size_t Application::_num_samples() const {
  return _rms_history.size();
}

std::vector<std::string> Application::_serialize_rms_history() const {
  std::vector<std::string> result(_rms_history.size());

  for (std::size_t i = 0; i < _rms_history.size(); ++i) {
    std::stringstream stream;
    stream << std::setprecision(10) << _rms_history[i].first << " "
           << _rms_history[i].second;
    result[i] = stream.str();
  }

  return result;
}
}
