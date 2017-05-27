#pragma once
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

struct window_context_t {
    int texture_width = 0;
    int texture_height = 0;
    bool texture_created = false;
    GLuint texture_id = 0;
    GLuint program_id = 0;
    GLuint sampler_id = 0;
    GLuint sampler_location = 0;
    GLuint scale_location = 0;
    GLuint buffer_id = 0;
    GLuint varray_id = 0;
    GLuint pixel_buffer_id = 0;
};

int run(int width, int height, const std::function<void(GLFWwindow* window)>& func);

void draw_fullscreen_quad(
    GLFWwindow* window,
    const std::vector<glm::vec4>& image,
    float scale);

void loop(GLFWwindow* window, const std::function<void(int, int, float&, void*)>& loop);

class Framework {
public:
    virtual ~Framework();

    virtual void render(size_t width, size_t height, glm::dvec4* data) = 0;

    virtual void updateUI(
        size_t width,
        size_t height,
        const glm::vec4* data,
        double elapsed) = 0;

    virtual void postproc(glm::vec4* dst, const glm::dvec4* src, size_t width, size_t height);
    virtual bool updateScene();

    int run(size_t width, size_t height, const std::string& caption);
    int runBatch(size_t width, size_t height);
    void quit();
    bool batch() const;

protected:
    float _scale = 10.0f;

private:
    GLFWwindow* _window = nullptr;
    bool _quit = false;
};


