using System.Collections.ObjectModel;
using System.Globalization;
using System.Text.Json;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.App.ViewModels;

[Flags]
public enum SnapshotChange { None = 0, Added = 1, Removed = 2, Properties = 4, Moved = 8, Reordered = 16 }

public sealed record SnapshotPropertyChange(string Name, JsonElement? Before, JsonElement? After);

public sealed record SnapshotResourceRow(string Metric, int? ResourceIndex, bool IsBytes, decimal? Before, decimal? After)
{
    public decimal? Delta => Before.HasValue && After.HasValue ? After.Value - Before.Value : null;
}

public sealed class SnapshotResourceComparison
{
    public IReadOnlyList<SnapshotResourceRow> Rows { get; }
    public bool HasChanges { get; }

    public SnapshotResourceComparison(SnapshotResourceCapture? before, SnapshotResourceCapture? after)
    {
        var old = before?.Values;
        var current = after?.Values;
        var rows = new List<SnapshotResourceRow>
        {
            new("AllocCount", null, false, old?.AllocCount, current?.AllocCount),
            new("AllocSize", null, true, old?.AllocSize, current?.AllocSize),
            new("AllocatedMemory", null, true, old?.AllocatedMemory, current?.AllocatedMemory),
            new("OutstandingAllocCount", null, false, old?.OutstandingAllocCount, current?.OutstandingAllocCount),
            new("OutstandingAllocSize", null, true, old?.OutstandingAllocSize, current?.OutstandingAllocSize)
        };
        var oldResources = old?.Resources.ToDictionary(resource => resource.Index) ?? [];
        var newResources = current?.Resources.ToDictionary(resource => resource.Index) ?? [];
        foreach (var index in oldResources.Keys.Union(newResources.Keys).Order())
        {
            oldResources.TryGetValue(index, out var previous);
            newResources.TryGetValue(index, out var next);
            rows.Add(new("TotalCreated", index, false, previous?.TotalCreated, next?.TotalCreated));
            rows.Add(new("TotalDestroyed", index, false, previous?.TotalDestroyed, next?.TotalDestroyed));
            rows.Add(new("LiveCount", index, false, previous?.LiveCount, next?.LiveCount));
        }
        Rows = rows.AsReadOnly();
        HasChanges = before?.Status != after?.Status || before?.HResult != after?.HResult ||
            rows.Any(row => row.Before != row.After);
    }
}

public sealed class SnapshotNodeDiff(SnapshotTreeNode? before, SnapshotTreeNode? after,
    IReadOnlyList<SnapshotPropertyChange> properties)
{
    public SnapshotTreeNode? Before { get; } = before;
    public SnapshotTreeNode? After { get; } = after;
    public IReadOnlyList<SnapshotPropertyChange> Properties { get; } = properties;
    public SnapshotChange Change { get; internal set; } = before is null ? SnapshotChange.Added :
        after is null ? SnapshotChange.Removed : properties.Count > 0 ? SnapshotChange.Properties : SnapshotChange.None;
    public bool ContainsChanges { get; internal set; }
    public SnapshotNodeDiff? Parent { get; internal set; }
    public IReadOnlyList<SnapshotNodeDiff> Children { get; internal set; } = [];
}

public sealed record SnapshotDiffMatch(SnapshotNodeDiff Node, bool Before, bool After);

public sealed record SnapshotRootDiff(TreeSnapshotRoot Root, IReadOnlyList<SnapshotNodeDiff> Children)
{
    public bool ContainsChanges => Children.Any(node => node.ContainsChanges);
}

public sealed class SnapshotTreeDiff
{
    public IReadOnlyList<SnapshotRootDiff> Roots { get; }
    public IReadOnlyList<SnapshotNodeDiff> Nodes { get; }
    public IReadOnlyDictionary<ulong, SnapshotNodeDiff> BeforeNodes { get; }
    public IReadOnlyDictionary<ulong, SnapshotNodeDiff> AfterNodes { get; }
    public int Count(SnapshotChange change) => Nodes.Count(node => (node.Change & change) != 0);

    internal SnapshotTreeDiff(IReadOnlyList<SnapshotRootDiff> roots, IReadOnlyList<SnapshotNodeDiff> nodes,
        Dictionary<ulong, SnapshotNodeDiff> beforeNodes, Dictionary<ulong, SnapshotNodeDiff> afterNodes)
    {
        Roots = roots;
        Nodes = nodes;
        BeforeNodes = new ReadOnlyDictionary<ulong, SnapshotNodeDiff>(beforeNodes);
        AfterNodes = new ReadOnlyDictionary<ulong, SnapshotNodeDiff>(afterNodes);
    }
}

public sealed class SnapshotDiffData
{
    private readonly record struct RootKey(uint ThreadId, ulong Address);
    private readonly record struct NodeKey(RootKey Root, ulong Address, string Type);
    public SnapshotViewerData Before { get; }
    public SnapshotViewerData After { get; }
    public SnapshotTreeDiff Xaml { get; }
    public SnapshotTreeDiff Composition { get; }
    public SnapshotResourceComparison Resources { get; }

    private SnapshotDiffData(SnapshotViewerData before, SnapshotViewerData after, CancellationToken token)
    {
        if (before.Document.ProcessId != after.Document.ProcessId ||
            before.Document.CreationTimeFileTime != after.Document.CreationTimeFileTime)
            throw new InvalidDataException("Snapshots must belong to the same process ID and creation time.");
        Before = before;
        After = after;
        Resources = new(before.Document.ResourceSnapshot, after.Document.ResourceSnapshot);
        Xaml = CompareTree(before.Xaml, after.Xaml, before.Document.Roots, after.Document.Roots, token);
        Composition = CompareTree(before.Composition, after.Composition, before.Document.Roots, after.Document.Roots, token);
    }

    public static bool CanCompare(SnapshotHistoryEntry before, SnapshotHistoryEntry after) =>
        before.Process.ProcessId == after.Process.ProcessId &&
        before.Process.CreationTimeFileTime is > 0 &&
        before.Process.CreationTimeFileTime == after.Process.CreationTimeFileTime;

    public static async Task<SnapshotDiffData> LoadAsync(SnapshotHistoryEntry before, SnapshotHistoryEntry after,
        CancellationToken token = default)
    {
        if (!CanCompare(before, after))
            throw new InvalidDataException("Snapshots must belong to the same process ID and creation time.");
        var baseline = await SnapshotViewerData.LoadAsync(before, token).ConfigureAwait(false);
        var comparison = await SnapshotViewerData.LoadAsync(after, token).ConfigureAwait(false);
        token.ThrowIfCancellationRequested();
        return new(baseline, comparison, token);
    }

    public IReadOnlyList<SnapshotDiffMatch> SelectMatches(SnapshotTreeKind source, SnapshotNodeDiff node)
    {
        if (!Enum.IsDefined(source)) throw new ArgumentOutOfRangeException(nameof(source));
        ArgumentNullException.ThrowIfNull(node);
        var sourceTree = source == SnapshotTreeKind.Xaml ? Xaml : Composition;
        if ((node.Before is null && node.After is null) ||
            (node.Before is { } oldNode && (!sourceTree.BeforeNodes.TryGetValue(oldNode.Id, out var oldDiff) ||
                !ReferenceEquals(oldDiff, node))) ||
            (node.After is { } newNode && (!sourceTree.AfterNodes.TryGetValue(newNode.Id, out var newDiff) ||
                !ReferenceEquals(newDiff, node))))
            throw new ArgumentException("The selected node does not belong to this comparison tree.", nameof(node));

        var target = source == SnapshotTreeKind.Xaml ? Composition : Xaml;
        var matches = new Dictionary<SnapshotNodeDiff, SnapshotDiffMatch>();
        AddMatches(Before, node.Before, target.BeforeNodes, true);
        AddMatches(After, node.After, target.AfterNodes, false);
        return Array.AsReadOnly(matches.Values.OrderBy(match => !match.After)
            .ThenBy(match => (match.Node.After ?? match.Node.Before!).RootId)
            .ThenBy(match => (match.Node.After ?? match.Node.Before!).Id).ToArray());

        void AddMatches(SnapshotViewerData capture, SnapshotTreeNode? selected,
            IReadOnlyDictionary<ulong, SnapshotNodeDiff> targets, bool baseline)
        {
            if (selected is null) return;
            var selection = capture.SelectNodes(source, [selected.Id]);
            foreach (var id in source == SnapshotTreeKind.Xaml ? selection.VisualIds : selection.XamlIds)
            {
                var match = targets[id];
                matches.TryGetValue(match, out var previous);
                matches[match] = new(match, baseline || previous?.Before == true, !baseline || previous?.After == true);
            }
        }
    }

    private static ulong Address(string value) =>
        ulong.Parse(value.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture);

    private static RootKey Key(TreeSnapshotRoot root) => new(root.ThreadId, Address(root.XamlRootAddress));

    private static SnapshotTreeDiff CompareTree(SnapshotTreeData before, SnapshotTreeData after,
        IReadOnlyList<TreeSnapshotRoot> beforeRoots, IReadOnlyList<TreeSnapshotRoot> afterRoots, CancellationToken token)
    {
        var oldRoots = beforeRoots.ToDictionary(root => root.Id, Key);
        var newRoots = afterRoots.ToDictionary(root => root.Id, Key);
        var roots = new Dictionary<RootKey, TreeSnapshotRoot>();
        foreach (var source in new[] { beforeRoots, afterRoots })
        {
            var seen = new HashSet<RootKey>();
            foreach (var root in source)
            {
                if (!seen.Add(Key(root)))
                    throw new InvalidDataException("Snapshot contains ambiguous XamlRoot identities.");
                roots[Key(root)] = root;
            }
        }

        static Dictionary<NodeKey, SnapshotTreeNode> Index(SnapshotTreeData tree,
            Dictionary<ulong, RootKey> rootKeys, CancellationToken token)
        {
            var result = new Dictionary<NodeKey, SnapshotTreeNode>();
            foreach (var node in tree.Nodes.Values)
            {
                token.ThrowIfCancellationRequested();
                if (!result.TryAdd(new(rootKeys[node.RootId], Address(node.ObjectAddress), node.Type), node))
                    throw new InvalidDataException("Snapshot contains ambiguous object identities within a root.");
            }
            return result;
        }

        var oldIndex = Index(before, oldRoots, token);
        var newIndex = Index(after, newRoots, token);
        var oldNodes = new Dictionary<ulong, SnapshotNodeDiff>();
        var newNodes = new Dictionary<ulong, SnapshotNodeDiff>();
        var differences = new List<SnapshotNodeDiff>();
        foreach (var (key, node) in newIndex)
        {
            token.ThrowIfCancellationRequested();
            oldIndex.TryGetValue(key, out var previous);
            var diff = new SnapshotNodeDiff(previous, node, CompareProperties(previous, node, token));
            if (previous is not null) oldNodes.Add(previous.Id, diff);
            newNodes.Add(node.Id, diff);
            differences.Add(diff);
        }
        foreach (var node in before.Nodes.Values)
        {
            token.ThrowIfCancellationRequested();
            if (oldNodes.ContainsKey(node.Id)) continue;
            var diff = new SnapshotNodeDiff(node, null, CompareProperties(node, null, token));
            oldNodes.Add(node.Id, diff);
            differences.Add(diff);
        }

        var matched = differences.Where(node => node.Before is not null && node.After is not null).ToArray();
        foreach (var node in matched)
        {
            token.ThrowIfCancellationRequested();
            var oldParent = node.Before!.ParentId is { } oldId ? oldNodes[oldId] : null;
            var newParent = node.After!.ParentId is { } newId ? newNodes[newId] : null;
            if (!ReferenceEquals(oldParent, newParent)) node.Change |= SnapshotChange.Moved;
        }
        // Compare surviving siblings, so insertions/removals do not make all later children look reordered.
        foreach (var group in matched.Where(node => node.After!.ParentId is not null &&
            !node.Change.HasFlag(SnapshotChange.Moved))
            .GroupBy(node => (newRoots[node.After!.RootId], node.After.ParentId)))
        {
            token.ThrowIfCancellationRequested();
            var oldOrder = group.OrderBy(node => node.Before!.ChildIndex).ThenBy(node => node.Before!.Id).ToArray();
            var newOrder = group.OrderBy(node => node.After!.ChildIndex).ThenBy(node => node.After!.Id).ToArray();
            for (var i = 0; i < oldOrder.Length; i++)
                if (!ReferenceEquals(oldOrder[i], newOrder[i])) newOrder[i].Change |= SnapshotChange.Reordered;
        }

        var children = new Dictionary<SnapshotNodeDiff, List<SnapshotNodeDiff>>();
        var rootChildren = roots.Keys.ToDictionary(key => key, _ => new List<SnapshotNodeDiff>());
        foreach (var diff in differences)
        {
            token.ThrowIfCancellationRequested();
            var node = diff.After ?? diff.Before!;
            var parents = diff.After is null ? oldNodes : newNodes;
            if (node.ParentId is { } id)
            {
                var parent = parents[id];
                diff.Parent = parent;
                if (!children.TryGetValue(parent, out var list)) children.Add(parent, list = []);
                list.Add(diff);
            }
            else rootChildren[(diff.After is null ? oldRoots : newRoots)[node.RootId]].Add(diff);
        }
        static IReadOnlyList<SnapshotNodeDiff> Ordered(List<SnapshotNodeDiff> nodes) =>
            Array.AsReadOnly(nodes.OrderBy(node => (node.After ?? node.Before!).ChildIndex)
                .ThenBy(node => node.After is null ? 0 : 1).ThenBy(node => (node.After ?? node.Before!).Id).ToArray());
        foreach (var pair in children) pair.Key.Children = Ordered(pair.Value);

        // Include both old and new ancestor paths when a node moves to a different parent.
        foreach (var side in new[] { oldNodes, newNodes })
        {
            var visited = new HashSet<ulong>();
            foreach (var diff in differences.Where(node => node.Change != SnapshotChange.None))
            {
                token.ThrowIfCancellationRequested();
                diff.ContainsChanges = true;
                var current = ReferenceEquals(side, oldNodes) ? diff.Before : diff.After;
                while (current?.ParentId is { } id && visited.Add(id))
                {
                    var parent = side[id];
                    parent.ContainsChanges = true;
                    current = ReferenceEquals(side, oldNodes) ? parent.Before : parent.After;
                }
            }
        }
        return new(Array.AsReadOnly(roots.OrderBy(pair => pair.Key.ThreadId).ThenBy(pair => pair.Key.Address)
            .Select(pair => new SnapshotRootDiff(pair.Value, Ordered(rootChildren[pair.Key]))).ToArray()),
            differences.AsReadOnly(), oldNodes, newNodes);
    }

    private static IReadOnlyList<SnapshotPropertyChange> CompareProperties(SnapshotTreeNode? before,
        SnapshotTreeNode? after, CancellationToken token)
    {
        var changes = new List<SnapshotPropertyChange>();
        if (before?.Name != after?.Name)
            changes.Add(new("name", before is null ? null : JsonSerializer.SerializeToElement(before.Name),
                after is null ? null : JsonSerializer.SerializeToElement(after.Name)));
        var oldValues = before?.Properties.EnumerateObject().ToDictionary(property => property.Name, property => property.Value) ?? [];
        var newValues = after?.Properties.EnumerateObject().ToDictionary(property => property.Name, property => property.Value) ?? [];
        foreach (var name in oldValues.Keys.Union(newValues.Keys).Order(StringComparer.Ordinal))
        {
            token.ThrowIfCancellationRequested();
            var hasOld = oldValues.TryGetValue(name, out var oldValue);
            var hasNew = newValues.TryGetValue(name, out var newValue);
            if (hasOld && hasNew && JsonElement.DeepEquals(oldValue, newValue)) continue;
            changes.Add(new("properties." + name, hasOld ? oldValue : null, hasNew ? newValue : null));
        }
        return changes.AsReadOnly();
    }
}
