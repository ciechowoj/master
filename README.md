
# Path tracer.

![Cornelbox](https://github.com/ciechowoj/master/blob/master/splashscreen.png)
## Reference
![Cornelbox](https://github.com/ciechowoj/master/blob/master/images/Bearings.BPT.83900s.png)
## BPT
![Cornelbox](https://github.com/ciechowoj/master/blob/master/images/Bearings.BPT.1790s.png)
## UPG
![Cornelbox](https://github.com/ciechowoj/master/blob/master/images/Bearings.UPG.0_1.1797s.png)

# Building

Required dependencies:
* g++
* python-setuptools
* libtbb-dev
* libopenexr-dev
* zlib1g-dev
* OpenEXR (`apt-get install libopenexr-dev`)
* ispc (`ispc.github.io/downloads.html`)

Depenedencies are provided as git submodules:
* assimp
* embree
* glad
* glfw
* glm
* googletest
* imgui

To build the project run the `make` command in the main directory.
