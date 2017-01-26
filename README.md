# Extreme Gpu Friendly Video Format

## what is this?
- Original file format optimized for random access at any frame
- Frame data is compressed DXT1, DXT3, DXT5 format
- Storage format is compressed by lz4

## setup

```

$ cd scripts
$ ruby download_resource.rb

```

## How to encode
Required ffmpeg.<br>
Please install ffmpeg by brew(osx) or chocolatey(windows).<br>
or png tiff sequence directly. <br>

### 1. create \*.gvintermediate from any video file

```

$ cd scripts
$ ruby video_to_gvintermediate.rb footage.mov

```

### 2. open openframeworks project file
- converter-osx/converter-osx.xcodeproj  (osx)
- converter-win/ofxExtremeGpuVideo_win.sln (windows)

### 3. select format dxt1, dxt3, dxt5
- unity is not supported dxt3 yet.

### 4. click compress, and select \*.gvintermediate directory
- please wait

### 5. done

## binary file format (gv)

```

0: uint32_t width
4: uint32_t height
8: uint32_t frame count
12: float fps
16: uint32_t fmt (DXT1 = 1, DXT3 = 3, DXT5 = 5, BC7 = 7)
20: uint32_t frame bytes
24: raw frame storage (lz4 compressed)
eof - (frame count) * 16: [(uint64_t, uint64_t)..<frame count] (address, size) of lz4, address is zero based from file head

```

nvtt encoder supported
https://developer.nvidia.com/gameworksdownload#?dn=gpu-accelerated-texture-tools-2-08
