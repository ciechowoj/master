
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
CCFLAGS = -march=native -g -O2 -w -Wall $(INCLUDE_DIRS) -DGLM_FORCE_RADIANS -DGLM_SWIZZLE

CXX = g++
CXXFLAGS = -march=native -g -O2 -w -Wall -std=c++11 $(INCLUDE_DIRS) -DGLM_FORCE_RADIANS -DGLM_SWIZZLE

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
MAIN_SOURCES := $(filter-out main.cpp benchmark.cpp, $(MAIN_SOURCES))
MAIN_OBJECTS = $(MAIN_SOURCES:%.cpp=build/master/%.o)
MAIN_LIBS = $(GLFW_LIBS) $(STD_LIBS) -lglad -limgui -lgtest $(EMBREE_LIBS) -lassimp -lz
MAIN_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/$*.Td
MAIN_POST = mv -f build/master/$*.Td build/master/$*.d

TEST_SOURCES = $(wildcard unit_tests/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:%.cpp=build/master/%.o)
TEST_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/unit_tests/$*.Td
TEST_POST = mv -f build/master/unit_tests/$*.Td build/master/unit_tests/$*.d

LIB_DEPENDENCIES = \
	$(glfw.target) \
	$(assimp.target) \
	$(glad.target) \
	$(imgui.target) \
	$(googletest.target) \
	$(embree.target) \
	$(glm.submodule)

all: master unittest

include submodules/assimp.makefile
include submodules/glfw.makefile
include submodules/embree.makefile
include submodules/glad.makefile
include submodules/imgui.makefile
include submodules/googletest.makefile
include submodules/glm.makefile

.PHONY: all master unittest benchmark

master: build/master/master.bin

unittest: build/master/unittest.bin

benchmark: \
	build/master/benchmark.bin
	./build/master/benchmark.bin

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

build/master/benchmark.bin: \
	$(LIB_DEPENDENCIES) \
	Makefile \
	$(MAIN_OBJECTS) \
	build/master/benchmark.o
	$(CXX) $(MAIN_OBJECTS) build/master/benchmark.o $(LIBRARY_DIRS) $(MAIN_LIBS) -o build/master/benchmark.bin

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
-include build/master/benchmark.d
-include $(TEST_OBJECTS:build/master/unit_tests/%.o=build/master/unit_tests/%.d)

build/imgui/sentinel:
	mkdir -p build
	mkdir -p build/imgui
	mkdir -p build/imgui/GL
	mkdir -p build/imgui/examples
	mkdir -p build/imgui/examples/opengl3_example
	touch build/imgui/sentinel

run: all
	./build/master/master.bin models/CornellBoxDiffuse.blend --BPT --parallel

profile:
	valgrind \
	--tool=callgrind \
	./build/master/master.bin \
	models/CornellBoxDiffuse.blend \
	--PT --max-radius=0.05 --num-photons=2000000 --parallel

test: all
	./build/master/unittest.bin

clean:
	rm -rf build/master

distclean:
	rm -rf build

# integration tests stuff

ALL_MODELS = $(wildcard models/*.blend)
TEST_MODELS = \
	models/LightPathNone.blend \
	models/LightOverBox.blend \
	models/IndirectCubeHalf.blend \
	models/IndirectCubeIOR1.blend \
	models/IndirectCubeNone.blend \
	models/IndirectCubeH25.blend \
	models/IndirectCubeEye.blend \
	models/IndirectCubeLens1.blend

IMAGES_512_512 = $(TEST_MODELS:models/%.blend=images/%.512.512.exr)
IMAGES_100_ = $(IMAGES_512_512:images/%.exr=images/%.100.exr)
IMAGES_1000_ = $(IMAGES_512_512:images/%.exr=images/%.1000.exr)
IMAGES_10000_ = $(IMAGES_512_512:images/%.exr=images/%.10000.exr)
IMAGES_100000_ = $(IMAGES_512_512:images/%.exr=images/%.100000.exr)

IMAGES_100 = \
	$(IMAGES_100_:images/%.exr=images/%.VCM.exr)


IMAGES_1000 = \
	$(IMAGES_1000_:images/%.exr=images/%.VCM.exr)

IMAGES_10000 = \
	$(IMAGES_10000_:images/%.exr=images/%.PT.exr) \
	$(IMAGES_10000_:images/%.exr=images/%.BPT.exr)

IMAGES_100000 = \
	$(IMAGES_100000_:images/%.exr=images/%.PT.exr) \
	$(IMAGES_100000_:images/%.exr=images/%.BPT.exr) \
	$(IMAGES_100000_:images/%.exr=images/%.MBPT.exr) \
	$(IMAGES_100000_:images/%.exr=images/%.VCM.exr)

# 100 samples
images/%.512.512.100.PT.exr: models/%.blend
	./build/master/master.bin $< --PT --output=$@ --batch --num-samples=100 --parallel

images/%.512.512.100.BPT.exr: models/%.blend
	./build/master/master.bin $< --BPT --output=$@ --batch --num-samples=100 --parallel

images/%.512.512.100.MBPT.exr: models/%.blend
	./build/master/master.bin $< --BPT --output=$@ --batch --num-samples=100 --parallel --beta=2

images/%.512.512.100.VCM.exr: models/%.blend
	./build/master/master.bin $< --VCM --output=$@ --batch --num-samples=100 --parallel

# 1000 samples
images/%.512.512.1000.PT.exr: models/%.blend
	./build/master/master.bin $< --PT --output=$@ --batch --num-samples=1000 --parallel

images/%.512.512.1000.BPT.exr: models/%.blend
	./build/master/master.bin $< --BPT --output=$@ --batch --num-samples=1000 --parallel

images/%.512.512.1000.MBPT.exr: models/%.blend
	./build/master/master.bin $< --BPT --output=$@ --batch --num-samples=1000 --parallel --beta=2

images/%.512.512.1000.VCM.exr: models/%.blend
	./build/master/master.bin $< --VCM --output=$@ --batch --num-samples=1000 --parallel

# 10000 samples
images/%.512.512.10000.PT.exr: models/%.blend
	./build/master/master.bin $< --PT --output=$@ --batch --num-samples=10000 --parallel

images/%.512.512.10000.BPT.exr: models/%.blend
	./build/master/master.bin $< --BPT --output=$@ --batch --num-samples=10000 --parallel

images/%.512.512.10000.MBPT.exr: models/%.blend
	./build/master/master.bin $< --BPT --output=$@ --batch --num-samples=10000 --parallel --beta=2

images/%.512.512.10000.VCM.exr: models/%.blend
	./build/master/master.bin $< --VCM --output=$@ --batch --num-samples=10000 --parallel

# 100000 samples
images/%.512.512.100000.PT.exr: models/%.blend
	./build/master/master.bin $< --PT --output=$@ --batch --num-samples=100000 --parallel

images/%.512.512.100000.BPT.exr: models/%.blend
	./build/master/master.bin $< --BPT --output=$@ --batch --num-samples=100000 --parallel

images/%.512.512.100000.MBPT.exr: models/%.blend
	./build/master/master.bin $< --BPT --output=$@ --batch --num-samples=100000 --parallel --beta=2

images/%.512.512.100000.VCM.exr: models/%.blend
	./build/master/master.bin $< --VCM --output=$@ --batch --num-samples=100000 --parallel

render-100: $(IMAGES_100)
render-1000: $(IMAGES_1000)
render-10000: $(IMAGES_10000)
render-100000: $(IMAGES_100000)
