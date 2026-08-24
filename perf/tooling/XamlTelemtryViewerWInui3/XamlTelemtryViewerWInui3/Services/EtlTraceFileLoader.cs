using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Windows.EventTracing;
using Microsoft.Windows.EventTracing.Events;
using Microsoft.Windows.EventTracing.Processes;
using XamlTelemtryViewerWInui3.Models;

namespace XamlTelemtryViewerWInui3.Services;

public sealed class EtlTraceFileLoader
{
    public Task<TraceData> LoadAsync(string path, CancellationToken cancellationToken)
    {
        return Task.Run(() =>
        {
            if (string.IsNullOrWhiteSpace(path))
            {
                throw new ArgumentException("Trace file path is required.", nameof(path));
            }

            if (!File.Exists(path))
            {
                throw new FileNotFoundException("Trace file not found.", path);
            }

            var settings = new TraceProcessorSettings
            {
                AllowLostEvents = true,
                AllowTimeInversion = true,
                SuppressFirstTimeSetupMessage = true,
            };

            using var trace = TraceProcessor.Create(path, settings);
            var pendingEvents = trace.UseGenericEvents();
            var pendingProcesses = trace.UseProcesses();

            try
            {
                trace.Process();
            }
            catch (InvalidOperationException ex)
            {
                throw new InvalidOperationException("Failed to process trace file. The file may be corrupted or in an unsupported format.", ex);
            }

            if (cancellationToken.IsCancellationRequested)
            {
                return TraceData.Empty;
            }

            IReadOnlyList<IGenericEvent> sourceEvents;
            IReadOnlyList<IProcess> sourceProcesses;
            try
            {
                sourceEvents = pendingEvents.Result?.Events ?? [];
                sourceProcesses = pendingProcesses.Result?.Processes ?? [];
            }
            catch (InvalidOperationException ex)
            {
                throw new InvalidOperationException("Failed to retrieve trace data results.", ex);
            }

            var events = new List<TelemetryEvent>(capacity: Math.Max(sourceEvents.Count, 4096));
            
            
            foreach (var ev in sourceEvents)
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                try
                {
                    events.Add(ToTelemetryEvent(ev));
                }
                catch
                {
                    // Skip events that can't be parsed
                }
            }
            

            events.Sort((a, b) => a.Timestamp.CompareTo(b.Timestamp));

            var eventsByPid = events
                .GroupBy(e => e.ProcessId)
                .ToDictionary(g => g.Key, g => g.Count());

            var processes = BuildProcessList(sourceProcesses, eventsByPid);

            var traceStart = events.Count > 0 ? events[0].Timestamp : DateTime.MinValue;
            var traceEnd = events.Count > 0 ? events[^1].Timestamp : DateTime.MinValue;

            return new TraceData
            {
                Label = Path.GetFileName(path),
                FilePath = path,
                Events = events,
                Processes = processes,
                TraceStart = traceStart,
                TraceEnd = traceEnd,
            };
        }, cancellationToken);
    }

    private static IReadOnlyList<ProcessInfo> BuildProcessList(
        IReadOnlyList<IProcess> processes,
        IReadOnlyDictionary<int, int> eventCountByPid)
    {
        var result = new List<ProcessInfo>(processes.Count);
        foreach (var p in processes)
        {
            eventCountByPid.TryGetValue(p.Id, out var eventCount);
            result.Add(new ProcessInfo(
                Id: p.Id,
                Name: p.ImageName ?? string.Empty,
                ImagePath: p.Images.Count > 0 ? p.Images[0].Path : null,
                CommandLine: p.CommandLine,
                CreateTime: p.CreateTime?.DateTimeOffset.LocalDateTime,
                ExitTime: p.ExitTime?.DateTimeOffset.LocalDateTime,
                EventCount: eventCount));
        }

        return result
            .OrderByDescending(p => p.EventCount)
            .ThenBy(p => p.Name, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static TelemetryEvent ToTelemetryEvent(IGenericEvent ev)
    {
        // Extract EventGuid and ActivityGuid from payload
        var (eventGuid, activityGuid) = ExtractGuidsFromEvent(ev);

        return new TelemetryEvent
        {
            Timestamp = ev.Timestamp.DateTimeOffset.LocalDateTime,
            ProcessName = ev.Process?.ImageName ?? string.Empty,
            ProviderName = ResolveProviderName(ev),
            ProviderGuid = ev.ProviderId,
            EventName = ResolveEventName(ev),
            Level = ev.Level.ToString(),
            Keywords = "0x" + ((ulong)ev.Keyword).ToString("X"),
            Opcode = (int)ev.Opcode,
            ProcessId = ev.ProcessId,
            ThreadId = ev.ThreadId,
            EventId = ev.Id,
            EventGuid = eventGuid,
            ActivityGuid = activityGuid,
            Fields = BuildFields(ev),
        };
    }

    private static (Guid EventGuid, Guid ActivityGuid) ExtractGuidsFromEvent(IGenericEvent ev)
    {
        Guid eventGuid = Guid.Empty;
        Guid activityGuid = Guid.Empty;

        try
        {
            // Try to find EventGuid and ActivityGuid fields in the event payload
            if (ev.Fields != null)
            {
                foreach (var field in ev.Fields)
                {
                    try
                    {
                        if (field.Name.Equals("EventGuid", StringComparison.OrdinalIgnoreCase) ||
                            field.Name.Equals("Guid", StringComparison.OrdinalIgnoreCase))
                        {
                            eventGuid = field.AsGuid;
                        }
                        else if (field.Name.Equals("ActivityId", StringComparison.OrdinalIgnoreCase) ||
                                 field.Name.Equals("Activity", StringComparison.OrdinalIgnoreCase) ||
                                 field.Name.Equals("ActivityGuid", StringComparison.OrdinalIgnoreCase))
                        {
                            activityGuid = field.AsGuid;
                        }
                    }
                    catch { }
                }
            }
        }
        catch { }

      

        return (eventGuid, activityGuid);
    }

    private static string ResolveProviderName(IGenericEvent ev)
    {
        if (!string.IsNullOrWhiteSpace(ev.ProviderName))
        {
            return ev.ProviderName;
        }

        return ev.ProviderId.ToString();
    }

    private static string ResolveEventName(IGenericEvent ev)
    {
        var task = ev.TaskName;
        var opcode = ev.OpcodeName;
        if (!string.IsNullOrWhiteSpace(task) || !string.IsNullOrWhiteSpace(opcode))
        {
            if (string.IsNullOrWhiteSpace(task))
            {
                return opcode!;
            }

            if (string.IsNullOrWhiteSpace(opcode))
            {
                return task!;
            }

            return $"{task}/{opcode}";
        }

        return $"EventId({ev.Id})";
    }

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
            catch (Exception)
            {
                // Silently skip field formatting errors
            }
        }

        return map;
    }

    private static string FormatFieldValue(IGenericEventField field)
    {
        // Select the accessor based on the field's declared type instead of probing
        // each As* accessor and catching InvalidOperationException. Exceptions are
        // expensive, and most fields are non-string, so the old approach paid that
        // cost on nearly every field.
        return field.Type switch
        {
            GenericEventFieldType.String => field.AsString,
            GenericEventFieldType.Boolean => field.AsBoolean.ToString(),
            GenericEventFieldType.Char => field.AsChar.ToString(),
            GenericEventFieldType.Byte => field.AsByte.ToString(),
            GenericEventFieldType.SByte => field.AsSByte.ToString(),
            GenericEventFieldType.Int16 => field.AsInt16.ToString(),
            GenericEventFieldType.UInt16 => field.AsUInt16.ToString(),
            GenericEventFieldType.Int32 => field.AsInt32.ToString(),
            GenericEventFieldType.UInt32 => field.AsUInt32.ToString(),
            GenericEventFieldType.Int64 => field.AsInt64.ToString(),
            GenericEventFieldType.UInt64 => field.AsUInt64.ToString(),
            GenericEventFieldType.Single => field.AsSingle.ToString(),
            GenericEventFieldType.Double => field.AsDouble.ToString(),
            GenericEventFieldType.Guid => field.AsGuid.ToString(),
            GenericEventFieldType.DateTime => field.AsDateTime.ToString(),
            GenericEventFieldType.TimeSpan => field.AsTimeSpan.ToString(),
            GenericEventFieldType.Address => field.AsAddress.ToString(),
            GenericEventFieldType.IPAddress => field.AsIPAddress.ToString(),
            GenericEventFieldType.SecurityIdentifier => field.AsSecurityIdentifier.ToString(),
            GenericEventFieldType.SocketAddress => field.AsSocketAddress.ToString(),
            GenericEventFieldType.Null => string.Empty,
            _ => "<binary>",
        };
    }
}

