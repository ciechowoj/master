
INCLUDE_DIRS = \
	-Isubmodules/AntTweakBar/include \
	-Isubmodules/glfw/include \
	-Isubmodules/glm \
	-I.

LIBRARY_DIRS = \
	-Lbuild/glfw/src

CC = g++
CC_FLAGS = -g -O0 -w -Wall -std=c++11 $(INCLUDE_DIRS)

MAIN_HEADERS = $(wildcard *.hpp)
MAIN_SOURCES = $(wildcard *.cpp)
MAIN_OBJECTS = $(MAIN_SOURCES:%.cpp=build/master/%.o)
MAIN_LIBS = -lglfw3 -lX11 -lXrandr -lXi -lXxf86vm -lXcursor -lXinerama -ldl -lpthread
MAIN_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/$*.Td
MAIN_POST = mv -f build/master/$*.Td build/master/$*.d

all: master

master: build/master/master.bin

build/master/master.bin: $(MAIN_OBJECTS) build/glfw/src/libglfw3.a
	$(CC) $(MAIN_OBJECTS) $(LIBRARY_DIRS) $(MAIN_LIBS) -o build/master/master.bin

build/master/%.o: %.cpp build/master/%.d build/master/sentinel
	$(CC) -c $(MAIN_DEPENDENCY_FLAGS) $(CC_FLAGS) $< -o $@
	$(MAIN_POST)

master_dir: build/master/sentinel

build/master/sentinel:
	mkdir -p build
	mkdir -p build/master
	touch build/master/sentinel

build/master/%.d: ;

include $(MAIN_OBJECTS:build/master/%.o=build/master/%.d)

build/glfw/src/libglfw3.a:
	mkdir -p build
	mkdir -p build/glfw
	cd build/glfw && cmake ../../submodules/glfw/ && make

run: all
	./build/master/master.bin

clean:
	rm -rf build
