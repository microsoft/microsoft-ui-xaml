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
                string masters = Path.Combine(directory, "masters");
                string common = Path.Combine(masters, "common", "generated");
                string codegen = Path.Combine(directory, "codegen");
                Directory.CreateDirectory(common);
                Directory.CreateDirectory(codegen);
                string targets = Path.Combine(directory, "targets.txt");
                File.WriteAllText(targets, "generated|generated");
                Directory.CreateDirectory(Path.Combine(codegen, "generated"));
                string path = Path.Combine(common, "Test.g.cs");
                string original = "#pragma checksum \"original checksum\"\r\nclass Test { }\r\n";
                var encoding = new UTF8Encoding(hasByteOrderMark);
                File.WriteAllText(path, original, encoding);
                File.WriteAllText(Path.Combine(codegen, "generated", "Test.g.cs"), original, encoding);
                string[] arguments = { masters, "chk", codegen, targets };
                try
                {
                    using (var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
                    using (var mapping = MemoryMappedFile.CreateFromFile(stream, null, 0, MemoryMappedFileAccess.Read, HandleInheritability.None, false))
                    using (var reader = mapping.CreateViewStream(0, stream.Length, MemoryMappedFileAccess.Read))
                    {
                        Assert.AreEqual(0, FixMasters.Program.Main(arguments));
                        string checkedPath = Path.Combine(masters, "chk", "generated", "Test.g.cs");
                        Assert.AreEqual("#pragma checksum...\r\nclass Test { }\r\n", File.ReadAllText(checkedPath));
                        byte[] bytes = File.ReadAllBytes(checkedPath);
                        Assert.AreEqual(hasByteOrderMark, bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF);
                        Assert.AreEqual(original, File.ReadAllText(Path.Combine(masters, "fre", "generated", "Test.g.cs")));

                        Assert.AreEqual(0, FixMasters.Program.Main(arguments));
                        CollectionAssert.AreEqual(bytes, File.ReadAllBytes(checkedPath));

                        arguments[1] = "fre";
                        Assert.AreEqual(0, FixMasters.Program.Main(arguments));
                        CollectionAssert.AreEqual(bytes, File.ReadAllBytes(path));
                        Assert.AreEqual(1, Directory.GetFiles(masters, "*.g.*", SearchOption.AllDirectories).Length);
                        Assert.AreEqual(0, Directory.GetDirectories(directory, ".masters-*").Length);

                        using (var originalReader = new StreamReader(reader, Encoding.UTF8))
                        {
                            Assert.AreEqual(original, originalReader.ReadToEnd());
                        }
                    }
                }
                finally
                {
                    Directory.Delete(directory, true);
                }
            }
        }

        [TestMethod]
        public void FixMasters_RestoresOriginalMastersWhenInstallationFails()
        {
            string directory = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));
            string masters = Path.Combine(directory, "masters");
            string common = Path.Combine(masters, "common", "generated");
            string codegen = Path.Combine(directory, "codegen");
            Directory.CreateDirectory(common);
            Directory.CreateDirectory(Path.Combine(codegen, "generated"));
            string targets = Path.Combine(directory, "targets.txt");
            File.WriteAllText(targets, "generated|generated");
            string path = Path.Combine(common, "Test.g.cs");
            File.WriteAllText(path, "original", new UTF8Encoding(true));
            byte[] original = File.ReadAllBytes(path);
            File.WriteAllText(Path.Combine(codegen, "generated", "Test.g.cs"), "new");
            string sharedPath = Path.Combine(common, "Shared.g.cs");
            File.WriteAllText(sharedPath, "shared\r\n");
            byte[] shared = File.ReadAllBytes(sharedPath);
            File.WriteAllBytes(Path.Combine(codegen, "generated", "Shared.g.cs"), shared);

            // An existing directory at a file's destination forces installation to fail after
            // the originals have been displaced and the shared file has been installed.
            Directory.CreateDirectory(Path.Combine(masters, "chk", "generated", "Test.g.cs"));
            try
            {
                bool failed = false;
                try
                {
                    FixMasters.Program.Main(new[] { masters, "chk", codegen, targets });
                }
                catch (IOException)
                {
                    failed = true;
                }
                Assert.IsTrue(failed, "Expected installation to reject a directory at a file's destination.");
                CollectionAssert.AreEqual(original, File.ReadAllBytes(path));
                CollectionAssert.AreEqual(shared, File.ReadAllBytes(sharedPath));
                Assert.AreEqual(2, Directory.GetFiles(masters, "*.g.*", SearchOption.AllDirectories).Length);
                Assert.AreEqual(0, Directory.GetDirectories(directory, ".masters-*").Length);
            }
            finally
            {
                Directory.Delete(directory, true);
            }
        }
    }
}
