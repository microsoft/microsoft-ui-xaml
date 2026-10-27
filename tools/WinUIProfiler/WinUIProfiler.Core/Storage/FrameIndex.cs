// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// The drawn frame each UI-thread event belongs to. Runs on the session writer's thread inside its transactions, like
// ObjectIndex, so an event's frame is saved and trimmed together with the event.
//
// CoreServices_Frame brackets every UI-thread tick with the core's frame counter, which only advances when a tick
// submits composition commands (xcpcore.cpp). So frame N on a UI thread is the ticks that carried N: any that drew
// nothing, then the one that drew.
// - Events that carry FrameNumber (DependencyObject_Enter, ApplyTemplate, GoToStateOptimized, ...) belong to that frame.
// - Other events between a tick's start and stop belong to that tick's frame. The code uses the latest frame number seen
//   on the thread, which in recorded sessions is always the tick's.
// - Other events between ticks belong to the frame that renders them, so they are held until the thread's next event
//   with a FrameNumber.
// Threads that never report a frame number (the render and image-decode threads) are not attributed.
internal sealed class FrameIndex : IDisposable
{
    // Where an event type keeps its frame number, whether it marks a tick, and its bucket.
    private sealed record EventShape(int FrameNumberField, int IsStartField, string Bucket);

    private sealed class Frame(long id)
    {
        public long Id { get; } = id;
        public long LastSequence { get; set; }
    }

    private sealed class ThreadState
    {
        // The highest frame number seen on the thread; 0 until it reports one.
        public long Latest { get; set; }
        public bool InTick { get; set; }
        public List<(long Sequence, string Bucket)> Held { get; } = [];
    }

    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertFrame;
    private readonly SqliteCommand startTick;
    private readonly SqliteCommand endTick;
    private readonly SqliteCommand insertEvent;
    private readonly Dictionary<long, EventShape> shapes = [];
    private readonly Dictionary<(int Thread, long Number), Frame> frames = [];
    private readonly Dictionary<int, ThreadState> threads = [];
    private long frameCount;

    internal FrameIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertFrame = Sql.Prepare(connection,
            "INSERT INTO Frames (FrameId, ThreadId, FrameNumber, Ticks, TickTimeQpc) VALUES ($id, $thread, $number, 0, 0)",
            ("$id", SqliteType.Integer), ("$thread", SqliteType.Integer), ("$number", SqliteType.Integer));
        startTick = Sql.Prepare(connection, """
            UPDATE Frames SET Ticks = Ticks + 1, StartQpc = COALESCE(StartQpc, $qpc), LastTickStartQpc = $qpc
            WHERE FrameId = $id
            """, ("$id", SqliteType.Integer), ("$qpc", SqliteType.Integer));
        // A tick whose start came before the recording adds no time.
        endTick = Sql.Prepare(connection, """
            UPDATE Frames SET EndQpc = $qpc, TickTimeQpc = TickTimeQpc + COALESCE($qpc - LastTickStartQpc, 0)
            WHERE FrameId = $id
            """, ("$id", SqliteType.Integer), ("$qpc", SqliteType.Integer));
        insertEvent = Sql.Prepare(connection, "INSERT INTO FrameEvents (Seq, FrameId, Bucket) VALUES ($seq, $frame, $bucket)",
            ("$seq", SqliteType.Integer), ("$frame", SqliteType.Integer), ("$bucket", SqliteType.Text));
    }

    // Records the frame the event belongs to, when it is known. Call right after inserting the event, in the same
    // transaction.
    internal void Add(long sequence, long typeId, ProfilerEvent profilerEvent, SqliteTransaction transaction)
    {
        insertFrame.Transaction = transaction;
        startTick.Transaction = transaction;
        endTick.Transaction = transaction;
        insertEvent.Transaction = transaction;
        var shape = ShapeOf(typeId, profilerEvent);
        if (!threads.TryGetValue(profilerEvent.ThreadId, out var thread))
        {
            threads[profilerEvent.ThreadId] = thread = new();
        }

        var number = shape.FrameNumberField >= 0 ? Number(profilerEvent.Fields[shape.FrameNumberField].Value) : 0;
        if (number > 0)
        {
            foreach (var (heldSequence, bucket) in thread.Held)
            {
                Attribute(heldSequence, profilerEvent.ThreadId, number, bucket);
            }

            thread.Held.Clear();
            var frame = Attribute(sequence, profilerEvent.ThreadId, number, shape.Bucket);
            if (shape.IsStartField >= 0)
            {
                thread.InTick = profilerEvent.Fields[shape.IsStartField].Value is true;
                var tick = thread.InTick ? startTick : endTick;
                tick.Parameters["$id"].Value = frame.Id;
                tick.Parameters["$qpc"].Value = profilerEvent.TimestampQpc;
                tick.ExecuteNonQuery();
            }

            thread.Latest = Math.Max(thread.Latest, number);
        }
        else if (thread.InTick)
        {
            Attribute(sequence, profilerEvent.ThreadId, thread.Latest, shape.Bucket);
        }
        else if (thread.Latest != 0)
        {
            thread.Held.Add((sequence, shape.Bucket));
        }
    }

    // Removes the rows of trimmed events, and the frames left without any. Call in the trimming transaction.
    internal void Trim(long cutoff, SqliteTransaction transaction)
    {
        Sql.Execute(connection, "DELETE FROM FrameEvents WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
        Sql.Execute(connection, """
            DELETE FROM Frames
            WHERE NOT EXISTS (SELECT 1 FROM FrameEvents WHERE FrameEvents.FrameId = Frames.FrameId)
            """, transaction);
        foreach (var (key, frame) in frames)
        {
            if (frame.LastSequence < cutoff)
            {
                frames.Remove(key);
            }
        }

        foreach (var thread in threads.Values)
        {
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
            shape = new(Array.IndexOf(names, "FrameNumber"),
                profilerEvent.Name == "CoreServices_Frame" ? Array.IndexOf(names, "IsStart") : -1, BucketOf(profilerEvent.Name));
            shapes[typeId] = shape;
        }

        return shape;
    }

    // Events arrive in Seq order.
    private Frame Attribute(long sequence, int threadId, long number, string bucket)
    {
        if (!frames.TryGetValue((threadId, number), out var frame))
        {
            frame = new(++frameCount);
            insertFrame.Parameters["$id"].Value = frame.Id;
            insertFrame.Parameters["$thread"].Value = threadId;
            insertFrame.Parameters["$number"].Value = number;
            insertFrame.ExecuteNonQuery();
            frames[(threadId, number)] = frame;
        }

        insertEvent.Parameters["$seq"].Value = sequence;
        insertEvent.Parameters["$frame"].Value = frame.Id;
        insertEvent.Parameters["$bucket"].Value = bucket;
        insertEvent.ExecuteNonQuery();
        frame.LastSequence = sequence;
        return frame;
    }

    public void Dispose()
    {
        insertFrame.Dispose();
        startTick.Dispose();
        endTick.Dispose();
        insertEvent.Dispose();
    }
}