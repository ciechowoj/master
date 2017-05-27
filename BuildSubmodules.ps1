

# /I"C:\openexr-2.2.0\build\IlmImf" 
# /I"C:\openexr-2.2.0\IlmImf" /I"C:\openexr-2.2.0\build\config" 
# /I"C:\openexr-2.2.0\IlmImfUtil" /I"C:\openexr-2.2.0\exrmaketiled"
# /I"C:\openexr-2.2.0\exrenvmap" /I"C:\openexr-2.2.0\exrmakepreview" 
# /I"C:\openexr-2.2.0\exrmultiview" /I"C:\openexr-2.2.0\IlmImfFuzzTest" 
# /I"C:\ilmbase-2.2.0\include\OpenEXR" /I"C:\zlib-1.2.11" /I"C:\zlib-1.2.11\build" 
# /Fd"IlmImf.dir\Release\IlmImf.pdb" 
#  
# /D "WIN32" /D "_WINDOWS" /D "NDEBUG" /D "HAVE_CONFIG_H" 
# /D "ILM_IMF_TEST_IMAGEDIR=\"C:/openexr-2.2.0/IlmImfTest/\"" 
# /D "CMAKE_INTDIR=\"Release\"" /D "_MBCS" 
# /Fa"Release/" 
# /Fo"IlmImf.dir\Release\" /Fp"IlmImf.dir\Release\IlmImf-2_2.pch" /diagnostics:classic 

mkdir -Force build | Out-Null

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

        $warningFlags = "/W3",  "/WX-"
        $generalFlags = "/GS /GR /TP /MT /Gd /Zc:wchar_t /nologo /diagnostics:classic /errorReport:prompt /EHsc "
        $optimizationFlags = "/O2 /Ob2 /Zc:inline /fp:precise "
        $securityFlags = "/Zc:forScope "
        $otherFlags = "/Gm- "

        $flags = $warningFlags, '/I"loader/include"'

        cl @flags /c "loader/src/glad.c"
        lib '/out:"glad.lib"' '/MACHINE:X64' '/NOLOGO' 'glad.obj'
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
            -DEMBREE_ISPC_EXECUTABLE="C:\ispc-v1.9.1-windows-vs2015\ispc.exe" `
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

# BuildGLAD -skipGenerating
BuildEmbree