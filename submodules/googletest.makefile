
googletest.target=build/googletest/libgtest.a
googletest.submodule=submodules/googletest/README.md

$(googletest.submodule):
	git submodule update --init submodules/googletest

build/googletest/libgtest.a: $(googletest.submodule)
	mkdir -p build
	mkdir -p build/googletest
	cd build/googletest && cmake ../../submodules/googletest/googletest
	cd build/googletest && make $(NUM_THREADS)
