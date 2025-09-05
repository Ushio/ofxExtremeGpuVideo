using UnityEngine;

namespace ExtremeGpuVideo
{
    /// <summary>
    /// https://github.com/ARM-software/astc-encoder/blob/main/Docs/FileFormat.md
    /// </summary>
    public class ASTCFormat
    {
        public const int FILE_HEADER_SIZE = 16;
        public static readonly byte[] MAGIC_NUMBER = new byte[] { 0x13, 0xAB, 0xA1, 0x5C };

        public int Width { private set; get; }
        public int Height { private set; get; }
        public int Depth { private set; get; }
        public int BlockSizeX { private set; get; }
        public int BlockSizeY { private set; get; }
        public int BlockSizeZ { private set; get; }

        public TextureFormat GetTextureFormat()
        {
            if (BlockSizeX != BlockSizeY)
            {
                throw new System.Exception($"[ASTCFormat] Unsupported ASTC block size: {BlockSizeX}x{BlockSizeY}");
            }

            switch (BlockSizeX)
            {
                case 4:
                    return TextureFormat.ASTC_4x4;
                case 5:
                    return TextureFormat.ASTC_5x5;
                case 6:
                    return TextureFormat.ASTC_6x6;
                case 8:
                    return TextureFormat.ASTC_8x8;
                case 10:
                    return TextureFormat.ASTC_10x10;
                case 12:
                    return TextureFormat.ASTC_12x12;
            }

            throw new System.Exception($"[ASTCFormat] Unsupported ASTC block size: {BlockSizeX}x{BlockSizeY}");
        }

        public static ASTCFormat Read(byte[] bytes)
        {
            // Check magic number
            for (int i = 0; i < MAGIC_NUMBER.Length; i++)
            {
                if (bytes[i] != MAGIC_NUMBER[i])
                {
                    throw new System.Exception("[ASTCFormat] Invalid ASTC magic number.");
                }
            }

            var format = new ASTCFormat();

            using (var memoryStream = new System.IO.MemoryStream(bytes))
            {
                memoryStream.Position = 4;

                format.BlockSizeX = memoryStream.ReadByte();
                format.BlockSizeY = memoryStream.ReadByte();
                format.BlockSizeZ = memoryStream.ReadByte();

                format.Width = memoryStream.ReadByte() | (memoryStream.ReadByte() << 8) | (memoryStream.ReadByte() << 16);
                format.Height = memoryStream.ReadByte() | (memoryStream.ReadByte() << 8) | (memoryStream.ReadByte() << 16);
                format.Depth = memoryStream.ReadByte() | (memoryStream.ReadByte() << 8) | (memoryStream.ReadByte() << 16);
            }

            return format;
        }
    }
}