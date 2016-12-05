
embree.CMakeFlags = \
	-DEMBREE_STATIC_LIB=ON \
	-DEMBREE_TUTORIALS=OFF \
	-DEMBREE_RAY_MASK=ON \
	-DEMBREE_TASKING_SYSTEM=INTERNAL \
	-DEMBREE_GEOMETRY_LINES=OFF \
	-DEMBREE_GEOMETRY_HAIR=OFF \
	-DEMBREE_GEOMETRY_SUBDIV=OFF \
	-DEMBREE_GEOMETRY_USER=OFF

embree.target=build/embree/libembree.a
embree.submodule=submodules/embree/README.md

build/embree/libembree.a: $(embree.submodule)
	mkdir -p build
	mkdir -p build/embree
	cd build/embree && cmake ../../submodules/embree $(embree.CMakeFlags)
	cd build/embree && make embree $(NUM_THREADS)


$(embree.submodule):
	git submodule update --init submodules/embree
