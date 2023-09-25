/*
Mass Compress!

Takes a folder full of textures and compresses all of them, using Intel's ISPC Texture Compressor, and outputs them as .dds files.
    
Only works on.tga, .png, and .jpg files for inputs.
Does it's best to figure out compression formats:
* 1 channel  (grayscale)        - BC4
* 2 channels (normal maps)      - BC5
* 3 channels (color)            - BC1
* 4 channels (color and alpha)  - BC7
    
The whole thing is really bare bones, a lot of it is probably terrible, use at your own risk!

* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* mpetro wrote this file.  As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return.
* ----------------------------------------------------------------------------
*/
#include "ispc_texcomp.h"
#include <iostream>
#include <cstdio>
#include <filesystem>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef __int64 int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;

const char* c_help = "-h";
const char* c_helpLong = "--help";
const uint32 c_optionsCount = 2;
const char* c_options[c_optionsCount] = { c_help , c_helpLong };

struct args_output
{
    std::filesystem::path input_dir;
    std::filesystem::path output_dir;
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
        "\t-h,--help : print this help message!\n" \
        "Description\n" \
        "Takes a folder full of textures and compresses all of them, using Intel's ISPC Texture Compressor, and outputs them as .dds files. Only works on.tga, .png, and .jpg files for inputs.\nDoes it's best to figure out compression formats:\n"\
        "\t* 1 channel  (grayscale) \t- BC4\n\t* 2 channels (normal maps) \t- BC5\n\t* 3 channels (color) \t\t- BC1\n\t* 4 channels (color and alpha) \t- BC7\n";
    std::printf(helpMsg);
}

bool ParseArgs(int argc, char* argv[], args_output& out)
{

    if (argc < 2)
    {
        std::printf("Missing input directory!\n");
        PrintHelpMessage();
        return false;
    }
    out.input_dir = std::filesystem::path(argv[1]);

    for (uint32 i = 2; i < (uint32)argc; i++)
    {
        std::string arg(argv[i]);
        for (uint32 j = 0; j < c_optionsCount; j++)
        {
            std::string opt(c_options[j]);
            if (arg == opt)
            {
                PrintHelpMessage();
                return false;
            }
        }
        if (out.output_dir.empty() && arg[0] != *"-")
        {
            out.output_dir = std::filesystem::path(argv[i]);
        }
        if (arg[0] == *"-")
        {
            std::printf("Unrecongized option: %s\n", arg.c_str());
            PrintHelpMessage();
            return false;
        }
    }

    return true;
}

#if !DXGI_FORMAT_DEFINED
typedef enum DXGI_FORMAT
{
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    DXGI_FORMAT_R32G32B32A32_UINT = 3,
    DXGI_FORMAT_R32G32B32A32_SINT = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS = 5,
    DXGI_FORMAT_R32G32B32_FLOAT = 6,
    DXGI_FORMAT_R32G32B32_UINT = 7,
    DXGI_FORMAT_R32G32B32_SINT = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM = 11,
    DXGI_FORMAT_R16G16B16A16_UINT = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    DXGI_FORMAT_R16G16B16A16_SINT = 14,
    DXGI_FORMAT_R32G32_TYPELESS = 15,
    DXGI_FORMAT_R32G32_FLOAT = 16,
    DXGI_FORMAT_R32G32_UINT = 17,
    DXGI_FORMAT_R32G32_SINT = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM = 24,
    DXGI_FORMAT_R10G10B10A2_UINT = 25,
    DXGI_FORMAT_R11G11B10_FLOAT = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DXGI_FORMAT_R8G8B8A8_UINT = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM = 31,
    DXGI_FORMAT_R8G8B8A8_SINT = 32,
    DXGI_FORMAT_R16G16_TYPELESS = 33,
    DXGI_FORMAT_R16G16_FLOAT = 34,
    DXGI_FORMAT_R16G16_UNORM = 35,
    DXGI_FORMAT_R16G16_UINT = 36,
    DXGI_FORMAT_R16G16_SNORM = 37,
    DXGI_FORMAT_R16G16_SINT = 38,
    DXGI_FORMAT_R32_TYPELESS = 39,
    DXGI_FORMAT_D32_FLOAT = 40,
    DXGI_FORMAT_R32_FLOAT = 41,
    DXGI_FORMAT_R32_UINT = 42,
    DXGI_FORMAT_R32_SINT = 43,
    DXGI_FORMAT_R24G8_TYPELESS = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
    DXGI_FORMAT_R8G8_TYPELESS = 48,
    DXGI_FORMAT_R8G8_UNORM = 49,
    DXGI_FORMAT_R8G8_UINT = 50,
    DXGI_FORMAT_R8G8_SNORM = 51,
    DXGI_FORMAT_R8G8_SINT = 52,
    DXGI_FORMAT_R16_TYPELESS = 53,
    DXGI_FORMAT_R16_FLOAT = 54,
    DXGI_FORMAT_D16_UNORM = 55,
    DXGI_FORMAT_R16_UNORM = 56,
    DXGI_FORMAT_R16_UINT = 57,
    DXGI_FORMAT_R16_SNORM = 58,
    DXGI_FORMAT_R16_SINT = 59,
    DXGI_FORMAT_R8_TYPELESS = 60,
    DXGI_FORMAT_R8_UNORM = 61,
    DXGI_FORMAT_R8_UINT = 62,
    DXGI_FORMAT_R8_SNORM = 63,
    DXGI_FORMAT_R8_SINT = 64,
    DXGI_FORMAT_A8_UNORM = 65,
    DXGI_FORMAT_R1_UNORM = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
    DXGI_FORMAT_BC1_TYPELESS = 70,
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    DXGI_FORMAT_BC2_TYPELESS = 73,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    DXGI_FORMAT_BC3_TYPELESS = 76,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    DXGI_FORMAT_BC4_TYPELESS = 79,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC4_SNORM = 81,
    DXGI_FORMAT_BC5_TYPELESS = 82,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC5_SNORM = 84,
    DXGI_FORMAT_B5G6R5_UNORM = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    DXGI_FORMAT_BC6H_TYPELESS = 94,
    DXGI_FORMAT_BC6H_UF16 = 95,
    DXGI_FORMAT_BC6H_SF16 = 96,
    DXGI_FORMAT_BC7_TYPELESS = 97,
    DXGI_FORMAT_BC7_UNORM = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    DXGI_FORMAT_AYUV = 100,
    DXGI_FORMAT_Y410 = 101,
    DXGI_FORMAT_Y416 = 102,
    DXGI_FORMAT_NV12 = 103,
    DXGI_FORMAT_P010 = 104,
    DXGI_FORMAT_P016 = 105,
    DXGI_FORMAT_420_OPAQUE = 106,
    DXGI_FORMAT_YUY2 = 107,
    DXGI_FORMAT_Y210 = 108,
    DXGI_FORMAT_Y216 = 109,
    DXGI_FORMAT_NV11 = 110,
    DXGI_FORMAT_AI44 = 111,
    DXGI_FORMAT_IA44 = 112,
    DXGI_FORMAT_P8 = 113,
    DXGI_FORMAT_A8P8 = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM = 115,

    DXGI_FORMAT_P208 = 130,
    DXGI_FORMAT_V208 = 131,
    DXGI_FORMAT_V408 = 132,


    DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,


    DXGI_FORMAT_FORCE_UINT = 0xffffffff
} DXGI_FORMAT;
#endif

#include "DDS.h"

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
            rgba[c] = rgba[c] > 0.4045f ? pow((rgba[c] + 0.055f) / 1.055f, 2.4f) : rgba[c] / 12.92f;
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

bool CompressFile(const std::filesystem::path& path, const std::filesystem::path& outPath)
{
    std::string name = std::string(path.filename().string().c_str());
    std::printf("Compressing Image: %s\n", name.c_str());

    FILE* fp;
    fopen_s(&fp, path.string().c_str(), "rb");
    if (!fp)
    {
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
            inImgs[0].data.push_back(pixel);
        }
    }
    inImgs[0].width = width;
    inImgs[0].height = height;
    delete readImg;

    bool isNormal = IsProbablyNormalMap(inImgs[0], width, height, reqChannels) || name.find("Normal") != std::string::npos;

    // I have no idea why sRGB doesn't work the way I want it to...
    bool sRGB = false;// (name.find("Albedo") != std::string::npos || name.find("BaseColor") != std::string::npos) && !isNormal;

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
        std::printf("Couldn't figure out texture format from channel count.  %d is either less than 1 or greater than 4.", channels);
        return false;
    }

    uint32 blockSize = FormatToBlocksize(format);

    // Really silly, RGB textures have to be RGBA for the compressor..... >:[
    if (format == DXGI_FORMAT_BC1_UNORM || format == DXGI_FORMAT_BC1_UNORM_SRGB)
        channels = 4;

    for (uint32 mip = 0; mip < mipCount; mip++)
    {
        uint32 mipWidth = inImgs[mip].width;
        uint32 mipHeight = inImgs[mip].height;
        uint32 compImgSize = std::max(1u, ((mipWidth + 3) / 4)) * std::max(1u, ((mipHeight + 3) / 4)) * blockSize;

        std::vector<uint8> smallData(mipWidth * mipHeight * channels);

        for (uint32 y = 0; y < mipWidth; y++)
        {
            for (uint32 x = 0; x < mipHeight; x++)
            {
                uint32 srcIdx = x + y * mipWidth;
                uint32 dstIdx = srcIdx * channels;
                if (sRGB)
                {
                    inImgs[mip].data[srcIdx].sRGBToLinear();
                }
                for (uint32 c = 0; c < channels; c++)
                {
                    smallData[dstIdx + c] = (uint8)(inImgs[mip].data[srcIdx].rgba[c] * 255.0f);
                }
            }
        }

        rgba_surface comp_surf = {};
        comp_surf.width = mipWidth;
        comp_surf.height = mipHeight;
        comp_surf.stride = mipWidth * channels * sizeof(uint8);
        comp_surf.ptr = &smallData.front();

        outImgs[mip].resize(compImgSize);

        switch (format)
        {
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        {
            CompressBlocksBC1(&comp_surf, &outImgs[mip].front());
        }
        break;
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        {
            bc7_enc_settings settings = {};
            GetProfile_slow(&settings);
            CompressBlocksBC7(&comp_surf, &outImgs[mip].front(), &settings);
        }
        break;
        case DXGI_FORMAT_BC4_UNORM:
        {
            CompressBlocksBC4(&comp_surf, &outImgs[mip].front());
        }
        break;
        case DXGI_FORMAT_BC5_UNORM:
        {
            CompressBlocksBC5(&comp_surf, &outImgs[mip].front());
        }
        break;
        default:
            assert("Invalid Format");
        }
    }

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
    return true;
}

int main(int argc, char* argv[])
{

    args_output args;
    if (!ParseArgs(argc, argv, args))
        return EXIT_FAILURE;

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
    bool success = true;

    for (const auto& path : paths)
    {
        if (!CompressFile(path, args.output_dir))
            success = false;
    }

    std::printf("Complete\n");

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}