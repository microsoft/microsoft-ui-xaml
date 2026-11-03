// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Ingestion;

// Watches a trace for one process's launch: the kernel's event for its creation, which names the process that created
// it, its launcher, and the launcher's events from the launcher providers (MarkerProviderSettings.Launcher) that are part
// of the launch: within LauncherWindow of the creation, and nearer it than the creation of any other process the launcher
// created, as when File Explorer or a script launches several apps. A process ID can be reused within a trace, so the
// process's creation is the last one before its first event. The launcher is known for certain only then, so until then
// the launcher providers' events of every other process are kept: those since LauncherWindow before the creation once it
// was read, otherwise those of the last LauncherWindow, and at most MaxKept. A launcher's event after the creation waits
// until the launcher creates another process, or until the trace is far enough past it that a later creation couldn't be
// nearer, or until Complete. Only a trace with the kernel's process events has the creation, and so the launcher's events.
// Not thread-safe: TraceEvent calls it on the thread that reads the trace.
internal sealed class LaunchWatcher
{
    private const int MaxKept = 10_000;

    private readonly int processId;
    private readonly Action<TraceProcessStart>? started;
    private readonly Action<ProfilerEvent> deliver;
    private readonly double window = MarkerProviderSettings.LauncherWindow.TotalMilliseconds;
    // The executable name of each process ID, from its latest start.
    private readonly Dictionary<int, string> names = [];
    // When each process last created another, until the process's first event.
    private readonly Dictionary<int, double> lastCreated = [];
    private readonly Queue<ProfilerEvent> kept = new();
    // The launcher's events after the process's creation that its next creation may yet be nearer.
    private readonly List<ProfilerEvent> waiting = [];
    private TraceProcessStart? start;
    private double createdMilliseconds;
    // When the launcher created other processes, just before and after this one.
    private double? previousCreatedMilliseconds;
    private double? nextCreatedMilliseconds;
    private bool processSeen;

    // Started gets each creation of the process read before its first event; the last one is its creation. Deliver gets
    // the launcher's events that are part of the launch.
    public LaunchWatcher(ETWTraceEventSource source, int processId, Action<TraceProcessStart>? started, Action<ProfilerEvent> deliver)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(deliver);
        this.processId = processId;
        this.started = started;
        this.deliver = deliver;
        source.Kernel.ProcessStartGroup += OnProcessStart;
    }

    // Call after delivering each of the process's events, with its time. The first time, delivers the launcher's events
    // kept from LauncherWindow before the process's creation that are part of the launch. They are earlier than that
    // first event, but come after it, so a session file's times still start from the process's first event.
    public void ProcessEvent(double milliseconds)
    {
        if (!processSeen)
        {
            processSeen = true;
            if (start is { } launch)
            {
                foreach (var profilerEvent in kept)
                {
                    if (profilerEvent.ProcessId == launch.LauncherProcessId)
                    {
                        Consider(profilerEvent, milliseconds);
                    }
                }
            }

            kept.Clear();
            kept.TrimExcess();
            lastCreated.Clear();
            lastCreated.TrimExcess();
        }

        Settle(milliseconds);
    }

    // Call with each event of a launcher provider from another process (XamlEventSubscription.Match's otherProcess).
    public void OtherProcessEvent(TraceEvent data, MarkerProvider marker)
    {
        var time = data.TimeStampRelativeMSec;
        if (processSeen)
        {
            if (start is { } launch && data.ProcessID == launch.LauncherProcessId)
            {
                Consider(XamlEventSubscription.Capture(data, XamlProvider.Marker, marker, data.ProcessID), time);
            }

            Settle(time);
            return;
        }

        kept.Enqueue(XamlEventSubscription.Capture(data, XamlProvider.Marker, marker, data.ProcessID));
        var oldest = (start is null ? time : createdMilliseconds) - window;
        while (kept.Count > MaxKept || (kept.TryPeek(out var first) && first.RelativeMilliseconds < oldest))
        {
            kept.Dequeue();
        }
    }

    // Call when the whole trace was read: the launcher created no other process nearer the events still waiting.
    public void Complete()
    {
        foreach (var profilerEvent in waiting)
        {
            deliver(profilerEvent);
        }

        waiting.Clear();
    }

    // Delivers the launcher's event if it is part of the launch, or keeps it waiting while that's not known yet.
    private void Consider(ProfilerEvent profilerEvent, double now)
    {
        switch (PartOfLaunch(profilerEvent.RelativeMilliseconds, now))
        {
            case true:
                deliver(profilerEvent);
                break;
            case null:
                waiting.Add(profilerEvent);
                break;
        }
    }

    // Delivers or drops the waiting events whose launch is known by now.
    private void Settle(double now)
    {
        var stillWaiting = 0;
        for (var index = 0; index < waiting.Count; index++)
        {
            var profilerEvent = waiting[index];
            switch (PartOfLaunch(profilerEvent.RelativeMilliseconds, now))
            {
                case true:
                    deliver(profilerEvent);
                    break;
                case null:
                    waiting[stillWaiting++] = profilerEvent;
                    break;
            }
        }

        waiting.RemoveRange(stillWaiting, waiting.Count - stillWaiting);
    }

    // Whether the launcher's event at time is part of the process's launch: within LauncherWindow of its creation, and
    // nearer it than the launcher's creations of other processes; at the same distance, the earlier creation's. Null when
    // the launcher may still create a process nearer it, now being the trace's time.
    private bool? PartOfLaunch(double time, double now)
    {
        var sinceCreated = time - createdMilliseconds;
        if (Math.Abs(sinceCreated) > window)
        {
            return false;
        }

        if (sinceCreated <= 0)
        {
            return previousCreatedMilliseconds is not { } previous || -sinceCreated < time - previous;
        }

        if (nextCreatedMilliseconds is { } next)
        {
            return sinceCreated <= next - time;
        }

        return now - time >= sinceCreated ? true : null;
    }

    private void OnProcessStart(ProcessTraceData data)
    {
        if (OfflineEtlSource.ExecutableName(data) is { } name)
        {
            names[data.ProcessID] = name;
        }

#pragma warning disable CS0618 // The raw QPC timestamp is the clock the process's events are stored with.
        var qpc = data.TimeStampQPC;
#pragma warning restore CS0618
        // A process that was running when the trace started has a rundown event instead, which doesn't say when it
        // was created.
        if (data.Opcode != TraceEventOpcode.Start || qpc <= 0)
        {
            return;
        }

        var time = data.TimeStampRelativeMSec;
        if (!processSeen && data.ProcessID == processId)
        {
            start = new(qpc, OfflineEtlSource.ExecutableName(data), data.ParentID, names.GetValueOrDefault(data.ParentID));
            createdMilliseconds = time;
            previousCreatedMilliseconds = lastCreated.TryGetValue(data.ParentID, out var previous) ? previous : null;
            nextCreatedMilliseconds = null;
            started?.Invoke(start);
        }
        else if (start is { } launch && data.ParentID == launch.LauncherProcessId && time > createdMilliseconds)
        {
            nextCreatedMilliseconds ??= time;
        }

        if (!processSeen)
        {
            lastCreated[data.ParentID] = time;
        }

        Settle(time);
    }
}
