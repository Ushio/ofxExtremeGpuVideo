using System;
using System.Linq;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;

namespace ExtremeGpuVideo.Encoder 
{
    public partial class GpuVideoEncoder : EditorWindow
    {
        public static readonly string[] SuppertFileExts = new[]
        {
            ".png", ".jpg", ".jpeg", ".astc"
        };

        private bool ValidateFiles(string comparisonFile, IEnumerable<string> files)
        {
            string ext = Path.GetExtension(comparisonFile).ToLower();

            if (SuppertFileExts.Contains(ext) == false)
            {
                return false;
            }

            return files.All(f => Path.GetExtension(f).ToLower() == ext);
        }

        private Texture2D CreateFirstTexture(string path)
        {
            var extension = Path.GetExtension(path).ToLower();

            switch (extension)
            {
                case ".png":
                    return CreateTempTexture(path, TextureFormat.DXT5);
                case ".jpg":
                case ".jpeg":
                    return CreateTempTexture(path, TextureFormat.DXT1);
                case ".astc":
                    var astcBytes = File.ReadAllBytes(path);
                    var astcFormat = ASTCFormat.Read(astcBytes);
                    return CreateTempTexture(path, astcFormat.GetTextureFormat());
            }

            throw new NotSupportedException($"[GpuVideoEncoder] File extension '{extension}' is not supported.");
        }

        private Texture2D CreateTempTexture(string path, TextureFormat textureFormat)
        {
            switch(textureFormat)
            {
                case TextureFormat.DXT1:
                case TextureFormat.DXT5:
                    var texture = new Texture2D(8, 8, textureFormat, false);
                    texture.LoadImage(File.ReadAllBytes(path));
                    texture.Apply();
                    texture.Compress(true);
                    return texture;
                case TextureFormat.ASTC_4x4:
                case TextureFormat.ASTC_5x5:
                case TextureFormat.ASTC_6x6:
                case TextureFormat.ASTC_8x8:
                case TextureFormat.ASTC_10x10:
                case TextureFormat.ASTC_12x12:
                    var astcBytes = File.ReadAllBytes(path);
                    var astcFormat = ASTCFormat.Read(astcBytes);
                    var astcTexture = new Texture2D(astcFormat.Width, astcFormat.Height, astcFormat.GetTextureFormat(), false);
                    astcTexture.LoadRawTextureData(astcBytes);
                    astcTexture.Apply(updateMipmaps: false, makeNoLongerReadable: true);
                    return astcTexture;
                default:
                    throw new NotSupportedException($"[GpuVideoEncoder] Texture format '{textureFormat}' is not supported.");
            }
        }

        public void StartEncode()
        {
            List<GpuVideoWriter> writers = new List<GpuVideoWriter>();
    
            foreach (var queue in encodeQueues)
            {
                // get target frame image files
                var files = System.IO.Directory.GetFiles(queue.Path);

                if (files.Length < 1)
                {
                    queue.Complete(EncodeQueue.ResultType.Error);
                    queue.LogError("Directory is empty.");
                    continue;
                }

                var firstFile = files[0];

                // check files
                if (ValidateFiles(firstFile, files) == false)
                {
                    queue.Complete(EncodeQueue.ResultType.Error);
                    queue.LogError("File extensions do not match.");
                    continue;
                }

                var exportPath = string.IsNullOrEmpty(queue.ExportPath) ?
                    queue.GetDefaultExportPath(defaultExportDir) :
                    queue.ExportPath;

                if (Directory.Exists(System.IO.Path.GetDirectoryName(exportPath)) == false)
                {
                    Directory.CreateDirectory(System.IO.Path.GetDirectoryName(exportPath));
                }

                var writeStream = File.OpenWrite(exportPath);
                var writer = new GpuVideoWriter(writeStream);
                var firstTexture = CreateFirstTexture(firstFile);

                // header
                writer.WriteHeader(firstTexture, files.Length, queue.FrameRate);
                // allocate
                writer.Allocate(files.Length);

                bool cancelled = false;

                // Encode frames
                for(int i = 0; i < files.Length; i++)
                {
                    var texture = CreateTempTexture(files[i], firstTexture.format);
                    writer.WriteFrame(i, texture);
                    DestroyImmediate(texture);

                    var title = $"GpuVideoEncoder: {System.IO.Path.GetFileName(exportPath)}";
                    var message = $"Encoding... ({i + 1} / {files.Length})";
                    var progress = (float)(i + 1) / (float)files.Length;

                    cancelled = EditorUtility.DisplayCancelableProgressBar(title, message, progress);

                    if(cancelled)
                    {
                        queue.LogError("Cancelled.");
                        break;
                    }
                }

                EditorUtility.ClearProgressBar();

                // footer
                writer.WriteFooter();
                writer.Dispose();

                // release
                writeStream.Close();
                writeStream.Dispose();

                queue.Complete(cancelled ? EncodeQueue.ResultType.Error : EncodeQueue.ResultType.Success);
            }
        }
    }

}
#endif