// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Ingestion;

// Delivers the target's kernel hard page faults and, optionally, its image loads and unloads (classic kernel events), for
// the DLLs tab. A hard fault names its thread; TraceEvent names its process from the threads it has seen, so in a
// circular trace, or before a live session's thread rundown, a fault's process can be unknown (-1). Such a fault is held,
// as StackSampleCollector holds samples, until a thread event names the thread's process, and then delivered if the
// process is the target. Released faults arrive after later events; the DLLs tab orders them by Qpc.
internal sealed class DllEventSubscription
{
    internal const int MaxHeldFaults = 100_000;

    private readonly Func<int, long, bool> isTarget;
    private readonly Action<ProfilerEvent> sink;
    private readonly int capacity;
    private readonly Dictionary<int, List<HeldFault>> held = [];
    private int heldCount;

    internal DllEventSubscription(Func<int, long, bool> isTarget, Action<ProfilerEvent> sink, int capacity = MaxHeldFaults)
    {
        this.isTarget = isTarget ?? throw new ArgumentNullException(nameof(isTarget));
        this.sink = sink ?? throw new ArgumentNullException(nameof(sink));
        this.capacity = capacity;
    }

    // The faults held now, waiting for their thread's process.
    internal int HeldCount => heldCount;

    // The faults left out: past the capacity, before their thread's start (an earlier thread with the ID), or never placed.
    internal long Unattributed { get; private set; }

    internal static DllEventSubscription SubscribeKernel(ETWTraceEventSource source, Func<int, long, bool> isTarget,
        Action<ProfilerEvent> sink, bool includeImages)
    {
        var subscription = new DllEventSubscription(isTarget, sink);
        var kernel = source.Kernel;
        kernel.MemoryHardFault += data => subscription.OnFault(data.ProcessID, data.ThreadID, data.TimeStampRelativeMSec,
            data.TimeStamp.ToFileTimeUtc(), Capture(data, XamlProvider.KernelMemory, [new("VirtualAddress", data.VirtualAddress)]));
        kernel.ThreadStart += data => subscription.Learn(data.ThreadID, data.ProcessID, data.TimeStampRelativeMSec);
        kernel.ThreadDCStart += data => subscription.Learn(data.ThreadID, data.ProcessID);
        kernel.ThreadStop += data => subscription.Learn(data.ThreadID, data.ProcessID);
        kernel.ThreadDCStop += data => subscription.Learn(data.ThreadID, data.ProcessID);

        if (includeImages)
        {
            // WPR's kernel loader events use the classic schema; the live session uses Kernel-Process.
            kernel.ImageLoad += Image;
            kernel.ImageUnload += Image;
        }

        void Image(ImageLoadTraceData data)
        {
            if (isTarget(data.ProcessID, data.TimeStamp.ToFileTimeUtc()))
            {
                sink(Capture(data, XamlProvider.KernelImage,
                    [new("ImageBase", data.ImageBase), new("ImageSize", data.ImageSize), new("ImageName", data.FileName)]));
            }
        }

        return subscription;
    }

    // A hard fault of a thread, copied: delivered now if its process is known, or held until it is.
    internal void OnFault(int processId, int threadId, double milliseconds, long fileTime, ProfilerEvent fault)
    {
        if (processId >= 0)
        {
            if (isTarget(processId, fileTime))
            {
                sink(fault);
            }

            return;
        }

        if (heldCount >= capacity)
        {
            Unattributed++;
            return;
        }

        if (!held.TryGetValue(threadId, out var faults))
        {
            held[threadId] = faults = [];
        }

        faults.Add(new(fault, milliseconds, fileTime));
        heldCount++;
    }

    // A thread's process is now known: delivers the target's faults held for it, from the given time. Those before a
    // thread's start were of an earlier thread with the ID, whose process is unknown.
    internal void Learn(int threadId, int processId, double fromMilliseconds = double.MinValue)
    {
        if (processId < 0 || !held.Remove(threadId, out var faults))
        {
            return;
        }

        heldCount -= faults.Count;
        foreach (var fault in faults)
        {
            if (fault.Milliseconds < fromMilliseconds)
            {
                Unattributed++;
            }
            else if (isTarget(processId, fault.FileTime))
            {
                sink(fault.Event with { ProcessId = processId });
            }
        }
    }

    // Leaves out every fault still held.
    internal void Clear()
    {
        Unattributed += heldCount;
        held.Clear();
        heldCount = 0;
    }

    private readonly record struct HeldFault(ProfilerEvent Event, double Milliseconds, long FileTime);

    private static ProfilerEvent Capture(TraceEvent data, XamlProvider provider, EventField[] fields)
    {
#pragma warning disable CS0618 // The raw QPC is shared by the live ETW sessions and saved traces.
        return new(data.TimeStampQPC, data.TimeStampRelativeMSec, data.ProcessID, data.ThreadID,
            provider, false, data.EventName, (int)data.Opcode, fields, Version: data.Version);
#pragma warning restore CS0618
    }
}
