using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Target;

public sealed record BaselineDomainEvidence(ListenerDomainInterval Interval,
    SequenceReceiptEvidence? Receipt = null, SequenceDeliveryClosure? Delivery = null);
public sealed record BaselinePreparation(SequenceContinuityState State, PreparedBaseline? Capture, string? Reason);
public sealed record PreparedBaseline(NormalizedEvent Begin, NormalizedEvent End,
    IReadOnlyList<CaptureChunkPayload> Chunks, IReadOnlyList<NormalizedEvent> BufferedDeltas);

public sealed class BaselineCoordinator
{
    private const int MaximumSources = 8192;
    private const int MaximumChunks = 4096;
    private const long MaximumBytes = 16 * 1024 * 1024;
    private readonly NormalizedEvent begin;
    private readonly CaptureBoundaryPayload boundary;
    private readonly Dictionary<SequenceDomainKey, SequenceDomainRange> domains;
    private readonly SortedDictionary<uint, NormalizedEvent> chunks = [];
    private readonly Dictionary<(SequenceDomainKey Domain, ulong Sequence), (NormalizedEvent Source, SequenceCheckpoint Checkpoint)> observations = [];
    private readonly HashSet<long> receiptOrdinals = [];
    private readonly Dictionary<(SequenceDomainKey Domain, ulong Sequence), NormalizedEvent> sourcesBySequence = [];
    private NormalizedEvent? end;
    private string? failure;
    private long bytes;
    private int sources;

    public BaselineCoordinator(NormalizedEvent begin)
    {
        this.begin = EventCodec.Normalize(Require.NotNull(begin).Envelope, begin.Stream);
        boundary = ReadBoundary(this.begin, CapturePhase.Begin);
        if (boundary.RootId is not null && boundary.DispatcherId is null ||
            boundary.Quality.Coverage == CoverageState.Unavailable || boundary.Domains.Count == 0)
            throw Invalid("Baseline Begin lacks an eligible capture scope or domain vector.");
        domains = boundary.Domains.ToDictionary(d => d.Domain);
        Reserve(this.begin);
    }

    public void StageChunk(NormalizedEvent source)
    {
        Check();
        var owned = Own(source);
        if (owned.Envelope.Identity.EventId != 14)
            throw Invalid("Expected a runtime capture chunk.");
        var chunk = EventCodec.ReadPayload<CaptureChunkPayload>(owned.Envelope);
        if (chunk.CaptureId != boundary.CaptureId || chunk.ChunkIndex >= MaximumChunks ||
            chunks.ContainsKey(chunk.ChunkIndex) ||
            owned.Envelope.TimestampQpc < begin.Envelope.TimestampQpc ||
            end is not null && owned.Envelope.TimestampQpc > end.Envelope.TimestampQpc ||
            boundary.RootId is { } root && chunk.RootId != root ||
            boundary.DispatcherId is { } dispatcher && chunk.DispatcherId != dispatcher)
            throw Invalid("Capture chunk has a foreign capture/scope, duplicate index or excessive index.");
        if (end is not null && chunk.ChunkIndex >= ReadBoundary(end, CapturePhase.End).ChunkCount)
            throw Invalid("Capture chunk lies beyond the declared End count.");
        Reserve(owned);
        chunks.Add(chunk.ChunkIndex, owned);
    }

    public void StageEnd(NormalizedEvent source)
    {
        Check();
        var owned = Own(source);
        var terminal = ReadBoundary(owned, CapturePhase.End);
        if (end is not null || terminal.CaptureId != boundary.CaptureId ||
            terminal.RootId != boundary.RootId || terminal.DispatcherId != boundary.DispatcherId ||
            owned.Envelope.TimestampQpc < begin.Envelope.TimestampQpc || terminal.ChunkCount > MaximumChunks ||
            terminal.Quality.Coverage == CoverageState.Unavailable || terminal.Domains.Count != domains.Count ||
            terminal.Domains.Any(d => !domains.TryGetValue(d.Domain, out var start) || start.BeforeSequence != d.BeforeSequence) ||
            chunks.Values.Any(chunk => chunk.Envelope.TimestampQpc > owned.Envelope.TimestampQpc) ||
            chunks.Keys.Any(index => index >= terminal.ChunkCount))
            throw Invalid("Capture End does not match Begin's identity, scope, time, vector or chunk count.");
        Reserve(owned);
        end = owned;
    }

    public void Observe(NormalizedEvent source, SequenceCheckpoint checkpoint)
    {
        Check();
        var owned = Own(source);
        Require.NotNull(checkpoint);
        if (checkpoint.Domain != owned.SequenceDomain || checkpoint.Sequence != owned.Envelope.SourceSequence ||
            checkpoint.TimestampQpc != owned.Envelope.TimestampQpc || checkpoint.IngestOrdinal != owned.Envelope.IngestOrdinal ||
            checkpoint.ListenerId == Guid.Empty || checkpoint.ContinuityId == Guid.Empty)
            throw Invalid("Receipt checkpoint does not identify its actual source observation.");
        if (owned.Envelope.Identity.EventId == 15)
            throw Invalid("A producer loss observation interrupted baseline continuity.");
        if (!domains.TryGetValue(owned.SequenceDomain, out var range) || range.BeforeSequence is null ||
            checkpoint.Sequence <= range.BeforeSequence)
            return;
        var key = (owned.SequenceDomain, checkpoint.Sequence);
        if (observations.TryGetValue(key, out var previous))
        {
            if (!SameObservation(previous.Source, owned))
                throw Invalid("Conflicting payloads reuse one producer-domain sequence.");
            return;
        }
        if (!receiptOrdinals.Add(checkpoint.IngestOrdinal))
            throw Invalid("Different processed receipts reuse a canonical ingestion ordinal.");
        Reserve(owned, allowStagedReceipt: true);
        observations.Add(key, (owned, checkpoint));
    }

    public BaselinePreparation Prepare(IReadOnlyList<BaselineDomainEvidence> evidence)
    {
        Check();
        Require.NotNull(evidence);
        if (evidence.Count > domains.Count || evidence.Any(e => e is null || e.Interval is null) ||
            evidence.Select(e => e.Interval.Domain).Distinct().Count() != evidence.Count ||
            evidence.Any(e => !domains.ContainsKey(e.Interval.Domain)))
            throw Invalid("Capture evidence contains duplicate or foreign delivery domains.");
        if (end is null) return Pending("Capture End has not arrived.");
        var terminal = ReadBoundary(end, CapturePhase.End);
        if (chunks.Count != terminal.ChunkCount ||
            chunks.Keys.Where((index, position) => index != position).Any())
            return Pending("Capture chunks are incomplete.");
        foreach (var range in terminal.Domains)
        {
            var proof = evidence.SingleOrDefault(e => e.Interval.Domain == range.Domain);
            if (proof is null || range.BeforeSequence is null || range.ThroughSequence is null ||
                range.Quality.Kind != MeasurementKind.Measured || range.Quality.Coverage != CoverageState.Complete ||
                domains[range.Domain].Quality.Kind != MeasurementKind.Measured ||
                domains[range.Domain].Quality.Coverage != CoverageState.Complete)
                return Pending("A capture domain lacks eligible Begin/End and listener evidence.");
            var window = observations.Values.Where(o => o.Checkpoint.Domain == range.Domain &&
                o.Checkpoint.ListenerId == proof.Interval.ListenerId && o.Checkpoint.ContinuityId == proof.Interval.ContinuityId &&
                o.Checkpoint.Sequence > range.BeforeSequence && o.Checkpoint.Sequence <= range.ThroughSequence)
                .Select(o => o.Checkpoint).ToArray();
            var assessment = SequenceContinuity.Assess(new(range.BeforeSequence.Value, range.ThroughSequence.Value, window),
                proof.Interval, range.SettledCut, proof.Delivery, proof.Receipt);
            if (assessment.State != SequenceContinuityState.Contiguous)
                return new(assessment.State, null, assessment.Reason);
        }
        var payloads = chunks.Values.Select(s => EventCodec.ReadPayload<CaptureChunkPayload>(s.Envelope)).ToArray();
        try { ProjectionSupport.ValidateCaptureGraph(payloads); }
        catch (ArgumentException error) { throw Invalid("Capture graph is inconsistent.", error); }
        // Delivery proof is not delta reconciliation or an atomic projection commit.
        var deltas = observations.Values.Select(o => o.Source)
            .Where(s => s.Envelope.Identity.EventId is not (13 or 14))
            .OrderBy(s => s.Envelope.IngestOrdinal).ToArray();
        return new(SequenceContinuityState.Contiguous,
            new(begin, end, Array.AsReadOnly(payloads), Array.AsReadOnly(deltas)), null);
    }

    private NormalizedEvent Own(NormalizedEvent source)
    {
        Require.NotNull(source);
        if (source.Stream != begin.Stream || source.Envelope.SessionId != begin.Envelope.SessionId ||
            source.Envelope.Process != begin.Envelope.Process || source.Envelope.SourceSequence is null or 0)
            throw Invalid("Capture source belongs to another session, process, epoch or missing sequence.");
        return EventCodec.Normalize(source.Envelope, source.Stream);
    }

    private CaptureBoundaryPayload ReadBoundary(NormalizedEvent source, CapturePhase phase)
    {
        if (source.Stream.ProviderId != EventCatalog.RuntimeProviderId || source.Envelope.Identity.EventId != 13 ||
            source.Envelope.SourceSequence is null or 0)
            throw Invalid("Expected a sequenced runtime baseline boundary.");
        var value = EventCodec.ReadPayload<CaptureBoundaryPayload>(source.Envelope);
        if (value.Kind != CaptureKind.Baseline || value.Phase != phase)
            throw Invalid("Unexpected capture kind or phase.");
        return value;
    }

    private void Reserve(NormalizedEvent source, bool allowStagedReceipt = false)
    {
        var key = (source.SequenceDomain, source.Envelope.SourceSequence!.Value);
        if (sourcesBySequence.TryGetValue(key, out var staged))
        {
            if (allowStagedReceipt && SameObservation(source, staged))
                return;
            throw Invalid("Different staged sources reuse one producer-domain sequence.");
        }
        var size = BatchAccounting.SourceBytes(source, null) + 256;
        if (sources == MaximumSources || bytes + size > MaximumBytes)
        {
            failure = "Capture staging exceeded its bounded source/byte budget.";
            throw new ProcessingLimitException();
        }
        sourcesBySequence.Add(key, source);
        sources++;
        bytes += size;
    }

    private static bool SameObservation(NormalizedEvent first, NormalizedEvent second) =>
        first.Envelope.Identity == second.Envelope.Identity &&
        first.Envelope.ThreadId == second.Envelope.ThreadId && first.Envelope.TimestampQpc == second.Envelope.TimestampQpc &&
        first.Envelope.ActivityId == second.Envelope.ActivityId &&
        first.Envelope.OwnedPayload.Span.SequenceEqual(second.Envelope.OwnedPayload.Span);
    private static BaselinePreparation Pending(string reason) => new(SequenceContinuityState.Unproven, null, reason);
    private void Check()
    {
        if (failure is not null) throw new InvalidOperationException(failure);
    }
    private InvalidOperationException Invalid(string reason, Exception? inner = null)
    {
        failure ??= reason;
        return new(failure, inner);
    }
}
