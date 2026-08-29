// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Timers;
using System.Windows.Threading;
using XamlCompilerEtlReader;
using XamlCompilerEtlReader.Structure;

namespace XamlCompilerEtlViewer
{
    public class PerfDataContext : INotifyPropertyChanged
    {
        String _eventListText = String.Empty;
        List<CollectedEventBase> _eventList = new List<CollectedEventBase>();
        XamlCompilerTraceEventReader _traceEventListener;
        XamlProjectTotals _xamlProjects;
        DispatcherTimer _dispatcherTimer;

        public event PropertyChangedEventHandler PropertyChanged;

        public PerfDataContext()
        {
            _traceEventListener = new XamlCompilerTraceEventReader();
            _traceEventListener.HandleCodeMarker += CodeMarkerHandler;
            _xamlProjects = new XamlProjectTotals();
            XamlCompiles = new ObservableCollection<XamlProjectTrace>();

            _dispatcherTimer = new DispatcherTimer();
            _dispatcherTimer.Interval = new TimeSpan(0, 0, 0, 0, 500);
            _dispatcherTimer.Tick += CollectProjectsBuildOnBackgroundThread;
            _dispatcherTimer.Start();
        }

        void CollectProjectsBuildOnBackgroundThread(object sender, EventArgs e)
        {
            List<XamlProjectTrace> projects = _xamlProjects.RemoveAllProjects();
            foreach (XamlProjectTrace proj in projects)
            {
                string nameSuffix = ((proj.Pass1 != null) ? "1" : String.Empty) + ((proj.Pass2 != null) ? "/2" : String.Empty);
                proj.Name += proj.ProjectName + " (" + nameSuffix + ")";
                XamlCompiles.Add(proj);
            }
        }

        public ObservableCollection<XamlProjectTrace> XamlCompiles { get; private set; }

        private void CodeMarkerHandler(XamlCompilerEtlReader.CollectedEventBase ev)
        {

            var cmEv = (XamlCompilerEtlReader.XamlCompilerEvent)ev;
            if (cmEv.IsInteresting)
            {
                AddEvent(cmEv);
                _xamlProjects.ProcessXamlCompilerEvent(cmEv);
            }
        }

        private void AddEvent(CollectedEventBase ev)
        {
            _eventList.Add(ev);
            AddToEventListText(ev);
            NotifyPropertyChanged("EventListText");
        }

        internal void ClearEvents()
        {
            _eventList.Clear();
            _eventListText = String.Empty;
            NotifyPropertyChanged("EventListText");
        }

        public void Dispose()
        {
            if (_traceEventListener != null)
            {
                _traceEventListener.Dispose();
            }
        }

        public String EventListText
        {
            get
            {
                return _eventListText;
            }
        }

        private void AddToEventListText(CollectedEventBase ev)
        {
            String eventString = ev.ToString();
            _eventListText += eventString + "\r\n";
        }

        private void NotifyPropertyChanged(string propName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propName));
            }
        }
    }
}
