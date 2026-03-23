/*
CompressorCMP — AMD Compressonator CPU backend.

Calls CMP_ConvertTexture from cmp_compressonatorlib. Compressonator's CPU path
uses internal multi-threading (dwnumThreads) to parallelise block compression
within a single image, so it gets good CPU utilisation without needing
file-level parallelism from the caller.

CMP_ConvertTexture uses global library state and is not thread-safe across
concurrent calls, so SupportsParallelFiles() returns false — images are
processed sequentially, one at a time.

Quality is controlled by the fquality parameter (0.0–1.0). Lower values are
faster with slightly reduced quality; the default of 0.05 gives a good
speed/quality trade-off.

Format support: BC1, BC4, BC5, BC7 (and sRGB variants).
*/
#pragma once

#include "Compressor.h"

struct CompressorCMP : Compressor
{
    float quality = 0.05f;

    CompressorCMP(float q = 0.05f) : quality(q) {}

    // CMP_ConvertTexture uses global state — not thread-safe for file-level parallelism.
    // Internal threading via dwnumThreads handles per-image parallelism instead.
    bool SupportsParallelFiles() const override { return false; }

    bool CompressMip(
        const uint8* srcData,
        uint32 width, uint32 height,
        DXGI_FORMAT format,
        std::vector<uint8>& outData
    ) override;
};
