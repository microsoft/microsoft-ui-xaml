// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.FileIO
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;

    internal class StreamXbfOutput : StreamImpl, IXamlStream
    {
        private string _filePath;
        private string _sourceXamlPath;
        private MemoryStream _memoryStream;

        public StreamXbfOutput(string filePath, string sourceXamlPath)
        {
            _filePath = filePath;
            _sourceXamlPath = sourceXamlPath;
            _underlyingStream = _memoryStream = new MemoryStream();
        }

        private bool ContentChanged()
        {
            var fi = new FileInfo(_filePath);
            if (!fi.Exists || fi.Length != _memoryStream.Length)
                return true;

            var fileContent = new ReadOnlySpan<byte>(File.ReadAllBytes(_filePath));
            var memoryContent = new ReadOnlySpan<byte>(_memoryStream.GetBuffer(), 0, (int)_memoryStream.Length);
            return !fileContent.SequenceEqual(memoryContent);
        }

        // Pass2 treats an XBF older than its source XAML as out of date. When identical bytes are
        // regenerated for a touched XAML, advance the XBF so it isn't rebuilt on every build.
        // Leave it alone when regeneration was forced for other reasons (e.g. a referenced
        // assembly changed) so unchanged XBFs don't dirty downstream targets.
        private void RefreshTimestampIfOlderThanSource()
        {
            if (string.IsNullOrEmpty(_sourceXamlPath) || !File.Exists(_sourceXamlPath))
            {
                return;
            }

            DateTime sourceTime = File.GetLastWriteTimeUtc(_sourceXamlPath);
            if (sourceTime > File.GetLastWriteTimeUtc(_filePath))
            {
                DateTime now = DateTime.UtcNow;
                File.SetLastWriteTimeUtc(_filePath, sourceTime > now ? sourceTime : now);
            }
        }

        protected override void Dispose(bool disposing)
        {
            // If already disposed, do nothing
            if (_underlyingStream != null)
            {
                // Avoid rewriting identical bytes to prevent downstream build ripples (copy, PRI, packaging).
                if (ContentChanged())
                {
                    using (var fileStream = new FileStream(_filePath, FileMode.Create, FileAccess.Write))
                    {
                        _memoryStream.WriteTo(fileStream);
                    }
                }
                else
                {
                    RefreshTimestampIfOlderThanSource();
                }
            }

            base.Dispose(disposing);
        }

        public StreamType StreamType
        {
            get
            {
                return StreamType.Output;
            }
        }
    }
}