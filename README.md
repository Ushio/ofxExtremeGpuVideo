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
<br>

### 1. create \*.gvintermediate from any video file

```

$ cd scripts
$ ruby video_to_gvintermediate.rb footage.mov

```

### 2. open openframeworks project file
- ofxExtremeGpuVideo_osx/GpuVideo.xcodeproj  (osx)
- ofxExtremeGpuVideo_win/ofxExtremeGpuVideo_win.vcxproj (windows)

### 3. select format dxt1, dxt3, dxt5
- unity is not supported dxt3 yet.

### 4. click compress, and select \*.gvintermediate directory
- please wait

### 5. done
