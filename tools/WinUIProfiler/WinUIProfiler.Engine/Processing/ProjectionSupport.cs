using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Processing;

public interface IVerifiedBaselineProcessor : IEventProcessor
{
    // The owner calls this only after its coordinator has validated Begin/chunks/End,
    // scoped cuts and continuity. End must be a source in the current writer transaction.
    IEnumerable<ProjectionChange> PublishBaseline(NormalizedEvent validatedEnd,
        IReadOnlyList<CaptureChunkPayload> chunks, ProcessingContext context);
}

internal sealed class ProjectionIndexBudget(int maximumEntries)
{
    private readonly int maximum = maximumEntries is >= 1 and <= 1_048_576
        ? maximumEntries : throw new ArgumentOutOfRangeException(nameof(maximumEntries));
    private int entries;
    private long bytes;
    internal void Add(object value)
    {
        var size = BatchAccounting.JsonBytes(value, ContractVersions.MaximumPayloadBytes);
        if (entries == maximum || bytes + size > 16 * 1024 * 1024) throw new ProcessingLimitException();
        entries++; bytes += size;
    }
    internal void Remove(object value)
    {
        entries--; bytes -= BatchAccounting.JsonBytes(value, ContractVersions.MaximumPayloadBytes);
    }
}

internal static class ProjectionSupport
{
    internal static readonly XamlTypeClassification UnknownType =
        new(XamlObjectKind.Unknown, ClassificationEvidence.Unavailable, "No authoritative type classification observed.");
    internal static readonly ChangeCause UnknownCause = new(CauseKind.Unknown, null, "Producer did not report a causal attempt.");
    internal static DataQuality Partial(DataQuality quality, string reason) =>
        new(quality.Kind, quality.Coverage == CoverageState.Unavailable ? CoverageState.Unavailable : CoverageState.Partial,
            quality.Reason is null ? reason : quality.Reason + " " + reason);
    internal static DataQuality Combine(DataQuality first, DataQuality second) =>
        new((MeasurementKind)Math.Max((int)first.Kind, (int)second.Kind),
            (CoverageState)Math.Max((int)first.Coverage, (int)second.Coverage), first.Reason ?? second.Reason);
    internal static DataQuality Uncertain(string reason) => new(MeasurementKind.Unavailable, CoverageState.Partial, reason);
    internal static ProjectionChange Candidate(long ordinal, IQueryValue value, ProcessingContext context, bool deleted = false)
    {
        if (context.OutputBudget.RemainingRecords == 0) throw new ProcessingLimitException();
        _ = BatchAccounting.JsonBytes(value, (int)Math.Min(ProjectionBudget.MaximumRecordBytes,
            context.OutputBudget.RemainingBytes - BatchAccounting.ChangeHeaderBytes));
        return new(ordinal, value, deleted);
    }
    internal static void ValidateScope(NormalizedEvent source, ProcessingContext context)
    {
        if (source.Envelope.SessionId != context.Scope.SessionId || source.Envelope.Process != context.Scope.Process)
            throw new ArgumentException("Projection source belongs to another session or target.");
    }
    internal static long ContinuityFailures(HealthSnapshot health) =>
        health.Issues.Where(i => i.Issue is not (IngestionIssue.IdentityPartial or IngestionIssue.MissingMetadata)).Sum(i => i.Count);
    internal static void ValidatePublication(NormalizedEvent end, IReadOnlyList<CaptureChunkPayload> chunks,
        ProcessingContext context)
    {
        ValidateScope(end, context);
        if (end.Envelope.Identity.ProviderId != EventCatalog.RuntimeProviderId || end.Envelope.Identity.EventId != 13)
            throw new ArgumentException("Baseline publication requires its validated runtime End source.");
        var boundary = EventCodec.ReadPayload<CaptureBoundaryPayload>(end.Envelope);
        if (boundary.Kind != CaptureKind.Baseline || boundary.Phase != CapturePhase.End ||
            boundary.ChunkCount != chunks.Count || chunks.Count > 4096 || boundary.Quality.Coverage == CoverageState.Unavailable ||
            end.Envelope.IngestOrdinal > context.BatchBoundary.IngestOrdinal)
            throw new ArgumentException("Invalid staged baseline publication boundary.");
        long bytes = 0;
        for (var i = 0; i < chunks.Count; i++)
        {
            var chunk = chunks[i];
            bytes += BatchAccounting.JsonBytes(chunk, ContractVersions.MaximumPayloadBytes);
            if (bytes > 16 * 1024 * 1024) throw new ProcessingLimitException();
            if (chunk.CaptureId != boundary.CaptureId || chunk.ChunkIndex != i ||
                boundary.RootId is { } root && root != chunk.RootId ||
                boundary.DispatcherId is { } dispatcher && dispatcher != chunk.DispatcherId)
                throw new ArgumentException("Staged baseline chunk identity/order/scope mismatch.");
            EventValidation.Validate(chunk, context.Scope.Process, end.Stream);
        }
        ValidateCaptureGraph(chunks);
    }
    internal static void ValidateCaptureGraph(IReadOnlyList<CaptureChunkPayload> chunks)
    {
        var objects = chunks.SelectMany(c => c.Objects).ToArray();
        var keys = objects.Select(o => o.Key).ToHashSet();
        if (keys.Count != objects.Length) throw new ArgumentException("Duplicate staged object identity.");
        var edges = chunks.SelectMany(c => c.Tree).ToArray();
        if (edges.Select(e => (e.Child, e.Relationship, e.Membership)).Distinct().Count() != edges.Length ||
            edges.Any(e => !keys.Contains(e.Child) || e.Parent is { } p && !keys.Contains(p)))
            throw new ArgumentException("Duplicate or dangling staged membership.");
        foreach (var channel in edges.Where(e => e.Connected && e.Parent is not null).GroupBy(e => e.Membership))
        {
            var outgoing = channel.ToLookup(e => e.Child, e => e.Parent!.Value);
            var degree = new Dictionary<ObjectKey, int>();
            foreach (var edge in channel)
            {
                degree.TryAdd(edge.Child, 0);
                degree[edge.Parent!.Value] = degree.GetValueOrDefault(edge.Parent.Value) + 1;
            }
            var ready = new Queue<ObjectKey>(degree.Where(p => p.Value == 0).Select(p => p.Key));
            var visited = 0;
            while (ready.TryDequeue(out var node))
            {
                visited++;
                foreach (var parent in outgoing[node])
                    if (--degree[parent] == 0) ready.Enqueue(parent);
            }
            if (visited != degree.Count) throw new ArgumentException("A staged membership graph contains a cycle.");
        }
    }
}
