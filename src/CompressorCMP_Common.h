#pragma once

#include "Types.h"
#include "compressonator.h"

inline CMP_FORMAT DXGIToCMPFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return CMP_FORMAT_BC1;
    case DXGI_FORMAT_BC4_UNORM:
        return CMP_FORMAT_BC4;
    case DXGI_FORMAT_BC5_UNORM:
        return CMP_FORMAT_BC5;
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return CMP_FORMAT_BC7;
    default:
        return CMP_FORMAT_Unknown;
    }
}
