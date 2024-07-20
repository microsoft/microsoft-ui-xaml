// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Structure
{
    public class XamlPageTrace : XamlProcessorBase
    {
        XamlPhase _load = new XamlPhase() { Name = "DomLoad" };
        XamlPhase _validation = new XamlPhase() { Name = "Validation" };
        XamlPhase _fileIo = new XamlPhase() { Name = "WriteFilesToDisk" };
        XamlPhase _edit = new XamlPhase() { Name = "EditXAML" };
        XamlPhase _harvest = new XamlPhase() { Name = "Harvest" };
        XamlPhase _typeCollection = new XamlPhase() { Name = "TypeCollection" };

        public XamlPageTrace(XamlCompilerEvent ev, bool isPass1)
        {
            IsPass1 = isPass1;
            Phases = new List<XamlPhase>();
        }

        public List<XamlPhase> Phases { get; private set; }

        public bool IsPass1 { get; private set; }
        public String FileName { get; private set; }
        public String FullPathName { get; private set; }

        internal override void ProcessCompilerEvent(XamlCompilerEvent ev)
        {
            switch (ev.XamlCompilerEventId)
            {
                case XamlCompilerEventId.PageStart:
                    FullPathName = ev.FileName;
                    FileName = Path.GetFileName(ev.FileName);
                    StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.PageDone:
                    EndTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.PageLoadStart:
                    _load.StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.PageLoadEnd:
                    _load.EndTime = ev.TimeMSecs;
                    Phases.Add(_load);
                    break;

                case XamlCompilerEventId.PageValidateStart:
                    _validation.StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.PageValidateEnd:
                    _validation.EndTime = ev.TimeMSecs;
                    Phases.Add(_validation);
                    break;

                case XamlCompilerEventId.WriteFilesToDiskStart:
                    _fileIo.StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.WriteFilesToDiskEnd:
                    _fileIo.EndTime = ev.TimeMSecs;
                    Phases.Add(_fileIo);
                    break;

                case XamlCompilerEventId.PageEditStart:
                    _edit.StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.PageEditEnd:
                    _edit.EndTime = ev.TimeMSecs;
                    Phases.Add(_edit);
                    break;

                case XamlCompilerEventId.PageHarvestStart:
                    _harvest.StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.PageHarvestEnd:
                    _harvest.EndTime = ev.TimeMSecs;
                    Phases.Add(_harvest);
                    break;

                case XamlCompilerEventId.PageTypeCollectStart:
                    _typeCollection.StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.PageTypeCollectEnd:
                    _typeCollection.EndTime = ev.TimeMSecs;
                    Phases.Add(_typeCollection);
                    break;
            }
        }
    }
}
