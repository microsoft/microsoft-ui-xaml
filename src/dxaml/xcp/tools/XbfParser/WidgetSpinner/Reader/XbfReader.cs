// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Common;
using Microsoft.Xaml.WidgetSpinner.XBF;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Microsoft.Xaml.WidgetSpinner.Reader
{
    internal partial class XbfReader : BinaryReader
    {
        // The XBF magic number ('X' 'B' 'F' '\0')
        private static readonly byte[] s_xbfMagicNumber = { 0x58, 0x42, 0x46, 0x00 };

        internal XbfMetadata Metadata { get; private set; }

        internal XbfReader(Stream input) : base(input, Encoding.Unicode)
        {
        }

        internal new int Read7BitEncodedInt()
        {
            return base.Read7BitEncodedInt();
        }

        internal XbfFile DeserializeXbfFile()
        {
            using (var scopeGuard = new ScopeGuard(() => BaseStream.Seek(0, SeekOrigin.Begin)))
            {
                BaseStream.Seek(0, SeekOrigin.Begin);

                var magicNumber = ReadBytes(4);
                if (!magicNumber.SequenceEqual(s_xbfMagicNumber))
                {
                    throw new ArgumentException("Stream does not represent an XBF file (missing XBF magic number)");
                }

                var sectionSizes = ReadHeader();
                Metadata = ReadMetadata();
                var nodestreams = ReadNodestreams();

                return new XbfFile(Metadata, sectionSizes.Item1, nodestreams, sectionSizes.Item2);
            }
        }

        private Tuple<uint, uint> ReadHeader()
        {
            var metadataSize = ReadUInt32();
            var nodestreamSize = ReadUInt32();

            return new Tuple<uint, uint>(metadataSize, nodestreamSize);
        }

        private XbfMetadata ReadMetadata()
        {
            return XbfMetadata.Deserialize(this);
        }

        private List<XbfNodeStream> ReadNodestreams()
        {
            var substreamTable = ReadSubstreamTable();
            var nodeStreams = new List<XbfNodeStream>();

            var substreamsStart = BaseStream.Position;

            for (var index = 0; index < substreamTable.Count; index++)
            {
                if (substreamsStart + substreamTable[index].NodestreamOffset != BaseStream.Position)
                {
                    throw new InvalidDataException("Current stream position does not match expected position of nodestream beginning.");
                }
                var nodeStreamSize = substreamTable[index].LinestreamOffset - substreamTable[index].NodestreamOffset;
                var lineStreamSize = (index + 1 < substreamTable.Count)
                                     ? substreamTable[index + 1].NodestreamOffset - substreamTable[index].LinestreamOffset
                                     : BaseStream.Length - substreamTable[index].LinestreamOffset - substreamsStart;

                nodeStreams.Add(XbfNodeStream.Deserialize(this, nodeStreamSize, lineStreamSize));
            }

            return nodeStreams;
        }

        private List<SubstreamInfo> ReadSubstreamTable()
        {
            return ReadVector((r) =>
                {
                    var nodeStreamOffset = r.ReadUInt32();
                    var lineStreamOffset = r.ReadUInt32();

                    return new SubstreamInfo(nodeStreamOffset, lineStreamOffset);
                });
        }
    }
}