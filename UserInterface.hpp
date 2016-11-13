#pragma once
#include <glm>
#include <utility.hpp>
#include <Technique.hpp>
#include <GLFW/glfw3.h>

namespace haste {

enum DisplayMode : int {
    DisplayModeCurrent,
    DisplayModeReference,
    DisplayModeAbsolute,
    DisplayModeRelative,
    DisplayModeUnsignedAbsolute,
    DisplayModeUnsignedRelative,
};

struct UserInterface {
    DisplayMode displayMode;
    float maxError = 0.0f;

    vector<float> maxErrors;

    float avgRelError = 0.0f;
    float avgAbsError = 0.0f;
    static const size_t pathSize = 255;
    char path[pathSize];

    double mainStart = high_resolution_time();
    string scenePath;
    string defpath = homePath() + "/";

    double timePerFrame = 0.0;
    double raysPerSecond = 0.0;

    float& brightness;

    bool computeAverage = true;
    vec3 averageValue = vec3(0.0f);

    UserInterface(
        string scenePath,
        float& brightness);

    void updateCamera();

    void update(
        const Technique& technique,
        size_t width,
        size_t height,
        const vec4* image,
        double elapsed);
private:
    void _updateComputeAverage();

    void _updateStatistics(
        const Technique& technique,
        double elapsed);

    void _displayRadiance(
        size_t width,
        size_t height,
        const vec4* image);

    void _updateSaveRegion(
        const Technique& technique,
        size_t width,
        size_t height,
        const vec4* image);
};

}
