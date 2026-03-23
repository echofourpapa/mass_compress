/*
Mass Compress!

Takes a folder full of textures and batch-compresses them to BC-format DDS files.
Supports .tga, .png, and .jpg inputs.

Format selection by channel count:
* 1 channel  (grayscale)        - BC4
* 2 channels (normal maps)      - BC5
* 3 channels (color)            - BC1
* 4 channels (color and alpha)  - BC7

Normal maps are detected automatically (or by "Normal" in the filename) and
compressed as BC5. sRGB textures are detected by "Albedo" or "BaseColor" in
the filename and use sRGB BC format variants.

Three compression backends are available:
* cmp-cpu (default) - AMD Compressonator CPU path, multi-threaded  (-b=cmp-cpu)
* ispc               - Intel ISPC Texture Compressor                (-b=ispc)
* cmp-gpu            - AMD Compressonator GPU path (DirectX compute) (-b=cmp-gpu)
                       Note: not faster than CPU in practice due to
                       Compressonator's per-call GPU overhead.

* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* mpetro wrote this file.  As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return.
* ----------------------------------------------------------------------------
*/
#include "Types.h"

#include <cstdio>
#include <filesystem>
#include <vector>
#include <future>
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "DDS.h"

#include "Compressor.h"
#include "CompressorISPC.h"
#include "CompressorCMP.h"
#include "CompressorCMP_GPU.h"

std::mutex g_printMutex;

enum class BackendType { ISPC, CMP_CPU, CMP_GPU };

struct args_output
{
    std::filesystem::path input_dir;
    std::filesystem::path output_dir;
    BackendType backend = BackendType::CMP_CPU;
    uint32 threadCount = 0;  // 0 = auto (hardware_concurrency)
    float quality = 0.05f;   // CMP quality, ignored for ISPC
    bool verbose = false;
};

void PrintHelpMessage()
{
    const char* helpMsg = \
        "Mass Compress!\n"\
        "\tUsage: mass-compress.exe INPUT_DIR <OUTPUT_DIR> [OPTIONS]\n" \
        "Arguements:\n" \
        "\tINPUT_DIR : directory containing images you want to compress.\n" \
        "\tOUTPUT_DIR : directory to output compress images too. If none is provided, it defaults to INPUT_DIR/compressed\n" \
        "Options:\n" \
        "\t-h,--help                  : print this help message!\n" \
        "\t-v,--verbose               : print timing info per image and total\n" \
        "\t-b=<backend>,--backend=    : compression backend (ispc, cmp-cpu, cmp-gpu). Default: cmp-cpu\n" \
        "\t-t=<N>,--threads=          : number of images to compress in parallel. Default: auto\n" \
        "\t-q=<F>,--quality=          : compression quality 0.0-1.0 (CMP backends only). Default: 0.05\n" \
        "Description\n" \
        "Takes a folder full of textures and compresses all of them and outputs them as .dds files. Only works on.tga, .png, and .jpg files for inputs.\n" \
        "Does it's best to figure out compression formats:\n"\
        "\t* 1 channel  (grayscale) \t- BC4\n\t* 2 channels (normal maps) \t- BC5\n\t* 3 channels (color) \t\t- BC1\n\t* 4 channels (color and alpha) \t- BC7\n" \
        "Backends:\n" \
        "\tispc    : Intel ISPC Texture Compressor (default)\n" \
        "\tcmp-cpu : AMD Compressonator (CPU, multi-threaded)\n" \
        "\tcmp-gpu : AMD Compressonator (GPU, DirectX compute)\n";
    std::printf(helpMsg);
}

// Returns: 0 = success, 1 = help requested (not an error), -1 = error
int ParseArgs(int argc, char* argv[], args_output& out)
{
    if (argc < 2)
    {
        std::printf("Missing input directory!\n");
        PrintHelpMessage();
        return -1;
    }

    // Check if first arg is --help before treating it as input dir
    {
        std::string first(argv[1]);
        if (first == "-h" || first == "--help")
        {
            PrintHelpMessage();
            return 1;
        }
    }

    out.input_dir = std::filesystem::path(argv[1]);

    for (uint32 i = 2; i < (uint32)argc; i++)
    {
        std::string arg(argv[i]);

        if (arg == "-h" || arg == "--help")
        {
            PrintHelpMessage();
            return 1;
        }

        if (arg == "-v" || arg == "--verbose")
        {
            out.verbose = true;
            continue;
        }

        if (arg.find("--backend=") == 0 || arg.find("-b=") == 0)
        {
            std::string val = arg.substr(arg.find('=') + 1);
            if (val == "ispc")
                out.backend = BackendType::ISPC;
            else if (val == "cmp-cpu")
                out.backend = BackendType::CMP_CPU;
            else if (val == "cmp-gpu")
                out.backend = BackendType::CMP_GPU;
            else
            {
                std::printf("Unknown backend: %s\n", val.c_str());
                PrintHelpMessage();
                return -1;
            }
            continue;
        }

        if (arg.find("--threads=") == 0 || arg.find("-t=") == 0)
        {
            std::string val = arg.substr(arg.find('=') + 1);
            out.threadCount = (uint32)std::stoi(val);
            continue;
        }

        if (arg.find("--quality=") == 0 || arg.find("-q=") == 0)
        {
            std::string val = arg.substr(arg.find('=') + 1);
            out.quality = std::stof(val);
            continue;
        }

        if (arg[0] == '-')
        {
            std::printf("Unrecongized option: %s\n", arg.c_str());
            PrintHelpMessage();
            return -1;
        }

        if (out.output_dir.empty())
        {
            out.output_dir = std::filesystem::path(argv[i]);
        }
    }

    return 0;
}

DXGI_FORMAT ChannelsToFormat(uint32 channels, bool sRGB=false)
{
    switch (channels)
    {
        case 1:
            return DXGI_FORMAT_BC4_UNORM;
        case 2:
            return DXGI_FORMAT_BC5_UNORM;
        case 3:
            return sRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
        case 4:
            return sRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
        default:
            assert("Invalid Channel Count");
            return DXGI_FORMAT_UNKNOWN;
    }
}

uint32 FormatToBlocksize(DXGI_FORMAT format)
{
    // Almost all formats are 16 bytes per block, only a few are 8 bytes.
    switch (format)
    {
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_UNORM:
            return 8;
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        default:
            return 16;

    }
}

struct Pixel
{
    float rgba[4] = {0.0f, 0.0f ,0.0f ,0.0f };

    float Length()
    {
        return sqrt((rgba[0] * rgba[0]) + (rgba[1] * rgba[1]) + (rgba[2] * rgba[2]));
    }

    void ScaleBias(float scale, float bias)
    {
        for (uint32 c = 0; c < 4; c++)
        {
            rgba[c] *= scale;
            rgba[c] += bias;
        }
    }

    void Normalize()
    {
        float length = Length();
        rgba[0] /= length;
        rgba[1] /= length;
        rgba[2] /= length;
    }

    void sRGBToLinear()
    {
        // Don't scale alpha channels
        for (uint32 c = 0; c < 3; c++)
        {
            rgba[c] = rgba[c] > 0.04045f ? pow((rgba[c] + 0.055f) / 1.055f, 2.4f) : rgba[c] / 12.92f;
        }
    }

    void LinearTosRGB()
    {
        for (uint32 c = 0; c < 3; c++)
        {
            rgba[c] = rgba[c]  > 0.0031308f ? 1.055f * pow(rgba[c], 1.f / 2.4f) - 0.055f : rgba[c] * 12.92f;
        }
    }
};

struct Image
{
    uint32 width = 0;
    uint32 height = 0;
    std::vector<Pixel> data;

    bool AllWhiteAlpha()
    {
        uint32 totalWhiteAlpha = 0;
        for (uint32 p = 0; p < data.size(); p++)
        {
            if (fabs(data[p].rgba[3] - 1.0f) <= FLT_EPSILON)
                totalWhiteAlpha++;
        }
        return totalWhiteAlpha == data.size();
    }
};

Pixel Lerp(const Pixel& A, const Pixel& B, const float T)
{
    Pixel out = {};
    for (uint32 c = 0; c < 4; c++)
        out.rgba[c] = (A.rgba[c] * (1.0f - T)) + (B.rgba[c] * T);
    return out;
}

// This might be dumb, it tries to see if a texture contains a normal map or not.
// If _most_ of the pixels are already normalized.
// That's very unlikely to happen for other texture types besides normal maps.
bool IsProbablyNormalMap(const Image& image, uint32 width, uint32 height, uint32 channels)
{
    if (channels < 3)
        return false;
   // uint32 fcount = 0;
    uint32 lcount = 0;

    float qaunt_size = 2.0f / 256.0f;

    for (uint32 p = 0; p < image.data.size(); p++)
    {
        Pixel pix = image.data[p];
        pix.ScaleBias(2.0f, -1.0f);
        float len = pix.Length();
        if (fabs(len - 1.0f) <= qaunt_size)
            lcount++;
    }
    uint32 high = (uint32)(image.data.size() * 0.95f);
    float percentage = (float)lcount / (float)image.data.size();
    if (lcount > high)
        return true;
    return false;
}

bool IsImageExtension(const std::string& ext)
{
    if (ext == ".png")
        return true;
    if (ext == ".tga")
        return true;
    if (ext == ".jpg")
        return true;

    return false;
}

bool CompressFile(const std::filesystem::path& path, const std::filesystem::path& outPath, Compressor& compressor, bool verbose)
{
    std::string name = std::string(path.filename().string().c_str());
    {
        std::lock_guard<std::mutex> lock(g_printMutex);
        std::printf("Compressing Image: %s\n", name.c_str());
    }

    auto imageStart = std::chrono::high_resolution_clock::now();

    FILE* fp;
    fopen_s(&fp, path.string().c_str(), "rb");
    if (!fp)
    {
        std::lock_guard<std::mutex> lock(g_printMutex);
        std::printf("Couldn't read file: %s\n", path.string().c_str());
        return false;
    }

    bool res = false;
    uint32 width = 0;
    uint32 height = 0;
    uint32 channels = 0;
    uint32 reqChannels = 4;

    uint8* readImg = stbi_load_from_file(fp, (int*)&width, (int*)&height, (int*)&channels, reqChannels);

    fclose(fp);

    // We substract 2 because each block is 4x4, and the two smallest mips are 2x2 and 1x1, which have to become 4x4.
    // Seems like a waste...
    // So total mips - 2, + 1 for total number of mips(counting index 0), means - 1.  Right?
    // This will make somethings like getting to total average value of a texture using the smallest mip impossible, though...
    // Maybe make this an option?
    uint32 mipCount = (uint32)log2(std::max(width, height)) + 1;
    std::vector<Image> inImgs(mipCount);

    bool isNormal = IsProbablyNormalMap(inImgs[0], width, height, reqChannels) || name.find("Normal") != std::string::npos;

    // I have no idea why sRGB doesn't work the way I want it to...
    bool sRGB = (name.find("Albedo") != std::string::npos || name.find("BaseColor") != std::string::npos) && !isNormal;

    // Make it all floats, colors love floats
    for (uint32 y = 0; y < width; y++)
    {
        for (uint32 x = 0; x < height; x++)
        {
            uint32 srcIndex = (x * reqChannels) + (y * width * reqChannels);
            Pixel pixel = {};
            for (uint32 c = 0; c < reqChannels; c++)
            {
                pixel.rgba[c] = readImg[srcIndex + c] / 255.0f;
            }

            if (sRGB)
                pixel.sRGBToLinear();
            inImgs[0].data.push_back(pixel);
        }
    }
    inImgs[0].width = width;
    inImgs[0].height = height;
    stbi_image_free(readImg);



    // Make the mips, simple averaging downsample
    for (uint32 mip = 1; mip < mipCount; mip++)
    {
        uint32 srcWidth = width >> (mip - 1);
        uint32 srcHeight = height >> (mip - 1);

        uint32 mipWidth = width >> mip;
        uint32 mipHeight = height >> mip;
        uint32 size = mipWidth * mipHeight;
        inImgs[mip].width = mipWidth;
        inImgs[mip].height = mipHeight;
        inImgs[mip].data.resize(size);

        for (uint32 y = 0; y < mipWidth; y++)
        {
            for (uint32 x = 0; x < mipHeight; x++)
            {
                Pixel pix = {};
                if (isNormal)
                {
                    uint32 srcIndexA = (x * 2 + 0) + ((y * 2 + 0) * srcWidth);
                    uint32 srcIndexB = (x * 2 + 1) + ((y * 2 + 0) * srcWidth);
                    uint32 srcIndexC = (x * 2 + 0) + ((y * 2 + 1) * srcWidth);
                    uint32 srcIndexD = (x * 2 + 1) + ((y * 2 + 1) * srcWidth);

                    Pixel pixA = inImgs[mip - 1].data[srcIndexA];
                    Pixel pixB = inImgs[mip - 1].data[srcIndexB];
                    Pixel pixC = inImgs[mip - 1].data[srcIndexC];
                    Pixel pixD = inImgs[mip - 1].data[srcIndexD];

                    pixA.ScaleBias(2.0f, -1.0f);
                    pixB.ScaleBias(2.0f, -1.0f);
                    pixC.ScaleBias(2.0f, -1.0f);
                    pixD.ScaleBias(2.0f, -1.0f);

                    Pixel pixAB = Lerp(pixA, pixB, 0.5f);
                    Pixel pixCD = Lerp(pixC, pixD, 0.5f);
                    pixAB.Normalize();
                    pixCD.Normalize();
                    Pixel pixABCD = Lerp(pixAB, pixCD, 0.5f);
                    pixABCD.Normalize();
                    pixABCD.ScaleBias(0.5f, 0.5f);
                    pix = pixABCD;
                }
                else
                {
                    for (uint32 yb = 0; yb < 2; yb++)
                    {
                        for (uint32 xb = 0; xb < 2; xb++)
                        {
                            uint32 bxb = (x * 2) + xb;
                            uint32 byb = (y * 2) + yb;
                            uint32 srcIndex = (bxb)+(byb * srcWidth);
                            for (uint32 c = 0; c < reqChannels; c++)
                            {
                                pix.rgba[c] += inImgs[mip - 1].data[srcIndex].rgba[c] * 0.25f;
                            }
                        }
                    }
                }
                uint32 dstIndex = (x)+(y * mipWidth);
                inImgs[mip].data[dstIndex] = pix;
            }
        }
    }

    std::vector<std::vector<uint8>> outImgs(mipCount);

    // Drop Blue/Z for normal maps
    if (isNormal)
        channels = 2;

    // If we have an alpha channel that's solid 1, then why even bother with it?
    if (channels == 4 && inImgs[0].AllWhiteAlpha())
        channels = 3;

    DXGI_FORMAT format = ChannelsToFormat(channels, sRGB);

    if (format == DXGI_FORMAT_UNKNOWN)
    {
        std::lock_guard<std::mutex> lock(g_printMutex);
        std::printf("Couldn't figure out texture format from channel count.  %d is either less than 1 or greater than 4.", channels);
        return false;
    }

    uint32 blockSize = FormatToBlocksize(format);

    for (uint32 mip = 0; mip < mipCount; mip++)
    {
        uint32 mipWidth = inImgs[mip].width;
        uint32 mipHeight = inImgs[mip].height;
        uint32 compImgSize = std::max(1u, ((mipWidth + 3) / 4)) * std::max(1u, ((mipHeight + 3) / 4)) * blockSize;

        // Always pack 4 channels (RGBA) for uniform backend input
        std::vector<uint8> smallData(mipWidth * mipHeight * 4);

        for (uint32 y = 0; y < mipWidth; y++)
        {
            for (uint32 x = 0; x < mipHeight; x++)
            {
                uint32 srcIdx = x + y * mipWidth;
                uint32 dstIdx = srcIdx * 4;

                Pixel pixel = inImgs[mip].data[srcIdx];
                if (sRGB)
                    pixel.LinearTosRGB();

                for (uint32 c = 0; c < 4; c++)
                {
                    smallData[dstIdx + c] = (uint8)(pixel.rgba[c] * 255.0f);
                }
            }
        }

        outImgs[mip].resize(compImgSize);

        if (!compressor.CompressMip(smallData.data(), mipWidth, mipHeight, format, outImgs[mip]))
        {
            std::lock_guard<std::mutex> lock(g_printMutex);
            std::printf("Failed to compress mip %d of %s\n", mip, name.c_str());
            return false;
        }
    }

    compressor.OnImageComplete();

    DirectX::DDS_HEADER header = {};
    header.size = 124;
    header.flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE | DDS_HEADER_FLAGS_MIPMAP;
    header.width = width;
    header.height = height;
    header.depth = 1;
    header.mipMapCount = mipCount;
    header.pitchOrLinearSize = std::max(1u, ((width + 3) / 4)) * blockSize;
    header.ddspf = DirectX::DDSPF_DX10;

    DirectX::DDS_HEADER_DXT10 header_dx10 = {};
    header_dx10.dxgiFormat = format;
    header_dx10.resourceDimension = 3;// Texture 2D
    header_dx10.arraySize = 1;

    std::filesystem::path out_path(outPath);
    out_path /= name;
    out_path.replace_extension("dds");
    {
        FILE* fp;
        fopen_s(&fp, out_path.string().c_str(), "wb");
        if (!fp)
        {
            std::lock_guard<std::mutex> lock(g_printMutex);
            std::printf("Couldn't write file: %s\n", path.string().c_str());
            return false;
        }

        fwrite(&DirectX::DDS_MAGIC, sizeof(uint32), 1, fp);
        fwrite(&header, sizeof(DirectX::DDS_HEADER), 1, fp);
        fwrite(&header_dx10, sizeof(DirectX::DDS_HEADER_DXT10), 1, fp);
        for (uint32 mip = 0; mip < mipCount; mip++) {
            fwrite(&outImgs[mip].front(), sizeof(uint8), outImgs[mip].size(), fp);
        }

        fclose(fp);
    }

    if (verbose)
    {
        auto imageEnd = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(imageEnd - imageStart).count();
        std::lock_guard<std::mutex> lock(g_printMutex);
        std::printf("  %s: %.1f ms\n", name.c_str(), ms);
    }

    return true;
}

int main(int argc, char* argv[])
{

    args_output args;
    int parseResult = ParseArgs(argc, argv, args);
    if (parseResult != 0)
        return parseResult < 0 ? EXIT_FAILURE : EXIT_SUCCESS;

    if (args.output_dir.empty())
    {
        args.output_dir = args.input_dir / "compressed";
    }

    if (!std::filesystem::exists(args.input_dir)) {
        std::printf("Input path \"%s\" doesn't exist.\n", args.input_dir.string().c_str());
        return EXIT_FAILURE;
    }

    std::vector<std::filesystem::path> paths;
    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(args.input_dir))
    {
        std::string ext = dirEntry.path().extension().string();
        if(IsImageExtension(ext))
            paths.push_back(dirEntry.path());
    }

    // Only make the output dir if we've got textures to compress.
    if (paths.size() > 0)
    {
        std::filesystem::create_directory(args.output_dir);
    }
    else
    {
        std::printf("No files in \"%s\" to compress.", args.input_dir.string().c_str());
        return EXIT_FAILURE;
    }

    // Create the compression backend
    std::unique_ptr<Compressor> compressor;
    const char* backendName = "unknown";
    switch (args.backend)
    {
    case BackendType::ISPC:
        compressor = std::make_unique<CompressorISPC>();
        backendName = "ISPC";
        break;
    case BackendType::CMP_CPU:
        compressor = std::make_unique<CompressorCMP>(args.quality);
        backendName = "CMP CPU";
        break;
    case BackendType::CMP_GPU:
        compressor = std::make_unique<CompressorCMP_GPU>(args.quality);
        backendName = "CMP GPU";
        break;
    }
    compressor->Init();

    if (args.verbose)
        std::printf("Backend: %s\n", backendName);

    auto totalStart = std::chrono::high_resolution_clock::now();

    bool success = true;

    if (compressor->SupportsParallelFiles())
    {
        uint32 maxThreads = args.threadCount;
        if (maxThreads == 0)
            maxThreads = std::max(1u, (uint32)std::thread::hardware_concurrency());

        std::vector<std::future<bool>> futures;

        for (const auto& path : paths)
        {
            // Throttle: wait for a slot if we're at max capacity
            while (futures.size() >= maxThreads)
            {
                bool found = false;
                for (auto it = futures.begin(); it != futures.end(); ++it)
                {
                    if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                    {
                        if (!it->get()) success = false;
                        futures.erase(it);
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    // None ready yet, wait for the first one
                    if (!futures.front().get()) success = false;
                    futures.erase(futures.begin());
                }
            }

            futures.push_back(std::async(std::launch::async, CompressFile, path, args.output_dir, std::ref(*compressor), args.verbose));
        }

        // Wait for remaining
        for (auto& f : futures)
        {
            if (!f.get()) success = false;
        }
    }
    else
    {
        // Sequential processing (GPU backend parallelizes internally)
        for (const auto& path : paths)
        {
            if (!CompressFile(path, args.output_dir, *compressor, args.verbose))
                success = false;
        }
    }

    compressor->Shutdown();

    if (args.verbose)
    {
        auto totalEnd = std::chrono::high_resolution_clock::now();
        double totalMs = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();
        std::printf("Total: %.1f ms\n", totalMs);
    }

    std::printf("Complete\n");

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
