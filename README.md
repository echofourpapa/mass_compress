# Mass Compress

Takes a folder full of textures and compresses all of them, using Intel's [ISPC Texture Compressor](https://github.com/GameTechDev/ISPCTextureCompressor), and outputs them as .dds files.

Only works on.tga, .png, and .jpg files for inputs.
Does it's best to figure out compression formats:
* 1 channel  (grayscale)        - BC4
* 2 channels (normal maps)      - BC5
* 3 channels (color)            - BC1
* 4 channels (color and alpha)  - BC7
    
The whole thing is really bare bones, a lot of it is probably terrible, use at your own risk!

## Building
* Clone this repo
* Clone [ISPC Texture Compressor](https://github.com/GameTechDev/ISPCTextureCompressor) to `thirdparty/`
* * Don't forget to download the ISPC Compiler
* Run `premake5.exe vs2022`
* Build the solution `build/Mass Compress.sln` in Visual Studio

## Using
* run: `mass-compress.exe INPUT_DIR <OUTPUT_DIR> [OPTIONS]`
```
Mass Compress!
        Usage: mass-compress.exe INPUT_DIR <OUTPUT_DIR> [OPTIONS]
Arguements:
        INPUT_DIR : directory containing images you want to compress.
        OUTPUT_DIR : directory to output compress images too. If none is provided, it defaults to INPUT_DIR/compressed
Options:
        -h,--help : print this help message!
Description
Takes a folder full of textures and compresses all of them, using Intel's ISPC Texture Compressor, and outputs them as .dds files. Only works on.tga, .png, and .jpg files for inputs.
Does it's best to figure out compression formats:
        * 1 channel  (grayscale)        - BC4
        * 2 channels (normal maps)      - BC5
        * 3 channels (color)            - BC1
        * 4 channels (color and alpha)  - BC7
```

## License
```
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* mpetro wrote this file.  As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return.
* ----------------------------------------------------------------------------
```