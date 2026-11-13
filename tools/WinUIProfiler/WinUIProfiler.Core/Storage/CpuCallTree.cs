// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Storage;

// A sampled function (a frame: Module!Function, or Module!0xRva while Named is false) and its samples: Self is the samples
// it was the frame that ran, Total the samples it was anywhere on the stack, once per sample when it recursed. FrameIndex
// identifies it to CpuStacks.Callers and CpuStacks.Callees.
public sealed record CpuFunctionSamples(string Module, string Frame, int FrameIndex, bool Named, long Self, long Total);

// The call stacks of a session's CPU samples, of all its threads or of ThreadId, as call trees: from the stacks' root frames
// (TopDown), or around one function, up to its callers (Callers) or down to the functions it called (Callees). Samples counts
// every sample, including any whose stack was not stored.
public sealed class CpuStacks
{
    private readonly IReadOnlyList<(string Module, string Frame, bool Named)> frames;
    // The frame of each code address (CodeAddresses.AddressId) on the stacks.
    private readonly IReadOnlyDictionary<long, int> addressFrames;

    internal CpuStacks(CpuSamplingInfo sampling, int? threadId, IReadOnlyList<CpuThreadSamples> threads,
        IReadOnlyList<(string Module, string Frame, bool Named)> frames, int[][] stacks, long[] counts,
        IReadOnlyDictionary<long, int>? addressFrames = null)
    {
        Sampling = sampling;
        ThreadId = threadId;
        Threads = threads;
        this.frames = frames;
        this.addressFrames = addressFrames ?? new Dictionary<long, int>();
        Stacks = stacks;
        Counts = counts;
        Samples = counts.Sum();
    }

    public CpuSamplingInfo Sampling { get; }

    public int? ThreadId { get; }

    // Every sampled thread, most samples first, whatever ThreadId is.
    public IReadOnlyList<CpuThreadSamples> Threads { get; }

    public long Samples { get; }

    // Each stored stack from its root frame to the frame that ran, as frame indexes, and its samples.
    internal int[][] Stacks { get; }

    internal long[] Counts { get; }

    // The time samples stand for at the sampling interval; null when the interval isn't known.
    public double? Ms(long samples) => samples * Sampling.IntervalMs;

    internal (string Module, string Frame, bool Named) Frame(int frameIndex) => frames[frameIndex];

    // A code address of a frame (its lowest CodeAddresses.AddressId), which finds the frame again in a later read
    // (FrameOfAddress), where its index differs and, once its function is named, its text; null for an unknown frame.
    public long? AddressOf(int frameIndex)
    {
        long? lowest = null;
        foreach (var (address, frame) in addressFrames)
        {
            if (frame == frameIndex && (lowest is null || address < lowest))
            {
                lowest = address;
            }
        }

        return lowest;
    }

    // The frame (index) of a code address, by CodeAddresses.AddressId; null when no stored stack has it.
    public int? FrameOfAddress(long addressId) => addressFrames.TryGetValue(addressId, out var frame) ? frame : null;

    // The sampled functions, those that ran the most first (by Self, then Total).
    public IReadOnlyList<CpuFunctionSamples> Functions()
    {
        var self = new long[frames.Count];
        var total = new long[frames.Count];
        var onStack = new HashSet<int>();
        for (var stack = 0; stack < Stacks.Length; stack++)
        {
            if (Stacks[stack] is not [.., var leaf] path)
            {
                continue;
            }

            self[leaf] += Counts[stack];
            onStack.Clear();
            foreach (var frame in path)
            {
                if (onStack.Add(frame))
                {
                    total[frame] += Counts[stack];
                }
            }
        }

        return [.. Enumerable.Range(0, frames.Count)
            .Where(frame => total[frame] > 0)
            .OrderByDescending(frame => self[frame]).ThenByDescending(frame => total[frame])
            .ThenBy(frame => frames[frame].Frame, StringComparer.Ordinal)
            .Select(frame => new CpuFunctionSamples(frames[frame].Module, frames[frame].Frame, frame, frames[frame].Named,
                self[frame], total[frame]))];
    }

    // Every sample (the root, with FrameIndex -1 and no frame) by the root frames of their stacks, then by the functions
    // each called, down to the frames that ran.
    public CpuCallTreeNode TopDown() =>
        new(this, -1, [.. Enumerable.Range(0, Stacks.Length).Select(stack => (stack, -1))], 1, root: true);

    // The samples with the function on their stack (the root) by its callers, then by theirs, up to the stacks' root frames.
    // A recursive function's callers are those of its outermost call.
    public CpuCallTreeNode Callers(int frameIndex) => new(this, frameIndex, Outermost(frameIndex), -1, root: true);

    // The samples with the function on their stack (the root) by the functions its outermost call called, then by the
    // functions those called, down to the frames that ran.
    public CpuCallTreeNode Callees(int frameIndex) => new(this, frameIndex, Outermost(frameIndex), 1, root: true);

    private (int Stack, int Position)[] Outermost(int frameIndex)
    {
        ArgumentOutOfRangeException.ThrowIfNegative(frameIndex);
        ArgumentOutOfRangeException.ThrowIfGreaterThanOrEqual(frameIndex, frames.Count);
        var members = new List<(int Stack, int Position)>();
        for (var stack = 0; stack < Stacks.Length; stack++)
        {
            var position = Array.IndexOf(Stacks[stack], frameIndex);
            if (position >= 0)
            {
                members.Add((stack, position));
            }
        }

        return [.. members];
    }
}

// A node of a call tree: a function and the samples whose stacks reach it along the node's path from the root, then the
// functions the path goes on to (Children), most samples first. Children are found when first read. In a top-down or callees
// tree, Self is the samples where the node's function is the frame that ran, and its children have the rest. In a callers
// tree, the root's Self is all the samples its function ran in, and the callers have none.
public sealed class CpuCallTreeNode
{
    private readonly CpuStacks owner;
    // The stacks the node is on (indexes into owner.Stacks) and where its frame is in each.
    private readonly (int Stack, int Position)[] members;
    // 1 when the children are the functions the node's function called, -1 when they are its callers.
    private readonly int step;
    private IReadOnlyList<CpuCallTreeNode>? children;

    internal CpuCallTreeNode(CpuStacks owner, int frameIndex, (int Stack, int Position)[] members, int step, bool root)
    {
        this.owner = owner;
        this.members = members;
        this.step = step;
        FrameIndex = frameIndex;
        (Module, Frame, Named) = frameIndex < 0 ? (string.Empty, string.Empty, false) : owner.Frame(frameIndex);
        long samples = 0, self = 0;
        var hasChildren = false;
        foreach (var (stack, position) in members)
        {
            var path = owner.Stacks[stack];
            samples += owner.Counts[stack];
            var ran = step > 0 ? position == path.Length - 1 : path.Length > 0 && path[^1] == frameIndex;
            if (ran)
            {
                self += owner.Counts[stack];
            }

            hasChildren |= position + step >= 0 && position + step < path.Length;
        }

        Samples = samples;
        Self = step > 0 || root ? self : null;
        HasChildren = hasChildren;
    }

    public string Module { get; }

    public string Frame { get; }

    // The function's index for CpuStacks.Callers and CpuStacks.Callees; -1 for the root of all samples.
    public int FrameIndex { get; }

    public bool Named { get; }

    public long Samples { get; }

    // Null for a callers tree's callers.
    public long? Self { get; }

    public bool HasChildren { get; }

    public IReadOnlyList<CpuCallTreeNode> Children => LazyInitializer.EnsureInitialized(ref children, FindChildren);

    private IReadOnlyList<CpuCallTreeNode> FindChildren()
    {
        var groups = new Dictionary<int, List<(int Stack, int Position)>>();
        foreach (var (stack, position) in members)
        {
            var path = owner.Stacks[stack];
            var next = position + step;
            if (next < 0 || next >= path.Length)
            {
                continue;
            }

            if (!groups.TryGetValue(path[next], out var group))
            {
                groups[path[next]] = group = [];
            }

            group.Add((stack, next));
        }

        return [.. groups
            .Select(group => new CpuCallTreeNode(owner, group.Key, [.. group.Value], step, root: false))
            .OrderByDescending(node => node.Samples).ThenBy(node => node.Frame, StringComparer.Ordinal)];
    }
}
