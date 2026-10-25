using System.Collections.ObjectModel;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.ViewModels;

public enum SnapshotTreeKind { Xaml, Composition }
public sealed record SnapshotVisualTypeCount(string Type, int Count);
public sealed record SnapshotSelection(IReadOnlyList<ulong> XamlIds, IReadOnlyList<ulong> VisualIds);

public sealed class SnapshotTreeNode
{
    public ulong Id { get; }
    public ulong RootId { get; }
    public ulong? ParentId { get; }
    public int ChildIndex { get; }
    public string Type { get; }
    public string? Name { get; }
    public string ObjectAddress { get; }
    public string? VisualAddress { get; }
    public string? DiagnosticsHandle { get; }
    public JsonElement Properties { get; }
    public IReadOnlyList<SnapshotTreeNode> Children { get; internal set; } = [];
    public string Label => $"#{Id} {Type[(Type.LastIndexOf('.') + 1)..]}" + (string.IsNullOrEmpty(Name) ? "" : $" ({Name})");
    public override string ToString() => Label;
    public string Details => $"{Type}\nObject: {ObjectAddress}" +
        (VisualAddress is null ? "" : $"\nVisual interface: {VisualAddress}") +
        (DiagnosticsHandle is null ? "" : $"\nDiagnostics handle: {DiagnosticsHandle}") +
        "\n" + Properties.GetRawText();

    internal SnapshotTreeNode(ulong id, ulong rootId, ulong? parentId, int childIndex, string type,
        string? name, string address, string? visualAddress, string? diagnosticsHandle, JsonElement properties)
    {
        Id = id; RootId = rootId; ParentId = parentId; ChildIndex = childIndex; Type = type;
        Name = name; ObjectAddress = address; VisualAddress = visualAddress;
        DiagnosticsHandle = diagnosticsHandle; Properties = properties;
    }
}

public sealed class SnapshotTreeData
{
    public IReadOnlyDictionary<ulong, SnapshotTreeNode> Nodes { get; }
    public IReadOnlyList<SnapshotTreeNode> Roots { get; }
    public int MaximumDepth { get; }

    internal SnapshotTreeData(IEnumerable<SnapshotTreeNode> source)
    {
        var nodes = source.ToDictionary(n => n.Id);
        foreach (var group in nodes.Values.Where(n => n.ParentId.HasValue).GroupBy(n => n.ParentId!.Value))
            nodes[group.Key].Children = Array.AsReadOnly(group.OrderBy(n => n.ChildIndex).ThenBy(n => n.Id).ToArray());
        Roots = Array.AsReadOnly(nodes.Values.Where(n => n.ParentId is null)
            .OrderBy(n => n.RootId).ThenBy(n => n.ChildIndex).ThenBy(n => n.Id).ToArray());
        Nodes = new ReadOnlyDictionary<ulong, SnapshotTreeNode>(nodes);
        var pending = new Stack<(SnapshotTreeNode Node, int Depth)>(Roots.Select(n => (n, 1)));
        while (pending.TryPop(out var current))
        {
            MaximumDepth = Math.Max(MaximumDepth, current.Depth);
            foreach (var child in current.Node.Children) pending.Push((child, current.Depth + 1));
        }
    }
}

public sealed class SnapshotViewerData
{
    public TreeSnapshotDocument Document { get; }
    public SnapshotTreeData Xaml { get; }
    public SnapshotTreeData Composition { get; }
    public IReadOnlyList<SnapshotVisualTypeCount> VisualTypes { get; }
    private readonly IReadOnlyDictionary<ulong, ulong[]> xamlToVisual;
    private readonly IReadOnlyDictionary<ulong, ulong[]> visualToXaml;

    private SnapshotViewerData(TreeSnapshotDocument document)
    {
        Document = document;
        Xaml = new(document.XamlNodes.Select(n => new SnapshotTreeNode(n.Id, n.RootId, n.ParentId, n.ChildIndex,
            n.Type, n.Name, n.ObjectAddress, null, n.DiagnosticsHandle, n.Properties)));
        Composition = new(document.CompositionNodes.Select(n => new SnapshotTreeNode(n.Id, n.RootId, n.ParentId,
            n.ChildIndex, n.Type, null, n.ObjectAddress, n.VisualAddress, null, n.Properties)));
        VisualTypes = Array.AsReadOnly(document.CompositionNodes.GroupBy(n => n.Type, StringComparer.Ordinal)
            .Select(g => new SnapshotVisualTypeCount(g.Key, g.Count()))
            .OrderByDescending(g => g.Count).ThenBy(g => g.Type, StringComparer.Ordinal).ToArray());
        xamlToVisual = document.Correlations.GroupBy(c => c.XamlNodeId)
            .ToDictionary(g => g.Key, g => g.Select(c => c.VisualNodeId).Order().ToArray());
        visualToXaml = document.Correlations.GroupBy(c => c.VisualNodeId)
            .ToDictionary(g => g.Key, g => g.Select(c => c.XamlNodeId).Order().ToArray());
    }

    public static async Task<SnapshotViewerData> LoadAsync(SnapshotHistoryEntry entry, CancellationToken token = default)
    {
        ArgumentNullException.ThrowIfNull(entry);
        await using var file = new FileStream(entry.Saved.Path, FileMode.Open, FileAccess.Read, FileShare.Read,
            64 * 1024, FileOptions.Asynchronous | FileOptions.SequentialScan);
        if (file.Length is <= 0 or > TapProtocol.MaximumSnapshotBytes)
            throw new InvalidDataException("Saved snapshot exceeds its bounded JSON size.");
        var bytes = new byte[checked((int)file.Length)];
        await file.ReadExactlyAsync(bytes, token).ConfigureAwait(false);
        token.ThrowIfCancellationRequested();
        var document = TreeSnapshotCodec.Decode(bytes, entry.Process, entry.Saved.CaptureId);
        var data = new SnapshotViewerData(document);
        token.ThrowIfCancellationRequested();
        return data;
    }

    public SnapshotSelection SelectNodes(SnapshotTreeKind source, IEnumerable<ulong> nodeIds)
    {
        Require.Enum(source);
        ArgumentNullException.ThrowIfNull(nodeIds);
        var ids = nodeIds.Distinct().Order().ToArray();
        var nodes = source == SnapshotTreeKind.Xaml ? Xaml.Nodes : Composition.Nodes;
        var links = source == SnapshotTreeKind.Xaml ? xamlToVisual : visualToXaml;
        if (ids.Any(id => !nodes.ContainsKey(id)))
            throw new ArgumentException("The selected node does not belong to this snapshot tree.", nameof(nodeIds));
        var matches = ids.SelectMany(id => links.TryGetValue(id, out var mapped) ? mapped : [])
            .Distinct().Order().ToArray();
        return source == SnapshotTreeKind.Xaml
            ? new(Array.AsReadOnly(ids), Array.AsReadOnly(matches))
            : new(Array.AsReadOnly(matches), Array.AsReadOnly(ids));
    }
}
