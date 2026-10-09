// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class XbfOutputStreamTests
    {
        private string _directory;

        [TestInitialize]
        public void Initialize()
        {
            _directory = Path.Combine(Path.GetTempPath(), "XamlCompilerXbfOutputTests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_directory);
        }

        [TestCleanup]
        public void Cleanup()
        {
            Directory.Delete(_directory, true);
        }

        [TestMethod]
        public void UnchangedXbfOlderThanSourceUpdatesTimestamp()
        {
            AssertUnchangedOutputIsCurrent(new byte[] { 1, 2, 3 });
        }

        [TestMethod]
        public void UnchangedEmptyXbfOlderThanSourceUpdatesTimestamp()
        {
            AssertUnchangedOutputIsCurrent(new byte[0]);
        }

        [TestMethod]
        public void UnchangedXbfNewerThanSourceKeepsTimestamp()
        {
            // Simulates forced regeneration (e.g. a referenced assembly changed) where the XAML
            // wasn't touched: identical output must not dirty downstream targets.
            string path = Path.Combine(_directory, "forced.xbf");
            byte[] contents = { 1, 2, 3 };
            File.WriteAllBytes(path, contents);
            string source = CreateSource(DateTime.UtcNow.AddDays(-3));
            DateTime xbfTimestamp = DateTime.UtcNow.AddDays(-2);
            File.SetLastWriteTimeUtc(path, xbfTimestamp);
            using (CreateOutput(path, source, contents)) { }
            CollectionAssert.AreEqual(contents, File.ReadAllBytes(path));
            Assert.AreEqual(xbfTimestamp, File.GetLastWriteTimeUtc(path));
        }

        [TestMethod]
        public void UnchangedXbfWithMissingSourceKeepsTimestamp()
        {
            string path = Path.Combine(_directory, "nosource.xbf");
            byte[] contents = { 1, 2, 3 };
            File.WriteAllBytes(path, contents);
            DateTime xbfTimestamp = DateTime.UtcNow.AddDays(-2);
            File.SetLastWriteTimeUtc(path, xbfTimestamp);
            using (CreateOutput(path, Path.Combine(_directory, "missing.xaml"), contents)) { }
            Assert.AreEqual(xbfTimestamp, File.GetLastWriteTimeUtc(path));
        }

        [TestMethod]
        public void MissingXbfIsCreated()
        {
            string path = Path.Combine(_directory, "new.xbf");
            byte[] contents = { 1, 2, 3 };
            using (CreateOutput(path, CreateSource(DateTime.UtcNow), contents)) { }
            CollectionAssert.AreEqual(contents, File.ReadAllBytes(path));
        }

        [TestMethod]
        public void ChangedXbfIsWritten()
        {
            byte[] original = { 1, 2, 3 };
            foreach (byte[] contents in new[] { new byte[] { 1, 2, 4 }, new byte[] { 1, 2, 3, 4 } })
            {
                string path = Path.Combine(_directory, "changed.xbf");
                File.WriteAllBytes(path, original);
                File.SetLastWriteTimeUtc(path, DateTime.UtcNow.AddDays(-2));
                DateTime sourceTimestamp = DateTime.UtcNow.AddMinutes(-1);
                using (CreateOutput(path, CreateSource(sourceTimestamp), contents)) { }
                CollectionAssert.AreEqual(contents, File.ReadAllBytes(path));
                Assert.IsTrue(File.GetLastWriteTimeUtc(path) >= sourceTimestamp);
            }
        }

        [TestMethod]
        public void RepeatedDisposeDoesNotUpdateTimestamp()
        {
            string path = Path.Combine(_directory, "disposed.xbf");
            byte[] contents = { 1, 2, 3 };
            File.WriteAllBytes(path, contents);
            using (IDisposable output = CreateOutput(path, CreateSource(DateTime.UtcNow), contents))
            {
                output.Dispose();
                File.SetLastWriteTimeUtc(path, DateTime.UtcNow.AddDays(-2));
                DateTime timestamp = File.GetLastWriteTimeUtc(path);
                output.Dispose();
                Assert.AreEqual(timestamp, File.GetLastWriteTimeUtc(path));
            }
        }

        private void AssertUnchangedOutputIsCurrent(byte[] contents)
        {
            string path = Path.Combine(_directory, "unchanged.xbf");
            File.WriteAllBytes(path, contents);
            File.SetLastWriteTimeUtc(path, DateTime.UtcNow.AddDays(-2));
            DateTime sourceTimestamp = DateTime.UtcNow.AddMinutes(-1);
            using (CreateOutput(path, CreateSource(sourceTimestamp), contents)) { }
            CollectionAssert.AreEqual(contents, File.ReadAllBytes(path));
            Assert.IsTrue(File.GetLastWriteTimeUtc(path) >= sourceTimestamp,
                "Byte-identical generation must advance an XBF that is older than its source XAML.");
        }

        private string CreateSource(DateTime lastWriteTimeUtc)
        {
            string source = Path.Combine(_directory, Guid.NewGuid().ToString("N") + ".xaml");
            File.WriteAllText(source, "<Page />");
            File.SetLastWriteTimeUtc(source, lastWriteTimeUtc);
            return source;
        }

        private static IDisposable CreateOutput(string path, string sourceXamlPath, byte[] contents)
        {
            object output = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.FileIO.StreamXbfOutput")
                .CreateInstance(new object[] { path, sourceXamlPath });
            IntPtr written = Marshal.AllocHGlobal(sizeof(int));
            try
            {
                ((IStream)output).Write(contents, contents.Length, written);
                Assert.AreEqual(contents.Length, Marshal.ReadInt32(written));
            }
            finally
            {
                Marshal.FreeHGlobal(written);
            }
            return (IDisposable)output;
        }
    }

    [TestClass]
    public class XbfGeneratorTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
#if DO_NOT_USE_GENXBF
        [Ignore]
#endif
        public void CanCallXbfGen()
        {
            string genericXaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyNamespace.MyClass'>
    <Grid>
        <Button x:Name='btn' Click='ClickHandler' {0}/>
        <Button x:Name='btn2' Click='ClickHandler' Loaded='LoadedHandler' />
        <Button x:Name='btn3' x:FieldModifier='public' />
    </Grid>
</Page>";

            string rs1Xaml = string.Format(genericXaml, "");
            string rs3Xaml = string.Format(genericXaml, "XYFocusDownNavigationStrategy = 'NavigationDirectionDistance'");

            // RS3 Xaml should generate just fine in Latest
            var xbgGenerator = _testHelper.GenerateXbf(new Version(KnownVersions.Latest), rs3Xaml);
            Assert.AreEqual(0, xbgGenerator.XbfErrors.Count);

            // RS1 Xaml should generate just fine in Latest
            xbgGenerator = _testHelper.GenerateXbf(new Version(KnownVersions.Latest), rs1Xaml);
            Assert.AreEqual(0, xbgGenerator.XbfErrors.Count);
        }

        [TestMethod]
        [Ignore]
        public void IsTemplateCollectedOnce()
        {
            string genericXaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyNamespace.MyClass'>
    <Grid>
        <Grid.Resources>
            <ControlTemplate x:Name='namedControlTemplate' TargetType='Button'>
                <StackPanel>
                    <TextBox Text='hello world' Height='{x:Bind Height}' />
                </StackPanel>
            </ControlTemplate>

            <DataTemplate x:Name='namedDataTemplate' x:DataType='Button'>
                <StackPanel>
                    <TextBox Text='hello world' Height='{x:Bind Height}' />
                </StackPanel>
            </DataTemplate>
        </Grid.Resources>
        <Button x:Name='btn' Click='ClickHandler' />
    </Grid>
</Page>";

            // If a template was harvested twice and had two different connection IDs on the same template,
            // GenXBF would error due to a duplicate attribute assignment.  Verify we only collected
            // each one once by verifying there are no errors.
            var xbgGenerator = _testHelper.GenerateXbf(new Version(KnownVersions.Latest), genericXaml);
            Assert.AreEqual(0, xbgGenerator.XbfErrors.Count);
        }
    }
}
