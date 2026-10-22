using System.Text.Json.Serialization;

namespace WinUIProfiler.Contracts;

public enum SnapshotKind { Lightweight, NativeHeap, ManagedHeap, OwnershipGraph }
public enum SnapshotState { Pending, Capturing, Complete, Partial, Failed, Cancelled }
public enum CaptureMechanism { RuntimeBridge, ProcessSnapshot, ManagedRuntime, Polling, Synthetic }
public enum CaptureConsistency { NonAtomic, PerScopeAtomic, ProcessAtomic }
public enum CaptureScopeKind { Process, XamlRoot, Dispatcher, Heap }
public sealed record CaptureScope(CaptureScopeKind Kind, Guid? RootId, ulong? DispatcherId, string? HeapName);
public sealed record CaptureScopeBoundary(CaptureScope Scope, TimeRange Interval, DataQuality Quality);
public sealed record CaptureExtent(long StartedQpc, long? EndedQpc, CaptureMechanism Mechanism, CaptureConsistency Consistency, IReadOnlyList<CaptureScopeBoundary> Boundaries, string? ConsistencyEvidence = null)
{
    public CaptureExtent Freeze()
    {
        Require.Nonnegative(StartedQpc); Require.Enum(Mechanism); Require.Enum(Consistency); Require.NotNull(Boundaries);
        if (EndedQpc < StartedQpc || Boundaries.Count > 4096) throw new ArgumentException("Invalid capture extent or scope bound.");
        if (Consistency != CaptureConsistency.NonAtomic) Require.Text(ConsistencyEvidence);
        if (Mechanism is CaptureMechanism.RuntimeBridge or CaptureMechanism.Polling && Consistency == CaptureConsistency.ProcessAtomic)
            throw new ArgumentException("Dispatcher traversal/polling cannot claim process-atomic capture.");
        foreach (var boundary in Boundaries)
        {
            Require.NotNull(boundary); Require.NotNull(boundary.Quality);
            var scope = Require.NotNull(boundary.Scope);
            Require.Enum(scope.Kind);
            if (scope.RootId is { } root) Require.Id(root);
            if (scope.DispatcherId is 0) throw new ArgumentException("A dispatcher identity cannot be zero.");
            var valid = scope.Kind switch
            {
                CaptureScopeKind.Process => scope.RootId is null && scope.DispatcherId is null && scope.HeapName is null,
                CaptureScopeKind.XamlRoot => scope.RootId is not null && scope.DispatcherId is not null && scope.HeapName is null,
                CaptureScopeKind.Dispatcher => scope.RootId is null && scope.DispatcherId is not null && scope.HeapName is null,
                CaptureScopeKind.Heap => scope.RootId is null && scope.DispatcherId is null && !string.IsNullOrWhiteSpace(scope.HeapName),
                _ => false
            };
            if (!valid || boundary.Interval.StartQpc < StartedQpc || EndedQpc is { } end && boundary.Interval.EndQpc > end)
                throw new ArgumentException("Capture scope or interval does not match its explicit extent.");
        }
        return this with { Boundaries = Array.AsReadOnly(Boundaries.ToArray()) };
    }
}
public sealed record SnapshotArtifact(string RelativePath, string Sha256)
{
    public void Validate()
    {
        Require.Text(RelativePath);
        if (RelativePath.StartsWith('\\') || RelativePath.StartsWith('/') || RelativePath.IndexOfAny([':', '*', '?', '"', '<', '>', '|', '\0']) >= 0 ||
            RelativePath.Split(['\\', '/']).Any(part => part is "" or "." or ".." || part.EndsWith(' ') || part.EndsWith('.')))
            throw new ArgumentException("Artifact path must be a validated local relative path.");
        if (Sha256 is not { Length: 64 } || !Sha256.All(Uri.IsHexDigit))
            throw new ArgumentException("Existing artifact requires a hexadecimal SHA-256 digest.");
    }
}
public sealed record SnapshotMetadata : IQueryValue
{
    public Guid SnapshotId { get; }
    public Guid SessionId { get; }
    public ProcessInstanceKey Process { get; }
    public SnapshotKind Kind { get; }
    public CaptureExtent Extent { get; }
    public long CommittedOrdinal { get; }
    public Guid? ProducerEpoch { get; }
    public SnapshotState State { get; }
    public DataQuality Quality { get; }
    public SnapshotArtifact? Artifact { get; }
    public string? StatusReason { get; }
    public int SchemaVersion { get; }
    public string? SourceVersion { get; }
    public long QpcFrequency { get; }
    [JsonConstructor]
    public SnapshotMetadata(Guid snapshotId, Guid sessionId, ProcessInstanceKey process, SnapshotKind kind,
        CaptureExtent extent, long committedOrdinal, Guid? producerEpoch, SnapshotState state, DataQuality quality,
        SnapshotArtifact? artifact, string? statusReason, int schemaVersion, string? sourceVersion, long qpcFrequency)
    {
        SnapshotId = Require.Id(snapshotId); SessionId = Require.Id(sessionId); Process = Require.Process(process); Kind = Require.Enum(kind);
        Extent = Require.NotNull(extent).Freeze(); CommittedOrdinal = Require.Nonnegative(committedOrdinal);
        ProducerEpoch = producerEpoch is { } epoch ? Require.Id(epoch) : null;
        State = Require.Enum(state); Quality = Require.NotNull(quality);
        if (schemaVersion < 1) throw new ArgumentOutOfRangeException(nameof(schemaVersion));
        Require.Positive(qpcFrequency);
        if (sourceVersion is not null) Require.Text(sourceVersion);
        artifact?.Validate();
        if (state is SnapshotState.Complete or SnapshotState.Partial)
        {
            if (artifact is null || Extent.EndedQpc is null || producerEpoch is null) throw new ArgumentException("Materialized snapshots require an artifact, known capture end and producer epoch.");
            Require.Text(sourceVersion);
        }
        if (state is SnapshotState.Pending or SnapshotState.Capturing && (artifact is not null || Extent.EndedQpc is not null))
            throw new ArgumentException("An unfinished attempt cannot claim a sealed artifact or completed extent.");
        if (state is SnapshotState.Failed or SnapshotState.Cancelled) Require.Text(statusReason);
        if (state == SnapshotState.Complete && quality.Coverage != CoverageState.Complete ||
            state == SnapshotState.Partial && quality.Coverage == CoverageState.Complete)
            throw new ArgumentException("Snapshot state and capture coverage disagree.");
        Artifact = artifact; StatusReason = statusReason; SchemaVersion = schemaVersion; SourceVersion = sourceVersion; QpcFrequency = qpcFrequency;
    }
}

public enum SnapshotAttribute { ObjectCount, ConnectedElementCount, CanonicalVisualCount, AllocatedBytes, RequestedBytes, ActualBytes }
public enum ComparisonGrouping { Session, Object, Type, Root, ResourceKind }
public sealed record SnapshotComparisonKey(SnapshotAttribute Attribute, ComparisonGrouping Grouping, ObjectKey? Object, string? TypeName, Guid? RootId, ResourceKind? ResourceKind);
public enum ComparisonCompatibility { Compatible, MissingBaseline, MissingCurrent, Incompatible }
public sealed record SnapshotDelta(Guid SnapshotId, SnapshotComparisonKey Key, TypedValue Baseline, TypedValue Current, TypedValue Delta, ComparisonCompatibility Compatibility, string? Reason) : IQueryValue;
