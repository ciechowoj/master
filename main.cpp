#include <gtest/gtest.h>
#include <framework.hpp>
#include <raytrace.hpp>
#include <imgui_ex.h>
#include <streamops.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

using namespace std;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (RUN_ALL_TESTS() != 0) {
        return 1;
    }

    return run(1000, 800, [](GLFWwindow* window) {
        std::vector<vec3> image;
        
        bool show_test_window = true;

        auto scene = obj::load("models/cube.obj");

        cout << scene << endl;
        cout << scene.shapes << endl;
        cout << scene.shapes[0].mesh << endl;
        cout << scene.shapes[0].mesh.indices << endl;

        camera_t camera;

        float yaw = 0, pitch = 0;
        vec3 position = vec3(0, 0, -10);
        int line = 0;

        loop(window, [&](int width, int height) {
            image.resize(width * height);
            raytrace(image, width, height, camera, scene, 0.033, line);
            draw_fullscreen_quad(window, image);
            
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);

            ImGui::Begin("Camera");
            vec3 t = vec3(1, 2, 3);
            ImGui::SliderFloat("yaw", &yaw, -glm::pi<float>(), glm::pi<float>());
            ImGui::SliderFloat("pitch",  &pitch, -glm::half_pi<float>(), glm::half_pi<float>());
            ImGui::InputVec("position", &position);
            camera.view = translate(position) * eulerAngleXY(pitch, yaw);

            ImGui::InputMat("view", &camera.view);
            ImGui::InputFloat("fovy", &camera.fovy);
            ImGui::End();
        });
    });
}
