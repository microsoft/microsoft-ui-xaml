// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Diagnostics.Tracing;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Core
{
    /// <summary>
    /// Handles registering and routing of the traces we are interested in listening to
    /// </summary>
    public class TraceSourceProcessor : IDisposable
    {
        /// <summary>
        /// EventSource
        /// </summary>
        public ETWTraceEventSource EventSource { get; private set; }

        /// <summary>
        /// genericParsers
        /// </summary>
        private Dictionary<GenericTraceEventParser, List<Action<TraceEvent>>> genericParsers = new Dictionary<GenericTraceEventParser, List<Action<TraceEvent>>>();

        /// <summary>
        /// lockObject
        /// </summary>
        private object lockObject = new object();

        /// <summary>
        /// Ctor
        /// </summary>
        /// <param name="source">The trace source</param>
        /// <param name="postProcessing">Should we do any processing when we stop processing</param>
        public TraceSourceProcessor(ETWTraceEventSource source)
        {
            if (source == null)
            {
                throw new ArgumentNullException("source");
            }

            this.EventSource = source;

            this.RegisterSourceEvents(source);
        }

        private void RegisterSourceEvents(ETWTraceEventSource source)
        {
            this.RegisterSourceEvents(source,
                                        ETWProviders.XamlCompilerGuid,
                                        ETWProviders.CodeMarkersProviderGuid
                                        );
        }

        /// <summary>
        /// Register the parsers for the providers you are listening to
        /// </summary>
        /// <param name="source"></param>
        private void RegisterSourceEvents(ETWTraceEventSource source, params Guid[] providers)
        {
            lock (this.lockObject)
            {
                foreach (Guid guid in providers)
                {
                    if (genericParsers.Any((existing) => existing.Key.ProviderGuid.Equals(guid)))
                    {
                        continue;
                    }

                    GenericTraceEventParser parser = new GenericTraceEventParser(source, guid);
                    genericParsers.Add(parser, new List<Action<TraceEvent>>());
                }
            }
        }

        public void RegisterCallbackForProvider(Guid etwProvider, Action<TraceEvent> callback)
        {
            lock (this.lockObject)
            {
                foreach (KeyValuePair<GenericTraceEventParser, List<Action<TraceEvent>>> parser in this.genericParsers)
                {
                    // TODO: If the parser is not found should we add it?
                    if (parser.Key.ProviderGuid == etwProvider)
                    {
                        parser.Key.All += callback;
                        parser.Value.Add(callback);
                        break;
                    }
                }
            }
        }

        public void UnregisterCallbackForProvider(Guid etwProvider, Action<TraceEvent> callback)
        {
            lock (this.lockObject)
            {
                foreach (KeyValuePair<GenericTraceEventParser, List<Action<TraceEvent>>> parser in this.genericParsers)
                {
                    // TODO: If the parser is not found should we add it?
                    if (parser.Key.ProviderGuid == etwProvider)
                    {
                        parser.Key.All -= callback;
                        parser.Value.Remove(callback);
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Start the event tracing
        /// </summary>
        public void StartProcess()
        {
            this.EventSource.Process();
        }

        #region IDisposible pattern
        /// <summary>
        /// Dispose
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
        }

        /// <summary>
        /// Dispose
        /// </summary>
        /// <param name="disposing">is disposing</param>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                lock (this.lockObject)
                {
                    foreach (KeyValuePair<GenericTraceEventParser, List<Action<TraceEvent>>> pair in genericParsers)
                    {
                        foreach (Action<TraceEvent> callback in pair.Value)
                        {
                            pair.Key.All -= callback;
                        }
                    }
                }
            }
        }
        #endregion
    }
}
