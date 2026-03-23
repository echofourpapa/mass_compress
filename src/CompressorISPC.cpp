#include "CompressorISPC.h"
#include "ispc_texcomp.h"
#include <cassert>

bool CompressorISPC::CompressMip(
    const uint8* srcData,
    uint32 width, uint32 height,
    DXGI_FORMAT format,
    std::vector<uint8>& outData)
{
    rgba_surface comp_surf = {};
    comp_surf.width = width;
    comp_surf.height = height;
    comp_surf.stride = width * 4 * sizeof(uint8);
    comp_surf.ptr = const_cast<uint8*>(srcData);

    switch (format)
    {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    {
        CompressBlocksBC1(&comp_surf, outData.data());
    }
    break;
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    {
        bc7_enc_settings settings = {};
        GetProfile_slow(&settings);
        CompressBlocksBC7(&comp_surf, outData.data(), &settings);
    }
    break;
    case DXGI_FORMAT_BC4_UNORM:
    {
        CompressBlocksBC4(&comp_surf, outData.data());
    }
    break;
    case DXGI_FORMAT_BC5_UNORM:
    {
        CompressBlocksBC5(&comp_surf, outData.data());
    }
    break;
    default:
        assert(!"Invalid Format");
        return false;
    }

    return true;
}
