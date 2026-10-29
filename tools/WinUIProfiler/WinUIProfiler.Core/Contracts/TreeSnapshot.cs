using System.Text.Json;

namespace WinUIProfiler.Contracts;

public sealed record TreeSnapshotDocument(int SchemaVersion, Guid CaptureId, uint ProcessId,
    long CreationTimeFileTime, long QpcFrequency, long StartQpc, long EndQpc,
    IReadOnlyList<TreeSnapshotRoot> Roots, IReadOnlyList<XamlSnapshotNode> XamlNodes,
    IReadOnlyList<CompositionSnapshotNode> CompositionNodes, IReadOnlyList<TreeSnapshotCorrelation> Correlations,
    IReadOnlyList<string> Limitations);
public sealed record TreeSnapshotRoot(ulong Id, uint ThreadId, long StartQpc, long EndQpc, string XamlRootAddress);
public sealed record XamlSnapshotNode(ulong Id, ulong RootId, ulong? ParentId, int ChildIndex,
    string Type, string? Name, string ObjectAddress, string? DiagnosticsHandle, JsonElement Properties);
public sealed record CompositionSnapshotNode(ulong Id, ulong RootId, ulong? ParentId, int ChildIndex,
    string Type, string ObjectAddress, string VisualAddress, JsonElement Properties);
public sealed record TreeSnapshotCorrelation(ulong XamlNodeId, ulong VisualNodeId);
public sealed record SavedTreeSnapshot(string Path, Guid CaptureId, int Roots, int XamlNodes,
    int CompositionNodes, int Correlations);
