// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Diagnostics.Tracing;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader
{
    internal enum XamlCompilerEventId
    {
        Empty = 0,
        StartPass1 = 1,
        StartPass2 = 2,
        EndPass1 = 3,
        EndPass2 = 4,
        CreatingTypeUniverse = 5,
        ReleasingTypeUniverse = 6,
        FingerprintCheckFailed = 7,
        RestoredTypeInfoBackup = 8,
        PageSkipped = 9,
        PageStart = 10,
        PageDone = 11,
        PageLoadStart = 12,
        PageLoadEnd = 13,
        PageValidateStart = 14,
        PageValidateEnd = 15,
        PageEditStart = 16,
        PageEditEnd = 17,
        PageHarvestStart = 18,
        PageHarvestEnd = 19,
        PageCodeGenStart = 20,
        PageCodeGenEnd = 21,
        PageTypeCollectStart = 22,
        PageTypeCollectEnd = 23,

        PagePlaceHolder1 = 24,
        PagePlaceHolder2 = 25,
        PagePlaceHolder3 = 26,
        PagePlaceHolder4 = 27,
        PagePlaceHolder5 = 28,
        PagePlaceHolder6 = 29,

        WriteFilesToDiskStart = 30,
        WriteFilesToDiskEnd = 31,
        WriteTypeinfoFilesToDiskStart = 32,
        WriteTypeinfoFilesToDiskEnd = 33,
        GenerateTypeInfoStart = 34,
        GenerateTypeInfoEnd = 35,
        SearchIxmpAndBindableStart = 36,
        SearchIxmpAndBindableEnd = 37,
        InitializeTypeNameMapStart = 38,
        InitializeTypeNameMapEnd = 39,
        GenerateXBFStart = 40,
        GenerateXBFEnd = 41,
        GenerateSdkXBFStart = 42,
        GenerateSdkXBFEnd = 43,
        InspectLocalAsm = 44,
    }

    public class XamlCompilerEvent : CollectedEventBase
    {
        private struct EventData
        {
            public XamlCompilerEventId Id;
            public XamlCompilerEventArgId Arg;

            public EventData(XamlCompilerEventId id, XamlCompilerEventArgId arg)
            {
                Id = id;
                Arg = arg;
            }
        }

        private enum XamlCompilerEventArgId
        {
            None = 0,
            ProjectName = 1,
            FileName = 2,
            Reason = 3
        }

        private static EventData[] EventDescriptions =
        {
            new EventData(XamlCompilerEventId.Empty,                   XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.StartPass1,              XamlCompilerEventArgId.ProjectName),
            new EventData(XamlCompilerEventId.StartPass2,              XamlCompilerEventArgId.ProjectName),
            new EventData(XamlCompilerEventId.EndPass1,                XamlCompilerEventArgId.ProjectName),
            new EventData(XamlCompilerEventId.EndPass2,                XamlCompilerEventArgId.ProjectName),
            new EventData(XamlCompilerEventId.CreatingTypeUniverse,    XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.ReleasingTypeUniverse,   XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.FingerprintCheckFailed,  XamlCompilerEventArgId.Reason),
            new EventData(XamlCompilerEventId.RestoredTypeInfoBackup,  XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageSkipped,             XamlCompilerEventArgId.FileName),
            new EventData(XamlCompilerEventId.PageStart,              XamlCompilerEventArgId.FileName),
            new EventData(XamlCompilerEventId.PageDone,               XamlCompilerEventArgId.FileName),
            new EventData(XamlCompilerEventId.PageLoadStart,          XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageLoadEnd,            XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageValidateStart,      XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageValidateEnd,        XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageEditStart,          XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageEditEnd,            XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageHarvestStart,       XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageHarvestEnd,         XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageCodeGenStart,       XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageCodeGenEnd,         XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageTypeCollectStart,   XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PageTypeCollectEnd,     XamlCompilerEventArgId.None),

            new EventData(XamlCompilerEventId.PagePlaceHolder1,       XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PagePlaceHolder2,       XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PagePlaceHolder3,       XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PagePlaceHolder4,       XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PagePlaceHolder5,       XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.PagePlaceHolder6,       XamlCompilerEventArgId.None),

            new EventData(XamlCompilerEventId.WriteFilesToDiskStart,  XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.WriteFilesToDiskEnd,    XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.WriteTypeinfoFilesToDiskStart, XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.WriteTypeinfoFilesToDiskEnd, XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.GenerateTypeInfoStart,  XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.GenerateTypeInfoEnd,    XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.SearchIxmpAndBindableStart, XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.SearchIxmpAndBindableEnd, XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.InitializeTypeNameMapStart, XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.InitializeTypeNameMapEnd, XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.GenerateXBFStart,       XamlCompilerEventArgId.FileName),
            new EventData(XamlCompilerEventId.GenerateXBFEnd,         XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.GenerateSdkXBFStart,    XamlCompilerEventArgId.FileName),
            new EventData(XamlCompilerEventId.GenerateSdkXBFEnd,      XamlCompilerEventArgId.None),
            new EventData(XamlCompilerEventId.InspectLocalAsm,        XamlCompilerEventArgId.Reason),
        };
        private const int MaxEventID = 29;

        public XamlCompilerEvent(TraceEvent traceEvent)
            : base(traceEvent)
        {
            XamlCompilerEventId = (XamlCompilerEventId)ID;
            CollectStringData(traceEvent);
        }

        internal XamlCompilerEventId XamlCompilerEventId { get; private set; }
        public String Name { get { return XamlCompilerEventId.ToString(); } }
        public String StringData { get; private set; }

        public bool HasArgument { get { return Arg != XamlCompilerEventArgId.None; } }
        public String ArgumentName { get { return Arg.ToString(); } }

        public String ProjectName { get { return GetArg(XamlCompilerEventArgId.ProjectName); } }
        public String FileName { get { return GetArg(XamlCompilerEventArgId.FileName); } }
        public String Reason { get { return GetArg(XamlCompilerEventArgId.Reason); } }

        public override string ToString()
        {
            String result = String.Format("XC {0}  {1}({2})", TimeMSecs.ToString("N3"), Name, DataString);
            return result;
        }

        public String DataString
        {
            get
            {
                string dataString = StringData;
                if (HasArgument)
                {
                    dataString = ArgumentName + "=" + dataString;
                }
                return dataString;
            }
        }

        //  ----- Private ----- 

        private void CollectStringData(TraceEvent traceEvent)
        {
            int length = traceEvent.EventDataLength;
            StringData = String.Empty;
            if (length > 2)
            {
                string d = new System.Text.UnicodeEncoding().GetString(traceEvent.EventData(), 0, length - 2);
                StringData = d;
            }
        }

        private XamlCompilerEventArgId Arg
        {
            get
            {
                Debug.Assert(ID == (int)XamlCompilerEvent.EventDescriptions[ID].Id);
                return XamlCompilerEvent.EventDescriptions[ID].Arg;
            }
        }

        private string GetArg(XamlCompilerEventArgId arg)
        {
            if (Arg == arg)
            {
                return StringData;
            }
            throw new InvalidOperationException(String.Format("No {0}", arg.ToString()));
        }
    }
}
