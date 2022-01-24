# GpuVideo_UE4

Developed with Unreal Engine 4
- Based on https://github.com/Ushio/ofxExtremeGpuVideo.
- Blueprint implementation of GpuVideoPlayer and its Utility.

## Demo

Please see `Content/Main.umap` or `Content/BP_GpuVideoPlayer.uasset`.

## About GpuVideo
- A file format optimized for random access at any frame.
- Frame data is compressed DX1T, DXT3, DXT5 format.
- Storage format is compressed by lz4.

## Known Issue
Unreal Engine 4 includes lz4 library, but it cound not compile in editor mode.

### Temporary Solution
1. Copy lz4 from Unreal Engine source code to `Souces/GpuVideo/lz4/` and `ThirdParty/GpuVideo/lz4`.
2. Add below codes to Build.cs

```cs
if (Target.bBuildEditor)
{
    PublicSystemIncludePaths.Add(ThirdPartyPath);
}
```
