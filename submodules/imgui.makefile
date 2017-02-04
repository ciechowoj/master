
imgui.target=build/imgui/libimgui.a
imgui.submodule=submodules/imgui/README.md

$(imgui.submodule):
	git submodule update --init submodules/imgui

IMGUI_SOURCES = \
	submodules/imgui/imgui.cpp \
	submodules/imgui/imgui_demo.cpp \
	submodules/imgui/imgui_draw.cpp \
	submodules/imgui/examples/opengl3_example/imgui_impl_glfw_gl3.cpp

IMGUI_OBJECTS = $(IMGUI_SOURCES:submodules/imgui/%.cpp=build/imgui/%.o)

build/imgui/libimgui.a: $(imgui.submodule) $(IMGUI_OBJECTS)
	ar rcs build/imgui/libimgui.a $(IMGUI_OBJECTS)

build/imgui/%.o: submodules/imgui/%.cpp build/imgui/GL/gl3w.h
	$(CC) -c $(CCFLAGS) $< -o $@ -Ibuild/glad/loader/include

build/imgui/GL/gl3w.h: | build/imgui
	echo "#include <glad/glad.h>\n" >> build/imgui/GL/gl3w.h

build/imgui:
	mkdir -p build
	mkdir -p build/imgui
	mkdir -p build/imgui/GL
	mkdir -p build/imgui/examples
	mkdir -p build/imgui/examples/opengl3_example
