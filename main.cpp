#include <gtest/gtest.h>
#include <framework.hpp>
#include <raytrace.hpp>
#include <imgui_ex.h>
#include <streamops.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <octree.hpp>

using namespace std;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (false && RUN_ALL_TESTS() != 0) {
        return 1;
    }

    return run(1000, 800, [](GLFWwindow* window) {
        std::vector<vec3> image;
        
        bool show_test_window = true;
 
        auto scene = obj::load("models/cornell_box.obj");

        octree_t octree(scene, 8);

        cout << "aabb: " << octree.aabb() << endl;


        camera_t camera;

        float yaw = glm::pi<float>(), pitch = -0.0;
        vec3 position = vec3(2.5, 2.5, -6);
        int line = 0;
        double tpp = 0.001;
        double tpf = 100;
        loop(window, [&](int width, int height) {
            image.resize(width * height);

            double start = glfwGetTime();
            int num_lines = raytrace(image, width, height, camera, scene, 0.033, line);
            int num_pixels = num_lines * width;

            double elapsed = glfwGetTime() - start;
            tpp = 0.99 * tpp + 0.01 * (elapsed / num_pixels) * 1000.0;
            tpf = 0.99 * tpf + 0.01 * (elapsed * height / num_lines) * 1000.0;

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

            ImGui::Begin("Statistics");
            float ftpp = float(tpp);
            float ftpf = float(tpf);
            ImGui::InputFloat("tpp [ms]", &ftpp);
            ImGui::InputFloat("tpf [ms]", &ftpf);
            ImGui::End();
        });
    });
}
