using UnityEngine;
using System.Collections.Generic;
using System.IO;
using System;
using System.Runtime.InteropServices;

public class Lz4Native {
	[DllImport ("lz4_native")]
	public static extern int lz4_decompress_safe_native (IntPtr source, IntPtr dest, int compressedSize, int maxDecompressedSize);
}
public class GpuVideo : System.IDisposable {
	public enum GpuTextureFormat : int {
		Dxt1 = 1,
		Dxt3 = 3,
		Dxt5 = 5
	};

	private FileStream _filestream;
	private int _width;
	private int _height;
	private int _frameCount;
	private float _framePerSecond;
	private GpuTextureFormat _format;
	private int _frameBytes;
	private Texture2D _texture;

	private struct Lz4Block {
		public ulong address;
		public ulong size;
	}
	private List<Lz4Block> _lz4Blocks = new List<Lz4Block> ();
	private byte[] _lz4Buffer;
	private IntPtr _lz4BufferNative;
	private byte[] _textureBuffer;
	private IntPtr _textureBufferNative;

	private int _frameAt = -1;

	public GpuVideo(string pathForStreamingAssets) {
		_filestream = File.OpenRead (Application.streamingAssetsPath + pathForStreamingAssets);
		{
			var reader = new BinaryReader (_filestream);
			_width = (int)reader.ReadUInt32 ();
			_height = (int)reader.ReadUInt32 ();
			_frameCount = (int)reader.ReadUInt32 ();
			_framePerSecond = reader.ReadSingle ();
			int fmt = (int)reader.ReadUInt32 ();
			_format = System.Enum.IsDefined (typeof(TextureFormat), fmt) ? (GpuTextureFormat)fmt : GpuTextureFormat.Dxt1;
			_frameBytes = (int)reader.ReadUInt32 ();
		}

		_filestream.Seek (_filestream.Length - _frameCount * 16 /*block def*/, SeekOrigin.Begin);
		{
			var reader = new BinaryReader (_filestream);
			_lz4Blocks.Capacity = _frameCount;

			ulong maxSize = 0;
			for (int i = 0; i < _frameCount; ++i) {
				Lz4Block block = new Lz4Block ();
				block.address = reader.ReadUInt64 ();
				block.size = reader.ReadUInt64 ();
				_lz4Blocks.Add (block);

				Debug.Log (string.Format ("block {0}, {0}", block.address, block.size));

				maxSize = maxSize < block.size ? block.size : maxSize;
			}
			_lz4Buffer = new byte[maxSize];
			_lz4BufferNative = Marshal.AllocHGlobal((int)maxSize);
		}

		TextureFormat textureFmt = TextureFormat.ARGB32;
		switch (_format) {
		case GpuTextureFormat.Dxt1:
			textureFmt = TextureFormat.DXT1;
			break;
		case GpuTextureFormat.Dxt3:
			Debug.Assert(false, "Not Supperted Fomat DXT 3");
			break;
		case GpuTextureFormat.Dxt5:
			textureFmt = TextureFormat.DXT5;
			break;
		}

		_texture = new Texture2D (_width, _height, textureFmt, false);
		_textureBuffer = new byte[_frameBytes];
		_textureBufferNative = Marshal.AllocHGlobal(_frameBytes);

		this.setFrame (0);
	}


	public int Width { get { return _width; } }
	public int Height { get { return _height; } }
	public int FrameCount { get { return _frameCount; } }
	public float FramePerSecond { get { return _framePerSecond; } }
	public GpuTextureFormat Format { get { return _format; } }
	public int FrameBytes { get { return _frameBytes; } }

	public Texture2D Texture { get { return _texture; } }

	public float Duration {
		get {
			return _frameCount * (1.0f / _framePerSecond);
		}
	}

	public void setTime(float atTime) {
		float framef = atTime * _framePerSecond;
		this.setFrame((int)framef);
	}
	public void setFrame(int frameAt) {
		frameAt = Mathf.Max(frameAt, 0);
		frameAt = Mathf.Min(frameAt, this.FrameCount - 1);
		if (_frameAt == frameAt) {
			return;
		}
		_frameAt = frameAt;
		this.UpdateTexture (_frameAt);
	}
	private void UpdateTexture(int frameAt) {
		var block = _lz4Blocks [frameAt];
		_filestream.Seek ((long)block.address, SeekOrigin.Begin);
		_filestream.Read (_lz4Buffer, 0, (int)block.size);
		Marshal.Copy (_lz4Buffer, 0, _lz4BufferNative, (int)block.size);
		Lz4Native.lz4_decompress_safe_native (_lz4BufferNative, _textureBufferNative, (int)block.size, _frameBytes);
		Marshal.Copy (_textureBufferNative, _textureBuffer, 0, _frameBytes);
		_texture.LoadRawTextureData (_textureBuffer);
		_texture.Apply ();
	}

	public void Dispose () {
		if (_filestream != null) {
			_filestream.Dispose ();
			_filestream = null;
		}
	
		Marshal.FreeHGlobal(_lz4BufferNative);
		_lz4BufferNative = IntPtr.Zero;
		Marshal.FreeHGlobal (_textureBufferNative);
		_textureBufferNative = IntPtr.Zero;
	}
}