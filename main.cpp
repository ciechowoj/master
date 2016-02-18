#include <string>
#include <iostream>
#include <functional>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <raytrace.hpp>
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

struct window_context_t {
    int texture_width = 0;
    int texture_height = 0;
    bool texture_created = false;
    GLuint texture_id = 0;
    GLuint program_id = 0;
    GLuint sampler_id = 0;
};

GLFWwindow* create_window(int x, int y) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    return glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
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

in vec2 texcoord;
out vec4 color;

void main()
{
    color = texture2D(sampler, texcoord); // vec4(1.0, 0.0, 1.0, 1.0);
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

void draw_fullscreen_quad(GLuint quad) {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
}

void window_resize(GLFWwindow* window, int width, int height) {
    window_context_t* context = (window_context_t*)glfwGetWindowUserPointer(window);

    if (!context->texture_created) {
        glGenTextures(1, &context->texture_id);
        context->texture_created = true;
    }

    if (context->texture_width != width || context->texture_height != height) {
        glBindTexture(GL_TEXTURE_2D, context->texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glGenSamplers(1, &context->sampler_id);
        glSamplerParameteri(context->sampler_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(context->sampler_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        context->texture_width = width;
        context->texture_height = height;
    }

    std::cerr << "Window resized to (" << width << ", " << height << ")." << std::endl;
}

int run(int width, int height, const std::function<void(GLFWwindow* window)>& func) {
    if (!glfwInit()) {
        std::cerr << "Cannot initialize glfw." << std::endl;
        return -1;
    }

    auto window = create_window(width, height);

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

    ImGui_ImplGlfwGL3_Init(window, true);

    func(window);

    if (context->texture_created) {
        glDeleteTextures(1, &context->texture_id);
    }

    delete context;

    glfwTerminate();

    return 0;
}

int main(void) {
    return run(640, 480, [](GLFWwindow* window) {
        window_context_t* context = (window_context_t*)glfwGetWindowUserPointer(window);

        GLuint global_vao = 0;
        glGenVertexArrays(1, &global_vao);
        glBindVertexArray(global_vao);

        GLuint quad = create_fullscreen_quad();

        glUseProgram(context->program_id);
        int sampler_location = glGetUniformLocation(context->program_id, "sampler");

        std::vector<vec3> image;
        
        bool show_test_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImColor(114, 144, 154);

        while (!glfwWindowShouldClose(window)) {
            glViewport(0, 0, context->texture_width, context->texture_height);
            
            ImGui_ImplGlfwGL3_NewFrame();

            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glBindVertexArray(global_vao);

            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, context->texture_id);
            glBindSampler(0, context->sampler_id);
            glUniform1i(sampler_location, 0);

            image.resize(context->texture_width * context->texture_height);
            raytrace(image, context->texture_width, context->texture_height, camera_t(), scene_t());

            glTexSubImage2D(
                GL_TEXTURE_2D, 0, 0, 0, 
                context->texture_width, 
                context->texture_height,
                GL_RGB,
                GL_FLOAT,
                image.data());

            draw_fullscreen_quad(quad);

            {
                static float f = 0.0f;
                ImGui::Text("Hello, world!");
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                ImGui::ColorEdit3("clear color", (float*)&clear_color);
                if (ImGui::Button("Test Window")) show_test_window ^= 1;
                if (ImGui::Button("Another Window")) show_another_window ^= 1;
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            }

            if (show_another_window)
            {
                ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
                ImGui::Begin("Another Window", &show_another_window);
                ImGui::Text("Hello");
                ImGui::End();
            }

            if (show_test_window)
            {
                ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
                ImGui::ShowTestWindow(&show_test_window);
            }

            ImGui::Render();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteVertexArrays(1, &global_vao);
        
    });
}
