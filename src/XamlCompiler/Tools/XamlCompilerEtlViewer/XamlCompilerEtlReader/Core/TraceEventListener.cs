// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Diagnostics.Tracing;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Core
{
    /// <summary>
    /// Listener for ETW events from various providers
    /// </summary>
    public class TraceEventListener : IDisposable
    {
        /// <summary>
        /// activeSession
        /// </summary>
        private TraceEventSession activeSession;

        /// <summary>
        /// ActiveSource
        /// </summary>
        private ETWTraceEventSource activeSource;

        /// <summary>
        /// Session name
        /// </summary>
        private string sessionName = "RealTimeETWListener";

        /// <summary>
        /// TraceProcessor
        /// </summary>
        private TraceSourceProcessor traceProcessor;

        /// <summary>
        /// Thread used to listen for ETW events
        /// </summary>
        private Thread listenerThread;

        /// <summary>
        /// Ctor
        /// </summary>
        public TraceEventListener()
        {
            this.Initialize();
        }

        /// <summary>
        /// Initialize the listener
        /// </summary>
        private void Initialize()
        {
            if (traceProcessor == null)
            {
                if (TraceEventSession.GetActiveSessionNames().Contains(this.sessionName))
                {
                    using (TraceEventSession tempSession = new TraceEventSession(this.sessionName, null))
                    {
                        tempSession.Stop(noThrow: false);
                    }
                }

                this.activeSession = new TraceEventSession(this.sessionName, null) { StopOnDispose = true };
                this.activeSource = new ETWTraceEventSource(this.sessionName, TraceEventSourceType.Session);

                this.traceProcessor = new TraceSourceProcessor(activeSource);
                this.activeSession.EnableProvider(ETWProviders.CodeMarkersProviderGuid, TraceEventLevel.Always);
                // activeSession.EnableProvider(ETWProviders.MeasurementBlockProviderGuid, TraceEventLevel.Always);
                this.activeSession.EnableProvider(ETWProviders.XamlCompilerGuid, TraceEventLevel.Always);

                this.listenerThread = new Thread((ThreadStart)delegate
                {
                    try
                    {
                        this.traceProcessor.StartProcess();
                    }
                    catch (Exception e)
                    {
                        Debug.WriteLine(e.ToString());
                    }
                    finally
                    {
                        this.Dispose();
                    }
                });

                this.listenerThread.Start();
            }
        }

        public void RegisterWithProvider(Guid providerGuid, Action<TraceEvent> callback)
        {
            this.traceProcessor.RegisterCallbackForProvider(providerGuid, callback);
        }

        public void UnregisterWithProvider(Guid providerGuid, Action<TraceEvent> callback)
        {
            this.traceProcessor.UnregisterCallbackForProvider(providerGuid, callback);
        }

        /// <summary>
        /// Dispose
        /// </summary>
        /// <param name="disposing">is disposing</param>
        public void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.traceProcessor != null)
                {
                    this.traceProcessor.Dispose();
                    this.traceProcessor = null;
                }

                if (this.activeSource != null)
                {
                    this.activeSource.Dispose();
                    this.activeSource = null;
                }

                if (this.activeSession != null)
                {
                    this.activeSession.Dispose();
                    this.activeSession = null;
                }
            }
        }

        /// <summary>
        /// Dispose
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }
    }
}
