// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.Xaml.WidgetSpinner.XBF;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace SampleXbfViewer
{
    public sealed class XbfFileViewModel : INotifyPropertyChanged
    {
        private readonly XbfFile m_xbfFile;

        public string NodeStreamSize => $"Total nodestream size: {m_xbfFile.NodeStreamsSize} bytes";

        public XbfMetadata Metadata => m_xbfFile.Metadata;

        public string MetadataSize => $"Total metadata size: {m_xbfFile.MetadataSize} bytes";

        public IReadOnlyList<XbfNodeStream> NodeStreams => m_xbfFile.NodeStreams;

        public XbfFileViewModel(XbfFile xbfFile)
        {
            this.m_xbfFile = xbfFile;

            OnPropertyChanged(nameof(MetadataSize));
            OnPropertyChanged(nameof(NodeStreamSize));
            OnPropertyChanged(nameof(NodeStreams));
            OnPropertyChanged(nameof(Metadata));
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}