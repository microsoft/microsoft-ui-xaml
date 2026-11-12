// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

    public bool Contains(ulong address) => address >= ImageBase && address - ImageBase < (uint)ImageSize;
}

// A frame of a sampled call stack, and the module whose code it's in, when one was loaded at its address.
internal readonly record struct CodeFrame(SampledModule? Module, ulong Address);

// A CPU sample of a target process's thread, with its call stack, leaf first.
internal sealed record CompletedSample(long Qpc, int ProcessId, int ThreadId, CodeFrame[] Frames);

// What the session writer stores about the sampling once the last sample is posted: the sampling interval, whether
// the trace had compressed stacks, and every module the trace loaded, so the identity read after a sample was stored,
// and the modules known only from the trace's end, reach the session file.
internal sealed record CpuSamplingSummary(double? IntervalMilliseconds, bool CompressedStacks, long Samples,
    IReadOnlyList<SampledModule> Modules);

// Collects the CPU samples of the target processes' threads, with their call stacks, from the kernel's sampled-profile
// and stack-walk events, as a WPR CPU profile or KernelSampleSession records them:
// - StackSampleMerger puts each sample's stack together from its fragments.
// - Each frame is placed in the module loaded at its address when the sample is complete: a kernel address in a driver,
//   another in an image of the sample's process. The kernel's image events give the modules, and a merged trace each
//   image's PDB identity, in events at the image event's time, as TraceEvent's TraceLog reads them.
// - A live session gets events in the order of the kernel's buffers rather than in time order, so with a reorder window
//   they wait until the session is that far past them, and are applied in time order. A trace file is read in time
//   order: with no window they're applied at once.
// Compressed stacks (stack keys) aren't read: their samples keep only their instruction pointer, and CompressedStacks
// is set. Not thread-safe: TraceEvent calls it on the thread that reads the trace; call Flush there, or once it's done.
internal sealed class StackSampleCollector
{
    private readonly Func<int, bool> isTarget;
    private readonly Action<CompletedSample> sink;
    private readonly StackSampleMerger merger;
    private readonly double windowMilliseconds;
    private readonly PriorityQueue<Action, (double Milliseconds, long Sequence)> waiting = new();
    // The process of each thread, from its start, for a sample whose event doesn't name it.
    private readonly Dictionary<int, int> threadProcesses = [];
    // The modules loaded now, in order of address: the kernel's, and each process's.
    private readonly List<SampledModule> kernelModules = [];
    private readonly Dictionary<int, List<SampledModule>> processModules = [];
    private readonly List<SampledModule> allModules = [];
    private long sequence;
    private double newestMilliseconds = double.MinValue;
    private int pointerSize = 8;
    // The latest image event's module and time, and the identity events not matched with an image event yet.
    private SampledModule? lastModule;
    private long lastModuleQpc;
    private (long Qpc, string Name, Guid Guid, int Age)? lastPdb;
    private (long Qpc, int TimeDateStamp)? lastImageId;

    // IsTarget says whether a process's samples are collected; it's called on the thread that reads the trace. Sink gets
    // each complete sample, on that thread or in Flush.
    public StackSampleCollector(ETWTraceEventSource source, Func<int, bool> isTarget, Action<CompletedSample> sink,
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
        kernel.ThreadStart += OnThreadStart;
        kernel.ThreadDCStart += OnThreadStart;
        kernel.ThreadStop += OnThreadStop;
        kernel.ProcessStop += OnProcessStop;
        kernel.ImageLoad += OnImageLoad;
        kernel.ImageDCStart += OnImageLoad;
        kernel.ImageUnload += data => OnImageEnd(data, unloaded: true);
        kernel.ImageDCStop += data => OnImageEnd(data, unloaded: false);
        kernel.PerfInfoSample += OnSample;
        kernel.StackWalkStack += OnStack;
        kernel.PerfInfoSetInterval += OnInterval;
        kernel.PerfInfoCollectionStart += OnInterval;
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

    public CpuSamplingSummary Summary => new(IntervalMilliseconds, CompressedStacks, Samples, allModules.ToArray());

    // Why a recorded trace's samples are missing or incomplete, stored with them; null when they are complete.
    public static string? TraceNote(CpuSamplingSummary summary) =>
        summary.Samples == 0
            ? "The trace has no CPU samples of the process. Record it with a CPU profile, such as WPR's GeneralProfile or CPU."
            : summary.CompressedStacks
                ? "The trace's stacks are compressed (stack keys), which aren't read: its samples have only the sampled function. Record without stack compression."
                : null;

    // Applies the events still waiting and gives the sink every sample left, with the stack it has.
    public void Flush()
    {
        while (waiting.TryDequeue(out var action, out _))
        {
            action();
        }

        merger.Flush();
    }

    private static long Qpc(TraceEvent data)
    {
#pragma warning disable CS0618 // The raw QPC timestamp is the clock the session's events are stored with.
        return data.TimeStampQPC;
#pragma warning restore CS0618
    }

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

    private void Apply(TraceEvent data, Action action)
    {
        if (windowMilliseconds <= 0)
        {
            action();
        }
        else
        {
            waiting.Enqueue(action, (data.TimeStampRelativeMSec, sequence++));
        }
    }

    private void OnThreadStart(ThreadTraceData data)
    {
        Note(data);
        threadProcesses[data.ThreadID] = data.ProcessID;
    }

    private void OnThreadStop(ThreadTraceData data)
    {
        Note(data);
        var threadId = data.ThreadID;
        Apply(data, () =>
        {
            threadProcesses.Remove(threadId);
            merger.OnThreadEnd(threadId);
        });
    }

    private void OnProcessStop(ProcessTraceData data)
    {
        Note(data);
        var processId = data.ProcessID;
        Apply(data, () =>
        {
            merger.OnProcessEnd(processId);
            processModules.Remove(processId);
        });
    }

    private void OnImageLoad(ImageLoadTraceData data)
    {
        Note(data);
        var kernelImage = StackSampleMerger.IsKernelAddress(data.ImageBase, data.PointerSize);
        var module = new SampledModule(kernelImage ? -1 : data.ProcessID, data.FileName, data.ImageBase, data.ImageSize,
            data.TimeDateStamp);
        allModules.Add(module);
        Identify(module, Qpc(data));
        Apply(data, () => Insert(kernelImage ? kernelModules : ModulesOf(module.ProcessId), module));
    }

    // An unload, or the image's rundown at the trace's end, which a trace that lost its start has for each image.
    private void OnImageEnd(ImageLoadTraceData data, bool unloaded)
    {
        Note(data);
        var imageBase = data.ImageBase;
        var kernelImage = StackSampleMerger.IsKernelAddress(imageBase, data.PointerSize);
        var processId = kernelImage ? -1 : data.ProcessID;
        var modules = kernelImage ? kernelModules : processModules.GetValueOrDefault(processId);
        var module = modules is null ? null : At(modules, imageBase);
        if (module is null)
        {
            module = new SampledModule(processId, data.FileName, imageBase, data.ImageSize, data.TimeDateStamp);
            allModules.Add(module);
        }

        Identify(module, Qpc(data));
        if (unloaded)
        {
            Apply(data, () =>
            {
                if ((kernelImage ? kernelModules : processModules.GetValueOrDefault(processId)) is { } loaded &&
                    At(loaded, imageBase) is { } current)
                {
                    loaded.Remove(current);
                }
            });
        }
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
        var threadId = data.ThreadID;
        var processId = data.ProcessID;
        if ((processId < 0 && !threadProcesses.TryGetValue(threadId, out processId)) || !isTarget(processId))
        {
            return;
        }

        var qpc = Qpc(data);
        var address = (ulong)data.InstructionPointer;
        Apply(data, () => merger.OnSample(qpc, processId, threadId, address));
    }

    private void OnStack(StackWalkStackTraceData data)
    {
        Note(data);
        var processId = data.ProcessID;
        if (!isTarget(processId))
        {
            return;
        }

        var threadId = data.ThreadID;
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

    private void OnInterval(SampledProfileIntervalTraceData data)
    {
        if (data.SampleSource == 0 && data.NewInterval > 0)
        {
            IntervalMilliseconds = data.NewInterval / 10_000.0;
        }
    }

    private void Emit(MergedSample sample)
    {
        var modules = processModules.GetValueOrDefault(sample.ProcessId);
        var frames = new CodeFrame[sample.Frames.Length];
        for (var index = 0; index < frames.Length; index++)
        {
            var address = sample.Frames[index];
            var loaded = StackSampleMerger.IsKernelAddress(address, pointerSize) ? kernelModules : modules;
            frames[index] = new(loaded is null ? null : Containing(loaded, address), address);
        }

        Samples++;
        sink(new(sample.Qpc, sample.ProcessId, sample.ThreadId, frames));
    }

    private List<SampledModule> ModulesOf(int processId)
    {
        if (!processModules.TryGetValue(processId, out var modules))
        {
            processModules[processId] = modules = [];
        }

        return modules;
    }

    // Adds the module in order of address, in place of those it overlaps: they were unloaded.
    private static void Insert(List<SampledModule> modules, SampledModule module)
    {
        var index = LastAtOrBelow(modules, module.ImageBase);
        if (index >= 0 && modules[index].Contains(module.ImageBase))
        {
            modules.RemoveAt(index);
        }
        else
        {
            index++;
        }

        while (index < modules.Count && module.Contains(modules[index].ImageBase))
        {
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
