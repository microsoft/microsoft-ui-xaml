using Microsoft.Windows.EventTracing;
using Microsoft.Windows.EventTracing.Events;
using System;
using System.Collections.Generic;
using AppBootingVisualizerPlugin.Models;
using XamlTimeline;

namespace AppBootingVisualizerPlugin.Services
{
    /// <summary>
    /// Loads an ETL file using <c>Microsoft.Windows.EventTracing</c> (TDH-based,
    /// the same library the WinUI viewer app uses) and projects every
    /// <see cref="IGenericEvent"/> to a <see cref="TelemetryEvent"/>.
    ///
    /// Provider name, event name, payload formatting, and numeric fallbacks are
    /// line-for-line ports of <c>XamlTelemtryViewerWInui3/Services/EtlTraceFileLoader.cs</c>
    /// so the WPA plugin and the WinUI viewer render identical values.
    /// </summary>
    public sealed class EtlEventLoader
    {
        public IReadOnlyList<TelemetryEvent> Load(string etlPath)
        {
            if (string.IsNullOrWhiteSpace(etlPath))
            {
                throw new ArgumentException("ETL path is required.", "etlPath");
            }

            var settings = new TraceProcessorSettings
            {
                AllowLostEvents = true,
                AllowTimeInversion = true,
                SuppressFirstTimeSetupMessage = true,
            };

            var events = new List<TelemetryEvent>(capacity: 8192);

            using (var trace = TraceProcessor.Create(etlPath, settings))
            {
                var pendingEvents = trace.UseGenericEvents();
                trace.Process();

                IReadOnlyList<IGenericEvent> sourceEvents;
                try
                {
                    sourceEvents = pendingEvents.Result == null
                        ? (IReadOnlyList<IGenericEvent>)Array.Empty<IGenericEvent>()
                        : pendingEvents.Result.Events;
                }
                catch (InvalidOperationException)
                {
                    sourceEvents = Array.Empty<IGenericEvent>();
                }

                if (sourceEvents == null)
                {
                    return events;
                }

                foreach (var ev in sourceEvents)
                {
                    try
                    {
                        events.Add(ToBootEvent(ev));
                    }
                    catch
                    {
                        // Skip events that can't be parsed.
                    }
                }
            }

            // MWE returns events in trace order but we sort to be safe.
            events.Sort((a, b) => a.Timestamp.CompareTo(b.Timestamp));
            return events;
        }

        private static TelemetryEvent ToBootEvent(IGenericEvent ev)
        {
            return new TelemetryEvent
            {
                Timestamp = ev.Timestamp.DateTimeOffset.LocalDateTime,
                ProviderGuid = ev.ProviderId,
                ProcessName = ev.Process != null ? (ev.Process.ImageName ?? string.Empty) : string.Empty,
                ProcessId = ev.ProcessId,
                ProviderName = ResolveProviderName(ev),
                EventName = ResolveEventName(ev),
                EventId = ev.Id,
                Level = ev.Level.ToString(),
                Opcode = (int)ev.Opcode,
                ThreadId = ev.ThreadId,
                Fields = BuildFields(ev),
            };
        }

        // Matches XamlTelemtryViewerWInui3's EtlTraceFileLoader.ResolveProviderName:
        // use the friendly provider name if available, else fall back to the GUID.
        private static string ResolveProviderName(IGenericEvent ev)
        {
            if (!string.IsNullOrWhiteSpace(ev.ProviderName))
            {
                return ev.ProviderName;
            }
            return ev.ProviderId.ToString();
        }

        // Matches XamlTelemtryViewerWInui3's EtlTraceFileLoader.ResolveEventName:
        // prefer "Task/Opcode", fall back to whichever is non-empty, else "EventId(N)".
        private static string ResolveEventName(IGenericEvent ev)
        {
            var task = ev.TaskName;
            var opcode = ev.OpcodeName;

            if (!string.IsNullOrWhiteSpace(task) || !string.IsNullOrWhiteSpace(opcode))
            {
                if (string.IsNullOrWhiteSpace(task))
                {
                    return opcode;
                }
                if (string.IsNullOrWhiteSpace(opcode))
                {
                    return task;
                }
                return task + "/" + opcode;
            }

            return "EventId(" + ev.Id + ")";
        }

        // Mirrors XamlTelemtryViewerWInui3's BuildFields. Produces a name->value map;
        // TelemetryEvent.Payload formats it for display.
        private static Dictionary<string, string> BuildFields(IGenericEvent ev)
        {
            var fields = ev.Fields;
            var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            if (fields == null || fields.Count == 0)
            {
                return map;
            }

            foreach (var field in fields)
            {
                try
                {
                    map[field.Name] = FormatFieldValue(field);
                }
                catch
                {
                    // Skip fields that can't be formatted.
                }
            }

            return map;
        }

        private static string FormatFieldValue(IGenericEventField field)
        {
            // Select the accessor based on the field's declared type instead of probing
            // each As* accessor and catching InvalidOperationException. Exceptions are
            // expensive, and most fields are non-string, so the probing approach paid that
            // cost on nearly every field. Mirrors XamlTelemtryViewerWInui3's FormatFieldValue
            // (written as a classic switch here because the plugin targets C# 7.3).
            switch (field.Type)
            {
                case GenericEventFieldType.String: return field.AsString;
                case GenericEventFieldType.Boolean: return field.AsBoolean.ToString();
                case GenericEventFieldType.Char: return field.AsChar.ToString();
                case GenericEventFieldType.Byte: return field.AsByte.ToString();
                case GenericEventFieldType.SByte: return field.AsSByte.ToString();
                case GenericEventFieldType.Int16: return field.AsInt16.ToString();
                case GenericEventFieldType.UInt16: return field.AsUInt16.ToString();
                case GenericEventFieldType.Int32: return field.AsInt32.ToString();
                case GenericEventFieldType.UInt32: return field.AsUInt32.ToString();
                case GenericEventFieldType.Int64: return field.AsInt64.ToString();
                case GenericEventFieldType.UInt64: return field.AsUInt64.ToString();
                case GenericEventFieldType.Single: return field.AsSingle.ToString();
                case GenericEventFieldType.Double: return field.AsDouble.ToString();
                case GenericEventFieldType.Guid: return field.AsGuid.ToString();
                case GenericEventFieldType.DateTime: return field.AsDateTime.ToString();
                case GenericEventFieldType.TimeSpan: return field.AsTimeSpan.ToString();
                case GenericEventFieldType.Address: return field.AsAddress.ToString();
                case GenericEventFieldType.IPAddress: return field.AsIPAddress.ToString();
                case GenericEventFieldType.SecurityIdentifier: return field.AsSecurityIdentifier.ToString();
                case GenericEventFieldType.SocketAddress: return field.AsSocketAddress.ToString();
                case GenericEventFieldType.Null: return string.Empty;
                default: return "<binary>";
            }
        }
    }
}
