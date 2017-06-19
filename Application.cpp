#include <Application.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <exr.hpp>

namespace haste {

Application::Application(Options& options) {
  _device = rtcNewDevice(NULL);
  runtime_assert(_device != nullptr);

  _options = options;
  _ui = make_shared<UserInterface>(options, _options.input0, _scale);

  _modificationTime = 0;

  bool reload = _options.reload;
  _options.reload = true;
  updateScene();
  _options.reload = reload;

  if (!_options.reference.empty()) {
    map<string, string> metadata;

    size_t width = 0, height = 0;

    load_exr(_options.reference, metadata, width, height, _reference);
    _options.width = width;
    _options.height = height;
  }

  options = _options;
}

Application::~Application() { rtcDeleteDevice(_device); }

void Application::render(size_t width, size_t height, glm::dvec4* data) {
  auto view = ImageView(data, width, height);

  if (_options.action == Options::Continue) {
    map<string, string> metadata;
    size_t width = 0; height = 0;
    vector<vec4> image;
    load_exr(_options.output, metadata, width, height, image);

    std::transform(
      image.begin(),
      image.end(),
      data,
      [](vec4 x) { return dvec4(x); });

    _technique->set_statistics(statistics_t(metadata));
    _options.action = Options::Render;

    _num_seconds_saved = _num_seconds();
  }

  if (_options.enable_seed) {
    _generator.seed(_options.seed + _technique->statistics().num_samples);
  }

  _technique->render(view, _generator, _options.camera_id, _reference);
  ++_num_samples;

  if (_options.technique != Options::Viewer) {
    _printStatistics(view, _technique->statistics().records.back().frame_duration, _technique->statistics().total_time, false);
    _saveIfRequired(view, _technique->statistics().total_time);
  }

  _updateQuitCond(view, _technique->statistics().total_time);
}

void Application::updateUI(size_t width, size_t height, const glm::vec4* data,
                           double elapsed) {
  _ui->update(*_technique, _num_samples, width, height, data, elapsed);
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
          double current = length(src[i].rgb() / src[i].a);
		  double reference = length(_reference[i].rgb());
		  float error = current == reference ? 0.0f : float(abs(current - reference) / reference);

          _ui->maxError = std::max(_ui->maxError, error);

          dst[i] = vec4(vec3(error), 1.0f);
        }

        break;

      case DisplayModeUnsignedAbsolute:
        _ui->avgAbsError = 0.0f;

        for (size_t i = 0; i < _reference.size(); ++i) {
          double current = length(src[i].rgb() / src[i].a);
		  double reference = length(_reference[i].rgb());
          float error = float(abs(current - reference));

          _ui->maxError = std::max(_ui->maxError, error);

          dst[i] = vec4(vec3(error), 1.0f);
        }

        break;

      case DisplayModeRelative:
        for (size_t i = 0; i < _reference.size(); ++i) {
          double current = length(src[i].rgb() / src[i].a);
          double reference = length(_reference[i].rgb());
          float error = current == reference ? 0.0f : float(abs(current - reference) /
                                                          reference);

          _ui->maxError = std::max(_ui->maxError, error);

          dst[i] = current < reference ? vec4(0.0f, 0.0f, error, 1.0f)
                                       : vec4(error, 0.0f, 0.0f, 1.0f);
        }

        break;

      case DisplayModeAbsolute:
        for (size_t i = 0; i < _reference.size(); ++i) {
          double current = length(src[i].rgb() / src[i].a);
          double reference = length(_reference[i].rgb());
          float error = float(abs(current - reference));

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
    auto modificationTime = getmtime(
        _options.technique != Options::Viewer
          ? _options.input0
          : _options.input1);

    if (_modificationTime < modificationTime) {
      if (_options.technique != Options::Viewer) {
        _scene = loadScene(_options);
        _scene->buildAccelStructs(_device);
      }

      _technique = makeTechnique(_scene, _options);

      if (!_options.quiet) {
        std::cout << "Using: " << to_string(_options.technique) << std::endl;
      }

      _modificationTime = modificationTime;
      _num_seconds_saved = _num_seconds();
      _num_samples = 0;
      return true;
    }
  }

  return false;
}

void Application::_printStatistics(const ImageView& view, double elapsed,
                                   double time,
                                   bool preprocessed) {
  if (!_options.quiet && _options.technique != Options::Viewer) {
    if (preprocessed) {
      std::cout << "Preprocessing finished..." << std::endl;
    } else {
      print_frame_summary(std::cout, _technique->statistics());
    }
  }
}

void Application::_saveIfRequired(const ImageView& view, double elapsed) {
  size_t num_samples = _num_samples;

  if (num_samples != 0) {
    if (_options.num_samples != 0 && _options.num_samples == num_samples) {
      _save(view, num_samples, false);
    } else if (_options.num_seconds != 0.0 && _options.num_seconds <= elapsed) {
      _save(view, num_samples, false);
    } else if (_options.snapshot != 0 && _num_seconds_saved + _options.snapshot < _num_seconds()) {
      _save(view, num_samples, true);
      _num_seconds_saved = _num_seconds();
    }
  }
}

void Application::_updateQuitCond(const ImageView& view, double elapsed) {
  if (_options.technique == Options::Viewer) {
    return;
  }

  if ((_options.num_samples != 0 && _options.num_samples == _num_samples) ||
      (_options.num_seconds != 0.0 && _options.num_seconds <= elapsed)) {
    quit();
  }
}

void Application::_save(const ImageView& view, size_t num_samples,
                        bool snapshot) {
  save_exr(_options, _technique->statistics(), view.data());

  if (!_options.quiet) {
    if (snapshot) {
      std::cout << "Snapshot saved to `" << _options.get_output() << "`." << std::endl;
    } else {
      std::cout << "Result saved to `" << _options.get_output() << "`." << std::endl;
      std::cout << _technique->statistics() << std::endl;
    }
  }
}

double Application::_num_seconds() const { return _technique->statistics().total_time; }

}
