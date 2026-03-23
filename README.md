# Mass Compress

Batch-compresses a folder of textures to BC-format DDS files.

Supports `.tga`, `.png`, and `.jpg` inputs. Does its best to figure out the right format:
* 1 channel  (grayscale)        → BC4
* 2 channels (normal maps)      → BC5
* 3 channels (color)            → BC1
* 4 channels (color and alpha)  → BC7

Normal maps are detected automatically (or by `Normal` in the filename) and compressed as BC5. sRGB textures are detected by `Albedo` or `BaseColor` in the filename and use the sRGB BC format variants. 4-channel textures with a fully-white alpha are treated as 3-channel (BC1).

The whole thing is really bare bones, a lot of it is probably terrible, use at your own risk!

## Building

* Clone this repo and initialize submodules (`git submodule update --init --recursive`)
* Install the [ISPC compiler](https://ispc.github.io/) (required for the ISPC backend)
* Run `premake5.exe vs2026` to generate the Visual Studio solution
* Build `build/Mass Compress.slnx` in Visual Studio 2026

## Using

```
mass-compress.exe INPUT_DIR [OUTPUT_DIR] [OPTIONS]
```

`OUTPUT_DIR` defaults to `INPUT_DIR/compressed` if not specified.

| Option | Description |
|--------|-------------|
| `-h`, `--help` | Print help |
| `-v`, `--verbose` | Print per-image and total timing |
| `-b=<name>`, `--backend=<name>` | Compression backend (see below). Default: `cmp-cpu` |
| `-t=N`, `--threads=N` | Images to compress in parallel. Default: auto (hardware concurrency) |
| `-q=F`, `--quality=F` | Quality 0.0–1.0 for CMP backends. Default: `0.05` |

### Backends

| Name | Description |
|------|-------------|
| `cmp-cpu` | AMD Compressonator CPU path, multi-threaded **(default)** |
| `ispc` | Intel ISPC Texture Compressor |
| `cmp-gpu` | AMD Compressonator GPU path (DirectX compute). Not faster than CPU in practice due to per-call GPU overhead. |

## License

```
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* mpetro wrote this file.  As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return.
* ----------------------------------------------------------------------------
```
