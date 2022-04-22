using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

namespace ExtremeGpuVideo.Plugin
{
	public class Lz4Native
	{
		public const int LZ4HC_CLEVEL_DEFAULT = 9;

		[DllImport("lz4_native")]
		public static extern int lz4_decompress_safe_native(IntPtr source, IntPtr dest, int compressedSize, int maxDecompressedSize);

		[DllImport("lz4_native")]
		public static extern int lz4_compress_default_native(IntPtr source, IntPtr dest, int srcSize, int maxDestSize);

		[DllImport("lz4_native")]
		public static extern int lz4_compressBound_native(int inputSize);

		[DllImport("lz4_native")]
		public static extern int lz4_compress_HC_native(IntPtr source, IntPtr dest, int srcSize, int maxDstSize, int compressionLevel);
	}
}