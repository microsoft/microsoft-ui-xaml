// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Diagnostics.Tracing;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Security;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Core
{
    /// <summary>
    /// The parser that handles the events from Sparkle in the trace
    /// </summary>
    [SecuritySafeCritical]
    public sealed class GenericTraceEventParser : TraceEventParser
    {
        /// <summary>
        /// allCallback
        /// </summary>
        private Action<TraceEvent> allCallback;

        /// <summary>
        /// Guid of this parser
        /// </summary>
        public Guid ProviderGuid { get; private set; }

        /// <summary>
        /// Ctor
        /// </summary>
        /// <param name="source">source of the trace</param>
        public GenericTraceEventParser(TraceEventSource source, Guid etwProvider)
            : base(source)
        {
            this.ProviderGuid = etwProvider;

            // Happens during deserialization
            if (source == null)
            {
                return;
            }

            this.source.RegisterUnhandledEvent(delegate(TraceEvent data)
            {
                if (data.ProviderGuid == this.ProviderGuid &&
                    this.allCallback != null &&
                    data.Opcode != (TraceEventOpcode)254) // This is the manifest dump event
                {
                    this.allCallback(data);
                }

                return data;
            });
        }

        /// <summary>
        /// All
        /// </summary>
        public override event Action<TraceEvent> All
        {
            add
            {
                if (value != null)
                {
                    this.allCallback += value;
                }
            }
            remove
            {
                if (value != null)
                {
                    this.allCallback -= value;
                }
            }
        }
    }
}
