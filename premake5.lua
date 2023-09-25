workspace "Mass Compress"
   configurations { "Debug", "Release" }
   platforms { "x64" }
   location "build"
   startproject "Mass Compress"
   filter { "platforms:Win64" }
      system "Windows"
      architecture "x86_64"

   group "thirdparty"
      -- TODO: Move all 3rd party libs to their own files
      externalproject "ispc_texcomp"
         location "thirdparty/ISPCTextureCompressor/ispc_texcomp"
         uuid(os.uuid("ispc_texcomp"))
         kind "SharedLib"
         language "C++"

   group ""
      project "Mass Compress"
         kind "ConsoleApp"
         language "C++"
         cppdialect "C++17"
         location "build/projects/mass-comp"
         targetdir "bin/%{cfg.buildcfg}"
         targetname "mass-compress"
         debugdir "bin/%{cfg.buildcfg}"
         files {"src/**.cpp", "src/**.h"}
         links {"ispc_texcomp"}
         externalincludedirs {
            "thirdparty/ISPCTextureCompressor/ispc_texcomp",
            "thirdparty/stb",
         }

         td = path.getabsolute("bin/%{cfg.buildcfg}")
         ap = path.getabsolute("build/x64/%{cfg.buildcfg}")
         postbuildcommands {          
               "copy /Y \"" .. ap .. "\\ispc_texcomp.dll\" \"" .. td .. "\\ispc_texcomp.dll\"",
               "copy /Y \"" .. ap .. "\\ispc_texcomp.pdb\" \"" .. td .. "\\ispc_texcomp.pdb\"",
            }

         filter "configurations:Debug"
            defines { "DEBUG", "NOMINMAX" }
            symbols "On"
   
         filter "configurations:Release"
            defines { "NDEBUG", "NOMINMAX" }
            optimize "On"

