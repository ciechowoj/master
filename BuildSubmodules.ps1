
mkdir -Force build | Out-Null

$warningFlags = "/W3",  "/WX-"
$generalFlags = "/GS /GR /TP /MT /Gd /Zc:wchar_t /nologo /diagnostics:classic /errorReport:prompt /EHsc "
$optimizationFlags = "/O2 /Ob2 /Zc:inline /fp:precise "
$securityFlags = "/Zc:forScope "
$otherFlags = "/Gm- "

$cxxFlags = $warningFlags

function UpdateRuntimeLibrary() {
    (Get-Content CMakeCache.txt).replace('/MD', '/MT').replace('/MDd', '/MTd') | Set-Content CMakeCache.txt
    cmake .
}

function BuildOpenEXR([switch]$skipIlmBase = $false, [switch]$skipOpenEXR = $false, $configuration = "Release") {
    $location = get-location

    try {
        mkdir -Force build/ilmBase | Out-Null
        mkdir -Force build/openexr | Out-Null
        set-location build/ilmBase
        $ilmBaseLocation = "../../submodules/openexr/IlmBase/"
        $openexrLocation = "../../submodules/openexr/OpenEXR/"

        if (-Not $skipIlmBase) {
            cmake $ilmBaseLocation `
                -DCMAKE_INSTALL_PREFIX="../openexr/deploy" `
                -DBUILD_SHARED_LIBS=OFF -G "Visual Studio 15 2017 Win64" 
            UpdateRuntimeLibrary
            cmake --build .
            cmake --build . --target INSTALL --config $configuration
        }

        set-location ../openexr

        if (-Not $skipOpenEXR) {
            cmake $openexrLocation `
                -DCMAKE_INSTALL_PREFIX="deploy" `
                -DBUILD_SHARED_LIBS=OFF `
                -DZLIB_LIBRARY="C:`zlib-1.2.11`build`Release`zlibstatic.lib" `
                -DZLIB_INCLUDE_DIR="C:`zlib-1.2.11" `
                -DILMBASE_PACKAGE_PREFIX="$(get-location)`deploy" `
                -G "Visual Studio 15 2017 Win64" 
            UpdateRuntimeLibrary            
            cmake --build .
            cmake --build . --target INSTALL --config $configuration
        }
    }
    finally {
        set-location $location
    }
}

function BuildGLAD([switch]$skipGenerating = $false) {
    $location = get-location

    try {
        if (-Not $skipGenerating) {
            mkdir -Force build/glad | Out-Null
            copy-item -Force -Recurse submodules/glad/* build/glad
        }

        set-location build/glad

        if (-Not $skipGenerating) {
            python -m glad --profile compatibility --out-path loader --api "gl=3.3" --generator c
        }

        $flags = $cxxFlags, '/I"loader/include"'

        cl @flags /c "loader/src/glad.c"
        lib '/out:"glad.lib"' '/MACHINE:X64' '/NOLOGO' 'glad.obj'
    }
    finally {
        set-location $location
    }
}

function BuildImGUI() {
    $location = get-location

    try {
        mkdir -Force build/imgui | Out-Null
        set-location build/imgui

        $imGuiBaseLocation = "../../submodules/imgui"

        Copy-Item "$imGuiBaseLocation/imconfig.h" ./
        Copy-Item "$imGuiBaseLocation/imgui.h" ./
        Copy-Item "$imGuiBaseLocation/imgui_internal.h" ./
        Copy-Item "$imGuiBaseLocation/stb_textedit.h" ./
        Copy-Item "$imGuiBaseLocation/stb_rect_pack.h" ./
        Copy-Item "$imGuiBaseLocation/stb_truetype.h" ./
        Copy-Item "$imGuiBaseLocation/imgui.cpp" ./
        Copy-Item "$imGuiBaseLocation/imgui_draw.cpp" ./
        Copy-Item "$imGuiBaseLocation/examples/opengl3_example/imgui_impl_glfw_gl3.h" ./
        Copy-Item "$imGuiBaseLocation/examples/opengl3_example/imgui_impl_glfw_gl3.cpp" ./
        (Get-Content imgui_impl_glfw_gl3.cpp).replace('GL/gl3w.h', 'glad/glad.h') | Set-Content imgui_impl_glfw_gl3.cpp

        $flags = $cxxFlags, '/I.', '/I../glad/loader/include', '/I../glfw/deploy/include'

        cl @flags /c "imgui.cpp" /Fo"imgui.obj"
        cl @flags /c "imgui_draw.cpp" /Fo"imgui_draw.obj"
        cl @flags /c "imgui_impl_glfw_gl3.cpp" /Fo"imgui_glfw.obj"
        lib '/out:"imgui.lib"' '/MACHINE:X64' '/NOLOGO' 'imgui.obj' 'imgui_draw.obj' 'imgui_glfw.obj'
    }
    finally {
        set-location $location
    }
}

function BuildEmbree() {
    $location = get-location

    try {
        mkdir -Force build/embree | Out-Null
        set-location build/embree

        cmake ../../submodules/embree/ -G 'Visual Studio 15 2017 Win64' `
            -DCMAKE_INSTALL_PREFIX="deploy" `
            -DEMBREE_ISPC_EXECUTABLE="C:`ispc-v1.9.1-windows-vs2015`ispc.exe" `
            -DEMBREE_STATIC_LIB=ON `
            -DEMBREE_TUTORIALS=OFF `
            -DEMBREE_RAY_MASK=ON `
            -DEMBREE_TASKING_SYSTEM=INTETNAL `
            -DEMBREE_GEOMETRY_LINES=OFF `
            -DEMBREE_GEOMETRY_HAIR=OFF `
            -DEMBREE_GEOMETRY_SUBDIV=OFF `
            -DEMBREE_GEOMETRY_USER=OFF

        devenv "embree2.sln" /build Release
    }
    finally {
        set-location $location
    }
}

function BuildGLFW() {
    $location = get-location

    try {
        mkdir -Force build/glfw | Out-Null
        set-location build/glfw

        cmake ../../submodules/glfw/ -G 'Visual Studio 15 2017 Win64' `
            -DCMAKE_INSTALL_PREFIX="deploy" `
            -DGLFW_BUILD_EXAMPLES=OFF `
            -DGLFW_BUILD_TESTS=OFF `
            -DGLFW_BUILD_DOCS=OFF `
            -DUSE_MSVC_RUNTIME_LIBRARY_DLL=OFF 

        cmake --build . --target INSTALL --config Release
    }
    finally {
        set-location $location
    }

}

function BuildAssimp() {
    $location = get-location

    try {
        mkdir -Force build/assimp | Out-Null
        set-location build/assimp

        $assimpBaseLocation = "../../submodules/assimp"

       cmake $assimpBaseLocation `
            -G 'Visual Studio 15 2017 Win64' `
            -DCMAKE_INSTALL_PREFIX="deploy" `
            -DBUILD_SHARED_LIBS=OFF `
            -DASSIMP_NO_EXPORT=ON `
            -DASSIMP_BUILD_ASSIMP_TOOLS=OFF `
            -DASSIMP_BUILD_TESTS=OFF `
            -DASSIMP_BUILD_3DS_IMPORTER=FALSE `
            -DASSIMP_BUILD_AC_IMPORTER=FALSE `
            -DASSIMP_BUILD_ASE_IMPORTER=FALSE `
            -DASSIMP_BUILD_ASSBIN_IMPORTER=FALSE `
            -DASSIMP_BUILD_ASSXML_IMPORTER=FALSE `
            -DASSIMP_BUILD_B3D_IMPORTER=FALSE `
            -DASSIMP_BUILD_BVH_IMPORTER=FALSE `
            -DASSIMP_BUILD_COLLADA_IMPORTER=FALSE `
            -DASSIMP_BUILD_DXF_IMPORTER=FALSE `
            -DASSIMP_BUILD_CSM_IMPORTER=FALSE `
            -DASSIMP_BUILD_HMP_IMPORTER=FALSE `
            -DASSIMP_BUILD_LWO_IMPORTER=FALSE `
            -DASSIMP_BUILD_LWS_IMPORTER=FALSE `
            -DASSIMP_BUILD_MD2_IMPORTER=FALSE `
            -DASSIMP_BUILD_MD3_IMPORTER=FALSE `
            -DASSIMP_BUILD_MD5_IMPORTER=FALSE `
            -DASSIMP_BUILD_MDC_IMPORTER=FALSE `
            -DASSIMP_BUILD_MDL_IMPORTER=FALSE `
            -DASSIMP_BUILD_NFF_IMPORTER=FALSE `
            -DASSIMP_BUILD_NDO_IMPORTER=FALSE `
            -DASSIMP_BUILD_OFF_IMPORTER=FALSE `
            -DASSIMP_BUILD_OGRE_IMPORTER=FALSE `
            -DASSIMP_BUILD_OPENGEX_IMPORTER=FALSE `
            -DASSIMP_BUILD_PLY_IMPORTER=FALSE `
            -DASSIMP_BUILD_MS3D_IMPORTER=FALSE `
            -DASSIMP_BUILD_COB_IMPORTER=FALSE `
            -DASSIMP_BUILD_IFC_IMPORTER=FALSE `
            -DASSIMP_BUILD_XGL_IMPORTER=FALSE `
            -DASSIMP_BUILD_FBX_IMPORTER=FALSE `
            -DASSIMP_BUILD_Q3D_IMPORTER=FALSE `
            -DASSIMP_BUILD_Q3BSP_IMPORTER=FALSE `
            -DASSIMP_BUILD_RAW_IMPORTER=FALSE `
            -DASSIMP_BUILD_SIB_IMPORTER=FALSE `
            -DASSIMP_BUILD_SMD_IMPORTER=FALSE `
            -DASSIMP_BUILD_STL_IMPORTER=FALSE `
            -DASSIMP_BUILD_TERRAGEN_IMPORTER=FALSE `
            -DASSIMP_BUILD_3D_IMPORTER=FALSE `
            -DASSIMP_BUILD_X_IMPORTER=FALSE `
            -DASSIMP_BUILD_GLTF_IMPORTER=FALSE

        UpdateRuntimeLibrary        
        cmake --build . --target INSTALL --config Release
    }
    finally {
        set-location $location
    }
}

# BuildOpenEXR
# BuildGLAD
# BuildEmbree
# BuildGLFW
# BuildImGUI
BuildAssimp