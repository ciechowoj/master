#pragma once
#include <framework.hpp>
#include <Options.hpp>
#include <Scene.hpp>
#include <Technique.hpp>
#include <UserInterface.hpp>

namespace haste {

class Application : public Framework {
 public:
  Application(Options& options);
  ~Application();

  void render(size_t width, size_t height, glm::dvec4* data) override;
  void updateUI(size_t width, size_t height, const glm::vec4* data,
                double elapsed) override;
  void postproc(glm::vec4* dst, const glm::dvec4* src, size_t width,
                size_t height) override;

  bool updateScene() override;

 private:
  void _printStatistics(const ImageView& view, double elapsed, double time,
                        double epsilon, bool preprocessed);
  void _saveIfRequired(const ImageView& view, double elapsed);
  void _updateQuitCond(const ImageView& view, double elapsed);
  void _save(const ImageView& view, size_t numSamples, bool snapshot);

  std::size_t _num_samples() const;
  double _num_seconds() const;

  Options _options;
  RTCDevice _device;
  random_generator_t _generator;
  shared<Technique> _technique;
  shared<Scene> _scene;
  shared<UserInterface> _ui;
  size_t _modificationTime;
  double _num_seconds_saved;
  vector<dvec4> _reference;
};
}
