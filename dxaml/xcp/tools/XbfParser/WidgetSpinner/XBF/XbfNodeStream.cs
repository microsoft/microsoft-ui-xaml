// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Reader;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    internal struct SubstreamInfo
    {
        internal long NodestreamOffset { get; }
        internal long LinestreamOffset { get; }

        internal SubstreamInfo(long nodestreamOffset, long linestreamOffset) : this()
        {
            NodestreamOffset = nodestreamOffset;
            LinestreamOffset = linestreamOffset;
        }
    }

    public class XbfNodeStream : IReadOnlyList<XbfNode>
    {
        internal XbfNodeStream()
        {
        }

        public IEnumerator<XbfNode> GetEnumerator()
        {
            return m_nodeStream.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return ((IEnumerable)m_nodeStream).GetEnumerator();
        }

        public int Count => m_nodeStream.Count;

        public XbfNode this[int index] => m_nodeStream[index];

        public int FindIndex(StreamOffsetToken token)
        {
            if (token.Equals(StreamOffsetToken.Default))
            {
                return 0;
            }

            var targetNodeStreamOffset = token.Offset + m_nodeStream[0].NodeStreamOffset;
            return m_nodeStream.BinarySearch(new XbfNode(XbfNodeType.None, targetNodeStreamOffset),
                Comparer<XbfNode>.Create((left, right) =>
                    Comparer<long>.Default.Compare(left.NodeStreamOffset, right.NodeStreamOffset)));
        }

        internal static XbfNodeStream Deserialize(XbfReader reader, long nodeStreamSize, long lineStreamSize)
        {
            var nodeStream = new XbfNodeStream();
            var nodeStreamStart = reader.BaseStream.Position;
            var lineStreamStart = nodeStreamStart + nodeStreamSize;

            var lineStream = ReadLineStream(reader, lineStreamStart, lineStreamSize);

            while (reader.BaseStream.Position < (nodeStreamStart + nodeStreamSize))
            {
                nodeStream.m_nodeStream.Add(XbfNode.Deserialize(reader, lineStream));
            }

            // Seek past the end of the lineStream
            reader.BaseStream.Seek(lineStreamSize, SeekOrigin.Current);

            return nodeStream;
        }

        private static List<Tuple<int, int, int>> ReadLineStream(XbfReader reader, long lineStreamStart, long lineStreamSize)
        {
            var lineStream = new List<Tuple<int, int, int>>();
            var originalPosition = reader.BaseStream.Position;
            reader.BaseStream.Seek(lineStreamStart, SeekOrigin.Begin);

            while (reader.BaseStream.Position < (lineStreamStart + lineStreamSize))
            {
                var deltaStreamOffset = reader.Read7BitEncodedInt();
                var deltaLineOffset = reader.Read7BitEncodedInt();
                var deltaPositionOffset = reader.Read7BitEncodedInt();

                lineStream.Add(new Tuple<int, int, int>(deltaStreamOffset, deltaLineOffset, deltaPositionOffset));
            }

            reader.BaseStream.Seek(originalPosition, SeekOrigin.Begin);

            return lineStream;
        }

        private readonly List<XbfNode> m_nodeStream = new List<XbfNode>();
    }
}