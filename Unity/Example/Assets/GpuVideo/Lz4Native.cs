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

#if UNITY_IOS && !UNITY_EDITOR
		const string dll = "__Internal";
#else
        const string dll = "lz4_native";
#endif

        [DllImport(dll)]
		public static extern int lz4_decompress_safe_native(IntPtr source, IntPtr dest, int compressedSize, int maxDecompressedSize);

		[DllImport(dll)]
		public static extern int lz4_compress_default_native(IntPtr source, IntPtr dest, int srcSize, int maxDestSize);

		[DllImport(dll)]
		public static extern int lz4_compressBound_native(int inputSize);

		[DllImport(dll)]
		public static extern int lz4_compress_HC_native(IntPtr source, IntPtr dest, int srcSize, int maxDstSize, int compressionLevel);
	}
}