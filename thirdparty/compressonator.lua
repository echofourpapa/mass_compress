-- thirdparty/compressonator.lua
-- AMD Compressonator external projects and GPU plugin
-- Note: paths are relative to the workspace root since premake include() changes to this file's directory

externalproject "cmp_compressonatorlib"
    location "compressonator/build_sdk"
    uuid(os.uuid("cmp_compressonatorlib"))
    kind "StaticLib"
    language "C++"

externalproject "cmp_framework"
    location "compressonator/build_sdk"
    uuid(os.uuid("cmp_framework"))
    kind "StaticLib"
    language "C++"

externalproject "cmp_core"
    location "compressonator/build_sdk"
    uuid(os.uuid("cmp_core"))
    kind "StaticLib"
    language "C++"

-- GPU DirectX compute plugin (new premake-native project)
project "EncodeWith_DXC"
    kind "SharedLib"
    language "C++"
    cppdialect "C++17"
    location "../build/projects/encode_dxc"
    targetdir "../bin/%{cfg.buildcfg}"
    targetname "EncodeWith_DXC"
    staticruntime "On"

    files {
        "compressonator/applications/_plugins/cmp_gpu/directx/cdirectx.cpp",
        "compressonator/applications/_plugins/cmp_gpu/directx/cdirectx.h",
        "compressonator/applications/_plugins/cmp_gpu/directx/compute_directx.cpp",
        "compressonator/applications/_plugins/cmp_gpu/directx/compute_directx.h",
    }

    defines { "BUILD_AS_PLUGIN_DLL" }

    links {
        "cmp_compressonatorlib", "cmp_framework", "cmp_core",
        "d3d11", "d3dcompiler", "dxguid", "advapi32",
    }

    externalincludedirs {
        "compressonator/cmp_compressonatorlib",
        "compressonator/cmp_framework",
        "compressonator/cmp_framework/common",
        "compressonator/cmp_framework/common/half",
        "compressonator/cmp_core/shaders",
        "compressonator/cmp_core/source",
        "compressonator/applications/_plugins/common",
        "compressonator/applications/_libs/cmp_math",
    }

    filter "configurations:Debug"
        defines { "DEBUG", "NOMINMAX" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG", "NOMINMAX" }
        optimize "On"
