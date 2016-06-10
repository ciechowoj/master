#include <sstream>
#include <UserInterface.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui_ex.h>
#include <cstring>

namespace haste {

UserInterface::UserInterface(string scenePath, float& brightness)
    : scenePath(scenePath)
    , brightness(brightness) {
    strcpy(path, defpath.c_str());
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

    ImGui::SetNextWindowPos(ImVec2(0, /*height - 150*/ 0), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(256, 256), ImGuiSetCond_Once);

    ImGui::Begin("Statistics");

    _updateStatistics(technique, elapsed);
    _updateSaveRegion(technique, width, height, image);

    ImGui::Combo(
        "Display",
        (int*)&displayMode,
        "Unsigned rel error\0Unsigned abs error\0Relative error\0Absolute error\0Current\0Reference\0\0");

    ImGui::InputFloat("max error", &maxError);

    float numSamples = technique.numSamples();
    ImGui::InputFloat("samples ", &numSamples);

    _updateComputeAverage();

    ImGui::SliderFloat("brightness", &brightness, 0.0f, 50.0f);

    size_t offset = min(100, int(maxErrors.size()));

    if (maxErrors.size() > 1)
    {
        ImGui::PlotLines(
            "log max error",
            maxErrors.data() + maxErrors.size() - offset + 1,
            offset - 1,
            0,
            NULL,
            0.0f,
            1.0f,
            ImVec2(0, 100));
    }

    ImGui::End();

    _displayRadiance(width, height, image);
}

void UserInterface::_displayRadiance(
    size_t width,
    size_t height,
    const vec4* image)
{
    if (ImGui::IsMouseDown(0)) {
        auto pos = ImGui::GetMousePos();
        std::stringstream stream;

        vec4 radiance = image[size_t((height - pos.y - 1) * width + pos.x)];

        stream << "[" << pos.x << ", " << pos.y << "]: " << radiance.xyz() / radiance.w << " W/(m*m*sr)";

        ImGui::SetTooltip(stream.str().c_str());
    }
}

void UserInterface::_updateSaveRegion(
    const Technique& technique,
    size_t width,
    size_t height,
    const vec4* image)
{
    if(ImGui::CollapsingHeader("Snapshot")) {
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
    }
}

void UserInterface::_updateComputeAverage() {
    ImGui::Checkbox("compute average", &computeAverage);
    ImGui::InputVec("", &averageValue);
}


void UserInterface::_updateStatistics(
    const Technique& technique,
    double elapsed)
{
    if (timePerFrame == 0.0) {
        timePerFrame = elapsed;
    }
    else {
        timePerFrame = 0.95 * timePerFrame + 0.05 * elapsed * 1000.0;
    }

    if(ImGui::CollapsingHeader("Time statistics")) {
        float localTimePerFrame = float(timePerFrame);
        ImGui::InputFloat("ms/frame", &localTimePerFrame);

        float mainElapsed = float(glfwGetTime() - mainStart);
        ImGui::InputFloat("real time [s] ", &mainElapsed);
    }
}

}
