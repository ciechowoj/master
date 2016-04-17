#pragma once
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct window_context_t {
    int texture_width = 0;
    int texture_height = 0;
    bool texture_created = false;
    GLuint texture_id = 0;
    GLuint program_id = 0;
    GLuint sampler_id = 0;
    GLuint sampler_location = 0;
    GLuint buffer_id = 0;
    GLuint varray_id = 0;
    GLuint pixel_buffer_id = 0;
};

int run(int width, int height, const std::function<void(GLFWwindow* window)>& func);

void draw_fullscreen_quad(
    GLFWwindow* window,
    const std::vector<glm::vec4>& image);

int loop(GLFWwindow* window, const std::function<void(int, int, void*)>& loop);