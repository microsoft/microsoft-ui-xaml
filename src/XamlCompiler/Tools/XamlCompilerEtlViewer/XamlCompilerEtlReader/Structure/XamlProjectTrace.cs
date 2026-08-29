// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Structure
{
    public class XamlProjectTrace: XamlProcessorBase
    {
        XamlProcessorBase _currentPass;

        public XamlProjectTrace(XamlCompilerEvent ev)
        {
            ProjectName = ev.ProjectName;
            Pass1 = new Pass1Trace(ev);
            Passes = new List<Object>();
            PageTotals = new PerPageTotals();
            Passes.Add(Pass1);
            _currentPass = Pass1;
        }

        public Pass1Trace Pass1 { get; private set; }
        public Pass2Trace Pass2 { get; private set; }
        public List<Object> Passes { get; private set; }
        public String ProjectName { get; private set; }
        public PerPageTotals PageTotals { get; private set; }

        public double TotalTime
        {
            get
            {
                double time = Pass1.ElapsedTime;
                if (Pass2 != null)
                {
                    time += Pass2.ElapsedTime;
                }
                return time;
            }
        }

        internal override void ProcessCompilerEvent(XamlCompilerEvent ev)
        {
            switch (ev.XamlCompilerEventId)
            {
                case XamlCompilerEventId.EndPass1:
                    Debug.Assert(ProjectName == ev.ProjectName);
                    _currentPass.ProcessCompilerEvent(ev);
                    TotalThePerPagePhases(Pass1);
                    ComputeUnaccountedTime(Pass1);
                    _currentPass = null;
                    break;

                case XamlCompilerEventId.StartPass2:
                    Debug.Assert(ProjectName == ev.ProjectName);
                    Pass2 = new Pass2Trace(ev);
                    Passes.Add(Pass2);
                    _currentPass = Pass2;
                    break;

                case XamlCompilerEventId.EndPass2:
                    Debug.Assert(ProjectName == ev.ProjectName);
                    _currentPass.ProcessCompilerEvent(ev);
                    TotalThePerPagePhases(Pass2);
                    ComputeUnaccountedTime(Pass2);
                    PrepareSummary();
                    _currentPass = null;
                    break;

                default:
                    if (_currentPass != null)
                    {
                        _currentPass.ProcessCompilerEvent(ev);
                    }
                    else
                    {
                        Passes.Add(ev);
                    }
                    break;
            }
        }

        private void PrepareSummary()
        {
            PageTotals.TotalPhases();
            Passes.Add(PageTotals);
        }

        private void TotalThePerPagePhases(PassTraceBase pass)
        {
            foreach (var obj in pass.PageTraces)
            {
                XamlPageTrace page = obj as XamlPageTrace;
                if (page == null)
                {
                    continue;
                }
                foreach (var pagePhase in page.Phases)
                {
                    XamlPhase phaseTotal = GetPhaseTotal(pagePhase.Name);
                    phaseTotal.EndTime += pagePhase.ElapsedTime;
                }
            }
        }

        private void ComputeUnaccountedTime(PassTraceBase pass)
        {
            double total = 0;

            foreach (var obj in pass.PageTraces)
            {
                XamlPhase phase = obj as XamlPhase;
                if (phase != null)
                {
                    total += phase.ElapsedTime;
                }
            }
            double missingTime = pass.ElapsedTime - total;
            XamlPhase missingTimePhase = new XamlPhase() { Name = "Unaccounted Time", EndTime = missingTime };
            pass.PageTraces.Add(missingTimePhase);
        }

        private XamlPhase GetPhaseTotal(string name)
        {
            foreach (var phaseTotal in PageTotals.Phases)
            {
                if (phaseTotal.Name == name)
                {
                    return phaseTotal;
                }
            }
            XamlPhase phase = new XamlPhase() { Name = name, StartTime = 0.0, EndTime = 0.0 };
            PageTotals.Phases.Add(phase);
            return phase;
        }
    }
}
