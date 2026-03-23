/*
CompressorISPC — Intel ISPC Texture Compressor backend.

Calls CompressBlocksBC* functions from ispc_texcomp (ispc_texcomp.dll).
ISPC (Implicit SPMD Program Compiler) generates SIMD-vectorised CPU code that
processes many BC blocks in parallel per call, making this the fastest CPU
option for raw throughput.

Supports file-level parallelism — each CompressMip call is stateless and
thread-safe, so multiple images can be compressed concurrently via std::async.

Format support: BC1, BC4, BC5, BC7.
No quality knob — ISPC uses fixed high-quality encoding settings.
*/
#pragma once

#include "Compressor.h"

struct CompressorISPC : Compressor
{
    bool CompressMip(
        const uint8* srcData,
        uint32 width, uint32 height,
        DXGI_FORMAT format,
        std::vector<uint8>& outData
    ) override;
};
