// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Ingestion;

public sealed record EtlReadResult(long EventsDelivered, int EventsLost, TimeSpan SessionDuration);

// A process with XAML events in a trace. Name is its executable's name without .exe, from the trace's kernel process
// events; null when the trace has none, as in a trace recorded with logman. The times are the trace times of its first
// and last event.
public sealed record TraceProcess(int ProcessId, string? Name, long XamlEvents, long ProfilerEvents, long GpuMemoryEvents,
    double FirstMilliseconds, double LastMilliseconds)
{
    public long Events => XamlEvents + ProfilerEvents + GpuMemoryEvents;
}

public sealed record TraceContents(IReadOnlyList<TraceProcess> Processes, int EventsLost, TimeSpan Duration);

// Reads a recorded .etl through the same subscription a live session will use.
public static class OfflineEtlSource
{
    // The processes with XAML events, most events first, as the CLI's processes command lists them. Reads the whole
    // trace without copying any event's fields.
    public static TraceContents ListProcesses(string path, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(path);
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("The .etl file was not found.", path);
        }

        cancellationToken.ThrowIfCancellationRequested();
        using var source = new ETWTraceEventSource(path);
        // A process ID can be reused within a trace, so a process takes the name its ID had at its first event.
        var names = new Dictionary<int, string>();
        void OnProcess(ProcessTraceData data)
        {
            if (ExecutableName(data) is { } name)
            {
                names[data.ProcessID] = name;
            }
        }

        source.Kernel.ProcessStartGroup += OnProcess;
        source.Kernel.ProcessEndGroup += OnProcess;
        var tallies = new Dictionary<int, ProcessTally>();
        var order = new List<ProcessTally>();
        XamlEventSubscription.Match(source, null, (data, provider, processId) =>
        {
            var time = data.TimeStampRelativeMSec;
            if (!tallies.TryGetValue(processId, out var tally))
            {
                tallies[processId] = tally = new(processId, names.GetValueOrDefault(processId), time);
                order.Add(tally);
            }

            tally.Count(provider, time);
        });
        if (cancellationToken.CanBeCanceled)
        {
            // Process clears a StopProcessing call made before it starts, so the token is checked as events are read.
            source.AllEvents += _ =>
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    source.StopProcessing();
                }
            };
        }

        source.Process();
        cancellationToken.ThrowIfCancellationRequested();
        // Every process that uses the GPU has GPU memory events; only those with XAML events are listed. The sort is
        // stable, so processes with as many events stay in the order they first appear.
        var processes = order
            .Where(tally => tally.XamlEvents + tally.ProfilerEvents > 0)
            .Select(tally => new TraceProcess(tally.ProcessId, tally.Name ?? names.GetValueOrDefault(tally.ProcessId),
                tally.XamlEvents, tally.ProfilerEvents, tally.GpuMemoryEvents, tally.FirstMilliseconds, tally.LastMilliseconds))
            .OrderByDescending(process => process.Events)
            .ToList();
        return new(processes, source.EventsLost, source.SessionDuration);
    }

    private static string? ExecutableName(ProcessTraceData data)
    {
        var name = Path.GetFileNameWithoutExtension(data.ImageFileName);
        return string.IsNullOrEmpty(name) ? null : name;
    }

    // A null processId reads every process.
    public static EtlReadResult Read(string path, int? processId, Action<ProfilerEvent> sink,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(path);
        ArgumentNullException.ThrowIfNull(sink);
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("The .etl file was not found.", path);
        }

        using var source = new ETWTraceEventSource(path);
        long delivered = 0;
        XamlEventSubscription.Subscribe(source, processId, profilerEvent =>
        {
            delivered++;
            sink(profilerEvent);
        });
        using (cancellationToken.Register(source.StopProcessing))
        {
            source.Process();
        }

        cancellationToken.ThrowIfCancellationRequested();
        return new(delivered, source.EventsLost, source.SessionDuration);
    }

    private sealed class ProcessTally(int processId, string? name, double firstMilliseconds)
    {
        public int ProcessId { get; } = processId;
        public string? Name { get; } = name;
        public double FirstMilliseconds { get; private set; } = firstMilliseconds;
        public double LastMilliseconds { get; private set; } = firstMilliseconds;
        public long XamlEvents { get; private set; }
        public long ProfilerEvents { get; private set; }
        public long GpuMemoryEvents { get; private set; }

        // Grouped as EventCounter groups them.
        public void Count(XamlProvider provider, double milliseconds)
        {
            switch (provider)
            {
                case XamlProvider.XamlProfiler:
                    ProfilerEvents++;
                    break;
                case XamlProvider.DxgKrnl:
                    GpuMemoryEvents++;
                    break;
                default:
                    XamlEvents++;
                    break;
            }

            FirstMilliseconds = Math.Min(FirstMilliseconds, milliseconds);
            LastMilliseconds = Math.Max(LastMilliseconds, milliseconds);
        }
    }
}
