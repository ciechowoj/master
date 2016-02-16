
CC = g++
CC_FLAGS = -g -O0 -w -Wall -std=c++11 -Iinclude -Igoogletest/googletest/include
CC_DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/$*.Td
POST_CC = mv -f build/$*.Td build/$*.d

HASTE_HEADERS = $(wildcard *.hpp)
HASTE_SOURCES = $(wildcard *.cpp)
HASTE_OBJECTS = $(HASTE_SOURCES:%.cpp=build/%.o)

TEST_SOURCES = $(wildcard tests/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:%.cpp=build/%.o)

GTEST_LIBS = -lgtest_main -lgtest -pthread
GTEST_LIBS_DIR = -Lgoogletest/googletest/build

run: build
	./build/test.bin

build: build/libformat.a build/test.bin;

build/test.bin: $(TEST_OBJECTS) build/libformat.a
	$(CC) $(TEST_OBJECTS) $(GTEST_LIBS_DIR) $(GTEST_LIBS) -Lbuild -lformat -o build/test.bin

build/libformat.a: $(HASTE_OBJECTS)
	ar rcs build/libformat.a $(HASTE_OBJECTS)

build/%.o : %.cpp build/%.d
	$(CC) -c $(CC_DEPENDENCY_FLAGS) $(CC_FLAGS) $< -o $@
	$(POST_CC)

build/tests/%.o : tests/%.cpp build/tests/%.d
	$(CC) -c $(CC_DEPENDENCY_FLAGS) $(CC_FLAGS) $< -o $@
	$(POST_CC)

build/%.d: ;

include $(HASTE_SOURCES:%.cpp=build/%.d)

# clean:
#	rm -f $(EXEC) $(OBJECTS)
