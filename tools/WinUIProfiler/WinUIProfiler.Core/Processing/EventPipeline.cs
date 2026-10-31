// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Concurrent;
using System.Diagnostics;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Processing;

// Moves events from the ETW reader thread to one processing thread, which runs every processor in order.
public sealed class EventPipeline : IDisposable
{
    // About 42 MB at the ~320 bytes per event measured on a real trace; holds more than a second of a
    // startup burst (75,757 events in one second of a WinUI 3 Gallery launch were measured).
    public const int DefaultCapacity = 131_072;

    private readonly BlockingCollection<ProfilerEvent> queue;
    private readonly IReadOnlyList<IEventProcessor> processors;
    private readonly Thread thread;
    private long droppedEvents;
    private int disposed;

    public EventPipeline(IReadOnlyList<IEventProcessor> processors, int capacity = DefaultCapacity)
    {
        ArgumentNullException.ThrowIfNull(processors);
        ArgumentOutOfRangeException.ThrowIfLessThan(capacity, 1);
        this.processors = processors;
        queue = new(capacity);
        thread = new(Run) { IsBackground = true, Name = "WinUIProfiler event processing" };
        thread.Start();
    }

    // Events that arrived while the queue was full.
    public long DroppedEvents => Interlocked.Read(ref droppedEvents);

    public Exception? Failure { get; private set; }

    // Never blocks the ETW reader: when the queue is full, the event is dropped and counted.
    public void Post(ProfilerEvent profilerEvent)
    {
        if (!queue.TryAdd(profilerEvent))
        {
            Interlocked.Increment(ref droppedEvents);
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
                foreach (var processor in processors)
                {
                    processor.Process(sequence, profilerEvent);
                }
            }
        }
        catch (Exception error)
        {
            Trace.TraceError($"Event processing stopped: {error}");
            Failure = error;
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
        queue.Dispose();
    }
}
