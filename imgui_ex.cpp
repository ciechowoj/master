#include <imgui_ex.h>

namespace ImGui {

using namespace glm;

bool InputFloatN(const char* label, float* v, int components, int decimal_precision, ImGuiInputTextFlags extra_flags);

bool InputVec(const char* label, vec2* v, int decimal_precision, ImGuiInputTextFlags extra_flags) {
	return InputFloatN(label, (float*)v, 2, decimal_precision, extra_flags);
}

bool InputVec(const char* label, vec3* v, int decimal_precision, ImGuiInputTextFlags extra_flags) {
	return InputFloatN(label, (float*)v, 3, decimal_precision, extra_flags);
}

bool InputVec(const char* label, vec4* v, int decimal_precision, ImGuiInputTextFlags extra_flags) {
	return InputFloatN(label, (float*)v, 4, decimal_precision, extra_flags);
}

bool InputMat(const char* label, glm::mat4* v, int decimal_precision, ImGuiInputTextFlags extra_flags) {
	return InputFloatN(label, ((float*)v) + 0, 4, decimal_precision, extra_flags)
		|| InputFloatN("", ((float*)v) + 4, 4, decimal_precision, extra_flags)
		|| InputFloatN("", ((float*)v) + 8, 4, decimal_precision, extra_flags)
		|| InputFloatN("", ((float*)v) + 12, 4, decimal_precision, extra_flags);
}


}




