// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Structure
{
    public class PerPageTotals
    {
        public PerPageTotals()
        {
            Phases = new List<XamlPhase>();
        }

        public List<XamlPhase> Phases { get; private set; }
        public Double Total { get; private set; }

        public void TotalPhases()
        {
            Double total = 0.0;
            foreach (var phase in Phases)
            {
                total += phase.ElapsedTime;
            }
            Total = total;
        }
    }
}
