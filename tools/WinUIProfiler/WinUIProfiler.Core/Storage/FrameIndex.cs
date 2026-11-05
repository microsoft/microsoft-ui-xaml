// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// The UI-thread tick each event belongs to. Runs on the session writer's thread inside its transactions, like
// ObjectIndex, so an event's tick is saved and trimmed together with the event.
//
// CoreServices_Frame brackets every UI-thread tick with the core's frame counter, which only advances after a tick
// submits composition commands (xcpcore.cpp). Each tick is a row of Frames, so a frame number has a row for each tick
// that carried it: any that drew nothing, then the one that drew.
// - Events between a tick's start and its stop belong to that tick.
// - Events between ticks belong to the thread's next tick, so they are held until it starts. So are a thread's events
//   before its first tick.
// - A stop without a start ends a tick that started before the recording. The held events and the stop belong to it,
//   and its start is unknown.
// Threads that never tick (the render and image-decode threads) are not attributed; only their first
// MaxHeldBeforeFirstTick events are held.
internal sealed class FrameIndex : IDisposable
{
    // A UI thread ticks within moments of starting, far sooner than this.
    private const int MaxHeldBeforeFirstTick = 100_000;

    // For CoreServices_Frame, where it keeps its frame number and whether it starts or stops a tick; -1 for other events.
    private sealed record EventShape(int FrameNumberField, int IsStartField, string Bucket);

    private sealed class Tick(long id)
    {
        public long Id { get; } = id;
        public long LastSequence { get; set; }
    }

    private sealed class ThreadState
    {
        public bool Ticked { get; set; }
        public bool InTick { get; set; }
        // The tick in progress and its frame number. Null in a tick when trimming deleted the tick's row.
        public Tick? Current { get; set; }
        public long Number { get; set; }
        public List<(long Sequence, string Bucket)> Held { get; } = [];
    }

    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertTick;
    private readonly SqliteCommand endTick;
    private readonly SqliteCommand insertEvent;
    private readonly Dictionary<long, EventShape> shapes = [];
    private readonly Dictionary<int, ThreadState> threads = [];
    private long tickCount;

    internal FrameIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertTick = Sql.Prepare(connection,
            "INSERT INTO Frames (FrameId, ThreadId, FrameNumber, StartQpc) VALUES ($id, $thread, $number, $start)",
            ("$id", SqliteType.Integer), ("$thread", SqliteType.Integer), ("$number", SqliteType.Integer), ("$start", SqliteType.Integer));
        endTick = Sql.Prepare(connection, "UPDATE Frames SET EndQpc = $qpc WHERE FrameId = $id",
            ("$id", SqliteType.Integer), ("$qpc", SqliteType.Integer));
        insertEvent = Sql.Prepare(connection, "INSERT INTO FrameEvents (Seq, FrameId, Bucket) VALUES ($seq, $frame, $bucket)",
            ("$seq", SqliteType.Integer), ("$frame", SqliteType.Integer), ("$bucket", SqliteType.Text));
    }

    // Records the tick the event belongs to, when it is known. Call right after inserting the event, in the same
    // transaction.
    internal void Add(long sequence, long typeId, ProfilerEvent profilerEvent, SqliteTransaction transaction)
    {
        insertTick.Transaction = transaction;
        endTick.Transaction = transaction;
        insertEvent.Transaction = transaction;
        var shape = ShapeOf(typeId, profilerEvent);
        var threadId = profilerEvent.ThreadId;
        if (!threads.TryGetValue(threadId, out var thread))
        {
            threads[threadId] = thread = new();
        }

        if (shape.IsStartField < 0)
        {
            if (thread.InTick)
            {
                Attribute(sequence, threadId, thread, shape.Bucket);
            }
            else if (thread.Ticked || thread.Held.Count < MaxHeldBeforeFirstTick)
            {
                thread.Held.Add((sequence, shape.Bucket));
            }

            return;
        }

        var starts = profilerEvent.Fields[shape.IsStartField].Value is true;
        if (starts || !thread.InTick)
        {
            thread.Number = Number(profilerEvent.Fields[shape.FrameNumberField].Value);
            thread.Current = InsertTick(threadId, thread.Number, starts ? profilerEvent.TimestampQpc : null);
            foreach (var (heldSequence, bucket) in thread.Held)
            {
                Attribute(heldSequence, threadId, thread, bucket);
            }

            thread.Held.Clear();
        }

        var tick = Attribute(sequence, threadId, thread, shape.Bucket);
        thread.Ticked = true;
        thread.InTick = starts;
        if (!starts)
        {
            endTick.Parameters["$id"].Value = tick.Id;
            endTick.Parameters["$qpc"].Value = profilerEvent.TimestampQpc;
            endTick.ExecuteNonQuery();
            thread.Current = null;
        }
    }

    // Removes the rows of trimmed events, and the ticks left without any. Call in the trimming transaction.
    internal void Trim(long cutoff, SqliteTransaction transaction)
    {
        Sql.Execute(connection, "DELETE FROM FrameEvents WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
        Sql.Execute(connection, """
            DELETE FROM Frames
            WHERE NOT EXISTS (SELECT 1 FROM FrameEvents WHERE FrameEvents.FrameId = Frames.FrameId)
            """, transaction);
        foreach (var thread in threads.Values)
        {
            if (thread.Current?.LastSequence < cutoff)
            {
                thread.Current = null;
            }

            thread.Held.RemoveAll(held => held.Sequence < cutoff);
        }
    }

    // What changed in a frame, by kind: XAML objects and the tree, the composition tree and surfaces, or anything else
    // (layout passes, rendering, scheduling, dispatching, snapshots).
    private static string BucketOf(string eventName) => eventName switch
    {
        "ElementEnteredTree" or "ElementLeftTree" or "ChildAdded" or "ChildInserted" or "ChildRemoved" or "ContentChanged" or
            "PeerAssociated" or "DependencyObject_Enter" or "FrameworkElement_ApplyTemplate" or
            "VisualStateManager_GoToStateOptimized" or "Control_RefreshTemplateBindings" or "EventManager_RaiseLoadedEvent" or
            "Application_LoadComponent" => "Xaml",
        "VisualSurfaceChanged" => "Composition",
        _ when eventName.StartsWith("CompPeer", StringComparison.Ordinal) || eventName.StartsWith("CompNode", StringComparison.Ordinal) ||
            eventName.StartsWith("WucVisual", StringComparison.Ordinal) || eventName.StartsWith("DCompSurface", StringComparison.Ordinal)
            => "Composition",
        _ => "Other"
    };

    // FrameNumber is written as a UInt32.
    private static long Number(object? value) => value switch
    {
        int number => number,
        uint number => number,
        _ => 0
    };

    private EventShape ShapeOf(long typeId, ProfilerEvent profilerEvent)
    {
        if (!shapes.TryGetValue(typeId, out var shape))
        {
            var names = profilerEvent.Fields.Select(field => field.Name).ToArray();
            var isFrame = profilerEvent.Name == "CoreServices_Frame";
            shape = new(isFrame ? Array.IndexOf(names, "FrameNumber") : -1, isFrame ? Array.IndexOf(names, "IsStart") : -1,
                BucketOf(profilerEvent.Name));
            shapes[typeId] = shape;
        }

        return shape;
    }

    private Tick InsertTick(int threadId, long number, long? startQpc)
    {
        var tick = new Tick(++tickCount);
        insertTick.Parameters["$id"].Value = tick.Id;
        insertTick.Parameters["$thread"].Value = threadId;
        insertTick.Parameters["$number"].Value = number;
        insertTick.Parameters["$start"].Value = startQpc ?? (object)DBNull.Value;
        insertTick.ExecuteNonQuery();
        return tick;
    }

    // Events arrive in Seq order. A tick whose row trimming deleted gets a new one, without its start.
    private Tick Attribute(long sequence, int threadId, ThreadState thread, string bucket)
    {
        var tick = thread.Current ??= InsertTick(threadId, thread.Number, null);
        insertEvent.Parameters["$seq"].Value = sequence;
        insertEvent.Parameters["$frame"].Value = tick.Id;
        insertEvent.Parameters["$bucket"].Value = bucket;
        insertEvent.ExecuteNonQuery();
        tick.LastSequence = sequence;
        return tick;
    }

    public void Dispose()
    {
        insertTick.Dispose();
        endTick.Dispose();
        insertEvent.Dispose();
    }
}