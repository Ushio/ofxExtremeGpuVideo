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
            ".png", ".jpg", ".jpeg"
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
            bool hasAlpha = path.ToLower().Contains("png");

            var firstTexture = CreateTempTexture(path, hasAlpha ? TextureFormat.DXT5 : TextureFormat.DXT1);

            return firstTexture;
        }

        private Texture2D CreateTempTexture(string path, TextureFormat textureFormat)
        {
            var texture = new Texture2D(8, 8, textureFormat, false);
            texture.LoadImage(File.ReadAllBytes(path));
            texture.Apply();
            texture.Compress(true);

            return texture;
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