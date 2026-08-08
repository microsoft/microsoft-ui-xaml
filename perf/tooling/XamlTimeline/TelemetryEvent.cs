using System;
using System.Collections.Generic;
using System.Linq;

namespace XamlTimeline
{
    /// <summary>
    /// A single trace event projected from an ETL file. Shared by the WinUI viewer
    /// (where it backs the events grid and filters) and the WPA plugin (where it is
    /// paired into phases). Both apps' ETL loaders populate this same type; the
    /// plugin simply leaves <see cref="Keywords"/> empty.
    /// </summary>
    public sealed class TelemetryEvent
    {
        public DateTime Timestamp { get; set; }

        public string ProcessName { get; set; } = string.Empty;

        public string ProviderName { get; set; } = string.Empty;

        public Guid ProviderGuid { get; set; }

        public string EventName { get; set; } = string.Empty;

        public string Level { get; set; } = string.Empty;

        public string Keywords { get; set; } = string.Empty;

        public int Opcode { get; set; }

        public int ProcessId { get; set; }

        public int ThreadId { get; set; }

        public int EventId { get; set; }

        public Guid EventGuid { get; set; }

        public Guid ActivityGuid { get; set; }

        /// <summary>
        /// Parsed payload fields keyed by name (case-insensitive). This is the source of
        /// truth; <see cref="Payload"/> is just a formatted view of it. Region payload
        /// matching does a direct key lookup here instead of re-parsing a string.
        /// </summary>
        public Dictionary<string, string> Fields { get; set; }
            = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        /// <summary>Display string "name=value, name=value" built from <see cref="Fields"/>.</summary>
        public string PayloadText => Fields.Count == 0
            ? string.Empty
            : string.Join(", ", Fields.Select(kv => kv.Key + "=" + kv.Value));

        public string DisplayKey => $"{ProviderName} :: {EventName}";
    }
}
