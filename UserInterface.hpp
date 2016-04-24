#pragma once
#include <glm>
#include <utility.hpp>
#include <Technique.hpp>
#include <GLFW/glfw3.h>

namespace haste {

struct UserInterface {
    static const size_t pathSize = 255;
    char path[pathSize];
    float yaw = 0, pitch = -0.0;
    // vec3 position = vec3(0.0f, 0.0f, 40.0f);
    //vec3 position = vec3(0, 0.0f, 5.0);
    vec3 position = vec3(0, -4.5f, 1.0);
    double tpp = 0.001;
    double tpf = 100;
    double mainStart = glfwGetTime();
    string scenePath;
    string defpath = homePath() + "/";

    double timePerFrame = 0.0;
    double raysPerSecond = 0.0;

    float time = 0.0f;

    mat4 view;
    float fovy = pi<float>() / 3.0f;

    UserInterface(string scenePath);

    void updateCamera();

    void update(
        const Technique& technique,
        size_t width,
        size_t height,
        const vec4* image,
        float elapsed);
private:
    void _updateStatistics(
        const Technique& technique,
        size_t numSamples,
        double elapsed);

};



}
