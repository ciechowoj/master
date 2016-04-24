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

    virtual void render(size_t width, size_t height, glm::vec4* data) {
        if (_preprocessed) {
            auto view = ImageView(data, width, height);
            _technique->render(view, _engine, _options.cameraId, _options.parallel);

            double elapsed = glfwGetTime() - _startTime;
            _saveIfRequired(view, elapsed);
            _updateQuitCond(view, elapsed);
        }
        else {
            _technique->preprocess(_scene, _engine, [](string, float) {});
            _preprocessed = true;
        }
    }

    virtual void updateUI(size_t width, size_t height, const glm::vec4* data) {
        _ui->update(
            *_technique,
            width,
            height,
            data,
            0.0f);
    }

    virtual void updateScene() {

    }

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
};




}
