// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Diagnostics.Tracing;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XamlCompilerEtlReader.Core;

namespace XamlCompilerEtlReader
{
    public class XamlCompilerTraceEventReader: IDisposable
    {
        private TraceEventListener _evListener; 

        public XamlCompilerTraceEventReader()
        {
            _evListener = new TraceEventListener();
            _evListener.RegisterWithProvider(ETWProviders.CodeMarkersProviderGuid, this.VSHandleEvent);
        }

        public delegate void CollectedEventHandler(CollectedEventBase ev);

        public event CollectedEventHandler HandleCodeMarker;

        private void VSHandleEvent(TraceEvent tEv)
        {
            XamlCompilerEvent cmEv = new XamlCompilerEvent(tEv);
            if (HandleCodeMarker != null)
            {
                HandleCodeMarker(cmEv);
            }
        }

        public void Dispose()
        {
            //_evListener.UnregisterWithProvider(ETWProviders.XamlCompilerGuid, this.HandleEvent);
            //_evListener.UnregisterWithProvider(ETWProviders.CodeMarkersProviderGuid, this.HandleEvent);
            _evListener.Dispose();
        }
    }
}
