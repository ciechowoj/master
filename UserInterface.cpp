#include <sstream>
#include <UserInterface.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui_ex.h>
#include <cstring>

namespace haste {

//using std::

UserInterface::UserInterface(string scenePath)
    : scenePath(scenePath) {
    strcpy(path, defpath.c_str());
}

void UserInterface::updateCamera() {
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
}

void UserInterface::update(
    const Technique& technique,
    size_t width,
    size_t height,
    const vec4* image,
    float elapsed)
{
    // bool show_test_window = true;
    // ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
    // ImGui::ShowTestWindow(&show_test_window);

    // updateCamera();

    ImGui::SetNextWindowPos(ImVec2(0, height - 150), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiSetCond_Once);

    ImGui::Begin("Statistics");

    if (timePerFrame == 0.0) {
        timePerFrame = elapsed;
    }
    else {
        timePerFrame = 0.95 * timePerFrame + 0.05 * elapsed * 1000.0;
    }

    float localTimePerFrame = float(timePerFrame);
    ImGui::InputFloat("ms/frame", &localTimePerFrame);

    float mainElapsed = float(glfwGetTime() - mainStart);
    ImGui::InputFloat("real time [s] ", &mainElapsed);

    float numSamples = technique.numSamples();
    ImGui::InputFloat("samples ", &numSamples);
    ImGui::Separator();

    ImGui::InputText("", path, int(pathSize - 1));
    ImGui::SameLine();

    if (ImGui::Button("Save EXR")) {
        saveEXR(path, width, height, image);
    }

    string fixed = fixedPath(path, scenePath, technique.numSamples());
    ImGui::LabelText("", "%s", fixed.c_str());
    ImGui::SameLine();

    ImGui::PushID("save-exr");

    if (ImGui::Button("Save EXR")) {
        saveEXR(fixed, width, height, image);
    }

    ImGui::PopID();
    ImGui::End();

    if (ImGui::IsMouseDown(0)) {
        auto pos = ImGui::GetMousePos();
        std::stringstream stream;

        vec4 radiance = image[size_t((height - pos.y - 1) * width + pos.x)];

        stream << "[" << pos.x << ", " << pos.y << "]: " << radiance.xyz() / radiance.w << " W/(m*m*sr)";

        ImGui::SetTooltip(stream.str().c_str());
    }
}

void UserInterface::_updateStatistics(
    const Technique& technique,
    size_t numSamples,
    double elapsed)
{

}

}
