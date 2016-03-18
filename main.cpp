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
#include <raycast.hpp>
#include <runtime_assert>

using namespace std;
using namespace haste;

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

        auto scene = haste::loadScene("models/cornell-box/CornellBox-Sphere.obj");
        scene.buildAccelStructs(device);

        for (auto name : scene.areaLights.names) {
            cout << name << endl;
        }

        Camera camera;

        float yaw = 0, pitch = -0.0;
        vec3 position = vec3(0, 0.75, 2.4);
        double tpp = 0.001;
        double tpf = 100;

        loop(window, [&](int width, int height) {
            image.resize(width * height);

            double start = glfwGetTime();
            size_t num_pixels = raytrace(image, width, camera, scene);

            double elapsed = glfwGetTime() - start;
            tpp = 0.95 * tpp + 0.05 * (elapsed / num_pixels) * 1000.0;
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
            ImGui::End();
        });

        rtcDeleteDevice(device);
    });
}
