// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Buffers;
using System.Collections.Concurrent;
using System.Diagnostics;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Processing;

// Moves events from the ETW reader thread to one processing thread, which runs every processor in order. A live
// session posts events one at a time and drops them when the queue is full; a trace file posts batches and waits
// instead, so none is dropped (ForTrace).
public sealed class EventPipeline : IDisposable
{
    // About 42 MB at the ~320 bytes per event measured on a real trace; holds more than a second of a
    // startup burst (75,757 events in one second of a WinUI 3 Gallery launch were measured).
    public const int DefaultCapacity = 131_072;

    private readonly BlockingCollection<ProfilerEvent>? queue;
    private readonly BlockingCollection<ArraySegment<ProfilerEvent>>? batches;
    private readonly IReadOnlyList<IEventProcessor> processors;
    private readonly Thread thread;
    private long droppedEvents;
    private int disposed;

    public EventPipeline(IReadOnlyList<IEventProcessor> processors, int capacity = DefaultCapacity)
        : this(processors, capacity, batched: false)
    {
    }

    private EventPipeline(IReadOnlyList<IEventProcessor> processors, int capacity, bool batched)
    {
        ArgumentNullException.ThrowIfNull(processors);
        ArgumentOutOfRangeException.ThrowIfLessThan(capacity, 1);
        this.processors = processors;
        if (batched)
        {
            batches = new(capacity);
        }
        else
        {
            queue = new(capacity);
        }

        thread = new(Run) { IsBackground = true, Name = "WinUIProfiler event processing" };
        thread.Start();
    }

    // A pipeline for a recorded trace: the reader posts batches of events in the file's order with PostBatch, and
    // waits while batchCapacity batches are queued.
    public static EventPipeline ForTrace(IReadOnlyList<IEventProcessor> processors, int batchCapacity) =>
        new(processors, batchCapacity, batched: true);

    // Events that arrived while the queue was full. Always 0 for a trace.
    public long DroppedEvents => Interlocked.Read(ref droppedEvents);

    public Exception? Failure { get; private set; }

    // Never blocks the ETW reader: when the queue is full, the event is dropped and counted.
    public void Post(ProfilerEvent profilerEvent)
    {
        if (queue is null)
        {
            throw new InvalidOperationException("A trace's events are posted in batches.");
        }

        if (!queue.TryAdd(profilerEvent))
        {
            Interlocked.Increment(ref droppedEvents);
        }
    }

    // Queues a copy of the events, so the caller can reuse its buffer, and waits while the queue is full. Returns false,
    // without queuing them, once processing has stopped because a processor failed (see Failure).
    public bool PostBatch(ReadOnlySpan<ProfilerEvent> events)
    {
        var lane = batches ?? throw new InvalidOperationException("A live session's events are posted one at a time.");
        if (events.IsEmpty)
        {
            return !lane.IsAddingCompleted;
        }

        var array = ArrayPool<ProfilerEvent>.Shared.Rent(events.Length);
        events.CopyTo(array);
        try
        {
            lane.Add(new(array, 0, events.Length));
            return true;
        }
        catch (InvalidOperationException) when (lane.IsAddingCompleted)
        {
            ArrayPool<ProfilerEvent>.Shared.Return(array, clearArray: true);
            return false;
        }
    }

    private void Run()
    {
        long sequence = 0;
        try
        {
            if (queue is not null)
            {
                foreach (var profilerEvent in queue.GetConsumingEnumerable())
                {
                    Process(++sequence, profilerEvent);
                }

                return;
            }

            foreach (var batch in batches!.GetConsumingEnumerable())
            {
                try
                {
                    foreach (var profilerEvent in batch.AsSpan())
                    {
                        Process(++sequence, profilerEvent);
                    }
                }
                finally
                {
                    ArrayPool<ProfilerEvent>.Shared.Return(batch.Array!, clearArray: true);
                }
            }
        }
        catch (Exception error)
        {
            Trace.TraceError($"Event processing stopped: {error}");
            Failure = error;
            // A trace's reader may be waiting for room; it can't drop events, so it stops instead.
            batches?.CompleteAdding();
        }
    }

    private void Process(long sequence, ProfilerEvent profilerEvent)
    {
        foreach (var processor in processors)
        {
            processor.Process(sequence, profilerEvent);
        }
    }

    // Processes what is already queued, then stops. Call after the event source has stopped posting.
    public void Dispose()
    {
        if (Interlocked.Exchange(ref disposed, 1) != 0)
        {
            return;
        }

        queue?.CompleteAdding();
        batches?.CompleteAdding();
        thread.Join();
        queue?.Dispose();
        batches?.Dispose();
    }
}
