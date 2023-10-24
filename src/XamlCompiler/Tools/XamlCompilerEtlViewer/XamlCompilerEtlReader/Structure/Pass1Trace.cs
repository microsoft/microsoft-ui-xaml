// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Structure
{
    public class Pass1Trace : PassTraceBase
    {
        public Pass1Trace(XamlCompilerEvent ev)
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
                case XamlCompilerEventId.StartPass1:
                    StartTime = ev.TimeMSecs;
                    break;

                case XamlCompilerEventId.EndPass1:
                    EndTime = ev.TimeMSecs;
                    break;

                default:
                    // Add the extra event to the Page Trace list.
                    // So everything is all in one place.
                    PageTraces.Add(ev);
                    break;
            }
        }
    }
}
