
glfw.CMakeFlags = \
	-DGLFW_BUILD_EXAMPLES=OFF \
	-DGLFW_BUILD_TESTS=OFF \
	-DGLFW_BUILD_DOCS=OFF \
	-DGLFW_VULKAN_STATIC=OFF \
	-DGLFW_INSTALL=OFF

glfw.target=build/glfw/src/libglfw3.a
glfw.submodule=submodules/glfw/README.md

$(glfw.submodule):
	git submodule update --init submodules/glfw

build/glfw/src/libglfw3.a: $(glfw.submodule)
	mkdir -p build
	mkdir -p build/glfw
	cd build/glfw && cmake ../../submodules/glfw $(glfw.CMakeFlags)
	cd build/glfw && make $(NUM_THREADS)
