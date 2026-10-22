using System.Text.Json.Serialization;

namespace WinUIProfiler.Contracts;

[method: JsonConstructor]
public readonly record struct SessionScope(Guid SessionId, ProcessInstanceKey Process)
{
    public Guid SessionId { get; } = Require.Id(SessionId);
    public ProcessInstanceKey Process { get; } = Require.Process(Process);
    public void Validate() { Require.Id(SessionId); Process.Validate(); }
}
[method: JsonConstructor]
public readonly record struct CommittedWatermark(long IngestOrdinal, long TimestampQpc)
{
    public long IngestOrdinal { get; } = Require.Nonnegative(IngestOrdinal);
    public long TimestampQpc { get; } = Require.Nonnegative(TimestampQpc);
}
public sealed record PageRequest(int Limit, long Offset = 0, CommittedWatermark? AtWatermark = null)
{
    public const int MaximumLimit = 4096;
}
public sealed record PageInfo
{
    public int Limit { get; }
    public long Offset { get; }
    public long? NextOffset { get; }
    public bool HasMore { get; }
    [JsonConstructor]
    public PageInfo(int limit, long offset, long? nextOffset, bool hasMore)
    {
        if (limit is < 1 or > PageRequest.MaximumLimit || offset < 0 || offset > long.MaxValue - limit ||
            hasMore != nextOffset.HasValue || nextOffset is { } next && next != offset + limit)
            throw new ArgumentException("Paging bounds/continuation are inconsistent.");
        Limit = limit; Offset = offset; NextOffset = nextOffset; HasMore = hasMore;
    }
}
public sealed record ResultContext(SessionScope Scope, CommittedWatermark Watermark, DataQuality Quality)
{
    public SessionScope Scope { get; } = ValidateScope(Scope);
    public DataQuality Quality { get; } = Require.NotNull(Quality);
    private static SessionScope ValidateScope(SessionScope scope) { scope.Validate(); return scope; }
}
public sealed record QueryResult<T>(ResultContext Context, T Value)
{
    public ResultContext Context { get; } = Require.NotNull(Context);
    public T Value { get; } = Value is not null ? Value : throw new ArgumentNullException(nameof(Value));
}
public sealed record QueryPage<T>
{
    public ResultContext Context { get; }
    public IReadOnlyList<T> Items { get; }
    public PageInfo Page { get; }
    [JsonConstructor]
    public QueryPage(ResultContext context, IReadOnlyList<T> items, PageInfo page)
    {
        Context = Require.NotNull(context); Page = Require.NotNull(page); Require.NotNull(items);
        if (items.Count > page.Limit || page.HasMore && items.Count != page.Limit || items.Any(item => item is null))
            throw new ArgumentException("Page item count does not match paging bounds.");
        Items = Array.AsReadOnly(items.ToArray());
    }
}
public sealed record GetSessionRequest(SessionScope Scope);
public sealed record GetMetricsRequest(SessionScope Scope, TimeRange Range, PageRequest Page, MetricKind? Metric = null);
public sealed record GetStartupTimelineRequest(SessionScope Scope, PageRequest Page);
public sealed record GetFramesRequest(SessionScope Scope, TimeRange Range, PageRequest Page);
public sealed record GetObjectHistoryRequest(SessionScope Scope, ObjectKey Object, TimeRange Range, PageRequest Page);
public sealed record GetXamlTreeRequest(SessionScope Scope, long AtQpc, PageRequest Page, ObjectKey? Root = null);
public sealed record GetCompositionTreeRequest(SessionScope Scope, long AtQpc, PageRequest Page, ObjectKey? Root = null);
public sealed record GetCorrelationsRequest(SessionScope Scope, ObjectKey Object, long AtQpc, PageRequest Page);
public sealed record GetResourceUsageRequest(SessionScope Scope, TimeRange Range, PageRequest Page, ObjectKey? Owner = null);
public sealed record GetSnapshotsRequest(SessionScope Scope, PageRequest Page, SnapshotKind? Kind = null);
public sealed record CompareSnapshotsRequest(SessionScope Scope, Guid BaselineId, IReadOnlyList<Guid> SnapshotIds, PageRequest Page);
public sealed record GetRetentionPathsRequest(SessionScope Scope, Guid SnapshotId, ObjectKey Object, int MaximumDepth, PageRequest Page);

public sealed record SessionInfo(SessionMode Mode, SessionStatus Status, long QpcFrequency, long? ProcessCreationQpc, IReadOnlyList<ProducerInfo> Producers, string? StopReason);
public sealed record MetricSample(long TimestampQpc, MetricKind Metric, TypedValue Value, TimeRange Interval, string Source, string SourceVersion);
public sealed record StartupEntry(string Phase, TimeRange Range, ulong? SubmittedFrameId, DataQuality Quality);
public enum FrameAttemptStatus { InProgress, Completed, Failed }
public sealed record FrameSubmission(ulong SubmittedFrameId, long SubmittedQpc, Guid ProducerEpoch);
public sealed record FrameInfo(ulong AttemptId, TimeRange Range, FrameAttemptStatus AttemptStatus, FrameSubmission? Submission, Guid RootId, ulong DispatcherId, int? HResult);
public sealed record ObjectHistoryEntry(ObjectKey Object, long TimestampQpc, LifetimeOperation Operation, string? TypeName, ulong? Address, DataQuality Quality);
public sealed record TreeEntry(ObjectKey Object, ObjectKey? Parent, RelationshipKind Relationship, int Order, bool Connected, string? TypeName, DataQuality Quality);
public sealed record CorrelationEntry(ObjectKey From, ObjectKey To, CorrelationKind Kind, TimeRange Validity, DataQuality Quality);
public sealed record ResourceUsageEntry(ObjectKey Resource, ObjectKey? Owner, Guid? BackingAllocationId, Guid DeviceEpoch, TypedValue Size, SizeProvenance Provenance, DataQuality Quality);

public interface IProfilerQueryService
{
    ValueTask<QueryResult<SessionInfo>> GetSession(GetSessionRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<MetricSample>> GetMetrics(GetMetricsRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<StartupEntry>> GetStartupTimeline(GetStartupTimelineRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<FrameInfo>> GetFrames(GetFramesRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<ObjectHistoryEntry>> GetObjectHistory(GetObjectHistoryRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<TreeEntry>> GetXamlTree(GetXamlTreeRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<TreeEntry>> GetCompositionTree(GetCompositionTreeRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<CorrelationEntry>> GetCorrelations(GetCorrelationsRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<ResourceUsageEntry>> GetResourceUsage(GetResourceUsageRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<SnapshotMetadata>> GetSnapshots(GetSnapshotsRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<SnapshotDelta>> CompareSnapshots(CompareSnapshotsRequest request, CancellationToken cancellationToken);
    ValueTask<QueryPage<RetentionPath>> GetRetentionPaths(GetRetentionPathsRequest request, CancellationToken cancellationToken);
}

public static class QueryValidation
{
    public static void Validate(object request, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        ArgumentNullException.ThrowIfNull(request);
        var (scope, page) = request switch
        {
            GetSessionRequest r => (r.Scope, (PageRequest?)null),
            GetMetricsRequest r => (r.Scope, r.Page),
            GetStartupTimelineRequest r => (r.Scope, r.Page),
            GetFramesRequest r => (r.Scope, r.Page),
            GetObjectHistoryRequest r => (r.Scope, r.Page),
            GetXamlTreeRequest r => (r.Scope, r.Page),
            GetCompositionTreeRequest r => (r.Scope, r.Page),
            GetCorrelationsRequest r => (r.Scope, r.Page),
            GetResourceUsageRequest r => (r.Scope, r.Page),
            GetSnapshotsRequest r => (r.Scope, r.Page),
            CompareSnapshotsRequest r => (r.Scope, r.Page),
            GetRetentionPathsRequest r => (r.Scope, r.Page),
            _ => throw new ArgumentException("Unknown typed query request.")
        };
        scope.Validate();
        if (request is not GetSessionRequest)
        {
            page = Require.NotNull(page);
            if (page.Limit is < 1 or > PageRequest.MaximumLimit || page.Offset < 0 ||
                page.Offset > long.MaxValue - page.Limit || page.Offset > 0 && page.AtWatermark is null)
                throw new ArgumentException("Page bounds are invalid; subsequent pages require the committed watermark.");
        }
        void Key(ObjectKey key)
        {
            key.Validate();
            if (key.Process != scope.Process) throw new ArgumentException("Query object belongs to a different process instance.");
        }
        switch (request)
        {
            case GetMetricsRequest r:
                if (r.Metric is { } metric) Require.Enum(metric);
                break;
            case GetObjectHistoryRequest r: Key(r.Object); break;
            case GetXamlTreeRequest r:
                Require.Nonnegative(r.AtQpc);
                if (r.Root is { } xamlRoot) { Key(xamlRoot); if (xamlRoot.Domain != IdentityDomain.XamlCore) throw new ArgumentException("Not a XAML root."); }
                break;
            case GetCompositionTreeRequest r:
                Require.Nonnegative(r.AtQpc);
                if (r.Root is { } compRoot) { Key(compRoot); if (compRoot.Domain is not (IdentityDomain.CompNode or IdentityDomain.CompositionVisual)) throw new ArgumentException("Not a composition root."); }
                break;
            case GetCorrelationsRequest r: Key(r.Object); Require.Nonnegative(r.AtQpc); break;
            case GetResourceUsageRequest r: if (r.Owner is { } owner) Key(owner); break;
            case GetSnapshotsRequest r: if (r.Kind is { } kind) Require.Enum(kind); break;
            case CompareSnapshotsRequest r:
                Require.Id(r.BaselineId); Require.NotNull(r.SnapshotIds);
                if (r.SnapshotIds.Count is < 1 or > 64 || r.SnapshotIds.Distinct().Count() != r.SnapshotIds.Count ||
                    r.SnapshotIds.Contains(r.BaselineId))
                    throw new ArgumentException("Compare 1–64 distinct snapshots against a separate baseline.");
                foreach (var id in r.SnapshotIds) Require.Id(id);
                break;
            case GetRetentionPathsRequest r:
                Key(r.Object); Require.Id(r.SnapshotId);
                if (r.MaximumDepth is < 1 or > 256) throw new ArgumentException("Retention depth must be 1–256.");
                break;
        }
    }
}
