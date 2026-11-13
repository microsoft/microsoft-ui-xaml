// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Parsers;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;
using Microsoft.Diagnostics.Tracing.Parsers.Symbol;

namespace WinUIProfiler.Engine.Ingestion;

// An image loaded in a process, or a driver in the kernel's address space (ProcessId -1), while the trace sampled it. A
// merged trace, as WPR saves, names the PDB of its symbols; otherwise the symbolizer reads it from the file at FilePath.
internal sealed class SampledModule(int processId, string filePath, ulong imageBase, int imageSize, int timeDateStamp)
{
    public int ProcessId { get; } = processId;

    public string FilePath { get; } = filePath;

    public ulong ImageBase { get; } = imageBase;

    public int ImageSize { get; } = imageSize;

    public int TimeDateStamp { get; set; } = timeDateStamp;

    public string? PdbName { get; set; }

    public Guid PdbGuid { get; set; }

    public int PdbAge { get; set; }

    // When the trace loaded and unloaded it, as QPC timestamps; an image loaded before the trace, or still loaded at its
    // end, is open on that side.
    public long LoadQpc { get; init; } = long.MinValue;

    public long UnloadQpc { get; set; } = long.MaxValue;

    // Whether the trace has the image only from its end, its unload or the rundown at the trace's end, so samples of it
    // may have been stored before it was known.
    public bool EndOnly { get; set; }

    public bool Contains(ulong address) => address >= ImageBase && address - ImageBase < (uint)ImageSize;

    public bool Covers(long qpc) => qpc >= LoadQpc && qpc < UnloadQpc;
}

// A frame of a sampled call stack, and the module whose code it's in, when one was loaded at its address.
internal readonly record struct CodeFrame(SampledModule? Module, ulong Address);

// A CPU sample of a target process's thread, with its call stack, leaf first.
internal sealed record CompletedSample(long Qpc, int ProcessId, int ThreadId, CodeFrame[] Frames);

// What the session writer stores about the sampling once the last sample is posted: the sampling interval, whether
// the trace had compressed stacks, and every module the trace loaded, so the identity read after a sample was stored,
// and the modules known only from the trace's end, reach the session file. Unattributed counts the samples left out
// because the trace didn't name their thread's process in time.
internal sealed record CpuSamplingSummary(double? IntervalMilliseconds, bool CompressedStacks, long Samples,
    IReadOnlyList<SampledModule> Modules, long Unattributed = 0);

// Collects the CPU samples of the target processes' threads, with their call stacks, from the kernel's sampled-profile
// and stack-walk events, as a WPR CPU profile or KernelSampleSession records them:
// - StackSampleMerger puts each sample's stack together from its fragments.
// - Each frame is placed in the module that had its address when the sample was taken: a kernel address in a driver,
//   another in an image of the sample's process. The kernel's image events give the modules, and a merged trace each
//   image's PDB identity, in events at the image event's time, as TraceEvent's TraceLog reads them. A module known only
//   from its end (its unload, or the rundown at the trace's end) is in the summary, for the frames stored before it.
// - A sample of a thread whose process isn't known yet, as in a circular trace that lost the thread's start, waits for a
//   later event of the thread to name its process. At most MaxHeldSamples wait; the summary counts those never placed.
// - A live session gets events in the order of the kernel's buffers rather than in time order, so with a reorder window
//   they wait until the session is that far past them, and are applied in time order. A trace file is read in time
//   order: with no window they're applied at once.
// Compressed stacks (stack keys) aren't read: their samples keep only their instruction pointer, and CompressedStacks
// is set. Not thread-safe: TraceEvent calls it on the thread that reads the trace; call Flush there, or once it's done.
internal sealed class StackSampleCollector
{
    // The samples held for threads whose process isn't known, and the modules kept after their unload for each process.
    private const int MaxHeldSamples = 100_000;
    private const int MaxUnloadedModules = 1024;

    private readonly Func<int, long, bool> isTarget;
    private readonly Action<CompletedSample> sink;
    private readonly StackSampleMerger merger;
    private readonly double windowMilliseconds;
    private readonly PriorityQueue<Action, (double Milliseconds, long Sequence)> waiting = new();
    // The process of each thread, from its start, for a sample whose event doesn't name it.
    private readonly Dictionary<int, int> threadProcesses = [];
    // The samples of the threads whose process isn't known yet.
    private readonly HeldSamples heldSamples = new(MaxHeldSamples);
    // The modules loaded now, in order of address: the kernel's, and each process's.
    private readonly List<SampledModule> kernelModules = [];
    private readonly Dictionary<int, List<SampledModule>> processModules = [];
    // The modules unloaded, or replaced, while the trace ran, oldest first: the kernel's (-1), and each process's.
    private readonly Dictionary<int, List<SampledModule>> unloadedModules = [];
    private readonly List<SampledModule> allModules = [];
    private long sequence;
    private double newestMilliseconds = double.MinValue;
    private int pointerSize = 8;
    private bool sampled;
    // The latest image event's module and time, and the identity events not matched with an image event yet.
    private SampledModule? lastModule;
    private long lastModuleQpc;
    private (long Qpc, string Name, Guid Guid, int Age)? lastPdb;
    private (long Qpc, int TimeDateStamp)? lastImageId;

    // IsTarget says whether a process's samples are collected, given its ID and the event's time as a UTC FILETIME, so a
    // process that had the ID earlier is told apart; it's called on the thread that reads the trace. Sink gets each
    // complete sample, on that thread or in Flush.
    public StackSampleCollector(ETWTraceEventSource source, Func<int, long, bool> isTarget, Action<CompletedSample> sink,
        TimeSpan reorderWindow = default)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(isTarget);
        ArgumentNullException.ThrowIfNull(sink);
        this.isTarget = isTarget;
        this.sink = sink;
        windowMilliseconds = reorderWindow.TotalMilliseconds;
        merger = new(Emit);
        var kernel = source.Kernel;
        kernel.ThreadStart += data => OnThreadStart(data, rundown: false);
        kernel.ThreadDCStart += data => OnThreadStart(data, rundown: true);
        kernel.ThreadStop += OnThreadStop;
        kernel.ThreadDCStop += OnThreadRundownEnd;
        kernel.ProcessStop += OnProcessStop;
        kernel.ImageLoad += data => OnImageLoad(data, rundown: false);
        kernel.ImageDCStart += data => OnImageLoad(data, rundown: true);
        kernel.ImageUnload += data => OnImageEnd(data, unloaded: true);
        kernel.ImageDCStop += data => OnImageEnd(data, unloaded: false);
        kernel.PerfInfoSample += OnSample;
        kernel.StackWalkStack += OnStack;
        kernel.PerfInfoCollectionStart += data => OnInterval(data, collection: true);
        kernel.PerfInfoCollectionEnd += data => OnInterval(data, collection: true);
        kernel.PerfInfoSetInterval += data => OnInterval(data, collection: false);
        kernel.StackWalkStackKeyKernel += _ => CompressedStacks = true;
        kernel.StackWalkStackKeyUser += _ => CompressedStacks = true;
        var symbols = new SymbolTraceEventParser(source);
        symbols.ImageIDDbgID_RSDS += OnPdb;
        symbols.ImageID += OnImageId;
    }

    // The timer's sampling interval, when the trace has it.
    public double? IntervalMilliseconds { get; private set; }

    public bool CompressedStacks { get; private set; }

    // The samples given to the sink so far.
    public long Samples { get; private set; }

    // The samples still held, before Flush, weren't placed either.
    public CpuSamplingSummary Summary => new(IntervalMilliseconds, CompressedStacks, Samples, allModules.ToArray(),
        heldSamples.Unattributed + heldSamples.Count);

    // Why a recorded trace's samples are missing or incomplete, stored with them; null when they are complete.
    public static string? TraceNote(CpuSamplingSummary summary)
    {
        ArgumentNullException.ThrowIfNull(summary);
        var unattributed = UnattributedNote(summary);
        if (summary.Samples == 0)
        {
            // Samples the trace didn't tie to a process don't mean it lacks a CPU profile.
            return unattributed ??
                "The trace has no CPU samples of the process. Record it with a CPU profile, such as WPR's GeneralProfile or CPU.";
        }

        return JoinNotes(summary.CompressedStacks
            ? "The trace's stacks are compressed (stack keys), which aren't read: its samples have only the sampled function. Record without stack compression."
            : null, unattributed);
    }

    // Why samples were left out, when the trace didn't name their thread's process in time; null when none were.
    public static string? UnattributedNote(CpuSamplingSummary summary)
    {
        ArgumentNullException.ThrowIfNull(summary);
        return summary.Unattributed > 0
            ? $"{summary.Unattributed.ToString(CultureInfo.InvariantCulture)} CPU samples were left out: the trace didn't name their threads' process in time, as when it lost the threads' starts, so some may be the process's."
            : null;
    }

    // The notes given, in order, as one; null when there are none.
    public static string? JoinNotes(string? first, string? second) =>
        first is null ? second : second is null ? first : $"{first} {second}";

    // Applies the events still waiting and gives the sink every sample left, with the stack it has. The samples of threads
    // whose process the trace never named are left out, and counted in the summary.
    public void Flush()
    {
        while (waiting.TryDequeue(out var action, out _))
        {
            action();
        }

        heldSamples.Clear();
        merger.Flush();
    }

    // The timer's interval while the trace sampled. A collection's start or end has it; an interval set before the first
    // sample replaces it, and one set after (the reset when a recording stops) keeps it, or gives the interval it reset
    // when the trace has no other.
    internal static double? NextInterval(double? current, bool collection, bool sampled, int newInterval, int oldInterval)
    {
        static double? Milliseconds(int interval) => interval > 0 ? interval / 10_000.0 : null;

        return collection ? current ?? Milliseconds(newInterval)
            : !sampled ? Milliseconds(newInterval) ?? current
            : current ?? Milliseconds(oldInterval) ?? Milliseconds(newInterval);
    }

    private static long Qpc(TraceEvent data)
    {
#pragma warning disable CS0618 // The raw QPC timestamp is the clock the session's events are stored with.
        return data.TimeStampQPC;
#pragma warning restore CS0618
    }

    // The event's time as a UTC FILETIME, which a process's creation time is compared with.
    private static long FileTime(TraceEvent data) => data.TimeStamp.ToFileTimeUtc();

    // Applies the events that waited the reorder window, before the event at hand.
    private void Note(TraceEvent data)
    {
        pointerSize = data.PointerSize;
        if (windowMilliseconds <= 0)
        {
            return;
        }

        newestMilliseconds = Math.Max(newestMilliseconds, data.TimeStampRelativeMSec);
        while (waiting.TryPeek(out var action, out var key) && key.Milliseconds <= newestMilliseconds - windowMilliseconds)
        {
            waiting.Dequeue();
            action();
        }
    }

    private void Apply(TraceEvent data, Action action) => Apply(data.TimeStampRelativeMSec, action);

    private void Apply(double milliseconds, Action action)
    {
        if (windowMilliseconds <= 0)
        {
            action();
        }
        else
        {
            waiting.Enqueue(action, (milliseconds, sequence++));
        }
    }

    private void OnThreadStart(ThreadTraceData data, bool rundown)
    {
        Note(data);
        // The samples held from before a thread's start were of an earlier thread with its ID.
        Learn(data.ThreadID, data.ProcessID, rundown ? double.MinValue : data.TimeStampRelativeMSec);
        threadProcesses[data.ThreadID] = data.ProcessID;
    }

    private void OnThreadStop(ThreadTraceData data)
    {
        Note(data);
        var threadId = data.ThreadID;
        Learn(threadId, data.ProcessID);
        Apply(data, () =>
        {
            threadProcesses.Remove(threadId);
            merger.OnThreadEnd(threadId);
        });
    }

    // The rundown at the trace's end names the process of each thread still running, which a trace that lost the thread's
    // start has nowhere else.
    private void OnThreadRundownEnd(ThreadTraceData data)
    {
        Note(data);
        Learn(data.ThreadID, data.ProcessID);
    }

    // Gives the merger the samples held for a thread whose process is now known, from the given time: those before a
    // thread's start were of an earlier thread with its ID, whose process is unknown.
    private void Learn(int threadId, int processId, double fromMilliseconds = double.MinValue)
    {
        foreach (var sample in heldSamples.Release(threadId, processId, fromMilliseconds, isTarget))
        {
            Apply(sample.Milliseconds, () => merger.OnSample(sample.Qpc, processId, threadId, sample.Address));
        }
    }

    private void OnProcessStop(ProcessTraceData data)
    {
        Note(data);
        var processId = data.ProcessID;
        Apply(data, () =>
        {
            merger.OnProcessEnd(processId);
            processModules.Remove(processId);
            unloadedModules.Remove(processId);
        });
    }

    private void OnImageLoad(ImageLoadTraceData data, bool rundown)
    {
        Note(data);
        var qpc = Qpc(data);
        var kernelImage = StackSampleMerger.IsKernelAddress(data.ImageBase, data.PointerSize);
        var key = kernelImage ? -1 : data.ProcessID;
        var module = new SampledModule(key, data.FileName, data.ImageBase, data.ImageSize, data.TimeDateStamp)
        {
            // The rundown at the trace's start has the images loaded before it.
            LoadQpc = rundown ? long.MinValue : qpc,
        };
        allModules.Add(module);
        Identify(module, qpc);
        Apply(data, () => Insert(key, kernelImage ? kernelModules : ModulesOf(key), module, qpc));
    }

    // An unload, or the image's rundown at the trace's end, which a trace that lost its start has for each image.
    private void OnImageEnd(ImageLoadTraceData data, bool unloaded)
    {
        Note(data);
        var qpc = Qpc(data);
        var imageBase = data.ImageBase;
        var kernelImage = StackSampleMerger.IsKernelAddress(imageBase, data.PointerSize);
        var key = kernelImage ? -1 : data.ProcessID;
        var modules = kernelImage ? kernelModules : processModules.GetValueOrDefault(key);
        var module = modules is null ? null : At(modules, imageBase);
        if (module is null)
        {
            module = new SampledModule(key, data.FileName, imageBase, data.ImageSize, data.TimeDateStamp) { EndOnly = true };
            allModules.Add(module);
        }

        Identify(module, qpc);
        var ended = module;
        Apply(data, () =>
        {
            var loaded = kernelImage ? kernelModules : processModules.GetValueOrDefault(key);
            var current = loaded is null ? null : At(loaded, imageBase);
            if (current is not null && current != ended)
            {
                // Its load was still waiting in the reorder window when the event came.
                ended.EndOnly = false;
            }

            if (unloaded)
            {
                if (current is not null)
                {
                    loaded!.Remove(current);
                }

                Unload(key, current ?? ended, qpc);
            }
            else if (current is null)
            {
                Insert(key, kernelImage ? kernelModules : ModulesOf(key), ended, qpc);
            }
        });
    }

    // A merged trace has each image's identity in events at the image event's time, before or after it.
    private void Identify(SampledModule module, long qpc)
    {
        if (lastPdb is { } pdb && pdb.Qpc == qpc && module.PdbName is null)
        {
            (module.PdbName, module.PdbGuid, module.PdbAge) = (pdb.Name, pdb.Guid, pdb.Age);
            lastPdb = null;
        }

        if (lastImageId is { } image && image.Qpc == qpc && module.TimeDateStamp == 0)
        {
            module.TimeDateStamp = image.TimeDateStamp;
            lastImageId = null;
        }

        lastModule = module;
        lastModuleQpc = qpc;
    }

    private void OnPdb(DbgIDRSDSTraceData data)
    {
        var qpc = Qpc(data);
        if (lastModule is { PdbName: null } module && lastModuleQpc == qpc)
        {
            (module.PdbName, module.PdbGuid, module.PdbAge) = (data.PdbFileName, data.GuidSig, data.Age);
            lastPdb = null;
        }
        else
        {
            lastPdb = (qpc, data.PdbFileName, data.GuidSig, data.Age);
        }
    }

    private void OnImageId(ImageIDTraceData data)
    {
        var qpc = Qpc(data);
        if (lastModule is { TimeDateStamp: 0 } module && lastModuleQpc == qpc)
        {
            module.TimeDateStamp = data.TimeDateStamp;
            lastImageId = null;
        }
        else
        {
            lastImageId = (qpc, data.TimeDateStamp);
        }
    }

    private void OnSample(SampledProfileTraceData data)
    {
        Note(data);
        sampled = true;
        var threadId = data.ThreadID;
        var processId = data.ProcessID;
        var qpc = Qpc(data);
        var address = (ulong)data.InstructionPointer;
        if (processId < 0 && !threadProcesses.TryGetValue(threadId, out processId))
        {
            // The thread started before the trace, which lost its rundown: a later event of the thread names its process.
            heldSamples.Hold(threadId, new(qpc, address, data.TimeStampRelativeMSec, FileTime(data)));
            return;
        }

        if (!isTarget(processId, FileTime(data)))
        {
            return;
        }

        Apply(data, () => merger.OnSample(qpc, processId, threadId, address));
    }

    private void OnStack(StackWalkStackTraceData data)
    {
        Note(data);
        var processId = data.ProcessID;
        var threadId = data.ThreadID;
        if (processId >= 0)
        {
            // A stack names its thread's process, before the stack is applied to the samples it releases.
            Learn(threadId, processId);
            threadProcesses.TryAdd(threadId, processId);
        }

        if (!isTarget(processId, FileTime(data)))
        {
            return;
        }

        var frames = new ulong[data.FrameCount];
        for (var index = 0; index < frames.Length; index++)
        {
            frames[index] = data.InstructionPointer(index);
        }

        var qpc = Qpc(data);
        var eventQpc = data.EventTimeStampQPC;
        var size = data.PointerSize;
        Apply(data, () => merger.OnStack(qpc, eventQpc, processId, threadId, frames, size));
    }

    // The timer's (source 0) interval, from a collection's start or end, or a change of it.
    private void OnInterval(SampledProfileIntervalTraceData data, bool collection)
    {
        if (data.SampleSource == 0)
        {
            IntervalMilliseconds = NextInterval(IntervalMilliseconds, collection, sampled, data.NewInterval, data.OldInterval);
        }
    }

    private void Emit(MergedSample sample)
    {
        var frames = new CodeFrame[sample.Frames.Length];
        for (var index = 0; index < frames.Length; index++)
        {
            var address = sample.Frames[index];
            frames[index] = new(Resolve(sample.ProcessId, address, sample.Qpc), address);
        }

        Samples++;
        sink(new(sample.Qpc, sample.ProcessId, sample.ThreadId, frames));
    }

    // The module that had the address when the sample was taken. A sample can be emitted after its time, once its stack is
    // complete or its thread ends, so the module loaded at the address now is skipped if it came later: then it's the one
    // unloaded, or replaced, since.
    private SampledModule? Resolve(int processId, ulong address, long qpc)
    {
        var kernel = StackSampleMerger.IsKernelAddress(address, pointerSize);
        var key = kernel ? -1 : processId;
        var loaded = kernel ? kernelModules : processModules.GetValueOrDefault(key);
        if (loaded is not null && Containing(loaded, address) is { } module && module.LoadQpc <= qpc)
        {
            return module;
        }

        if (unloadedModules.TryGetValue(key, out var unloaded))
        {
            // They're unloaded in time order, so the search stops at the first unloaded by the sample's time.
            for (var index = unloaded.Count - 1; index >= 0 && unloaded[index].UnloadQpc > qpc; index--)
            {
                if (unloaded[index].Contains(address) && unloaded[index].Covers(qpc))
                {
                    return unloaded[index];
                }
            }
        }

        return null;
    }

    // Keeps a module unloaded, or replaced, at the given time, for the samples taken before it that are still waiting.
    private void Unload(int key, SampledModule module, long qpc)
    {
        if (module.UnloadQpc != long.MaxValue)
        {
            return;
        }

        module.UnloadQpc = qpc;
        if (!unloadedModules.TryGetValue(key, out var modules))
        {
            unloadedModules[key] = modules = [];
        }

        modules.Add(module);
        if (modules.Count > MaxUnloadedModules)
        {
            modules.RemoveAt(0);
        }
    }

    private List<SampledModule> ModulesOf(int processId)
    {
        if (!processModules.TryGetValue(processId, out var modules))
        {
            processModules[processId] = modules = [];
        }

        return modules;
    }

    // Adds the module in order of address, in place of those it overlaps: they were unloaded by the time it was loaded.
    private void Insert(int key, List<SampledModule> modules, SampledModule module, long qpc)
    {
        var index = LastAtOrBelow(modules, module.ImageBase);
        if (index >= 0 && modules[index].Contains(module.ImageBase))
        {
            Unload(key, modules[index], qpc);
            modules.RemoveAt(index);
        }
        else
        {
            index++;
        }

        while (index < modules.Count && module.Contains(modules[index].ImageBase))
        {
            Unload(key, modules[index], qpc);
            modules.RemoveAt(index);
        }

        modules.Insert(index, module);
    }

    private static SampledModule? At(List<SampledModule> modules, ulong imageBase)
    {
        var index = LastAtOrBelow(modules, imageBase);
        return index >= 0 && modules[index].ImageBase == imageBase ? modules[index] : null;
    }

    private static SampledModule? Containing(List<SampledModule> modules, ulong address)
    {
        var index = LastAtOrBelow(modules, address);
        return index >= 0 && modules[index].Contains(address) ? modules[index] : null;
    }

    private static int LastAtOrBelow(List<SampledModule> modules, ulong address)
    {
        var (low, high, found) = (0, modules.Count - 1, -1);
        while (low <= high)
        {
            var middle = low + ((high - low) >> 1);
            if (modules[middle].ImageBase <= address)
            {
                found = middle;
                low = middle + 1;
            }
            else
            {
                high = middle - 1;
            }
        }

        return found;
    }
}

// A sample held until its thread's process is known: its time on the trace's clocks.
internal readonly record struct HeldSample(long Qpc, ulong Address, double Milliseconds, long FileTime);

// The samples of the threads whose process isn't known yet, up to a capacity, and the count of those left out: the ones
// past the capacity, the ones before their thread's start, which were an earlier thread's, and the ones Clear drops.
internal sealed class HeldSamples(int capacity)
{
    private readonly Dictionary<int, List<HeldSample>> threads = [];

    // The samples held now.
    public int Count { get; private set; }

    // The samples left out so far.
    public long Unattributed { get; private set; }

    public void Hold(int threadId, HeldSample sample)
    {
        if (Count >= capacity)
        {
            Unattributed++;
            return;
        }

        if (!threads.TryGetValue(threadId, out var samples))
        {
            threads[threadId] = samples = [];
        }

        samples.Add(sample);
        Count++;
    }

    // Takes the samples held for a thread whose process is now known and returns, in the order they were held, those from
    // the given time that isTarget (given the process ID and the sample's time as a UTC FILETIME) collects. The ones before
    // the time were of an earlier thread with the ID, whose process is unknown.
    public IReadOnlyList<HeldSample> Release(int threadId, int processId, double fromMilliseconds,
        Func<int, long, bool> isTarget)
    {
        ArgumentNullException.ThrowIfNull(isTarget);
        if (processId < 0 || !threads.Remove(threadId, out var samples))
        {
            return [];
        }

        Count -= samples.Count;
        var released = new List<HeldSample>(samples.Count);
        foreach (var sample in samples)
        {
            if (sample.Milliseconds < fromMilliseconds)
            {
                Unattributed++;
            }
            else if (isTarget(processId, sample.FileTime))
            {
                released.Add(sample);
            }
        }

        return released;
    }

    // Leaves out every sample still held.
    public void Clear()
    {
        Unattributed += Count;
        threads.Clear();
        Count = 0;
    }
}
