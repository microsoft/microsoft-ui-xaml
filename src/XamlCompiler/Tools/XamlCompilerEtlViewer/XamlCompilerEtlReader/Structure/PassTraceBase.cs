// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Structure
{
    public class PassTraceBase : XamlProcessorBase
    {
        List<Object> _pageTraces = new List<Object>();
        XamlPageTrace _currentPage;

        XamlPhase _initTypeMap = new XamlPhase() { Name = "InitTypeMap" };
        XamlPhase _searchIxmpAndBindable = new XamlPhase { Name = "SearchIxmpAndBindable" };
        XamlPhase _generateTypeInfo = new XamlPhase { Name = "GenerateTypeInfo" };
        XamlPhase _writeTypeinfoFilesToDisk = new XamlPhase { Name = "WriteTypeinfoFilesToDisk" };
        XamlPhase _generateXBF = new XamlPhase { Name = "GenerateXBF" };
        XamlPhase _generateSdkXBF = new XamlPhase { Name = "GenerateSdkXBF" };

        public List<Object> PageTraces { get { return _pageTraces; } }

        protected bool ProcessPageEvents(XamlCompilerEvent ev)
        {
            switch (ev.XamlCompilerEventId)
            {
                case XamlCompilerEventId.PageStart:
                    _currentPage = new XamlPageTrace(ev, true);
                    _currentPage.ProcessCompilerEvent(ev);
                    return true;

                case XamlCompilerEventId.PageDone:
                    _currentPage.ProcessCompilerEvent(ev);
                    PageTraces.Add(_currentPage);
                    _currentPage = null;
                    return true;

                case XamlCompilerEventId.InitializeTypeNameMapStart:
                    _initTypeMap.StartTime = ev.TimeMSecs;
                    return true;

                case XamlCompilerEventId.InitializeTypeNameMapEnd:
                    _initTypeMap.EndTime = ev.TimeMSecs;
                    PageTraces.Add(_initTypeMap);
                    return true;

                case XamlCompilerEventId.SearchIxmpAndBindableStart:
                    _searchIxmpAndBindable.StartTime = ev.TimeMSecs;
                    return true;

                case XamlCompilerEventId.SearchIxmpAndBindableEnd:
                    _searchIxmpAndBindable.EndTime = ev.TimeMSecs;
                    PageTraces.Add(_searchIxmpAndBindable);
                    return true;

                case XamlCompilerEventId.GenerateTypeInfoStart:
                    _generateTypeInfo.StartTime = ev.TimeMSecs;
                    return true;

                case XamlCompilerEventId.GenerateTypeInfoEnd:
                    _generateTypeInfo.EndTime = ev.TimeMSecs;
                    PageTraces.Add(_generateTypeInfo);
                    return true;

                case XamlCompilerEventId.WriteTypeinfoFilesToDiskStart:
                    _writeTypeinfoFilesToDisk.StartTime = ev.TimeMSecs;
                    return true;

                case XamlCompilerEventId.WriteTypeinfoFilesToDiskEnd:
                    _writeTypeinfoFilesToDisk.EndTime = ev.TimeMSecs;
                    PageTraces.Add(_writeTypeinfoFilesToDisk);
                    return true;

                case XamlCompilerEventId.GenerateXBFStart:
                    _generateXBF.StartTime = ev.TimeMSecs;
                    return true;

                case XamlCompilerEventId.GenerateXBFEnd:
                    _generateXBF.EndTime = ev.TimeMSecs;
                    PageTraces.Add(_generateXBF);
                    return true;

                case XamlCompilerEventId.GenerateSdkXBFStart:
                    _generateSdkXBF.StartTime = ev.TimeMSecs;
                    return true;

                case XamlCompilerEventId.GenerateSdkXBFEnd:
                    _generateSdkXBF.EndTime = ev.TimeMSecs;
                    PageTraces.Add(_generateSdkXBF);
                    return true;
                
                default:
                    if (_currentPage != null)
                    {
                        _currentPage.ProcessCompilerEvent(ev);
                        return true;
                    }
                    return false;
            }
        }

        internal override void ProcessCompilerEvent(XamlCompilerEvent ev)
        {
            throw new NotImplementedException();
        }
    }
}
