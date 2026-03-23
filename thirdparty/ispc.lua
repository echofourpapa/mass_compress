-- thirdparty/ispc.lua
-- ISPC Texture Compressor external project
-- Note: paths are relative to the workspace root since premake include() changes to this file's directory

externalproject "ispc_texcomp"
    location "ISPCTextureCompressor/ispc_texcomp"
    uuid(os.uuid("ispc_texcomp"))
    kind "SharedLib"
    language "C++"
