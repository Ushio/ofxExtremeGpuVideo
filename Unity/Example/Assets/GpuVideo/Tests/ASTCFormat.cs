using NUnit.Framework;
using UnityEngine;

namespace ExtremeGpuVideo.Tests
{
    public class ASTCFormat
    {
        [Test]
        public void TestReadASTC()
        {
            ReadASTC(4, 4, 1024, 1024);
            ReadASTC(5, 5, 1024, 1024);
            ReadASTC(6, 6, 1024, 1024);
            ReadASTC(8, 8, 1024, 1024);
            ReadASTC(10, 10, 1024, 1024);
            ReadASTC(12, 12, 1024, 1024);
        }

        private void ReadASTC(int x, int y, int w, int h)
        {
            string path = System.IO.Path.Combine(Application.streamingAssetsPath, $"test/test_{x}x{y}_{w}x{h}.astc");
            var bytes = System.IO.File.ReadAllBytes(path);
            var format = ExtremeGpuVideo.ASTCFormat.Read(bytes);

            Debug.Log(format.GetTextureFormat());
            Assert.AreEqual(w, format.Width);
            Assert.AreEqual(h, format.Height);
            Assert.AreEqual(x, format.BlockSizeX);
            Assert.AreEqual(y, format.BlockSizeY);
        }
    }
}