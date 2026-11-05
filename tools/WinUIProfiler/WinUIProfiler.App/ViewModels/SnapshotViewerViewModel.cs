using System.Collections.ObjectModel;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.ViewModels;

public enum SnapshotTreeKind { Xaml, Composition }
public sealed record SnapshotSelection(IReadOnlyList<ulong> XamlIds, IReadOnlyList<ulong> VisualIds);

public sealed class SnapshotTreeNode
{
    private static readonly JsonSerializerOptions DetailOptions = new() { WriteIndented = true };
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
    public string Label => Type[(Type.LastIndexOf('.') + 1)..] +
        (string.IsNullOrEmpty(Name) ? "" : $" ({Name})") + $" ({VisualAddress ?? ObjectAddress})";
    public string Details => JsonSerializer.Serialize(new
    {
        id = Id,
        type = Type,
        name = Name,
        objectAddress = ObjectAddress,
        visualAddress = VisualAddress,
        diagnosticsHandle = DiagnosticsHandle,
        properties = DetailProperties
    }, DetailOptions);
    public override string ToString() => Label;

    private JsonElement DetailProperties =>
        JsonSerializer.SerializeToElement(Properties.EnumerateObject()
            .Where(property => !(VisualAddress is null ?
                IsDefaultXamlProperty(property) : IsDefaultCompositionProperty(property)))
            .ToDictionary(property => property.Name, property => property.Value));

    private static bool IsDefaultXamlProperty(JsonProperty property) => property.Name switch
    {
        "opacity" => MatchesNumber(property.Value, 1),
        "visibility" => MatchesString(property.Value, "Visible"),
        "horizontalAlignment" or "verticalAlignment" => MatchesString(property.Value, "Stretch"),
        "name" or "text" => MatchesString(property.Value, ""),
        "background" => property.Value.ValueKind == JsonValueKind.Null,
        "margin" => IsZeroObject(property.Value, ["left", "top", "right", "bottom"]),
        "renderTransformOrigin" => IsZeroObject(property.Value, ["x", "y"]),
        "renderTransform" => IsDefaultXamlTransform(property.Value),
        _ => false
    };

    private static bool IsDefaultXamlTransform(JsonElement value) =>
        value.ValueKind == JsonValueKind.Null ||
        (value.ValueKind == JsonValueKind.Object &&
            value.EnumerateObject().Count() == 2 &&
            value.TryGetProperty("type", out var type) &&
            MatchesString(type, "Microsoft.UI.Xaml.Media.MatrixTransform") &&
            value.TryGetProperty("matrix", out var matrix) &&
            MatchesNumbers(matrix, [1, 0, 0, 1, 0, 0]));

    private static bool IsZeroObject(JsonElement value, ReadOnlySpan<string> names)
    {
        if (value.ValueKind != JsonValueKind.Object || value.EnumerateObject().Count() != names.Length) return false;
        foreach (var name in names)
            if (!value.TryGetProperty(name, out var component) || !MatchesNumber(component, 0)) return false;
        return true;
    }

    private static bool MatchesString(JsonElement value, string expected) =>
        value.ValueKind == JsonValueKind.String && value.GetString() == expected;

    private static bool IsDefaultCompositionProperty(JsonProperty property) => property.Name switch
    {
        "offset" or "centerPoint" => MatchesNumbers(property.Value, [0, 0, 0]),
        "size" => MatchesNumbers(property.Value, [0, 0]),
        "transformMatrix" => MatchesNumbers(property.Value, [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]),
        "opacity" => MatchesNumber(property.Value, 1),
        "isVisible" => property.Value.ValueKind == JsonValueKind.True,
        "scale" => MatchesNumbers(property.Value, [1, 1, 1]),
        "rotationAngle" => MatchesNumber(property.Value, 0),
        _ => false
    };

    private static bool MatchesNumber(JsonElement value, double expected) =>
        value.ValueKind == JsonValueKind.Number && value.TryGetDouble(out var number) && number == expected;

    private static bool MatchesNumbers(JsonElement value, ReadOnlySpan<double> expected)
    {
        if (value.ValueKind != JsonValueKind.Array || value.GetArrayLength() != expected.Length) return false;
        var index = 0;
        foreach (var component in value.EnumerateArray())
            if (!MatchesNumber(component, expected[index++])) return false;
        return true;
    }

    internal SnapshotTreeNode(ulong id, ulong rootId, ulong? parentId, int childIndex, string type,
        string? name, string address, string? visualAddress, string? diagnosticsHandle, JsonElement properties)
    {
        Id = id;
        RootId = rootId;
        ParentId = parentId;
        ChildIndex = childIndex;
        Type = type;
        Name = name;
        ObjectAddress = address;
        VisualAddress = visualAddress;
        DiagnosticsHandle = diagnosticsHandle;
        Properties = properties;
    }
}

public sealed class SnapshotTreeData
{
    public IReadOnlyDictionary<ulong, SnapshotTreeNode> Nodes { get; }
    public IReadOnlyList<SnapshotTreeNode> Roots { get; }

    internal SnapshotTreeData(IEnumerable<SnapshotTreeNode> source)
    {
        var nodes = source.ToDictionary(n => n.Id);
        foreach (var group in nodes.Values.Where(n => n.ParentId.HasValue).GroupBy(n => n.ParentId!.Value))
            nodes[group.Key].Children = Array.AsReadOnly(group.OrderBy(n => n.ChildIndex).ThenBy(n => n.Id).ToArray());
        Roots = Array.AsReadOnly(nodes.Values.Where(n => n.ParentId is null)
            .OrderBy(n => n.RootId).ThenBy(n => n.ChildIndex).ThenBy(n => n.Id).ToArray());
        Nodes = new ReadOnlyDictionary<ulong, SnapshotTreeNode>(nodes);
    }
}

public sealed class SnapshotViewerData
{
    public TreeSnapshotDocument Document { get; }
    public SnapshotTreeData Xaml { get; }
    public SnapshotTreeData Composition { get; }
    private readonly IReadOnlyDictionary<ulong, ulong[]> xamlToVisual;
    private readonly IReadOnlyDictionary<ulong, ulong[]> visualToXaml;

    private SnapshotViewerData(TreeSnapshotDocument document)
    {
        Document = document;
        Xaml = new(document.XamlNodes.Select(n => new SnapshotTreeNode(n.Id, n.RootId, n.ParentId, n.ChildIndex,
            n.Type, n.Name, n.ObjectAddress, null, n.DiagnosticsHandle, n.Properties)));
        Composition = new(document.CompositionNodes.Select(n => new SnapshotTreeNode(n.Id, n.RootId, n.ParentId,
            n.ChildIndex, n.Type, null, n.ObjectAddress, n.VisualAddress, null, n.Properties)));
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
        if (!Enum.IsDefined(source)) throw new ArgumentOutOfRangeException(nameof(source));
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
