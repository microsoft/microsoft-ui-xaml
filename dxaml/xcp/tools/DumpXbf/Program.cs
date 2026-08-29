// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DumpXBF
{
    class Program
    {
        static void Main(string[] args)
        {
            bool unicode = true;
            bool verbose = false;
            String inputFile = null;
            for (int i = 0; i < args.Length; ++i)
            {
                if (args[i].Equals("/v", StringComparison.InvariantCultureIgnoreCase) ||
                    args[i].Equals("/verbose", StringComparison.InvariantCultureIgnoreCase) ||
                    args[i].Equals("-v", StringComparison.InvariantCultureIgnoreCase) ||
                    args[i].Equals("-verbose", StringComparison.InvariantCultureIgnoreCase))
                {
                    verbose = true;
                }
                else if (args[i].Equals("/nounicode", StringComparison.InvariantCultureIgnoreCase) ||
                         args[i].Equals("-nounicode", StringComparison.InvariantCultureIgnoreCase))
                {
                    unicode = false;
                }
                else
                {
                    inputFile = args[i];
                }
            }

            if (inputFile == null)
            {
                Console.WriteLine("DumpXBF Usage:");
                Console.WriteLine("    DumpXBF.exe <XBF File> [/verbose] [/nounicode]");
                Console.WriteLine();
                Console.WriteLine(@"Eg: DumpXBF.exe MainPage.xbf");
                Console.WriteLine();

                return;
            }

            if (unicode)
            {
                Console.OutputEncoding = Encoding.Unicode;
            }

            XBFHeaderReader headerReader = new XBFHeaderReader(args[0]);
            if (headerReader.fileHeader.majorFileVersion < 2)
            {
                Console.WriteLine("xbf version 1 detected...");
                XBFReader reader = new XBFReader(inputFile, verbose);
                reader.Dump();
            }
            else
            {
                Console.WriteLine("xbf version 2 detected...");
                XBFReader2 reader = new XBFReader2(inputFile, verbose);
                reader.Dump();
            }
        }
    }
}
