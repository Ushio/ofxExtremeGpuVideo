using System;
using System.IO;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using ExtremeGpuVideo.Plugin;

namespace ExtremeGpuVideo.Encoder
{
    /// <summary>
    /// A file writer for GpuVideo. It makes file saving more easier.
    /// </summary>
    public class GpuVideoWriter : IDisposable
    {
        public static readonly TextureFormat[] SupportTextureFormats = new[]
        {
            TextureFormat.DXT1,
            TextureFormat.DXT5
        };

        private static readonly int headMemoryAt =
            Marshal.SizeOf(typeof(int))
            + Marshal.SizeOf(typeof(int))
            + Marshal.SizeOf(typeof(int))
            + Marshal.SizeOf(typeof(float))
            + Marshal.SizeOf(typeof(int))
            + Marshal.SizeOf(typeof(int));
        private BinaryWriter writer = null;
        private int hWidth = -1;
        private int hHeight = -1;
        private int hFrameCount = -1;
        private float hFramePerSecond = -1;
        private int hFormat = -1;
        private int hFrameBytes = -1;
        private Lz4Block[] blocks = null;

        public GpuVideoWriter(FileStream videoWriteStream)
        {
            if(videoWriteStream == null)
            {
                throw new IOException("[GpuVideoWriter] writeStream is null.");
            }

            writer = new BinaryWriter(videoWriteStream);
        }

        public void Dispose()
        {
            writer?.Close();
            writer?.Dispose();
            writer = null;
        }

        /// <summary>
        /// Allocate address records of frame data.
        /// </summary>
        /// <param name="frameCount"></param>
        public void Allocate(int frameCount)
        {
            blocks = null;
            blocks = new Lz4Block[frameCount];
        }

        /// <summary>
        /// Write video file information to head on stream.
        /// </summary>
        /// <param name="compressedTexture"></param>
        /// <param name="frameCount">frame count of video</param>
        /// <param name="framePerSecond">frame count per second of video (FPS)</param>
        public void WriteHeader(Texture2D compressedTexture, int frameCount, float framePerSecond)
        {
            WriteHeader(compressedTexture.width, compressedTexture.height, frameCount, framePerSecond, compressedTexture.format, compressedTexture.GetRawTextureData().Length);
        }

        /// <summary>
        /// Write video file information to head on stream.
        /// </summary>
        /// <param name="width">frame width</param>
        /// <param name="height">frame height</param>
        /// <param name="frameCount">frame count</param>
        /// <param name="framePerSecond">frame count per second</param>
        /// <param name="format">texture format of frame</param>
        /// <param name="frameBytes">byte length per texture</param>
        /// <exception cref="Exception"></exception>
        public void WriteHeader(int width, int height, int frameCount, float framePerSecond, TextureFormat format, int frameBytes)
        {
            if(SupportTextureFormats.Contains(format) == false)
            {
                throw new Exception($"[GpuVideoWriter] {format.ToString()} is not supported.");
            }

            writer.Seek(0, SeekOrigin.Begin);
            writer.Write(width);
            writer.Write(height);
            writer.Write(frameCount);
            writer.Write(framePerSecond);
            writer.Write(format.ToInt());
            writer.Write(frameBytes);

            hWidth = width;
            hHeight = height;
            hFrameCount = frameCount;
            hFramePerSecond = framePerSecond;
            hFormat = format.ToInt();
            hFrameBytes = frameBytes;
        }

        /// <summary>
        /// Write frame texture data to the video file stream.
        /// </summary>
        /// <param name="frameNumber"></param>
        /// <param name="texture"></param>
        /// <exception cref="Exception"></exception>
        public void WriteFrame(int frameCount, Texture2D texture)
        {
            if(texture == null)
            {
                throw new Exception("[GpuVideoWriter] texture is null.");
            }
            if(frameCount < 0 || frameCount > blocks.Length - 1)
            {
                throw new Exception($"[GpuVideoWriter] frameNumber: {frameCount} is out of range.");
            }

            Debug.Assert(texture.format == hFormat.ToTextureFormat(), "[GpuVideoWriter] texture format is differ from header.");
            Debug.Assert(texture.width == hWidth, "[GpuVideoWriter] texture width is differ from header.");
            Debug.Assert(texture.height == hHeight, "[GpuVideoWriter] texture height is differ from header.");

            byte[] rawTextureBytes = texture.GetRawTextureData();

            // Allocate for Copy
            IntPtr src = Marshal.AllocHGlobal(rawTextureBytes.Length);
            Marshal.Copy(rawTextureBytes, 0, src, rawTextureBytes.Length);

            int compressBound = Lz4Native.lz4_compressBound_native(rawTextureBytes.Length);
            IntPtr dst = Marshal.AllocHGlobal(compressBound);

            // Compress texture data
            int compressed = Lz4Native.lz4_compress_HC_native(src, dst, rawTextureBytes.Length, compressBound, Lz4Native.LZ4HC_CLEVEL_DEFAULT);

            // Copy
            byte[] compressedBytes = new byte[compressed];
            Marshal.Copy(dst, compressedBytes, 0, compressed);

            // Write the data
            long headPosition = writer.BaseStream.Position;
            writer.Write(compressedBytes);

            // Record the frame address and size
            blocks[frameCount].address = Convert.ToUInt64(headPosition);
            blocks[frameCount].size = Convert.ToUInt64(compressedBytes.Length);

            // Free IntPtrs
            Marshal.FreeHGlobal(src);
            Marshal.FreeHGlobal(dst);

            compressedBytes = null;
        }

        /// <summary>
        /// Write frame data address records as footer to the video file stream.
        /// </summary>
        /// <exception cref="Exception"></exception>
        public void WriteFooter()
        {
            if(blocks == null)
            {
                throw new Exception("[GpuVideoWriter] Address records are null.");
            }

            foreach(var block in blocks)
            {
                writer.Write(block.address);
                writer.Write(block.size);
            }
        }
    }
}