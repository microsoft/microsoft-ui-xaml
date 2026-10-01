// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Text;

namespace UnitTests
{
    [TestClass]
    public class FixMastersTests
    {
        [TestMethod]
        public void FixMasters_PreservesEncodingAndSupportsMappedReaders()
        {
            foreach (bool hasByteOrderMark in new[] { false, true })
            {
                string directory = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(directory);
                string path = Path.Combine(directory, "Test.g.cs");
                string original = "#pragma checksum \"original checksum\"\r\nclass Test { }\r\n";
                var encoding = new UTF8Encoding(hasByteOrderMark);
                File.WriteAllText(path, original, encoding);
                try
                {
                    using (var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
                    using (var mapping = MemoryMappedFile.CreateFromFile(stream, null, 0, MemoryMappedFileAccess.Read, HandleInheritability.None, false))
                    using (var reader = mapping.CreateViewStream(0, stream.Length, MemoryMappedFileAccess.Read))
                    {
                        FixMasters.Program.FixMasters(new FileInfo(path));
                        Assert.AreEqual("#pragma checksum...\r\nclass Test { }\r\n", File.ReadAllText(path));
                        byte[] bytes = File.ReadAllBytes(path);
                        Assert.AreEqual(hasByteOrderMark, bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF);
                        Assert.AreEqual(1, Directory.GetFiles(directory).Length);

                        using (var originalReader = new StreamReader(reader, Encoding.UTF8))
                        {
                            Assert.AreEqual(original, originalReader.ReadToEnd());
                        }
                    }

                    byte[] normalized = File.ReadAllBytes(path);
                    FixMasters.Program.FixMasters(new FileInfo(path));
                    CollectionAssert.AreEqual(normalized, File.ReadAllBytes(path));
                }
                finally
                {
                    File.Delete(path);
                    Directory.Delete(directory);
                }
            }
        }
    }
}
