
INCLUDE_DIRS = \
	-Isubmodules/glfw/include \
	-Isubmodules/glm \
	-Isubmodules/tinyobjloader \
	-Isubmodules/imgui \
	-Isubmodules/imgui/examples/opengl3_example \
	-Isubmodules/googletest/googletest/include \
	-Isubmodules/embree/include \
	-Isubmodules/assimp/include \
	-Ibuild/glad/loader/include \
	-Ibuild/imgui \
	-I.

LIBRARY_DIRS = \
	-Lbuild/glfw/src \
	-Lbuild/tinyobjloader \
	-Lbuild/imgui \
	-Lbuild/googletest \
	-Lbuild/glad \
	-Lbuild/embree \
	-Lbuild/assimp/code

CC = g++-4.9
CC_FLAGS = -march=native -O3 -w -Wall -std=c++11 $(INCLUDE_DIRS) -DGLM_FORCE_RADIANS -DGLM_SWIZZLE

EMBREE_LIBS = \
	-lembree \
	-ltbb \
	-lsys \
	-lsimd \
	-llexers \
	-lembree_avx \
	-lembree_avx2 \
	-lembree_sse42

GLFW_LIBS = \
	-lglfw3 \
	-lX11 \
	-lXrandr \
	-lXi \
	-lXxf86vm \
	-lXcursor \
	-lXinerama

MAIN_HEADERS = $(wildcard *.hpp)
MAIN_SOURCES = $(wildcard *.cpp)
MAIN_OBJECTS = $(MAIN_SOURCES:%.cpp=build/master/%.o)
MAIN_LIBS =  $(GLFW_LIBS) -ldl -lpthread -lglad -limgui -lgtest $(EMBREE_LIBS) -lassimp -lz
MAIN_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/$*.Td
MAIN_POST = mv -f build/master/$*.Td build/master/$*.d

TEST_SOURCES = $(wildcard unit_tests/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:%.cpp=build/master/%.o)
TEST_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/unit_tests/$*.Td
TEST_POST = mv -f build/master/unit_tests/$*.Td build/master/unit_tests/$*.d

all: master

master: build/master/master.bin

build/master/master.bin: \
	build/glad/libglad.a \
	build/glfw/src/libglfw3.a \
	build/imgui/libimgui.a \
	build/googletest/libgtest.a \
	build/embree/libembree.a \
	build/assimp/code/libassimp.a \
	Makefile \
	$(MAIN_OBJECTS) \
	$(TEST_OBJECTS)
	$(CC) $(MAIN_OBJECTS) $(TEST_OBJECTS) $(LIBRARY_DIRS) $(MAIN_LIBS) -pg -o build/master/master.bin

build/master/%.o: %.cpp build/master/%.d build/master/sentinel
	$(CC) -c $(MAIN_DEPENDENCY_FLAGS) $(CC_FLAGS) $< -o $@
	$(MAIN_POST)

build/master/unit_tests/%.o: unit_tests/%.cpp build/master/unit_tests/%.d build/master/sentinel
	$(CC) -c $(TEST_DEPENDENCY_FLAGS) $(CC_FLAGS) $< -o $@
	$(MAIN_POST)

build/master/sentinel:
	mkdir -p build
	mkdir -p build/master
	mkdir -p build/master/unit_tests
	touch build/master/sentinel

build/master/%.d: ;

-include $(MAIN_OBJECTS:build/master/%.o=build/master/%.d)

build/glfw/src/libglfw3.a:
	mkdir -p build
	mkdir -p build/glfw
	cd build/glfw && cmake ../../submodules/glfw/ && make

build/glad/libglad.a: build/glad/loader/src/glad.c
	cd build/glad && $(CC) -c loader/src/glad.c -o loader/src/glad.o -Iloader/include
	cd build/glad && ar rcs libglad.a loader/src/glad.o

build/glad/loader/src/glad.c:
	mkdir -p build
	cp -r submodules/glad build
	cd build/glad && python setup.py build
	cd build/glad && python -m glad --profile compatibility --out-path loader --api "gl=3.3" --generator c

#build/tinyobjloader/libtinyobjloader.a:
#	mkdir -p build
#	mkdir -p build/tinyobjloader
#	cd build/tinyobjloader && $(CC) $(CC_FLAGS) -c ../../submodules/tinyobjloader/tiny_obj_loader.cc -o tiny_obj_loader.o -Isubmodules/tinyobjloader
#	cd build/tinyobjloader && ar rcs libtinyobjloader.a tiny_obj_loader.o

IMGUI_SOURCES = \
	submodules/imgui/imgui.cpp \
	submodules/imgui/imgui_demo.cpp \
	submodules/imgui/imgui_draw.cpp \
	submodules/imgui/examples/opengl3_example/imgui_impl_glfw_gl3.cpp

IMGUI_OBJECTS = $(IMGUI_SOURCES:submodules/imgui/%.cpp=build/imgui/%.o)

build/imgui/libimgui.a: $(IMGUI_OBJECTS)
	ar rcs build/imgui/libimgui.a $(IMGUI_OBJECTS)

build/imgui/%.o: submodules/imgui/%.cpp build/imgui/GL/gl3w.h
	$(CC) -c $(CC_FLAGS) $< -o $@ -Ibuild/glad/loader/include

build/imgui/GL/gl3w.h: build/imgui/sentinel
	echo "#include <glad/glad.h>\n" >> build/imgui/GL/gl3w.h

build/imgui/sentinel:
	mkdir -p build
	mkdir -p build/imgui
	mkdir -p build/imgui/GL
	mkdir -p build/imgui/examples
	mkdir -p build/imgui/examples/opengl3_example
	touch build/imgui/sentinel

build/googletest/libgtest.a:
	mkdir -p build
	mkdir -p build/googletest
	cd build/googletest && cmake ../../submodules/googletest/googletest
	cd build/googletest && make

build/embree/libembree.a:
	mkdir -p build
	mkdir -p build/embree
	cd build/embree && cmake ../../submodules/embree -DENABLE_STATIC_LIB=ON
	cd build/embree && make embree

build/assimp/code/libassimp.a:
	mkdir -p build
	mkdir -p build/assimp
	cd build/assimp && cmake ../../submodules/assimp -DBUILD_SHARED_LIBS=OFF
	cd build/assimp && make

run: all
	./build/master/master.bin

clean:
	rm -rf build/master	

distclean:
	rm -rf build
