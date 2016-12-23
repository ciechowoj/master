#pragma once
#include <Options.hpp>
#include <Scene.hpp>
#include <Technique.hpp>
#include <UserInterface.hpp>
#include <framework.hpp>

namespace haste {

class Application : public Framework {
 public:
  Application(const Options& options);
  ~Application();

  void render(size_t width, size_t height, glm::vec4* data) override;
  void updateUI(size_t width, size_t height, const glm::vec4* data,
                double elapsed) override;
  void postproc(glm::vec4* dst, const glm::vec4* src, size_t width,
                size_t height) override;

  bool updateScene() override;

 private:
  static double _compute_rms(std::size_t width, std::size_t height,
                             const glm::vec4* left, const glm::vec4* right);
  void _update_rms_history(std::size_t width, std::size_t height,
                           const glm::vec4* data);
  void _reset_rms_history();

  void _printStatistics(const ImageView& view, double elapsed,
                        bool preprocessed);
  void _saveIfRequired(const ImageView& view, double elapsed);
  void _updateQuitCond(const ImageView& view, double elapsed);
  void _save(const ImageView& view, size_t numSamples, bool snapshot);

  Options _options;
  RTCDevice _device;
  RandomEngine _engine;
  shared<Technique> _technique;
  shared<Scene> _scene;
  bool _preprocessed = false;
  shared<UserInterface> _ui;
  double _startTime;
  double _num_samples;
  size_t _modificationTime;
  vector<vec4> _reference;
  vector<double> _rms_history;
};
}
