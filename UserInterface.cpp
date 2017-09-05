#include <sstream>
#include <iomanip>
#include <UserInterface.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui_ex.h>
#include <cstring>

namespace haste {

UserInterface::UserInterface(Options options, string scenePath, float& brightness)
    : options(options)
    , scenePath(scenePath)
    , brightness(brightness) {
    strcpy(path, defpath.c_str());
}

void UserInterface::update(
    const Technique& technique,
    size_t num_samples,
    size_t width,
    size_t height,
    const vec4* image,
    double elapsed)
{
    // bool show_test_window = true;
    // ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
    // ImGui::ShowTestWindow(&show_test_window);

    // updateCamera();

    ImGui::SetNextWindowPos(ImVec2(0, /*height - 150*/ 0), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 256), ImGuiSetCond_Once);

    ImGui::Begin("Statistics");

    _updateStatistics(technique, elapsed);
    _updateSaveRegion(technique, num_samples, width, height, image);

    ImGui::Combo(
        "Display",
        (int*)&displayMode,
        "Current\0"
        "Reference\0"
        "Absolute error\0"
        "Relative error\0"
        "Unsigned abs error\0"
        "Unsigned rel error\0"
        "\0");

    ImGui::InputFloat("max error", &maxError);

    float numSamples = float(num_samples);
    ImGui::InputFloat("samples ", &numSamples);

    _updateComputeAverage();

    ImGui::InputVec("center", &centerValue);

    ImGui::SliderFloat("brightness", &brightness, 0.0f, 10.0f);

    size_t offset = min(100, int(maxErrors.size()));

    if (maxErrors.size() > 1)
    {
        ImGui::PlotLines(
            "max error",
            maxErrors.data() + maxErrors.size() - offset + 1,
            int(offset - 1),
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

        vec3 radianceDivW = radiance.xyz() / radiance.w;

        stream
            << "[" << pos.x << ", " << pos.y << "]: "
            << std::fixed
            << std::setprecision(6)
            << "["
            << radianceDivW.x
            << ", "
            << radianceDivW.y
            << ", "
            << radianceDivW.z
            << "]"
            << " W/(m*m*sr)";

        ImGui::SetTooltip("%s", stream.str().c_str());
    }
}

void UserInterface::_updateSaveRegion(
    const Technique& technique,
    size_t num_samples,
    size_t width,
    size_t height,
    const vec4* image) {
    if(ImGui::CollapsingHeader("Snapshot")) {
        ImGui::InputText("", path, int(pathSize - 1));
        ImGui::SameLine();

        options.output = path;

        if (ImGui::Button("Save EXR")) {
            save_exr(options, technique.statistics(), image);
        }

        string fixed = fixedPath(path, scenePath, num_samples);
        ImGui::LabelText("", "%s", fixed.c_str());
        ImGui::SameLine();

        ImGui::PushID("save-exr");

        if (ImGui::Button("Save EXR")) {
            options.output = fixed;
            save_exr(options, technique.statistics(), image);
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
    timePerFrame = elapsed;

    if(ImGui::CollapsingHeader("Time statistics")) {
        float localTimePerFrame = float(timePerFrame);
        ImGui::InputFloat("ms/frame", &localTimePerFrame);

        float mainElapsed = float(high_resolution_time() - mainStart);
        ImGui::InputFloat("real time [s] ", &mainElapsed);
    }
}

}
