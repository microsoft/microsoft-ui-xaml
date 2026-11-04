// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Ingestion;

// Watches a trace for one process's launch: the kernel's event for its creation, which names the process that created
// it, its launcher, and the launcher's events from the launcher providers (MarkerProviderSettings.Launcher) that are
// part of the launch, those within LauncherWindow of the creation that are:
// - An event of an activity, when the launcher created the process during the activity and no other process after it
//   before the activity's stop event. An event with an activity ID, such as ShellExecuteExW/Start and /Stop, is of that
//   activity; one without is of the activity of its thread's latest event with one, if the activity's start event was
//   read and its stop event isn't earlier, as AppLaunch_UserClick, which ShellExecuteExW writes on a thread of its own
//   for Start-Process ms-settings:, a call that creates no process itself. A call that creates a process, such as
//   ShellExecuteExW, starts up to hundreds of milliseconds before the creation and stops after it, so its start event
//   can be nearer the launcher's previous creation, as when a script launches apps one after another; and a call can
//   start a launch whose process another process creates, as File Explorer's for a UWP app, which a Windows service
//   creates.
// - A start event of no activity, or whose activity's stop event wasn't read by LauncherWindow after the creation, when
//   the process was the first the launcher created after it.
// - Any other event, when it is nearer the creation than the launcher's creation of any other process, as
//   AppLaunch_UserClick, which File Explorer writes after it creates the process.
// A process ID can be reused within a trace, so the process's creation is the last one before its first event, and the
// launcher's events are those its process ID wrote from the launcher's start until its end. The launcher is known for
// certain only at that first event, so until then the launcher providers' events of every other process are kept for
// LauncherWindow, at most MaxKept, and the launcher's from LauncherWindow before the creation once it was read. An
// event whose part in the launch isn't known yet waits for its stop event, the launcher's next creation or its end, or
// the trace to be far enough past it, or for Complete. Only a trace with the kernel's process events has the creation,
// and so the launcher's events. Not thread-safe: TraceEvent calls it on the thread that reads the trace.
internal sealed class LaunchWatcher
{
    private const int MaxKept = 10_000;

    private readonly int processId;
    private readonly Action<TraceProcessStart>? started;
    private readonly Action<ProfilerEvent> deliver;
    private readonly double window = MarkerProviderSettings.LauncherWindow.TotalMilliseconds;
    // The executable name of each process ID, from its latest start.
    private readonly Dictionary<int, string> names = [];
    // Until the process's first event: when the latest process with each ID started, if the trace has it, and when it
    // last created another process.
    private readonly Dictionary<int, double> startedAt = [];
    private readonly Dictionary<int, double> lastCreated = [];
    // Until the process's first event: the launcher providers' events of every other process of the last
    // LauncherWindow, and the launcher's from LauncherWindow before the creation.
    private readonly Queue<LauncherProviderEvent> kept = new();
    private readonly List<LauncherProviderEvent> launcherKept = [];
    // The launcher's events whose part in the launch isn't known yet, when each of its activities started and stopped,
    // and the activity of each of its threads' latest event with an activity ID.
    private readonly List<LauncherProviderEvent> waiting = [];
    private readonly Dictionary<Guid, double> starts = [];
    private readonly Dictionary<Guid, double> stops = [];
    private readonly Dictionary<int, Guid> threadActivities = [];
    private TraceProcessStart? start;
    private double createdMilliseconds;
    private double? launcherStartedMilliseconds;
    private bool launcherEnded;
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
        source.Kernel.ProcessEndGroup += OnProcessEnd;
    }

    // Call after delivering each event the process wrote, with its time, but not one another process wrote about it,
    // such as the System process's GPU memory event for an earlier process with its ID. The first time, delivers the
    // launcher's events kept from LauncherWindow before the process's creation that are part of the launch. They are
    // earlier than that first event, but come after it, so a session file's times still start from the process's
    // first event.
    public void ProcessEvent(double milliseconds)
    {
        if (!processSeen)
        {
            processSeen = true;
            // An activity's stop event can come after its other events among them.
            for (var index = 0; index < launcherKept.Count; index++)
            {
                launcherKept[index] = NoteActivity(launcherKept[index]);
            }

            foreach (var launcherEvent in launcherKept)
            {
                Consider(launcherEvent, milliseconds);
            }

            kept.Clear();
            kept.TrimExcess();
            launcherKept.Clear();
            launcherKept.TrimExcess();
            startedAt.Clear();
            startedAt.TrimExcess();
            lastCreated.Clear();
            lastCreated.TrimExcess();
        }

        Settle(milliseconds);
    }

    // Call with each event of a launcher provider from another process (XamlEventSubscription.Match's otherProcess).
    public void OtherProcessEvent(TraceEvent data, MarkerProvider marker)
    {
        // Once the process's first event was read, only the launcher's events can still be part of the launch.
        if (processSeen && !IsLaunchers(data.ProcessID, data.TimeStampRelativeMSec))
        {
            Settle(data.TimeStampRelativeMSec);
            return;
        }

        ReadLauncherProviderEvent(XamlEventSubscription.Capture(data, XamlProvider.Marker, marker, data.ProcessID),
            data.ActivityID);
    }

    // Call when the whole trace was read: no later event can change whether the events still waiting are part of the
    // launch.
    public void Complete() => Settle(double.PositiveInfinity);

    private void OnProcessStart(ProcessTraceData data)
    {
        var name = OfflineEtlSource.ExecutableName(data);
        if (name is not null)
        {
            names[data.ProcessID] = name;
        }

#pragma warning disable CS0618 // The raw QPC timestamp is the clock the process's events are stored with.
        var qpc = data.TimeStampQPC;
#pragma warning restore CS0618
        // A process that was running when the trace started has a rundown event instead, which doesn't say when it
        // was created.
        if (data.Opcode == TraceEventOpcode.Start && qpc > 0)
        {
            ReadCreation(data.ProcessID, data.ParentID, name, qpc, data.TimeStampRelativeMSec);
        }
    }

    private void OnProcessEnd(ProcessTraceData data)
    {
        // Not the rundown event of a process still running when the trace ended.
        if (data.Opcode == TraceEventOpcode.Stop)
        {
            ReadEnd(data.ProcessID, data.TimeStampRelativeMSec);
        }
    }

    // A process's creation at time, by the process parentId.
    private void ReadCreation(int id, int parentId, string? name, long qpc, double time)
    {
        if (!processSeen)
        {
            // A new process with this ID: what an earlier one did isn't its.
            startedAt[id] = time;
            lastCreated.Remove(id);
            if (id == processId)
            {
                start = new(qpc, name, parentId, names.GetValueOrDefault(parentId));
                createdMilliseconds = time;
                launcherStartedMilliseconds = startedAt.TryGetValue(parentId, out var launcherStarted) ? launcherStarted : null;
                launcherEnded = false;
                previousCreatedMilliseconds = lastCreated.TryGetValue(parentId, out var previous) ? previous : null;
                nextCreatedMilliseconds = null;
                launcherKept.Clear();
                launcherKept.AddRange(kept.Where(launcherEvent =>
                    IsLaunchers(launcherEvent.Event.ProcessId, launcherEvent.Event.RelativeMilliseconds)));
                started?.Invoke(start);
            }

            lastCreated[parentId] = time;
        }

        if (start is { } launch && !launcherEnded && time > createdMilliseconds)
        {
            if (id == launch.LauncherProcessId)
            {
                // A new process with the launcher's ID: the launcher ended, though the trace lost its end event.
                launcherEnded = true;
            }
            else if (parentId == launch.LauncherProcessId)
            {
                nextCreatedMilliseconds ??= time;
            }
        }

        Settle(time);
    }

    // A process's end at time. Its ID can then be reused by another process.
    private void ReadEnd(int id, double time)
    {
        if (start is { } launch && id == launch.LauncherProcessId && time >= createdMilliseconds && !launcherEnded)
        {
            launcherEnded = true;
            Settle(time);
        }
    }

    // A launcher provider's event from another process, and its activity ID.
    private void ReadLauncherProviderEvent(ProfilerEvent profilerEvent, Guid activity)
    {
        var time = profilerEvent.RelativeMilliseconds;
        var launcherEvent = new LauncherProviderEvent(profilerEvent, activity);
        if (!processSeen)
        {
            kept.Enqueue(launcherEvent);
            while (kept.Count > MaxKept || (kept.TryPeek(out var oldest) && oldest.Event.RelativeMilliseconds < time - window))
            {
                kept.Dequeue();
            }

            if (launcherKept.Count < MaxKept && IsLaunchers(profilerEvent.ProcessId, time))
            {
                launcherKept.Add(launcherEvent);
            }

            return;
        }

        if (IsLaunchers(profilerEvent.ProcessId, time))
        {
            Consider(NoteActivity(launcherEvent), time);
        }

        Settle(time);
    }

    // Whether an event of process id at time is the launcher's, within LauncherWindow of the creation.
    private bool IsLaunchers(int id, double time) =>
        start is { } launch && id == launch.LauncherProcessId && !launcherEnded &&
        (launcherStartedMilliseconds is not { } launcherStarted || time >= launcherStarted) &&
        Math.Abs(time - createdMilliseconds) <= window;

    // Notes when the launcher's activity started or stopped, and the activity of its thread's latest event with an
    // activity ID. Call in the order of the launcher's events. Returns an event without one with that of its thread.
    private LauncherProviderEvent NoteActivity(LauncherProviderEvent launcherEvent)
    {
        var thread = launcherEvent.Event.ThreadId;
        if (launcherEvent.Activity == Guid.Empty)
        {
            return threadActivities.TryGetValue(thread, out var threadActivity)
                ? launcherEvent with { ThreadActivity = threadActivity }
                : launcherEvent;
        }

        threadActivities[thread] = launcherEvent.Activity;
        switch (launcherEvent.Event.Opcode)
        {
            case (int)TraceEventOpcode.Start:
                starts.TryAdd(launcherEvent.Activity, launcherEvent.Event.RelativeMilliseconds);
                break;
            case (int)TraceEventOpcode.Stop:
                stops.TryAdd(launcherEvent.Activity, launcherEvent.Event.RelativeMilliseconds);
                break;
        }

        return launcherEvent;
    }

    // The launcher's event's activity: its activity ID, or for an event without one, its thread's activity, if that
    // started and hadn't stopped before the event. Empty when it has none.
    private Guid ActivityOf(LauncherProviderEvent launcherEvent)
    {
        if (launcherEvent.Activity != Guid.Empty)
        {
            return launcherEvent.Activity;
        }

        var threadActivity = launcherEvent.ThreadActivity;
        return threadActivity != Guid.Empty && starts.ContainsKey(threadActivity) &&
            (!stops.TryGetValue(threadActivity, out var stop) || stop >= launcherEvent.Event.RelativeMilliseconds)
            ? threadActivity
            : Guid.Empty;
    }

    // Delivers the launcher's event if it is part of the launch, or keeps it waiting while that isn't known yet.
    private void Consider(LauncherProviderEvent launcherEvent, double now)
    {
        switch (PartOfLaunch(launcherEvent, now))
        {
            case true:
                deliver(launcherEvent.Event);
                break;
            case null:
                waiting.Add(launcherEvent);
                break;
        }
    }

    // Delivers or drops the waiting events whose part in the launch is known by now.
    private void Settle(double now)
    {
        var stillWaiting = 0;
        for (var index = 0; index < waiting.Count; index++)
        {
            var launcherEvent = waiting[index];
            switch (PartOfLaunch(launcherEvent, now))
            {
                case true:
                    deliver(launcherEvent.Event);
                    break;
                case null:
                    waiting[stillWaiting++] = launcherEvent;
                    break;
            }
        }

        waiting.RemoveRange(stillWaiting, waiting.Count - stillWaiting);
    }

    // Whether the launcher's event is part of the process's launch (see above), or null while that isn't known yet, now
    // being the trace's time. At the same distance from two creations, an event is the earlier one's.
    private bool? PartOfLaunch(LauncherProviderEvent launcherEvent, double now)
    {
        var time = launcherEvent.Event.RelativeMilliseconds;
        var sinceCreated = time - createdMilliseconds;
        var isStart = launcherEvent.Event.Opcode == (int)TraceEventOpcode.Start;
        if (Math.Abs(sinceCreated) > window || (isStart && sinceCreated > 0))
        {
            return false;
        }

        var activity = ActivityOf(launcherEvent);
        if (activity != Guid.Empty)
        {
            if (starts.TryGetValue(activity, out var activityStarted) && activityStarted > createdMilliseconds)
            {
                return false;
            }

            if (stops.TryGetValue(activity, out var stop))
            {
                return stop >= createdMilliseconds && (nextCreatedMilliseconds is not { } nextCreated || nextCreated > stop);
            }

            if (!launcherEnded && now <= createdMilliseconds + window)
            {
                return null;
            }
        }

        if (isStart)
        {
            return previousCreatedMilliseconds is not { } previousCreated || previousCreated < time;
        }

        if (sinceCreated <= 0)
        {
            return previousCreatedMilliseconds is not { } previous || -sinceCreated < time - previous;
        }

        if (nextCreatedMilliseconds is { } next)
        {
            return sinceCreated <= next - time;
        }

        return launcherEnded || now - time >= sinceCreated ? true : null;
    }

    // Activity is the event's activity ID, which an activity's start and stop events share; empty when it has none.
    // ThreadActivity is, for an event without one, the activity of its thread's latest earlier event with one.
    private readonly record struct LauncherProviderEvent(ProfilerEvent Event, Guid Activity, Guid ThreadActivity = default);
}
