
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

CXX = g++
CXXFLAGS = -march=native -g -O1 -Wall -std=c++11 $(INCLUDE_DIRS) -DGLM_FORCE_RADIANS -DGLM_SWIZZLE

EMBREE_LIBS = \
	-lembree \
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
MAIN_SOURCES := $(filter-out main.cpp benchmark.cpp unittest.cpp, $(MAIN_SOURCES))
MAIN_OBJECTS = $(MAIN_SOURCES:%.cpp=build/master/%.o)
MAIN_LIBS = $(GLFW_LIBS) $(STD_LIBS) -lglad -limgui -lgtest $(EMBREE_LIBS) -lassimp -lz
MAIN_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/$*.Td
MAIN_POST = mv -f build/master/$*.Td build/master/$*.d

LIB_DEPENDENCIES = \
	$(glfw.target) \
	$(assimp.target) \
	$(glad.target) \
	$(imgui.target) \
	$(googletest.target) \
	$(embree.target) \
	$(glm.submodule)

all: master

include submodules/assimp.makefile
include submodules/glfw.makefile
include submodules/embree.makefile
include submodules/glad.makefile
include submodules/imgui.makefile
include submodules/googletest.makefile
include submodules/glm.makefile

.PHONY: all master benchmark

master: build/master/master.bin

benchmark: \
	build/master/benchmark.bin
	./build/master/benchmark.bin

build/master/master.bin: \
	$(LIB_DEPENDENCIES) \
	Makefile \
	$(MAIN_OBJECTS) \
	build/master/main.o
	$(CXX) $(MAIN_OBJECTS) build/master/main.o $(LIBRARY_DIRS) $(MAIN_LIBS) -o build/master/master.bin

build/master/benchmark.bin: \
	$(LIB_DEPENDENCIES) \
	Makefile \
	$(MAIN_OBJECTS) \
	build/master/benchmark.o
	$(CXX) $(MAIN_OBJECTS) build/master/benchmark.o $(LIBRARY_DIRS) $(MAIN_LIBS) -o build/master/benchmark.bin

build/master/%.o: %.cpp build/master/%.d build/master/sentinel
	$(CXX) -c $(MAIN_DEPENDENCY_FLAGS) $(CXXFLAGS) $< -o $@
	$(MAIN_POST)

build/master/sentinel:
	mkdir -p build
	mkdir -p build/master
	touch build/master/sentinel

build/master/%.d: ;

-include $(MAIN_OBJECTS:build/master/%.o=build/master/%.d)
-include build/master/main.d
-include build/master/benchmark.d

build/imgui/sentinel:
	mkdir -p build
	mkdir -p build/imgui
	mkdir -p build/imgui/GL
	mkdir -p build/imgui/examples
	mkdir -p build/imgui/examples/opengl3_example
	touch build/imgui/sentinel

run: all
	./build/master/master.bin models/CornellBoxDiffuse.blend --UPG --beta=0 --max-radius=0.005 --parallel --no-vc --no-lights \
	--reference="/home/wojciech/cornell boxes references/CornellBoxDiffuse.512.512.100000.PT.indirect.exr"

profile:
	valgrind \
	--tool=callgrind \
	./build/master/master.bin \
	models/CornellBoxDiffuse.blend \
	--UPG --max-radius=0.01 --parallel --batch

clean:
	rm -rf build/master

distclean:
	rm -rf build
