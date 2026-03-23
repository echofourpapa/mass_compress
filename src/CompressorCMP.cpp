#include "CompressorCMP.h"
#include "CompressorCMP_Common.h"
#include <cstdio>
#include <cstring>

bool CompressorCMP::CompressMip(
    const uint8* srcData,
    uint32 width, uint32 height,
    DXGI_FORMAT format,
    std::vector<uint8>& outData)
{
    CMP_Texture srcTexture = {};
    srcTexture.dwSize    = sizeof(CMP_Texture);
    srcTexture.dwWidth   = width;
    srcTexture.dwHeight  = height;
    srcTexture.dwPitch   = width * 4;
    srcTexture.format    = CMP_FORMAT_RGBA_8888;
    srcTexture.dwDataSize = width * height * 4;
    srcTexture.pData     = const_cast<CMP_BYTE*>(srcData);

    CMP_FORMAT cmpFormat = DXGIToCMPFormat(format);
    if (cmpFormat == CMP_FORMAT_Unknown)
    {
        std::printf("CompressorCMP: Unsupported format\n");
        return false;
    }

    CMP_Texture destTexture = {};
    destTexture.dwSize   = sizeof(CMP_Texture);
    destTexture.dwWidth  = width;
    destTexture.dwHeight = height;
    destTexture.format   = cmpFormat;
    destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
    destTexture.pData    = outData.data();

    CMP_CompressOptions options = {};
    options.dwSize       = sizeof(CMP_CompressOptions);
    options.fquality     = quality;
    options.dwnumThreads = 0; // 0 = use all available cores
    options.dwmodeMask   = 0xFF; // BC7: enable all 8 modes

    CMP_ERROR status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);
    if (status != CMP_OK)
    {
        std::printf("CompressorCMP: CMP_ConvertTexture failed with error %d\n", (int)status);
        return false;
    }

    return true;
}
