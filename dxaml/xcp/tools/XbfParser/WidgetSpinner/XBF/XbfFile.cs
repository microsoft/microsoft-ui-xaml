// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public class XbfFile
    {
        public XbfMetadata Metadata { get; }

        public long MetadataSize { get; }

        public List<XbfNodeStream> NodeStreams { get; }

        public long NodeStreamsSize { get; }

        public XbfFile(XbfMetadata metadata, long metadataSize, List<XbfNodeStream> nodeStreams, long nodeStreamsSize)
        {
            Metadata = metadata;
            MetadataSize = metadataSize;
            NodeStreams = nodeStreams;
            NodeStreamsSize = nodeStreamsSize;
        }
    }
}