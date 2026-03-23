/*
Compressor — base interface for BC texture compression backends.

All backends receive one mip level at a time as RGBA uint8 data and write
compressed BC blocks into a pre-sized output buffer. The calling code in
main.cpp (CompressFile) owns mip generation, format selection, and DDS output;
the backend only handles the actual block compression.

Call sequence per image:
    Init()                        — one-time setup (called once for the whole batch)
    for each image:
        for each mip:
            CompressMip(...)      — compress one mip level
        OnImageComplete()         — per-image cleanup hook (optional)
    Shutdown()                    — one-time teardown

Parallelism:
    SupportsParallelFiles() controls whether CompressFile is dispatched across
    multiple threads. Backends that use global/shared state (CMP CPU, CMP GPU)
    return false and rely on internal threading instead.
*/
#pragma once

#include "Types.h"
#include <vector>

struct Compressor
{
    virtual ~Compressor() = default;

    // Compress a single mip level of RGBA uint8 data into BC-compressed output.
    // srcData: always 4-channel RGBA uint8, width*height*4 bytes.
    // outData: pre-sized output buffer for compressed blocks.
    virtual bool CompressMip(
        const uint8* srcData,
        uint32 width, uint32 height,
        DXGI_FORMAT format,
        std::vector<uint8>& outData
    ) = 0;

    // Called once before any compression (optional setup).
    virtual void Init() {}

    // Called after all mips of a single image are compressed (optional per-image cleanup).
    virtual void OnImageComplete() {}

    // Called once after all compression is done (optional teardown).
    virtual void Shutdown() {}

    // Whether this backend supports file-level parallelism.
    // GPU returns false — it parallelizes internally per image.
    virtual bool SupportsParallelFiles() const { return true; }
};
