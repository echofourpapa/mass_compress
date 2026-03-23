workspace "Mass Compress"
   configurations { "Debug", "Release" }
   platforms { "x64" }
   location "build"
   startproject "Mass Compress"
   filter { "platforms:Win64" }
      system "Windows"
      architecture "x86_64"

   group "thirdparty"
      include "thirdparty/ispc.lua"
      include "thirdparty/compressonator.lua"

   group ""
      project "Mass Compress"
         kind "ConsoleApp"
         language "C++"
         cppdialect "C++23"
         location "build/projects/mass-comp"
         targetdir "bin/%{cfg.buildcfg}"
         targetname "mass-compress"
         debugdir "bin/%{cfg.buildcfg}"
         files {"src/**.cpp", "src/**.h"}
         staticruntime "On"
         links {"ispc_texcomp", "cmp_compressonatorlib", "cmp_framework", "cmp_core"}
         dependson { "EncodeWith_DXC" }
         externalincludedirs {
            "thirdparty/ISPCTextureCompressor/ispc_texcomp",
            "thirdparty/compressonator/cmp_compressonatorlib",
            "thirdparty/compressonator/cmp_framework",
            "thirdparty/compressonator/cmp_framework/common",
            "thirdparty/compressonator/cmp_framework/common/half",
            "thirdparty/compressonator/cmp_core/source",
            "thirdparty/compressonator/applications/_plugins/common",
            "thirdparty/stb",
         }

         td = path.getabsolute("bin/%{cfg.buildcfg}")
         ap = path.getabsolute("build/x64/%{cfg.buildcfg}")
         shader_src = path.getabsolute("thirdparty/compressonator/cmp_core/shaders")
         postbuildcommands {
               -- ISPC DLL
               "copy /Y \"" .. ap .. "\\ispc_texcomp.dll\" \"" .. td .. "\\ispc_texcomp.dll\"",
               "copy /Y \"" .. ap .. "\\ispc_texcomp.pdb\" \"" .. td .. "\\ispc_texcomp.pdb\"",

               -- GPU shader directory
               "if not exist \"" .. td .. "\\plugins\\Compute\" mkdir \"" .. td .. "\\plugins\\Compute\"",

               -- HLSL shaders for DirectX compute compression
               "copy /Y \"" .. shader_src .. "\\bc1_encode_kernel.hlsl\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bc4_encode_kernel.hlsl\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bc5_encode_kernel.hlsl\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bc7_encode_kernel.hlsl\" \"" .. td .. "\\plugins\\Compute\\\"",

               -- Shader include headers
               "copy /Y \"" .. shader_src .. "\\bcn_common_kernel.h\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bcn_common_api.h\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\common_def.h\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bc1_common_kernel.h\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bc1_cmp.h\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bc7_common_encoder.h\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bc7_cmpmsc.h\" \"" .. td .. "\\plugins\\Compute\\\"",
               "copy /Y \"" .. shader_src .. "\\bc6_common_encoder.h\" \"" .. td .. "\\plugins\\Compute\\\"",
            }

         filter "configurations:Debug"
            defines { "DEBUG", "NOMINMAX" }
            symbols "On"

         filter "configurations:Release"
            defines { "NDEBUG", "NOMINMAX" }
            optimize "On"
