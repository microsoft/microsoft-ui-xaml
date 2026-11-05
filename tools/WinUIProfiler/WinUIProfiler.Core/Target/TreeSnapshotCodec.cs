using System.Globalization;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Target;

public static class TreeSnapshotCodec
{
    public const int MaximumRecords = 131072;
    private static readonly JsonSerializerOptions Options = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow,
        MaxDepth = 16
    };

    internal static Task WriteAsync(Stream destination, TreeSnapshotDocument snapshot, CancellationToken token) =>
        JsonSerializer.SerializeAsync(destination, snapshot, Options, token);

    public static TreeSnapshotDocument Decode(ReadOnlyMemory<byte> bytes, ProcessInstanceKey target, Guid captureId)
    {
        target.Validate();
        Check(bytes.Length is > 0 and <= TapProtocol.MaximumSnapshotBytes, "Snapshot size is invalid.");
        var value = JsonSerializer.Deserialize<TreeSnapshotDocument>(bytes.Span, Options)
            ?? throw new InvalidDataException("Snapshot document is null.");
        Check(value.SchemaVersion is 1 or 2 or 3 or 4 && value.CaptureId != Guid.Empty && value.CaptureId == captureId,
            "Snapshot capture/schema identity disagrees with its request.");
        Check(target.CreationTimeFileTime.HasValue && value.ProcessId == target.ProcessId &&
            value.CreationTimeFileTime == target.CreationTimeFileTime, "Snapshot belongs to a different process instance.");
        Check(value.QpcFrequency > 0 && value.StartQpc >= 0 && value.EndQpc >= value.StartQpc,
            "Snapshot clock interval is invalid.");
        if (value.SchemaVersion == 2)
        {
            Check(value.ResourceSnapshot is null && value.LegacyMemorySnapshot is not null,
                "Legacy memory metadata disagrees with its schema.");
            value = value with { SchemaVersion = 3, ResourceSnapshot = value.LegacyMemorySnapshot,
                LegacyMemorySnapshot = null };
        }
        Check(value.LegacyMemorySnapshot is null &&
            (value.SchemaVersion == 1 ? value.ResourceSnapshot is null : value.ResourceSnapshot is not null),
            "Snapshot resource metadata disagrees with its schema.");
        if (value.ResourceSnapshot is { } resources)
        {
            Check(resources.StartQpc >= value.StartQpc && resources.EndQpc >= resources.StartQpc &&
                resources.EndQpc <= value.EndQpc, "Resource snapshot interval is invalid.");
            Check(resources.Status switch
            {
                "available" => resources.HResult >= 0 && resources.Values is not null,
                "unavailable" => resources.HResult == unchecked((int)0x80004002) && resources.Values is null,
                "failed" => resources.HResult < 0 && resources.Values is null,
                _ => false
            }, "Resource snapshot status/result is inconsistent.");
            if (resources.Values is { } metrics)
            {
                var minimum = value.SchemaVersion >= 4 ? -1L : 0L;
                Check(metrics.AllocCount >= minimum && metrics.AllocSize >= minimum &&
                    metrics.OutstandingAllocCount >= minimum && metrics.OutstandingAllocSize >= minimum,
                    "Allocator snapshot counters must be nonnegative or use the schema-4 unavailable sentinel.");
                Check(metrics.AllocatedMemory >= minimum && metrics.Resources is { Count: <= 32 },
                    "Resource snapshot metrics/graphics counters are invalid.");
                var indices = new HashSet<int>();
                foreach (var resource in metrics.Resources!)
                {
                    Check(resource is not null && resource.Index is >= 0 and < 32 && indices.Add(resource.Index),
                        "Graphics resource slot is invalid or duplicated.");
                }
                value = value with { ResourceSnapshot = resources with
                {
                    Values = metrics with { Resources = Array.AsReadOnly(metrics.Resources.ToArray()) }
                }};
            }
        }
        Check(value.Roots is { Count: > 0 and <= 1024 } && value.XamlNodes is { Count: > 0 } &&
            value.CompositionNodes is { Count: > 0 } && value.Correlations is { Count: > 0 } &&
            value.Limitations is not null, "Snapshot requires actual roots, both trees and object correlations.");
        var roots = value.Roots!;
        var xaml = value.XamlNodes!;
        var composition = value.CompositionNodes!;
        var correlations = value.Correlations!;
        Check((long)roots.Count + xaml.Count + composition.Count + correlations.Count <= MaximumRecords,
            "Snapshot record bound exceeded.");
        Check(xaml.All(n => n is not null) && composition.All(n => n is not null),
            "Snapshot node arrays contain null records.");
        var rootIds = new HashSet<ulong>();
        foreach (var root in roots)
        {
            Check(root is not null && root.Id != 0 && rootIds.Add(root.Id) && root.ThreadId != 0 &&
                root.StartQpc >= value.StartQpc && root.EndQpc >= root.StartQpc && root.EndQpc <= value.EndQpc,
                "Snapshot root identity/interval is invalid.");
            Address(root!.XamlRootAddress);
        }
        var xamlIds = Nodes(xaml.Select(n => (n.Id, n.RootId, n.ParentId, n.ChildIndex, n.Type, n.ObjectAddress)), rootIds);
        var visualIds = Nodes(composition.Select(n => (n.Id, n.RootId, n.ParentId, n.ChildIndex, n.Type, n.ObjectAddress)), rootIds);
        foreach (var node in xaml)
        {
            if (node.Name is { } name) Text(name, true);
            if (node.DiagnosticsHandle is { } handle) Address(handle);
            Properties(node.Properties);
        }
        foreach (var node in composition) { Address(node.VisualAddress); Properties(node.Properties); }
        var links = new HashSet<(ulong, ulong)>();
        foreach (var link in correlations)
            Check(link is not null && xamlIds.TryGetValue(link.XamlNodeId, out var xamlNode) &&
                visualIds.TryGetValue(link.VisualNodeId, out var visualNode) && xamlNode.Root == visualNode.Root &&
                links.Add((link.XamlNodeId, link.VisualNodeId)), "Snapshot contains a duplicate or dangling object-to-visual link.");
        Check(value.Limitations!.Count <= 128, "Snapshot limitation metadata exceeds its bound.");
        foreach (var limitation in value.Limitations) Text(limitation);
        return value with
        {
            Roots = Array.AsReadOnly(roots.ToArray()),
            XamlNodes = Array.AsReadOnly(xaml.Select(n => n with { Properties = n.Properties.Clone() }).ToArray()),
            CompositionNodes = Array.AsReadOnly(composition.Select(n => n with { Properties = n.Properties.Clone() }).ToArray()),
            Correlations = Array.AsReadOnly(correlations.ToArray()),
            Limitations = Array.AsReadOnly(value.Limitations.ToArray())
        };
    }

    private static Dictionary<ulong, (ulong Root, ulong? Parent)> Nodes(
        IEnumerable<(ulong Id, ulong Root, ulong? Parent, int Index, string Type, string Address)> nodes,
        HashSet<ulong> roots)
    {
        var index = new Dictionary<ulong, (ulong Root, ulong? Parent)>();
        foreach (var node in nodes)
        {
            Check(node.Id != 0 && roots.Contains(node.Root) && node.Index >= 0 &&
                index.TryAdd(node.Id, (node.Root, node.Parent)), "Snapshot node identity/root/order is invalid.");
            Text(node.Type);
            Address(node.Address);
        }
        foreach (var (id, node) in index)
            if (node.Parent is { } parent)
                Check(parent != id && index.TryGetValue(parent, out var ancestor) && ancestor.Root == node.Root,
                    "Snapshot has a dangling/cross-root parent.");
        var done = new HashSet<ulong>();
        foreach (var id in index.Keys)
        {
            if (done.Contains(id)) continue;
            var path = new HashSet<ulong>();
            ulong? current = id;
            while (current is { } next && !done.Contains(next))
            {
                Check(path.Add(next), "Snapshot parent relationships contain a cycle.");
                current = index[next].Parent;
            }
            done.UnionWith(path);
        }
        return index;
    }

    private static void Properties(JsonElement value) =>
        Check(value.ValueKind == JsonValueKind.Object && Encoding.UTF8.GetByteCount(value.GetRawText()) <= 64 * 1024,
            "Snapshot node properties are invalid or oversized.");
    private static void Text(string value, bool allowEmpty = false) =>
        Check(value is not null && (allowEmpty || !string.IsNullOrWhiteSpace(value)) &&
            Encoding.UTF8.GetByteCount(value) <= 4096, "Snapshot text is missing or oversized.");
    private static void Address(string value) =>
        Check(value is not null && value.StartsWith("0x", StringComparison.OrdinalIgnoreCase) &&
            value.Length is > 2 and <= 18 &&
            ulong.TryParse(value.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var address) &&
            address != 0, "Snapshot object address metadata is invalid.");
    private static void Check(bool valid, string reason)
    {
        if (!valid) throw new InvalidDataException(reason);
    }
}
