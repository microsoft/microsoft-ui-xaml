// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Ingestion;

// One CPU sample and its call stack, leaf first: the kernel's frames, when the sample interrupted kernel code, then the
// user-mode frames.
internal sealed record MergedSample(long Qpc, int ProcessId, int ThreadId, ulong[] Frames);

// Puts each CPU sample's call stack together from the kernel's stack-walk fragments, as TraceEvent's TraceLog does:
// - A fragment names its sample by the sample's thread and time, and comes after it.
// - A fragment whose root frame is a kernel address is the kernel part of a sample that interrupted kernel code. The
//   kernel walks the user-mode part only once the thread returns to user mode, in a fragment whose leaf frame is a user
//   address, which is the user part of every sample of the thread still waiting for one. The System and Idle processes
//   have no user part.
// - Any other fragment is the whole stack of its sample, unless the thread returned to user mode after the sample: then
//   it's stale and ignored.
// A thread belongs to the process of its first sample. A sample or fragment of another process means the thread's ID
// was reused, so it ends the earlier thread.
// A sample whose thread or process ends, or that is still incomplete at Flush, is emitted with what it has: its kernel
// part if it's waiting for its user part, else its instruction pointer. So is a sample with no fragment yet at its
// thread's next sample: the kernel writes a sample's fragment from the same processor just after it. Not thread-safe.
internal sealed class StackSampleMerger
{
    // At most this many samples of a thread wait for its return to user mode, about 4 s of samples at the kernel's
    // default 1 ms interval. With more, the oldest is emitted with only its kernel part.
    internal const int MaxWaitingForUser = 4096;

    private readonly Action<MergedSample> emit;
    private readonly Dictionary<int, ThreadStacks> threads = [];

    public StackSampleMerger(Action<MergedSample> emit)
    {
        ArgumentNullException.ThrowIfNull(emit);
        this.emit = emit;
    }

    // Whether an address is in the kernel's half of the address space, for a trace of a system with this pointer size.
    internal static bool IsKernelAddress(ulong address, int pointerSize) =>
        pointerSize == 4 ? address >= 0x8000_0000UL : address >= 0xFFFF_0000_0000_0000UL;

    public void OnSample(long qpc, int processId, int threadId, ulong instructionPointer)
    {
        var thread = Find(processId, threadId);
        if (thread is null)
        {
            thread = new ThreadStacks(processId, threadId);
            threads.Add(threadId, thread);
        }
        else if (thread.Awaiting is { } earlier)
        {
            Emit(thread, earlier, [earlier.InstructionPointer]);
        }

        thread.Awaiting = new PendingSample(qpc, instructionPointer);
    }

    // Frames are leaf first, as the kernel writes them. The pointer size is the traced system's.
    public void OnStack(long qpc, long eventQpc, int processId, int threadId, ulong[] frames, int pointerSize)
    {
        ArgumentNullException.ThrowIfNull(frames);
        if (frames.Length == 0 || Find(processId, threadId) is not { } thread)
        {
            return;
        }

        var awaiting = thread.Awaiting is { } sample && sample.Qpc == eventQpc ? sample : null;
        if (IsKernelAddress(frames[^1], pointerSize))
        {
            if (awaiting is null)
            {
                return;
            }

            thread.Awaiting = null;
            if (thread.ProcessId is 0 or 4)
            {
                Emit(thread, awaiting, frames);
                return;
            }

            awaiting.Kernel = frames;
            if (thread.WaitingForUser.Count == MaxWaitingForUser)
            {
                var oldest = thread.WaitingForUser.Dequeue();
                Emit(thread, oldest, oldest.Kernel!);
            }

            thread.WaitingForUser.Enqueue(awaiting);
            return;
        }

        var previousReturn = thread.LastReturnQpc;
        if (!IsKernelAddress(frames[0], pointerSize))
        {
            // The thread is back in user mode, so these are the user part of each sample waiting for it.
            foreach (var waiting in thread.WaitingForUser)
            {
                Emit(thread, waiting, [.. waiting.Kernel!, .. frames]);
            }

            thread.WaitingForUser.Clear();
            thread.LastReturnQpc = qpc;
        }

        if (awaiting is not null && eventQpc >= previousReturn)
        {
            thread.Awaiting = null;
            Emit(thread, awaiting, frames);
        }
    }

    // Emits the thread's samples whose stacks aren't complete with what they have; they won't get more.
    public void OnThreadEnd(int threadId)
    {
        if (threads.Remove(threadId, out var thread))
        {
            Complete(thread);
        }
    }

    // Emits the samples of the process's threads whose stacks aren't complete with what they have.
    public void OnProcessEnd(int processId)
    {
        foreach (var threadId in threads.Where(pair => pair.Value.ProcessId == processId).Select(pair => pair.Key).ToList())
        {
            OnThreadEnd(threadId);
        }
    }

    // Emits every sample whose stack isn't complete with what it has, as at the end of the trace.
    public void Flush()
    {
        foreach (var thread in threads.Values)
        {
            Complete(thread);
        }

        threads.Clear();
    }

    // The thread's stacks, if it's known and still belongs to the process. A thread of another process is ended first:
    // its ID was reused.
    private ThreadStacks? Find(int processId, int threadId)
    {
        if (!threads.TryGetValue(threadId, out var thread))
        {
            return null;
        }

        if (thread.ProcessId == processId)
        {
            return thread;
        }

        threads.Remove(threadId);
        Complete(thread);
        return null;
    }

    private void Complete(ThreadStacks thread)
    {
        foreach (var waiting in thread.WaitingForUser)
        {
            Emit(thread, waiting, waiting.Kernel!);
        }

        thread.WaitingForUser.Clear();
        if (thread.Awaiting is { } awaiting)
        {
            thread.Awaiting = null;
            Emit(thread, awaiting, [awaiting.InstructionPointer]);
        }
    }

    private void Emit(ThreadStacks thread, PendingSample sample, ulong[] frames) =>
        emit(new MergedSample(sample.Qpc, thread.ProcessId, thread.ThreadId, frames));

    // A sample whose stack isn't complete yet.
    private sealed class PendingSample(long qpc, ulong instructionPointer)
    {
        public long Qpc { get; } = qpc;

        public ulong InstructionPointer { get; } = instructionPointer;

        // Its kernel part, once it has one.
        public ulong[]? Kernel { get; set; }
    }

    // A thread's samples whose stacks aren't complete: its latest sample, if it has no fragment yet, and those waiting
    // for the thread's return to user mode, oldest first.
    private sealed class ThreadStacks(int processId, int threadId)
    {
        public int ProcessId { get; } = processId;

        public int ThreadId { get; } = threadId;

        public PendingSample? Awaiting { get; set; }

        public Queue<PendingSample> WaitingForUser { get; } = new();

        // The time of the fragment of the thread's latest return to user mode.
        public long LastReturnQpc { get; set; } = long.MinValue;
    }
}
