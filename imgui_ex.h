#pragma once
#include <imgui.h>
#include <glm/glm.hpp>

namespace ImGui {

bool InputVec(const char* label, glm::vec2* v, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
bool InputVec(const char* label, glm::vec3* v, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
bool InputVec(const char* label, glm::vec4* v, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);

bool InputMat(const char* label, glm::mat4* v, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);


}
