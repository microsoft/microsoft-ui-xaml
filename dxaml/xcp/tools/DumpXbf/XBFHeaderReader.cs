// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using DumpXBF.FileFormat.Metadata;
using DumpXBF.FileFormat.NodeStream;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DumpXBF
{
    class XBFHeaderReader
    {
        public XBFHeaderReader(string filePath)
        {
            FileInfo f = new FileInfo(filePath);
            br = new BinaryReader(f.OpenRead(), Encoding.Unicode);
            if (!ReadHeader())
            {
                Console.WriteLine("ERROR: Invalid Header!");
                return;
            }
        }

        private bool ReadHeader()
        {
            Header xbfHeader = new Header();
            xbfHeader.magicNumber = br.ReadBytes(4);

            if (xbfHeader.magicNumber[0] != 0x58 && xbfHeader.magicNumber[1] != 0x42 && xbfHeader.magicNumber[2] != 0x46 && xbfHeader.magicNumber[3] == 0x00)
            {
                Console.WriteLine("ERROR: Invalid File Format Magic Number!");
                return false;
            }

            xbfHeader.metadataSize = br.ReadUInt32();
            xbfHeader.nodeSize = br.ReadUInt32();
            xbfHeader.majorFileVersion = br.ReadUInt32();
            xbfHeader.minorFileVersion = br.ReadUInt32();

            fileHeader = xbfHeader;
            return true;
        }

        public BinaryReader br;
        public Header fileHeader;
    }
}