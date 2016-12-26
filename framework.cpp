#include <string>
#include <iostream>
#include <functional>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <framework.hpp>
#include <utility.hpp>

#include <mutex>
#include <condition_variable>
#include <thread>
#include <cstring>
#include <chrono>
#include <atomic>

GLFWwindow* create_window(int x, int y, const std::string& caption) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    return glfwCreateWindow(x, y, caption.c_str(), NULL, NULL);
}

GLuint create_shader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);

    const GLchar* c_source = source.c_str();
    glShaderSource(shader, 1, &c_source, 0);
    glCompileShader(shader);

    GLint is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

    if(is_compiled == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        std::vector<GLchar> info(length);
        glGetShaderInfoLog(shader, length, &length, info.data());

        glDeleteShader(shader);

        std::cerr << "Failed to compile shader. Info log:\n"
            << info.data() << std::endl;

        return 0;
    }

    return shader;
}

GLuint create_program() {
    const char* fragment_shader_source = R""(
#version 330

uniform sampler2D sampler;
uniform float scale;

in vec2 texcoord;
out vec4 color;

void main()
{
    vec4 sample = texture2D(sampler, texcoord);
    color = clamp(vec4(sample.rgb / sample.a, 1) * scale, 0, 1);
}
    )"";

    const char* vertex_shader_source = R""(
#version 330

layout(location = 0)in vec3 position;
out vec2 texcoord;

void main()
{
    texcoord = (position.xy + vec2(1, 1)) * .5f;
    gl_Position = vec4(position, 1.f);
}
    )"";

    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);

    GLuint program = glCreateProgram();
    glAttachShader(program, fragment_shader);
    glAttachShader(program, vertex_shader);

    glBindAttribLocation(program, 0, "position");

    glLinkProgram(program);

    GLint is_linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int *)&is_linked);

    if(is_linked == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        std::vector<GLchar> info(length);
        glGetProgramInfoLog(program, length, &length, info.data());

        glDeleteProgram(program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        std::cerr << "Failed to link program. Info log:\n"
            << info.data() << std::endl;

        return 0;
    }

    glDetachShader(program, fragment_shader);
    glDetachShader(program, vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    return program;
}

GLuint create_fullscreen_quad() {
    float data[6][3] = {
       { -1.0, +1.0, 0.0  },
       { -1.0, -1.0, 0.0  },
       { +1.0, -1.0, 0.0  },
       { -1.0, +1.0, 0.0  },
       { +1.0, -1.0, 0.0  },
       { +1.0, +1.0, 0.0  },
   };

    GLuint result = 0;

    glGenBuffers(1, &result);
    glBindBuffer(GL_ARRAY_BUFFER, result);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    return result;
}

void bind_fullscreen_texture(GLFWwindow* window) {
    window_context_t* context = (window_context_t*)glfwGetWindowUserPointer(window);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, context->texture_id);
    glBindSampler(0, context->sampler_id);
    glUniform1i(context->sampler_location, 0);
}

void draw_fullscreen_quad(GLuint quad) {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
}

void draw_fullscreen_quad(
    GLFWwindow* window,
    const std::vector<glm::vec4>& image,
    float scale) {

    window_context_t* context = (window_context_t*)glfwGetWindowUserPointer(window);
    glUseProgram(context->program_id);
    glBindVertexArray(context->varray_id);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, context->pixel_buffer_id);

    glUniform1f(context->scale_location, scale);

    bind_fullscreen_texture(window);

    glTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0,
        context->texture_width,
        context->texture_height,
        GL_RGBA,
        GL_FLOAT,
        0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    draw_fullscreen_quad(context->buffer_id);
}

void window_resize(GLFWwindow* window, int width, int height) {
    window_context_t* context = (window_context_t*)glfwGetWindowUserPointer(window);

    if (!context->texture_created) {
        glGenTextures(1, &context->texture_id);
        glGenBuffers(1, &context->pixel_buffer_id);
        context->texture_created = true;
    }

    if (context->texture_width != width || context->texture_height != height) {
        glBindTexture(GL_TEXTURE_2D, context->texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glGenSamplers(1, &context->sampler_id);
        glSamplerParameteri(context->sampler_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(context->sampler_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, context->pixel_buffer_id);

        glBufferData(
            GL_PIXEL_UNPACK_BUFFER,
            width * height * sizeof(glm::vec4),
            nullptr,
            GL_STREAM_DRAW);

        void* pointer = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);

        if (pointer) {
            ::memset(pointer, 0, width * height * sizeof(glm::vec4));
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        context->texture_width = width;
        context->texture_height = height;
    }

    std::cerr << "Window resized to (" << width << ", " << height << ")." << std::endl;
}

int run(
    int width,
    int height,
    const std::string& caption,
    const std::function<void(GLFWwindow* window)>& func)
{
    if (!glfwInit()) {
        std::cerr << "Cannot initialize glfw." << std::endl;
        return -1;
    }

    auto window = create_window(width, height, caption);

    if (!window) {
        std::cerr << "Cannot create window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGL()) {
        std::cerr << "Cannot load glad extensions." << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cerr << "Loaded OpenGL "
        << GLVersion.major << "."
        << GLVersion.minor << " profile."
        << std::endl;

    window_context_t* context = new window_context_t();
    glfwSetWindowUserPointer(window, context);
    glfwSetWindowSizeCallback(window, window_resize);
    window_resize(window, width, height);
    context->program_id = create_program();
    context->sampler_location = glGetUniformLocation(context->program_id, "sampler");
    context->scale_location = glGetUniformLocation(context->program_id, "scale");

    glGenVertexArrays(1, &context->varray_id);
    glBindVertexArray(context->varray_id);

    context->buffer_id = create_fullscreen_quad();

    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui_ImplGlfwGL3_Init(window, true);

    func(window);

    if (context->texture_created) {
        glDeleteTextures(1, &context->texture_id);
    }

    glDeleteVertexArrays(1, &context->varray_id);

    delete context;

    glfwTerminate();

    return 0;
}

void loop(GLFWwindow* window, const std::function<void(int, int, float&, void*)>& loop) {
    while (!glfwWindowShouldClose(window)) {
        window_context_t* context = (window_context_t*)glfwGetWindowUserPointer(window);
        glViewport(0, 0, context->texture_width, context->texture_height);

        ImGui_ImplGlfwGL3_NewFrame();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, context->pixel_buffer_id);
        void* pointer = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);

        float scale = 0.0f;

        if (pointer) {
            loop(context->texture_width, context->texture_height, scale, pointer);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        draw_fullscreen_quad(window, std::vector<glm::vec4>(), scale);

        ImGui::Render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

Framework::~Framework() { }

bool Framework::updateScene() {
    return false;
}

void Framework::postproc(glm::vec4* dst, const glm::dvec4* src, size_t width, size_t height) {
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            dst[y * width + x] = glm::vec4(src[y * width + x]);
        }
    }
}

int Framework::run(size_t width, size_t height, const std::string& caption) {
    return ::run(width, height, caption, [=](GLFWwindow* window) {
        _window = window;

        std::vector<glm::dvec4> buffer;
        std::atomic<int> bufferWidth, bufferHeight;
        std::atomic<bool> trigger, done, quit;
        std::atomic<double> elapsed;
        std::mutex workerMutex;
        std::condition_variable workerCondition;
        std::condition_variable quitCondition;

        trigger = false;
        done = true;
        quit = false;
        elapsed = 0.0;

        auto worker = std::thread([&]() {
            while (!quit) {
                double start = haste::high_resolution_time();

                while (!trigger) {
                    std::unique_lock<std::mutex> lock(workerMutex);
                    workerCondition.wait(lock);
                }

                if (!quit) {
                    trigger = false;
                    render(bufferWidth, bufferHeight, buffer.data());
                    done = true;
                }

                elapsed = haste::high_resolution_time() - start;
            }

            quitCondition.notify_all();
        });

        loop(window, [&](int width, int height, float& scale, void* image) {
            scale = _scale;

            if (done) {
                if (bufferWidth == width && bufferHeight == height) {
                    postproc((glm::vec4*)image, buffer.data(), width, height);
                }
                else {
                    buffer.resize(width * height);
                    std::memset(buffer.data(), 0, buffer.size() * sizeof(buffer[0]));
                    bufferWidth = width;
                    bufferHeight = height;
                }

                trigger = true;
                if (updateScene()) {
                    std::memset(buffer.data(), 0, buffer.size() * sizeof(buffer[0]));
                }

                done = false;
                workerCondition.notify_all();
            }

            updateUI(width, height, (glm::vec4*)image, elapsed);

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

        quit = true;
        trigger = true;
        workerCondition.notify_all();

        std::unique_lock<std::mutex> lock(workerMutex);
        if (quitCondition.wait_for(lock, std::chrono::seconds(2)) != std::cv_status::timeout) {
            worker.join();
        }
    });

    _window = nullptr;

    return 0;
}

int Framework::runBatch(size_t width, size_t height) {
    std::vector<glm::dvec4> buffer;
    buffer.resize(width * height);
    std::memset(buffer.data(), 0, buffer.size() * sizeof(glm::vec4));

    while (!_quit) {
        updateScene();
        render(width, height, buffer.data());
    }

    return 0;
}

void Framework::quit() {
    if (_window) {
        glfwSetWindowShouldClose(_window, GLFW_TRUE);
    }
    else {
        _quit = true;
    }
}

bool Framework::batch() const {
    return _window != nullptr;
}
