#include "CompressorCMP_GPU.h"
#include "CompressorCMP_Common.h"
#include <cstdio>
#include <cstring>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// CPU fallback for mips smaller than 4x4 (GPU requires multiples of 4)
static bool CompressMipCPU(
    const uint8* srcData,
    uint32 width, uint32 height,
    DXGI_FORMAT format, float quality,
    std::vector<uint8>& outData)
{
    CMP_Texture srcTexture = {};
    srcTexture.dwSize     = sizeof(CMP_Texture);
    srcTexture.dwWidth    = width;
    srcTexture.dwHeight   = height;
    srcTexture.dwPitch    = width * 4;
    srcTexture.format     = CMP_FORMAT_RGBA_8888;
    srcTexture.dwDataSize = width * height * 4;
    srcTexture.pData      = const_cast<CMP_BYTE*>(srcData);

    CMP_FORMAT cmpFormat = DXGIToCMPFormat(format);

    CMP_Texture destTexture = {};
    destTexture.dwSize    = sizeof(CMP_Texture);
    destTexture.dwWidth   = width;
    destTexture.dwHeight  = height;
    destTexture.format    = cmpFormat;
    destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
    destTexture.pData     = outData.data();

    CMP_CompressOptions options = {};
    options.dwSize       = sizeof(CMP_CompressOptions);
    options.fquality     = quality;
    options.dwnumThreads = 1;
    options.dwmodeMask   = 0xFF;

    return CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr) == CMP_OK;
}

void CompressorCMP_GPU::Init()
{
    // CMP framework scans CWD for plugin DLLs — set CWD to the exe directory
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(nullptr, exePath, MAX_PATH))
    {
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
        SetCurrentDirectoryA(exeDir.string().c_str());
    }

    CMP_InitFramework();
}

void CompressorCMP_GPU::Shutdown()
{
    CMP_DestroyComputeLibrary(true);
}


bool CompressorCMP_GPU::CompressMip(
    const uint8* srcData,
    uint32 width, uint32 height,
    DXGI_FORMAT format,
    std::vector<uint8>& outData)
{
    CMP_FORMAT cmpFormat = DXGIToCMPFormat(format);
    if (cmpFormat == CMP_FORMAT_Unknown)
    {
        std::printf("CompressorCMP_GPU: Unsupported format\n");
        return false;
    }

    // GPU requires dimensions to be multiples of 4; fall back to CPU for small mips
    if (width < 4 || height < 4)
        return CompressMipCPU(srcData, width, height, format, quality, outData);

    // Point source MipSet directly at our input — no CMP allocation needed.
    CMP_MipSet srcMipSet    = {};
    srcMipSet.m_format      = CMP_FORMAT_RGBA_8888;
    srcMipSet.m_nMipLevels  = 1;
    srcMipSet.m_nChannels   = 4;
    srcMipSet.dwWidth       = width;
    srcMipSet.dwHeight      = height;
    srcMipSet.pData         = const_cast<CMP_BYTE*>(srcData);
    srcMipSet.dwDataSize    = width * height * 4;

    KernelOptions kernelOpts = {};
    kernelOpts.encodeWith = CMP_GPU_DXC;
    kernelOpts.format     = cmpFormat;
    kernelOpts.fquality   = quality;
    kernelOpts.threads    = 0;
    kernelOpts.width      = width;
    kernelOpts.height     = height;

    // Create or reuse the compute library. CMP_CreateComputeLibrary has an early-out
    // when g_ComputeBase is already alive with the same compute type, so the D3D11
    // device is not recreated on subsequent mips of the same image.
    if (CMP_CreateComputeLibrary(&srcMipSet, &kernelOpts, nullptr) != CMP_OK)
    {
        std::printf("CompressorCMP_GPU: CMP_CreateComputeLibrary failed\n");
        return false;
    }

    ComputeOptions compOpts = {};
    compOpts.force_rebuild  = false;
    CMP_SetComputeOptions(&compOpts);

    // Point destination MipSet directly at our pre-allocated output buffer.
    CMP_MipSet dstMipSet = {};
    dstMipSet.m_format   = cmpFormat;
    dstMipSet.dwWidth    = width;
    dstMipSet.dwHeight   = height;
    dstMipSet.pData      = outData.data();
    dstMipSet.dwDataSize = (CMP_DWORD)outData.size();

    bool ok = CMP_CompressTexture(&kernelOpts, srcMipSet, dstMipSet, nullptr) == CMP_OK;
    if (!ok)
        std::printf("CompressorCMP_GPU: CMP_CompressTexture failed\n");

    // false = no-op; keeps g_ComputeBase (D3D11 device) alive for the next mip.
    // Full teardown happens in OnImageComplete, once per image.
    CMP_DestroyComputeLibrary(false);
    return ok;
}

void CompressorCMP_GPU::OnImageComplete()
{
    // Keep the compute library alive across images — destroying it forces a full
    // D3D11 device re-init (2-3s overhead) on the next image. Per-call textures
    // and buffers inside CDirectX::Compress are released within each call, so
    // VRAM is bounded by the resources for one image. Full cleanup is in Shutdown.
    CMP_DestroyComputeLibrary(false);
}
