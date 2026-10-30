using System.Text.Json;
using System.Text.Json.Serialization;

namespace WinUIProfiler.Contracts;

public sealed record TreeSnapshotDocument(int SchemaVersion, Guid CaptureId, uint ProcessId,
    long CreationTimeFileTime, long QpcFrequency, long StartQpc, long EndQpc,
    IReadOnlyList<TreeSnapshotRoot> Roots, IReadOnlyList<XamlSnapshotNode> XamlNodes,
    IReadOnlyList<CompositionSnapshotNode> CompositionNodes, IReadOnlyList<TreeSnapshotCorrelation> Correlations,
    IReadOnlyList<string> Limitations, SnapshotResourceCapture? ResourceSnapshot = null)
{
    [JsonPropertyName("memorySnapshot")]
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public SnapshotResourceCapture? LegacyMemorySnapshot { get; init; }
}

public sealed record SnapshotResourceCapture(
    [property: JsonRequired] string Status,
    [property: JsonRequired] int HResult,
    [property: JsonRequired] long StartQpc,
    [property: JsonRequired] long EndQpc,
    [property: JsonRequired] XamlResourceSnapshot? Values);

public sealed record XamlResourceSnapshot(
    [property: JsonRequired] long AllocCount,
    [property: JsonRequired] long AllocSize,
    [property: JsonRequired] long AllocatedMemory,
    [property: JsonRequired] long OutstandingAllocCount,
    [property: JsonRequired] long OutstandingAllocSize,
    [property: JsonRequired] IReadOnlyList<GraphicsResourceSnapshot> Resources);

public sealed record GraphicsResourceSnapshot(
    [property: JsonRequired] int Index,
    [property: JsonRequired] ulong TotalCreated,
    [property: JsonRequired] ulong TotalDestroyed,
    [property: JsonRequired] ulong LiveCount);
public sealed record TreeSnapshotRoot(ulong Id, uint ThreadId, long StartQpc, long EndQpc, string XamlRootAddress);
public sealed record XamlSnapshotNode(ulong Id, ulong RootId, ulong? ParentId, int ChildIndex,
    string Type, string? Name, string ObjectAddress, string? DiagnosticsHandle, JsonElement Properties);
public sealed record CompositionSnapshotNode(ulong Id, ulong RootId, ulong? ParentId, int ChildIndex,
    string Type, string ObjectAddress, string VisualAddress, JsonElement Properties);
public sealed record TreeSnapshotCorrelation(ulong XamlNodeId, ulong VisualNodeId);
public sealed record SavedTreeSnapshot(string Path, Guid CaptureId, int Roots, int XamlNodes,
    int CompositionNodes, int Correlations);
