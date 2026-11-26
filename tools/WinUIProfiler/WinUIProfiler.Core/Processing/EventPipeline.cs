// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Concurrent;
using System.Diagnostics;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Processing;

public sealed record EventPipelineDiagnostics(int Capacity, int Depth, int HighWaterMark, long AcceptedEvents,
    long ProcessedEvents, long DroppedEvents, double ElapsedSeconds, double ProcessingMilliseconds);

// Moves events from the ETW reader thread to one processing thread, which runs every processor in order. A live
// session's events are dropped when the queue is full (Post); a trace's reader waits for room instead, so none is
// dropped (ForTrace, PostBlocking).
public sealed class EventPipeline : IDisposable
{
    // About 84 MB at the ~320 bytes per event measured on a real trace; gives live bursts room
    // while the processors catch up. Sustained overload still drops events.
    public const int DefaultCapacity = 262_144;

    private readonly BlockingCollection<ProfilerEvent> queue;
    private readonly IReadOnlyList<IEventProcessor> processors;
    private readonly Thread thread;
    private readonly bool trace;
    private long droppedEvents;
    private long acceptedEvents;
    private long processedEvents;
    private long processingTicks;
    private int highWaterMark;
    private readonly long started = Stopwatch.GetTimestamp();
    private readonly object diagnosticsGate = new();
    private EventPipelineDiagnostics? finalDiagnostics;
    private int disposed;

    public EventPipeline(IReadOnlyList<IEventProcessor> processors, int capacity = DefaultCapacity)
        : this(processors, capacity, trace: false)
    {
    }

    private EventPipeline(IReadOnlyList<IEventProcessor> processors, int capacity, bool trace)
    {
        ArgumentNullException.ThrowIfNull(processors);
        ArgumentOutOfRangeException.ThrowIfLessThan(capacity, 1);
        this.processors = processors;
        this.trace = trace;
        queue = new(capacity);
        thread = new(Run) { IsBackground = true, Name = "WinUIProfiler event processing" };
        thread.Start();
    }

    // A pipeline for a recorded trace: the reader posts the events in the file's order with PostBlocking, which waits
    // while the queue is full.
    public static EventPipeline ForTrace(IReadOnlyList<IEventProcessor> processors, int capacity = DefaultCapacity) =>
        new(processors, capacity, trace: true);

    // Events that arrived while the queue was full. Always 0 for a trace.
    public long DroppedEvents => Interlocked.Read(ref droppedEvents);

    public Exception? Failure { get; private set; }

    public EventPipelineDiagnostics Diagnostics
    {
        get
        {
            lock (diagnosticsGate)
            {
                return finalDiagnostics ?? new(queue.BoundedCapacity, queue.Count, Volatile.Read(ref highWaterMark),
                    Interlocked.Read(ref acceptedEvents), Interlocked.Read(ref processedEvents), DroppedEvents,
                    Stopwatch.GetElapsedTime(started).TotalSeconds,
                    Interlocked.Read(ref processingTicks) * 1000.0 / Stopwatch.Frequency);
            }
        }
    }

    private void Accepted()
    {
        Interlocked.Increment(ref acceptedEvents);
        ObserveDepth(queue.Count);
    }

    private void ObserveDepth(int depth)
    {
        var previous = Volatile.Read(ref highWaterMark);
        while (depth > previous)
        {
            var observed = Interlocked.CompareExchange(ref highWaterMark, depth, previous);
            if (observed == previous) break;
            previous = observed;
        }
    }

    // Never blocks the ETW reader: when the queue is full, the event is dropped and counted.
    public void Post(ProfilerEvent profilerEvent)
    {
        if (trace)
        {
            throw new InvalidOperationException("A trace's events are posted with PostBlocking, so none is dropped.");
        }

        if (!queue.TryAdd(profilerEvent))
        {
            ObserveDepth(queue.BoundedCapacity);
            Interlocked.Increment(ref droppedEvents);
        }
        else
        {
            Accepted();
        }
    }

    // Waits while the queue is full. Returns false, without queuing the event, once processing has stopped because a
    // processor failed (see Failure).
    public bool PostBlocking(ProfilerEvent profilerEvent)
    {
        if (!trace)
        {
            throw new InvalidOperationException("A live session's events are posted with Post, which never waits.");
        }

        try
        {
            // Sleeps instead of waiting in Add, which spins before it sleeps: while the queue stays full, the processing
            // thread makes room one event at a time, so a reader in Add would spin over and over, using more CPU for
            // the same speed. A full queue of DefaultCapacity events holds over a second of the processors' work.
            while (!queue.TryAdd(profilerEvent))
            {
                Thread.Sleep(1);
            }

            Accepted();
            return true;
        }
        catch (InvalidOperationException) when (queue.IsAddingCompleted)
        {
            return false;
        }
    }

    private void Run()
    {
        try
        {
            long sequence = 0;
            foreach (var profilerEvent in queue.GetConsumingEnumerable())
            {
                sequence++;
                var processingStarted = Stopwatch.GetTimestamp();
                foreach (var processor in processors)
                {
                    processor.Process(sequence, profilerEvent);
                }
                Interlocked.Add(ref processingTicks, Stopwatch.GetTimestamp() - processingStarted);
                Interlocked.Increment(ref processedEvents);
            }
        }
        catch (Exception error)
        {
            Trace.TraceError($"Event processing stopped: {error}");
            Failure = error;
            // A trace's reader may be waiting for room; it can't drop events, so it stops instead. A live queue stays
            // open, as Post would throw on the ETW thread; its events are dropped once the queue is full.
            if (trace)
            {
                queue.CompleteAdding();
            }
        }
    }

    // Processes what is already queued, then stops. Call after the event source has stopped posting.
    public void Dispose()
    {
        if (Interlocked.Exchange(ref disposed, 1) != 0)
        {
            return;
        }

        queue.CompleteAdding();
        thread.Join();
        lock (diagnosticsGate)
        {
            finalDiagnostics = Diagnostics;
            queue.Dispose();
        }
    }
}
