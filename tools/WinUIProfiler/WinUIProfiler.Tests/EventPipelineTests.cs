// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Processing;

// Covers the raised DefaultCapacity (EventPipeline.cs): a live session's queue must hold a burst bigger than the old
// 131,072 capacity without dropping events, now that it is 262,144.
internal static class EventPipelineTests
{
    internal static void Run()
    {
        HoldsABurstPastTheOldCapacity();
    }

    private static void HoldsABurstPastTheOldCapacity()
    {
        // Strictly between the old and new capacity: this must not have dropped events before the increase, and
        // must not drop any now.
        const int count = 132_000;
        using var gate = new BlockingProcessor();
        var pipeline = new EventPipeline([gate]);
        try
        {
            // Blocks the processing thread on the first event, so every Post below queues rather than being consumed.
            Post(pipeline, 1);
            Expect(gate.WaitUntilBlocked(TimeSpan.FromSeconds(10)), "The processor must block on the first event.");

            for (var i = 1; i < count; i++)
            {
                Post(pipeline, i);
            }

            Expect(pipeline.DroppedEvents == 0, $"Queuing {count} events, below the new capacity of {EventPipeline.DefaultCapacity}, must not drop any.");

            gate.Release();
            Expect(SpinWait.SpinUntil(() => Volatile.Read(ref gate.Processed) == count, TimeSpan.FromSeconds(30)),
                "The processing thread must drain the queued burst.");

            Expect(pipeline.DroppedEvents == 0, "No event must be dropped once the processor catches up.");
            Expect(gate.Processed == count, $"Every queued event must reach the processor; only {gate.Processed} of {count} did.");
        }
        finally
        {
            gate.Release();
            pipeline.Dispose();
        }

        Console.WriteLine($"Passed: a queue of {EventPipeline.DefaultCapacity} held a burst of {count} events without dropping any.");
    }

    private static void Post(EventPipeline pipeline, int sequence) =>
        pipeline.Post(new(sequence, sequence, 0, 0, XamlProvider.Xaml, false, "Test", 0, []));

    // Blocks the processing thread on its first event until Release is called, so events queued meanwhile are never
    // drained, then counts every event it processes.
    private sealed class BlockingProcessor : IEventProcessor, IDisposable
    {
        private readonly ManualResetEventSlim blocked = new();
        private readonly ManualResetEventSlim released = new();
        private bool first = true;
        public int Processed;

        // Called only by EventPipeline's single processing thread, in order, so plain reads/writes of `first` are safe.
        public void Process(long sequence, ProfilerEvent profilerEvent)
        {
            if (first)
            {
                first = false;
                blocked.Set();
                released.Wait();
            }

            Interlocked.Increment(ref Processed);
        }

        internal bool WaitUntilBlocked(TimeSpan timeout) => blocked.Wait(timeout);

        internal void Release() => released.Set();

        public void Dispose()
        {
            blocked.Dispose();
            released.Dispose();
        }
    }

    private static void Expect(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}
