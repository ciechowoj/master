
NUM_THREADS=-j4

INCLUDE_DIRS = \
	-I/usr/include/OpenEXR \
	-Isubmodules/glfw/include \
	-Isubmodules/glm \
	-Isubmodules/tinyobjloader \
	-Isubmodules/imgui \
	-Isubmodules/imgui/examples/opengl3_example \
	-Isubmodules/embree/include \
	-Isubmodules/assimp/include \
	-Ibuild/glad/loader/include \
	-Ibuild/imgui \
	-I.

LIBRARY_DIRS = \
	-Lbuild/glfw/src \
	-Lbuild/tinyobjloader \
	-Lbuild/imgui \
	-Lbuild/glad \
	-Lbuild/embree \
	-Lbuild/assimp/code

CXX = g++
CC = g++

CXXFLAGS = -march=native -g -O2 -Wall -std=c++11 $(INCLUDE_DIRS) -DGLM_FORCE_RADIANS -DGLM_SWIZZLE
CCFLAGS = -march=native -g -O2 -Wall -std=c++11 $(INCLUDE_DIRS) -DGLM_FORCE_RADIANS -DGLM_SWIZZLE

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
MAIN_LIBS = $(GLFW_LIBS) $(STD_LIBS) -lglad -limgui $(EMBREE_LIBS) -lassimp -lz
MAIN_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/$*.Td
MAIN_POST = mv -f build/master/$*.Td build/master/$*.d

LIB_DEPENDENCIES = \
	$(glfw.target) \
	$(assimp.target) \
	$(glad.target) \
	$(imgui.target) \
	$(embree.target) \
	$(glm.submodule)

all: master

include submodules/assimp.makefile
include submodules/glfw.makefile
include submodules/embree.makefile
include submodules/glad.makefile
include submodules/imgui.makefile
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

build/master/%.o: %.cpp build/master/%.d | build/master
	$(CXX) -c $(MAIN_DEPENDENCY_FLAGS) $(CXXFLAGS) $< -o $@
	$(MAIN_POST)

build:
	mkdir -p build

build/master: | build
	mkdir -p build/master

build/master/%.d: ;

-include $(MAIN_OBJECTS:build/master/%.o=build/master/%.d)
-include build/master/main.d
-include build/master/benchmark.d

run: all
	./build/master/master.bin --beta=2 --parallel models/TestCase17.blend --UPG --radius=0.075 --no-vc --output=TestCase17.UPG2.exr --snapshot=360 --batch

profile: all
	time master models/Bearings.blend --UPG --parallel --beta=2 --max-radius=0.2 --num-samples=1 --batch --quiet

clean:
	rm -rf build/master

distclean:
	rm -rf build
