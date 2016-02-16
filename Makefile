
INCLUDE_DIRS = \
	-Isubmodules/AntTweakBar/include \
	-Isubmodules/glfw/include \
	-Isubmodules/glm \
	-Ibuild/glad/loader/include \
	-I.

LIBRARY_DIRS = \
	-Lbuild/glfw/src \
	-Lbuild/glad

CC = g++
CC_FLAGS = -g -O0 -w -Wall -std=c++11 $(INCLUDE_DIRS)

MAIN_HEADERS = $(wildcard *.hpp)
MAIN_SOURCES = $(wildcard *.cpp)
MAIN_OBJECTS = $(MAIN_SOURCES:%.cpp=build/master/%.o)
MAIN_LIBS = -lglfw3 -lX11 -lXrandr -lXi -lXxf86vm -lXcursor -lXinerama -ldl -lpthread -lglad
MAIN_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/master/$*.Td
MAIN_POST = mv -f build/master/$*.Td build/master/$*.d

all: master

master: build/master/master.bin

build/master/master.bin: build/glad/libglad.a build/glfw/src/libglfw3.a $(MAIN_OBJECTS)
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

build/glad/libglad.a: build/glad/loader/src/glad.c
	cd build/glad && $(CC) -c loader/src/glad.c -o loader/src/glad.o -Iloader/include
	cd build/glad && ar rcs libglad.a loader/src/glad.o

build/glad/loader/src/glad.c:
	cp -r submodules/glad build/
	cd build/glad && python setup.py build
	cd build/glad && python -m glad --profile core --out-path loader --api "gl=3.3" --generator c
	
run: all
	./build/master/master.bin

clean:
	rm -rf build
