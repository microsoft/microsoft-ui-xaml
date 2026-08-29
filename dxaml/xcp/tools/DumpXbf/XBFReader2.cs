// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;

namespace DumpXBF
{
    class XBFReader2
    {
        public XBFReader2(string filePath, bool verbose)
        {
            _xbfFilePath = filePath;

            if (String.IsNullOrEmpty(filePath))
            {
                Console.WriteLine("No XBF file specified.");
                return;
            }
            var workingDirectory = Process.GetCurrentProcess().StartInfo.WorkingDirectory;
            if (!NativeMethodsHelper.EnsureGenXbfIsLoaded(workingDirectory))
            {
                Console.WriteLine("Unable to load GenXbf.dll from " + workingDirectory + " which is required for reading XBF2 files.");
                return;
            }

            _initialized = true;
        }

        public void Dump()
        {
            if (_initialized)
            {
                IStream xbfStream = new XbfStreamWrapper(_xbfFilePath);
                IStream xbfDumpStream = new DumpStreamWrapper(_xbfFilePath + ".out");
                int errorCode;

                try
                {
                    int result = NativeMethods.Dump(xbfStream, xbfDumpStream, 0, out errorCode);
                }
                catch (Exception)
                {
                    Console.WriteLine("Fatal exception!");
                }
            }
        }

        string _xbfFilePath = string.Empty;
        bool _initialized = false;
    }
}
