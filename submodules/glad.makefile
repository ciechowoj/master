
glad.target=build/glad/libglad.a
glad.submodule=submodules/glad/README.md

$(glad.submodule):
	git submodule update --init submodules/glad

build/glad/libglad.a: build/glad/loader/src/glad.c
	cd build/glad && $(CC) -c loader/src/glad.c -o loader/src/glad.o -Iloader/include
	cd build/glad && ar rcs libglad.a loader/src/glad.o

build/glad/loader/src/glad.c: $(glad.submodule)
	mkdir -p build
	cp -r submodules/glad build
	cd build/glad && python setup.py build
	cd build/glad && python -m glad --profile compatibility --out-path loader --api "gl=3.3" --generator c
