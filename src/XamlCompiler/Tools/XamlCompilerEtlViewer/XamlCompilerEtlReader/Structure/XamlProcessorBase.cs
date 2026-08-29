// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Structure
{
    public class XamlPhase
    {
        public String Name { get; set; }
        public Double StartTime { get; set; }
        public Double EndTime { get; set; }

        public Double ElapsedTime { get { return EndTime - StartTime; } }
    }

    public abstract class XamlProcessorBase: XamlPhase
    {
        internal abstract void ProcessCompilerEvent(XamlCompilerEvent ev);
    }

}
