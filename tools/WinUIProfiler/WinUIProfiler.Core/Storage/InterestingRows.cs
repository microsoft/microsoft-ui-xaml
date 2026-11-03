// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// The kinds of interesting rows.
public enum InterestingRowType
{
    Region,
    Frame,
    UpdateLayout,
    Operation,
    FocusChange,
    CreateGraphicsDevice,
    Navigating,
    Navigated
}

// One interesting row. Name is the row's type: the kind, or for an operation the name PerfXamlEvent gives it. A row's
// events are its thread's from StartSeq to StopSeq; a region that a focus change started begins at the focus event,
// which another thread can have written. Times are milliseconds from the start of the recording. FrameNumber is a frame
// row's. Trigger is a region's cause: the name of the row that started it.
public sealed record InterestingRow(InterestingRowType Type, string Name, int ThreadId, long StartSeq, long StopSeq,
    double StartMilliseconds, double StopMilliseconds, long? FrameNumber, string? Trigger = null)
{
    public double DurationMilliseconds => StopMilliseconds - StartMilliseconds;
}

// Finds the regions of interest in a session file, and the interesting rows (see the README):
// - A region of interest starts on a thread at a navigation (FrameNavigating, FrameNavigated), the start of a
//   PerfXamlEvent (initializing XAML, showing a flyout, menu or content dialog), or a window focus change, and ends at
//   the end of that thread's next tick. A trigger during a tick moves the start back to that tick's start; a trigger
//   while a region is open is ignored. A focus change on a thread without rows yet is held, and the next trigger's
//   region starts at it.
// - The rows are the ticks, UpdateLayout passes, PerfXamlEvent operations, navigations, focus changes and graphics
//   device creations. A row is interesting when it starts inside a region; the region itself, operations, focus
//   changes and graphics device creations always are. Only interesting rows are kept.
// - A start while one of the same kind is open on the thread is ignored, and so is a stop without a start, such as the
//   rest of a tick in progress when the recording started.
// Ticks come from CoreServices_Frame, which a tick writes right after the manifest FrameBegin and right after its
// FrameEnd, and which carries the frame number. Not read: the ApiFunctionCall events of WinAppSDK 1.4 and earlier.
// Events arrive in Seq order; Read takes the events stored since the last call.
public sealed class InterestingRows
{
    internal static readonly string[] XamlEventNames =
    [
        "CoreServices_Frame", "Layout/Start", "Layout/Stop", "CreateGraphicsDevice/Start", "CreateGraphicsDevice/Stop",
        "FrameNavigating", "FrameNavigated", "PerfXamlEvent"
    ];

    private sealed class Open(InterestingRowType type, string name, int threadId, long sequence, double milliseconds,
        bool interesting, long? frameNumber = null, string? trigger = null)
    {
        public InterestingRowType Type { get; } = type;
        public string Name { get; } = name;
        public int ThreadId { get; } = threadId;
        public long StartSeq { get; } = sequence;
        public double StartMilliseconds { get; } = milliseconds;
        public long? FrameNumber { get; } = frameNumber;
        public string? Trigger { get; } = trigger;
        public bool Interesting { get; set; } = interesting;
    }

    // Ticks, UpdateLayout passes and graphics device creations open on each thread.
    private readonly Dictionary<(int Thread, InterestingRowType Type), Open> pending = [];
    private readonly Dictionary<int, Open> regions = [];
    // PerfXamlEvent operations by thread, object and nesting depth: one object's operations can nest.
    private readonly Dictionary<(int Thread, long Pointer, int Depth), Open> operations = [];
    private readonly Dictionary<int, (long Seq, double Milliseconds)> heldFocus = [];
    // Threads with a row, interesting or not: a focus change on one is a row and a trigger right away.
    private readonly HashSet<int> threadsWithRows = [];
    private readonly List<InterestingRow> rows = [];
    private long? lastSeq;

    // Reads the session file's events stored after the last call, up to throughSeq when given, and returns the
    // interesting rows that ended.
    public IReadOnlyList<InterestingRow> Read(string path, long? throughSeq = null, CancellationToken cancellationToken = default)
    {
        SessionFile.ReadInterestingEvents(path, lastSeq, throughSeq, Add, cancellationToken);
        var taken = rows.ToArray();
        rows.Clear();
        return taken;
    }

    private void Add(long sequence, ProfilerEvent profilerEvent)
    {
        lastSeq = sequence;
        var thread = profilerEvent.ThreadId;
        var milliseconds = profilerEvent.RelativeMilliseconds;
        if (profilerEvent.Provider == XamlProvider.Win32k)
        {
            // The payload is the thread that lost focus and the one that got it, read by position.
            if (profilerEvent.Fields is [_, var gained, ..] && Number(gained.Value) is { } focused and > 0)
            {
                OnFocusChange((int)focused, sequence, milliseconds);
            }

            return;
        }

        if (profilerEvent.Provider != XamlProvider.Xaml)
        {
            return;
        }

        switch (profilerEvent.Name)
        {
            case "CoreServices_Frame":
                var frameNumber = Number(Field(profilerEvent, "FrameNumber"));
                if (Field(profilerEvent, "IsStart") is true)
                {
                    Start(InterestingRowType.Frame, "Frame", thread, sequence, milliseconds, frameNumber);
                }
                else if (Stop(InterestingRowType.Frame, thread, sequence, milliseconds) && regions.Remove(thread, out var region))
                {
                    Log(region, sequence, milliseconds);
                }

                break;
            case "Layout/Start":
                Start(InterestingRowType.UpdateLayout, "UpdateLayout", thread, sequence, milliseconds);
                break;
            case "Layout/Stop":
                Stop(InterestingRowType.UpdateLayout, thread, sequence, milliseconds);
                break;
            case "CreateGraphicsDevice/Start":
                Start(InterestingRowType.CreateGraphicsDevice, "Create graphics device", thread, sequence, milliseconds);
                break;
            case "CreateGraphicsDevice/Stop":
                Stop(InterestingRowType.CreateGraphicsDevice, thread, sequence, milliseconds);
                break;
            case "FrameNavigating":
                OnNavigation(InterestingRowType.Navigating, "Frame::Navigating", thread, sequence, milliseconds);
                break;
            case "FrameNavigated":
                OnNavigation(InterestingRowType.Navigated, "Frame::Navigated", thread, sequence, milliseconds);
                break;
            case "PerfXamlEvent":
                OnOperation(profilerEvent, thread, sequence, milliseconds);
                break;
        }
    }

    private static object? Field(ProfilerEvent profilerEvent, string name)
    {
        foreach (var field in profilerEvent.Fields)
        {
            if (field.Name == name)
            {
                return field.Value;
            }
        }

        return null;
    }

    private static long? Number(object? value) => value switch
    {
        int number => number,
        uint number => number,
        long number => number,
        ulong number => unchecked((long)number),
        _ => null
    };

    // Graphics device creation runs on a worker thread, outside any UI thread's region, and is interesting anyway.
    private void Start(InterestingRowType type, string name, int thread, long sequence, double milliseconds, long? frameNumber = null)
    {
        if (!pending.ContainsKey((thread, type)))
        {
            pending[(thread, type)] = new(type, name, thread, sequence, milliseconds,
                regions.ContainsKey(thread) || type == InterestingRowType.CreateGraphicsDevice, frameNumber);
        }
    }

    // False for a stop without a start.
    private bool Stop(InterestingRowType type, int thread, long sequence, double milliseconds)
    {
        if (!pending.Remove((thread, type), out var open))
        {
            return false;
        }

        LogHeldFocus(thread);
        Log(open, sequence, milliseconds);
        return true;
    }

    private void OnNavigation(InterestingRowType type, string name, int thread, long sequence, double milliseconds)
    {
        StartRegion(thread, sequence, milliseconds, name);
        LogHeldFocus(thread);
        Log(new(type, name, thread, sequence, milliseconds, regions.ContainsKey(thread)), sequence, milliseconds);
    }

    // Every operation's start starts a region; IsInteresting is not read.
    private void OnOperation(ProfilerEvent profilerEvent, int thread, long sequence, double milliseconds)
    {
        var pointer = Number(Field(profilerEvent, "ObjectPointer")) ?? 0;
        var depth = 0;
        while (operations.ContainsKey((thread, pointer, depth)))
        {
            depth++;
        }

        if (Field(profilerEvent, "IsStart") is true)
        {
            var name = Field(profilerEvent, "EventName") as string;
            name = string.IsNullOrEmpty(name) ? "PerfXamlEvent" : name;
            StartRegion(thread, sequence, milliseconds, name);
            operations[(thread, pointer, depth)] = new(InterestingRowType.Operation, name, thread, sequence, milliseconds,
                regions.ContainsKey(thread));
        }
        else if (depth > 0 && operations.Remove((thread, pointer, depth - 1), out var open))
        {
            LogHeldFocus(thread);
            Log(open, sequence, milliseconds);
        }
    }

    private void OnFocusChange(int thread, long sequence, double milliseconds)
    {
        if (threadsWithRows.Contains(thread))
        {
            Log(new(InterestingRowType.FocusChange, "Hwnd Focus", thread, sequence, milliseconds, true), sequence, milliseconds);
            StartRegion(thread, sequence, milliseconds, "Hwnd Focus");
        }
        else
        {
            // The latest focus change is taken as the cause of the thread's first XAML work.
            heldFocus[thread] = (sequence, milliseconds);
        }
    }

    // A region starts at a held focus change, at the start of the tick in progress, which becomes interesting, or now.
    private void StartRegion(int thread, long sequence, double milliseconds, string trigger)
    {
        if (regions.ContainsKey(thread))
        {
            return;
        }

        if (heldFocus.TryGetValue(thread, out var focus))
        {
            (sequence, milliseconds) = focus;
            trigger = "Hwnd Focus";
        }
        else if (pending.TryGetValue((thread, InterestingRowType.Frame), out var tick))
        {
            (sequence, milliseconds) = (tick.StartSeq, tick.StartMilliseconds);
            tick.Interesting = true;
        }

        regions[thread] = new(InterestingRowType.Region, "Region of Interest", thread, sequence, milliseconds, true, trigger: trigger);
    }

    private void LogHeldFocus(int thread)
    {
        if (heldFocus.Remove(thread, out var focus))
        {
            Log(new(InterestingRowType.FocusChange, "Hwnd Focus", thread, focus.Seq, focus.Milliseconds, true), focus.Seq,
                focus.Milliseconds);
        }
    }

    private void Log(Open open, long stopSequence, double stopMilliseconds)
    {
        threadsWithRows.Add(open.ThreadId);
        if (open.Interesting)
        {
            rows.Add(new(open.Type, open.Name, open.ThreadId, open.StartSeq, stopSequence, open.StartMilliseconds, stopMilliseconds,
                open.FrameNumber, open.Trigger));
        }
    }
}
