// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Structure
{
    public class Pass2Trace : PassTraceBase
    {
        public Pass2Trace(XamlCompilerEvent ev)
        {
            ProcessCompilerEvent(ev);
        }

        internal override void ProcessCompilerEvent(XamlCompilerEvent ev)
        {
            if (ProcessPageEvents(ev))
            {
                return;
            }

            switch (ev.XamlCompilerEventId)
            {
                case XamlCompilerEventId.StartPass2:
                    StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.EndPass2:
                    EndTime = ev.TimeMSecs;
                    break;



                default:
                    PageTraces.Add(ev);
                    break;
            }
        }
    }
}
