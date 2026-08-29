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
    public class XamlProjectTotals
    {
        List<XamlProjectTrace> _xamlCompiles = new List<XamlProjectTrace>();
        List<XamlCompilerEvent> _extraEvents = new List<XamlCompilerEvent>();
        XamlProjectTrace _currentTrace;
        String _currentProjectName;

        public void ProcessXamlCompilerEvent(XamlCompilerEvent ev)
        {
            switch (ev.XamlCompilerEventId)
            {
                case XamlCompilerEventId.StartPass1:
                    if (_currentProjectName != null)
                    {
                        _xamlCompiles.Add(_currentTrace);
                        _currentTrace = null;
                        _currentProjectName = null;
                    }
                    _currentTrace = new XamlProjectTrace(ev);
                    _currentProjectName = ev.ProjectName;
                    break;

                case XamlCompilerEventId.EndPass2:
                    Debug.Assert(_currentProjectName == ev.ProjectName);
                    _currentTrace.ProcessCompilerEvent(ev);
                    _xamlCompiles.Add(_currentTrace);
                    _currentTrace = null;
                    _currentProjectName = null;
                    break;

                default:
                    if (_currentTrace != null)
                    {
                        _currentTrace.ProcessCompilerEvent(ev);
                    }
                    else
                    {
                        _extraEvents.Add(ev);
                    }
                    break;
            }
        }

        public List<XamlProjectTrace> RemoveAllProjects()
        {
            // needs may need a lock, it is called x-thread.
            // on the other hand it is nearly attomic

            List<XamlProjectTrace> newList = new List<XamlProjectTrace>();
            List<XamlProjectTrace> oldList = _xamlCompiles;
            _xamlCompiles = newList;
            return oldList;
        }

    }
}
