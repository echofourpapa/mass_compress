/*
CompressorCMP_GPU — AMD Compressonator GPU backend (DirectX compute).

Uses CMP_CreateComputeLibrary + CMP_CompressTexture from cmp_framework, with
the EncodeWith_DXC.dll plugin that runs BC compression as a DirectX 11 compute
shader dispatch.

Device lifetime:
    CMP_CreateComputeLibrary initialises a D3D11 device on first call and
    returns early (reusing the existing device) on subsequent calls with the
    same format and compute type. CMP_DestroyComputeLibrary(false) is a no-op
    that keeps the device alive; (true) fully releases it. This backend uses
    (false) per mip and per image so the device persists for the whole batch,
    avoiding per-image reinitialisation overhead (~2–3s per image).
    Full teardown is deferred to Shutdown().

Limitations:
    - GPU path is not faster than CMP CPU in practice. Compressonator's compute
      shaders accumulate D3D11 resources across calls and the per-call
      upload/dispatch/readback round-trip adds significant latency.
    - Mips smaller than 4×4 fall back to CMP_ConvertTexture (CPU).
    - Files are processed sequentially (SupportsParallelFiles() = false).

Format support: BC1, BC4, BC5, BC7.
*/
#pragma once

#include "Compressor.h"

struct CompressorCMP_GPU : Compressor
{
    float quality = 0.05f;

    CompressorCMP_GPU(float q = 0.05f) : quality(q) {}

    void Init() override;
    void Shutdown() override;
    void OnImageComplete() override;

    bool CompressMip(
        const uint8* srcData,
        uint32 width, uint32 height,
        DXGI_FORMAT format,
        std::vector<uint8>& outData
    ) override;

    bool SupportsParallelFiles() const override { return false; }
};
