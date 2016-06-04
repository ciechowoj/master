#pragma once
#include <framework.hpp>
#include <Options.hpp>
#include <UserInterface.hpp>
#include <Scene.hpp>
#include <Technique.hpp>

namespace haste {

class Application : public Framework {
public:
	Application(const Options& options);
	~Application();

    void render(size_t width, size_t height, glm::vec4* data) override;
    void updateUI(size_t width, size_t height, const glm::vec4* data) override;
    void postproc(glm::vec4* dst, const glm::vec4* src, size_t width, size_t height) override;

    bool updateScene() override;

private:
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
    size_t _modificationTime;
    vector<vec4> _reference;
};

}
