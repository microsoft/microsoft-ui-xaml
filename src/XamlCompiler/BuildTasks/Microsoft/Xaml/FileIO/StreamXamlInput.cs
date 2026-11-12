// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.FileIO
{
    using System.IO;
    using System.Runtime.InteropServices.Marshalling;

    // The COM class information is looked up on the exact runtime type, so derived types need to be marked as well
    [GeneratedComClass]
    internal partial class StreamXamlInput : StreamImpl, IXamlStream
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