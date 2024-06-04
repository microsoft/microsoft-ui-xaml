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
        private MemoryStream _memoryStream;

        public StreamXbfOutput(string filePath)
        {
            _filePath = filePath;
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

        protected override void Dispose(bool disposing)
        {
            // If already disposed, do nothing
            if (_underlyingStream != null)
            {
                // Conditionally write output file only if content has changed,
                // to avoid unnecessary downstream build ripples.
                if (ContentChanged())
                {
                    using (var fileStream = new FileStream(_filePath, FileMode.Create, FileAccess.Write))
                    {
                        _memoryStream.WriteTo(fileStream);
                    }
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