#include <gtest/gtest.h>
#include <framework.hpp>
#include <raytrace.hpp>
#include <imgui_ex.h>
#include <streamops.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <loader.hpp>
#include <pathtrace.hpp>
#include <raycast.hpp>
#include <runtime_assert>

using namespace std;
using namespace haste;

string fixedPath(string base, string scene, int samples);

int main(int argc, char **argv) {
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    ::testing::InitGoogleTest(&argc, argv);

    if (false && RUN_ALL_TESTS() != 0) {
        return 1;
    }

    return run(1000, 800, [](GLFWwindow* window) {
        RTCDevice device = rtcNewDevice(NULL);

        runtime_assert(device != nullptr);

        std::vector<vec4> image;

        bool show_test_window = true;

        auto scenePath = "models/cornell-box/CornellBox-Original.obj";
        auto scene = haste::loadScene(scenePath);
        scene.buildAccelStructs(device);

        for (auto name : scene.lights.names) {
            cout << name << endl;
        }

        Camera camera;

        float yaw = 0, pitch = -0.0;
        vec3 position = vec3(0, 1.0f, 2.8);
        double tpp = 0.001;
        double tpf = 100;
        double mainStart = glfwGetTime();
        static const size_t pathSize = 255;
        char path[pathSize] = "./";
        string defpath = homePath() + "/";
        strcpy(path, defpath.c_str());

        loop(window, [&](int width, int height) {
            image.resize(width * height);

            double start = glfwGetTime();
            size_t num_pixels = pathtraceInteractive(image, width, camera, scene);

            double elapsed = glfwGetTime() - start;
            tpp = 0.99 * tpp + 0.01 * (elapsed / num_pixels) * 1000.0;
            tpf = tpp * image.size();

            draw_fullscreen_quad(window, image);

            // ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            // ImGui::ShowTestWindow(&show_test_window);

            ImGui::Begin("Camera");
            vec3 t = vec3(1, 2, 3);
            ImGui::SliderFloat("yaw", &yaw, -glm::pi<float>(), glm::pi<float>());
            ImGui::SliderFloat("pitch",  &pitch, -glm::half_pi<float>(), glm::half_pi<float>());
            ImGui::InputVec("position", &position);
            auto newView = translate(position) * eulerAngleXY(pitch, yaw);

            if (newView != camera.view) {
                image.clear();
                image.resize(width * height);
            }

            camera.view = newView;

            ImGui::InputMat("view", &camera.view);
            ImGui::InputFloat("fovy", &camera.fovy);
            ImGui::End();

            ImGui::Begin("Statistics");
            float ftpp = float(tpp);
            float ftpf = float(tpf);
            ImGui::InputFloat("tpp [ms]", &ftpp);
            ImGui::InputFloat("tpf [ms]", &ftpf);
            ImGui::InputFloat("samples ", &image[0].w);
            float mainElapsed = float(glfwGetTime() - mainStart);
            ImGui::InputFloat("elapsed [s] ", &mainElapsed);
            ImGui::Separator();

            ImGui::InputText("", path, int(pathSize - 1));
            ImGui::SameLine();
            
            if (ImGui::Button("Save EXR")) {
                saveEXR(path, image, width);
            }

            string fixed = fixedPath(path, scenePath, size_t(image[0].w));
            ImGui::LabelText("", "%s", fixed.c_str());
            ImGui::SameLine();

            ImGui::PushID("save-exr");

            if (ImGui::Button("Save EXR")) {
                saveEXR(fixed, image, width);
            }   

            ImGui::PopID();
            ImGui::End();
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

