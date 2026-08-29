// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DumpXBF
{
    using System.IO;

    internal class XbfStreamWrapper : StreamImpl
    {
        public XbfStreamWrapper(string filePath)
        {
            _underlyingStream = new FileStream(filePath, FileMode.Open, FileAccess.Read);
        }
    }

    internal class DumpStreamWrapper : StreamImpl
    { 
        public DumpStreamWrapper(string filePath)
        {
            Console.OutputEncoding = Encoding.Unicode;
            _underlyingStream = Console.OpenStandardOutput();
        }
    }
}
