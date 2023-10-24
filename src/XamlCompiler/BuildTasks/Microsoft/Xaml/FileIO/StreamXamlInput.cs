// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.FileIO
{
    using System.IO;

    internal class StreamXamlInput : StreamImpl, IXamlStream
    {
        public StreamXamlInput(string filePath)
        {
            _underlyingStream = new FileStream(filePath, FileMode.Open, FileAccess.Read);
        }

        public StreamType StreamType
        {
            get
            {
                return StreamType.Input;
            }
        }
    }
}