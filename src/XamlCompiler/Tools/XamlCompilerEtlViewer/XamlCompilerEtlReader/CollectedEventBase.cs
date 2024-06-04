// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Diagnostics.Tracing;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader
{
    public class CollectedEventBase
    {
        private const int CodeMarkers_Value_start = 29600;
        private const int CodeMarkers_Value_end = 29800;
        public CollectedEventBase(TraceEvent traceEvent)
        {
            GetEventData(traceEvent);
        }

        protected int ID { get; private set; }
        public int ThreadId { get; private set; }
        public int ProcessId { get; private set; }
        public String ProcessName { get; private set; }
        public double TimeMSecs { get; private set; }
        public bool IsInteresting { get; private set; }

        private void GetEventData(TraceEvent ev)
        {
            ID = (int)ev.ID;
            if (ID >= CodeMarkers_Value_start && ID <= CodeMarkers_Value_end)
            {
                ID -= CodeMarkers_Value_start - 1;
                IsInteresting = true;
            }
            ThreadId = ev.ThreadID;
            ProcessId = ev.ProcessID;
            ProcessName = ev.ProcessName;
            TimeMSecs = ev.TimeStampRelativeMSec;
        }

        public override string ToString()
        {
            string dataString = String.Empty;
            String result = String.Format("?? {0}  {1}", TimeMSecs.ToString("N3"), ID);
            return result;
        }
    }
}
