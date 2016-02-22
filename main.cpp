#include <gtest/gtest.h>
#include <framework.hpp>
#include <raytrace.hpp>
#include <imgui.h>

using namespace std;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (RUN_ALL_TESTS() != 0) {
        return 1;
    }

    return run(640, 480, [](GLFWwindow* window) {
        std::vector<vec3> image;
        
        bool show_test_window = true;

        auto scene = load_obj("models/triangle.obj");

        cout << scene.shapes << endl;
        cout << scene.shapes[0].mesh.positions << endl;

        loop(window, [&](int width, int height) {
            image.resize(width * height);
            raytrace(image, width, height, camera_t(vec3(), vec3(), width, height), scene);
            draw_fullscreen_quad(window, image);
            
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        });
    });
}
