
glm.submodule=submodules/glm/readme.md

$(glm.submodule):
	git submodule update --init submodules/glm
