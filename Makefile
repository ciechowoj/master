
NUM_THREADS=-j4

INCLUDE_DIRS = \
	-I/usr/include/OpenEXR \
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

CC = gcc
CCFLAGS = -march=native -O2 -ffast-math -w -Wall $(INCLUDE_DIRS) -DGLM_FORCE_RADIANS -DGLM_SWIZZLE

CXX = g++
CXXFLAGS = -march=native -O2 -ffast-math -w -Wall -std=c++11 $(INCLUDE_DIRS) -DGLM_FORCE_RADIANS -DGLM_SWIZZLE

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

STD_LIBS = \
	-ldl \
	-lpthread \
	-lIlmImf \
	-lIex

MAIN_HEADERS = $(wildcard *.hpp)
MAIN_SOURCES := $(wildcard *.cpp)
MAIN_SOURCES := $(filter-out main.cpp, $(MAIN_SOURCES))
MAIN_OBJECTS = $(MAIN_SOURCES:%.cpp=build/master/%.o)
MAIN_LIBS = $(GLFW_LIBS) $(STD_LIBS) -lglad -limgui -lgtest $(EMBREE_LIBS) -lassimp -lz
MAIN_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/$*.Td
MAIN_POST = mv -f build/master/$*.Td build/master/$*.d

TEST_SOURCES = $(wildcard unit_tests/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:%.cpp=build/master/%.o)
TEST_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/unit_tests/$*.Td
TEST_POST = mv -f build/master/unit_tests/$*.Td build/master/unit_tests/$*.d

LIB_DEPENDENCIES = \
	build/glad/libglad.a \
	$(glfw.target) \
	$(assimp.target) \
	build/imgui/libimgui.a \
	build/googletest/libgtest.a \
	build/embree/libembree.a

all: master unittest

include submodules/assimp.makefile
include submodules/glfw.makefile

.PHONY: all master unittest

master: build/master/master.bin

unittest: build/master/unittest.bin

build/master/master.bin: \
	$(LIB_DEPENDENCIES) \
	Makefile \
	$(MAIN_OBJECTS) \
	build/master/main.o
	$(CXX) $(MAIN_OBJECTS) build/master/main.o $(LIBRARY_DIRS) $(MAIN_LIBS) -o build/master/master.bin

build/master/unittest.bin: \
	$(LIB_DEPENDENCIES) \
	Makefile \
	$(MAIN_OBJECTS) \
	$(TEST_OBJECTS)
	$(CXX) $(MAIN_OBJECTS) $(TEST_OBJECTS) $(LIBRARY_DIRS) $(MAIN_LIBS) -o build/master/unittest.bin

build/master/%.o: %.cpp build/master/%.d build/master/sentinel
	$(CXX) -c $(MAIN_DEPENDENCY_FLAGS) $(CXXFLAGS) $< -o $@
	$(MAIN_POST)

build/master/unit_tests/%.o: unit_tests/%.cpp build/master/%.d build/master/unit_tests/%.d build/master/sentinel
	$(CXX) -c $(TEST_DEPENDENCY_FLAGS) $(CXXFLAGS) $< -o $@
	$(MAIN_POST)

build/master/sentinel:
	mkdir -p build
	mkdir -p build/master
	mkdir -p build/master/unit_tests
	touch build/master/sentinel

build/master/%.d: ;

-include $(MAIN_OBJECTS:build/master/%.o=build/master/%.d)
-include build/master/main.d
-include $(TEST_OBJECTS:build/master/unit_tests/%.o=build/master/unit_tests/%.d)

build/glad/libglad.a: build/glad/loader/src/glad.c
	cd build/glad && $(CC) -c loader/src/glad.c -o loader/src/glad.o -Iloader/include
	cd build/glad && ar rcs libglad.a loader/src/glad.o

build/glad/loader/src/glad.c:
	mkdir -p build
	cp -r submodules/glad build
	cd build/glad && python setup.py build
	cd build/glad && python -m glad --profile compatibility --out-path loader --api "gl=3.3" --generator c

IMGUI_SOURCES = \
	submodules/imgui/imgui.cpp \
	submodules/imgui/imgui_demo.cpp \
	submodules/imgui/imgui_draw.cpp \
	submodules/imgui/examples/opengl3_example/imgui_impl_glfw_gl3.cpp

IMGUI_OBJECTS = $(IMGUI_SOURCES:submodules/imgui/%.cpp=build/imgui/%.o)

build/imgui/libimgui.a: $(IMGUI_OBJECTS)
	ar rcs build/imgui/libimgui.a $(IMGUI_OBJECTS)

build/imgui/%.o: submodules/imgui/%.cpp build/imgui/GL/gl3w.h
	$(CC) -c $(CCFLAGS) $< -o $@ -Ibuild/glad/loader/include

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
	cd build/googletest && make $(NUM_THREADS)

build/embree/libembree.a:
	mkdir -p build
	mkdir -p build/embree
	cd build/embree && cmake ../../submodules/embree -DENABLE_STATIC_LIB=ON -DENABLE_TUTORIALS=OFF
	cd build/embree && make embree $(NUM_THREADS)

run: all
	./build/master/master.bin models/FurnaceTest.blend --PT --parallel

test: all
	./build/master/unittest.bin

clean:
	rm -rf build/master	

distclean:
	rm -rf build
