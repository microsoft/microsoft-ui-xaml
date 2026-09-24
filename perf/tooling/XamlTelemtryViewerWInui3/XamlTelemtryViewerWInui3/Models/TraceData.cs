using System;
using System.Collections.Generic;

namespace XamlTelemtryViewerWInui3.Models
{
    public sealed class TraceData
    {
        /// <summary>Short label (the trace file name) shown in the tab / timeline lane.</summary>
        public string Label { get; set; } = string.Empty;

        /// <summary>Full path of the trace file this data was loaded from.</summary>
        public string FilePath { get; set; } = string.Empty;

        public IReadOnlyList<TelemetryEvent> Events { get; set; } = Array.Empty<TelemetryEvent>();

        public IReadOnlyList<ProcessInfo> Processes { get; set; } = Array.Empty<ProcessInfo>();

        public DateTime TraceStart { get; set; }

        public DateTime TraceEnd { get; set; }

        public static TraceData Empty { get; } = new()
        {
            Events = Array.Empty<TelemetryEvent>(),
            Processes = Array.Empty<ProcessInfo>(),
            TraceStart = DateTime.MinValue,
            TraceEnd = DateTime.MinValue,
        };
    }
}
