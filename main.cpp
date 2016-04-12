#include <gtest/gtest.h>
#include <framework.hpp>
#include <imgui_ex.h>
#include <streamops.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <loader.hpp>
#include <raycast.hpp>
#include <runtime_assert>

#include <PathTracing.hpp>

using namespace std;
using namespace haste;

string fixedPath(string base, string scene, int samples);

struct GUI {
    static const size_t pathSize = 255;
    char path[pathSize];
    float yaw = 0, pitch = -0.0;
    vec3 position = vec3(0, 1.0f, 2.8);
    double tpp = 0.001;
    double tpf = 100;
    double mainStart = glfwGetTime();
    string scenePath;
    string defpath = homePath() + "/";

    float time = 0.0f;

    mat4 view;
    float fovy = pi<float>() / 3.0f;

    GUI(string scenePath);
    void update(
        const vector<vec4>& image,
        size_t width,
        float elapsed);
};

int main(int argc, char **argv) {
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    return run(1000, 800, [](GLFWwindow* window) {
        RTCDevice device = rtcNewDevice(NULL);
        runtime_assert(device != nullptr);

        auto scenePath = "models/cornell-box/CornellBox-Original.obj";
        auto scene = haste::loadScene(scenePath);
        auto camera = make_shared<Camera>();
        scene->buildAccelStructs(device);

        GUI gui(scenePath);

        PathTracing technique;
        technique.setScene(scene);
        technique.setCamera(camera);

        loop(window, [&](int width, int height) {
            technique.setImageSize(width, height);

            double start = glfwGetTime();
            technique.updateInteractive(0.33);

            draw_fullscreen_quad(window, technique.image());

            gui.update(
                technique.image(),
                width,
                float(glfwGetTime() - start));

            if (gui.view != camera->view || gui.fovy != camera->fovy) {
                camera->view = gui.view;
                camera->fovy = gui.fovy;
                technique.softReset();
            }
        });

        rtcDeleteDevice(device);
    });
}

string fixedPath(string base, string scene, int samples) {
    string ext;
    tie(base, ext) = splitext(base);

    if (ext.empty()) {
        ext = ".exr";
    }

    string sceneBase, sceneExt;
    tie(sceneBase, sceneExt) = splitext(scene);

    stringstream result;

    if (!base.empty() && base[base.size() - 1] != '/') {
        result << base << "." << baseName(sceneBase) << "." << samples << ext;
    }
    else {
        result << base << baseName(sceneBase) << "." << samples << ext;
    }

    return result.str();
}

GUI::GUI(string scenePath)
    : scenePath(scenePath) {
    strcpy(path, defpath.c_str());
}

void GUI::update(
    const vector<vec4>& image,
    size_t width,
    float elapsed)
{
    // ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
    // ImGui::ShowTestWindow(&show_test_window);

    tpp = 0.99 * tpp + 0.01 * (elapsed / image.size()) * 1000.0;
    tpf = tpp * image.size();

    ImGui::Begin("Camera");
    vec3 t = vec3(1, 2, 3);
    ImGui::SliderFloat("yaw", &yaw, -glm::pi<float>(), glm::pi<float>());
    ImGui::SliderFloat("pitch",  &pitch, -glm::half_pi<float>(), glm::half_pi<float>());
    ImGui::SliderFloat("time",  &time, 0.0f, 1.0f);
    ImGui::InputVec("position", &position);

    ImGui::InputMat("view", &view);
    ImGui::InputFloat("fovy", &fovy);
    ImGui::End();

    view = translate(position) * eulerAngleXY(pitch, yaw);

    ImGui::Begin("Statistics");
    float ftpp = float(tpp);
    float ftpf = float(tpf);
    ImGui::InputFloat("tpp [ms]", &ftpp);
    ImGui::InputFloat("tpf [ms]", &ftpf);

    float numSamples = image.empty() ? 0.f : image[0].w;
    ImGui::InputFloat("samples ", &numSamples);
    float mainElapsed = float(glfwGetTime() - mainStart);
    ImGui::InputFloat("elapsed [s] ", &mainElapsed);
    ImGui::Separator();

    ImGui::InputText("", path, int(pathSize - 1));
    ImGui::SameLine();

    if (ImGui::Button("Save EXR")) {
        saveEXR(path, image, width);
    }

    string fixed = fixedPath(path, scenePath, size_t(image.empty() ? 0.f : image[0].w));
    ImGui::LabelText("", "%s", fixed.c_str());
    ImGui::SameLine();

    ImGui::PushID("save-exr");

    if (ImGui::Button("Save EXR")) {
        saveEXR(fixed, image, width);
    }

    ImGui::PopID();
    ImGui::End();
}