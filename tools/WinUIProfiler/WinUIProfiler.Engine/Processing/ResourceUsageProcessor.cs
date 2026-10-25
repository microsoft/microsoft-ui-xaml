using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;
using static WinUIProfiler.Engine.Processing.ProjectionSupport;

namespace WinUIProfiler.Engine.Processing;

public sealed class ResourceUsageProcessor : IVerifiedBaselineProcessor
{
    private const string Accounting = "Resource/backing observations are not process GPU totals; shared backing IDs must be deduplicated, not summed per owner.";
    private readonly int maximumEntries;
    private readonly long maximumBytes;
    private readonly Dictionary<ObjectKey, SortedDictionary<long, ResourcePayload>> resources = [];
    private readonly Dictionary<(Guid Epoch, Guid Device, long Qpc), DevicePayload> devices = [];
    private readonly Dictionary<(Guid Epoch, Guid Device, Guid Backing, long Qpc), ulong> backingSizes = [];
    private readonly Dictionary<(ObjectKey From, ObjectKey To, CorrelationKind Kind, long Qpc), CorrelationPayload> correlations = [];
    private SessionScope? scope;
    private int entries;
    private long bytes;
    private long continuityFailures;
    private bool invalidationPending;
    private bool continuityLost;
    private long lastGlobalInvalidationQpc = -1;

    public ResourceUsageProcessor(int maximumEntries = 65536) : this(maximumEntries, 16 * 1024 * 1024) { }

    public ResourceUsageProcessor(int maximumEntries, long maximumBytes)
    {
        if (maximumEntries is < 1 or > 1_048_576) throw new ArgumentOutOfRangeException(nameof(maximumEntries));
        if (maximumBytes is < 1 or > 16 * 1024 * 1024) throw new ArgumentOutOfRangeException(nameof(maximumBytes));
        this.maximumEntries = maximumEntries;
        this.maximumBytes = maximumBytes;
    }

    public void Invalidate(HealthSnapshot health)
    {
        ArgumentNullException.ThrowIfNull(health);
        var failures = ContinuityFailures(health);
        if (failures <= continuityFailures) return;
        continuityFailures = failures;
        invalidationPending = continuityLost = true;
    }

    public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
    {
        foreach (var source in events)
        {
            CheckScope(source, context);
            var e = source.Envelope;
            var definition = EventCatalog.Get(e.Identity.ProviderId, e.Identity.EventId);
            if (definition.PayloadType == typeof(LossPayload))
                invalidationPending = continuityLost = true;
            ResourcePayload? resource = null;
            if (e.Identity.ProviderId == EventCatalog.RuntimeProviderId && e.Identity.EventId == 11)
                resource = EventCodec.ReadPayload<ResourcePayload>(e);
            var invalidatedAll = invalidationPending;
            if (invalidationPending)
            {
                var refresh = resource is not null &&
                    (!resources.TryGetValue(resource.Resource, out var priorTimeline) || !priorTimeline.ContainsKey(e.TimestampQpc))
                    ? resource.Resource : (ObjectKey?)null;
                foreach (var row in InvalidateRows(source, context, refresh, null,
                    "Resource continuity was lost; current backing size/ownership coverage is unknown."))
                    yield return row;
                invalidationPending = false;
                lastGlobalInvalidationQpc = Math.Max(lastGlobalInvalidationQpc, e.TimestampQpc);
            }
            if (e.Identity.ProviderId != EventCatalog.RuntimeProviderId) continue;
            switch (e.Identity.EventId)
            {
                case 11:
                    foreach (var row in Apply(resource!, source, context)) yield return row;
                    break;
                case 10:
                    var device = EventCodec.ReadPayload<DevicePayload>(e);
                    var key = (source.Stream.ProducerEpoch, device.DeviceEpoch, e.TimestampQpc);
                    if (devices.TryGetValue(key, out var previous))
                    {
                        if (previous != device) throw new InvalidDataException("Conflicting device facts at the same event time.");
                        break;
                    }
                    Reserve(new { source.Stream.ProducerEpoch, e.TimestampQpc, Device = device });
                    devices.Add(key, device);
                    if (!invalidatedAll && device.Operation is DeviceOperation.Lost or DeviceOperation.Destroy)
                        foreach (var row in InvalidateRows(source, context, null, key,
                            "Device lost/destroyed; backing availability is unknown, not proven physically freed."))
                            yield return row;
                    break;
                case 7:
                    foreach (var row in ApplyCorrelation(EventCodec.ReadPayload<CorrelationPayload>(e), source, context))
                        yield return row;
                    break;
                // Raw capture chunks and boundaries are deliberately not publication authority.
            }
        }
    }

    public IEnumerable<ProjectionChange> PublishBaseline(NormalizedEvent validatedEnd,
        IReadOnlyList<CaptureChunkPayload> chunks, ProcessingContext context)
    {
        ValidatePublication(validatedEnd, chunks, context);
        CheckScope(validatedEnd, context);
        var staged = chunks.SelectMany(c => c.Resources).ToArray();
        if (staged.Select(r => r.Resource).Distinct().Count() != staged.Length)
            throw new ArgumentException("Duplicate staged resource identity.");
        var links = chunks.SelectMany(c => c.Correlations)
            .Where(c => c.Kind is CorrelationKind.VisualResource or CorrelationKind.ResourceBacking).ToArray();
        if (links.Select(c => (c.From, c.To, c.Kind)).Distinct().Count() != links.Length)
            throw new ArgumentException("Duplicate staged resource correlation.");
        var end = EventCodec.ReadPayload<CaptureBoundaryPayload>(validatedEnd.Envelope);
        var publicationContext = context with { Quality = Combine(context.Quality, end.Quality) };
        if (invalidationPending)
        {
            var refreshed = staged.Select(r => r.Resource).ToHashSet();
            foreach (var row in InvalidateRows(validatedEnd, publicationContext, null, null,
                "Resource continuity was lost; the verified capture did not refresh this resource.", refreshed))
                yield return row;
            invalidationPending = false;
            lastGlobalInvalidationQpc = Math.Max(lastGlobalInvalidationQpc, validatedEnd.Envelope.TimestampQpc);
        }
        foreach (var item in staged)
        {
            var present = item with
            {
                Operation = ResourceOperation.BaselinePresent,
                Quality = Partial(item.Quality, "Verified capture establishes presence, not historical birth.")
            };
            foreach (var row in Apply(present, validatedEnd, publicationContext)) yield return row;
        }
        foreach (var link in links)
            foreach (var row in ApplyCorrelation(link, validatedEnd, publicationContext)) yield return row;
    }

    private void CheckScope(NormalizedEvent source, ProcessingContext context)
    {
        ValidateScope(source, context);
        if (scope is { } prior && prior != context.Scope)
            throw new ArgumentException("A resource state owner cannot be reused for another session.");
        scope = context.Scope;
    }

    private void Reserve(object fact)
    {
        // Charge fixed index/node overhead as well as serialized facts. Nothing is evicted:
        // old facts are needed to reject contradictions and disambiguate event-time reuse.
        var size = BatchAccounting.JsonBytes(fact, ContractVersions.MaximumPayloadBytes) + 256;
        if (entries == maximumEntries || size > maximumBytes - bytes) throw new ProcessingLimitException();
        entries++;
        bytes += size;
    }

    private static (ObjectKey? Owner, Guid? Backing) RowKey(ResourcePayload p) =>
        (p.Operation is ResourceOperation.OwnershipRelease or ResourceOperation.Close ? null : p.Owner, p.BackingAllocationId);

    private IEnumerable<ProjectionChange> Apply(ResourcePayload p, NormalizedEvent source, ProcessingContext context)
    {
        var e = source.Envelope;
        resources.TryGetValue(p.Resource, out var timeline);
        if (timeline is not null && timeline.TryGetValue(e.TimestampQpc, out var sameTime))
        {
            if (sameTime != p) throw new InvalidDataException("Conflicting resource facts at the same event time.");
            yield break;
        }
        var predecessor = timeline?.LastOrDefault(f => f.Key < e.TimestampQpc).Value;
        var successor = timeline?.FirstOrDefault(f => f.Key > e.TimestampQpc).Value;
        var invalidatedLater = lastGlobalInvalidationQpc > e.TimestampQpc || devices.Any(d =>
            d.Key.Epoch == p.Resource.ProducerEpoch && d.Key.Device == p.DeviceEpoch &&
            d.Key.Qpc > e.TimestampQpc && d.Value.Operation is DeviceOperation.Lost or DeviceOperation.Destroy);
        if (invalidatedLater && (timeline is null || RowKey(timeline.Last().Value) != RowKey(p)))
            throw new InvalidDataException("A late row identity cannot escape an already persisted invalidation; backdated revision repair is unsupported.");
        if (successor is not null && RowKey(successor) != RowKey(p))
            throw new InvalidDataException("Late resource owner/backing transition requires backdated revision repair unsupported by the existing writer contract.");
        if (timeline is not null)
        {
            if (p.Operation == ResourceOperation.Create && timeline.Values.Any(f => f.Operation == ResourceOperation.Create) ||
                p.Operation != ResourceOperation.Destroy && timeline.Any(f => f.Key < e.TimestampQpc && f.Value.Operation == ResourceOperation.Destroy) ||
                p.Operation == ResourceOperation.Destroy && timeline.Any(f => f.Key > e.TimestampQpc && f.Value.Operation != ResourceOperation.Destroy))
                throw new InvalidDataException("A full resource lifetime identity cannot be reused after birth/destruction.");
            if (p.Classification.Kind != ResourceKind.Unknown && timeline.Values.Any(f =>
                f.Classification.Kind != ResourceKind.Unknown && f.Classification.Kind != p.Classification.Kind))
                throw new InvalidDataException("A resource changed its authoritative kind.");
        }
        if (p.SizeProvenance == SizeProvenance.ActualAllocation)
        {
            var backingKey = (p.Resource.ProducerEpoch, p.DeviceEpoch, p.BackingAllocationId!.Value, e.TimestampQpc);
            if (backingSizes.TryGetValue(backingKey, out var actual) && actual != p.AllocationBytes)
                throw new InvalidDataException("The same backing has conflicting actual sizes at the same event time.");
        }
        Reserve(new { e.TimestampQpc, Resource = p });
        if (p.SizeProvenance == SizeProvenance.ActualAllocation)
            backingSizes.TryAdd((p.Resource.ProducerEpoch, p.DeviceEpoch, p.BackingAllocationId!.Value, e.TimestampQpc), p.AllocationBytes!.Value);
        if (timeline is null) resources.Add(p.Resource, timeline = []);
        timeline.Add(e.TimestampQpc, p);
        var quality = Quality(p, context, timeline.Any(f => f.Key <= e.TimestampQpc && f.Value.Operation == ResourceOperation.Create));
        var observed = Usage(p, quality);
        yield return Candidate(e.IngestOrdinal, new ObjectHistoryEntry(p.Resource, e.TimestampQpc, e.IngestOrdinal,
            new ResourceObservation(p.Operation, p.Classification, p.DeviceEpoch, p.BackingAllocationId,
                observed.Size, observed.Provenance, p.Owner), p.Cause ?? UnknownCause, observed.Quality), context);
        if (predecessor is not null && RowKey(predecessor) != RowKey(p) && predecessor.Operation != ResourceOperation.Destroy)
            yield return Candidate(e.IngestOrdinal, Usage(predecessor, Quality(predecessor, context, false)), context, true);
        yield return Candidate(e.IngestOrdinal, observed, context, p.Operation == ResourceOperation.Destroy);
    }

    private DataQuality Quality(ResourcePayload p, ProcessingContext context, bool birthKnown)
    {
        // Coverage can worsen independently of size provenance. Changing Measured into
        // Estimated for an actual allocation would violate the existing DTO validation.
        var coverage = (CoverageState)Math.Max((int)p.Quality.Coverage, (int)context.Quality.Coverage);
        var reason = $"{p.Quality.Reason} {context.Quality.Reason} {Accounting}";
        if (!birthKnown)
        {
            coverage = (CoverageState)Math.Max((int)coverage, (int)CoverageState.Partial);
            reason += " Unknown birth: no earlier creation was observed.";
        }
        if (continuityLost)
        {
            coverage = (CoverageState)Math.Max((int)coverage, (int)CoverageState.Partial);
            reason += " Resource continuity remains partial after loss.";
        }
        if (RowKey(p).Owner is null)
        {
            coverage = (CoverageState)Math.Max((int)coverage, (int)CoverageState.Partial);
            reason += " Owner unknown or released; backing destruction is not implied.";
        }
        return new(p.Quality.Kind, coverage, reason.Trim());
    }

    private static ResourceUsageEntry Usage(ResourcePayload p, DataQuality quality, string? invalidation = null)
    {
        var known = invalidation is null && p.AllocationBytes is <= long.MaxValue && p.Quality.Kind != MeasurementKind.Unavailable;
        var reason = invalidation ?? (p.AllocationBytes is > long.MaxValue
            ? "Reported byte count exceeds the exact signed integer DTO range." : "Allocation size was not observed.");
        var size = known
            ? new TypedValue(ValueKind.Integer, ValueUnit.Bytes, (long)p.AllocationBytes!.Value, null, null, null, quality)
            : new TypedValue(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null,
                new(MeasurementKind.Unavailable, CoverageState.Partial, reason));
        if (invalidation is not null)
            quality = new(MeasurementKind.Unavailable, CoverageState.Partial, invalidation + " " + Accounting);
        return new(p.Resource, RowKey(p).Owner, p.BackingAllocationId, p.DeviceEpoch, size,
            known ? p.SizeProvenance : SizeProvenance.Unknown, p.Classification, quality);
    }

    private IEnumerable<ProjectionChange> InvalidateRows(NormalizedEvent source, ProcessingContext context,
        ObjectKey? except, (Guid Epoch, Guid Device, long Qpc)? device, string reason,
        IReadOnlySet<ObjectKey>? excluded = null)
    {
        foreach (var timeline in resources.Values)
        {
            var current = timeline.Last();
            var p = current.Value;
            if (p.Operation == ResourceOperation.Destroy ||
                device is { } d && (p.Resource.ProducerEpoch != d.Epoch || p.DeviceEpoch != d.Device)) continue;
            if (current.Key > source.Envelope.TimestampQpc)
                throw new InvalidDataException("Late invalidation requires backdated revision repair unsupported by the existing writer contract.");
            if (p.Resource == except || excluded?.Contains(p.Resource) == true) continue;
            yield return Candidate(source.Envelope.IngestOrdinal, Usage(p, Quality(p, context, false), reason), context);
        }
    }

    private IEnumerable<ProjectionChange> ApplyCorrelation(CorrelationPayload p, NormalizedEvent source, ProcessingContext context)
    {
        if (p.Kind is not (CorrelationKind.VisualResource or CorrelationKind.ResourceBacking)) yield break;
        var e = source.Envelope;
        var key = (p.From, p.To, p.Kind, e.TimestampQpc);
        if (correlations.TryGetValue(key, out var prior))
        {
            if (prior != p) throw new InvalidDataException("Conflicting resource correlations at the same event time.");
            yield break;
        }
        Reserve(new { e.TimestampQpc, Correlation = p });
        correlations.Add(key, p);
        var quality = Partial(context.Quality, "Resource correlation is observed at this instant only; no lifetime or byte size is inferred.");
        yield return Candidate(e.IngestOrdinal, new CorrelationEntry(p.From, p.To, p.Kind,
            new(e.TimestampQpc, e.TimestampQpc), quality), context, !p.Linked);
        yield return Candidate(e.IngestOrdinal, new ObjectHistoryEntry(p.From, e.TimestampQpc, e.IngestOrdinal,
            new CorrelationObservation(p.To, p.Kind, p.Linked, true), p.Cause ?? UnknownCause, quality), context);
        yield return Candidate(e.IngestOrdinal, new ObjectHistoryEntry(p.To, e.TimestampQpc, e.IngestOrdinal,
            new CorrelationObservation(p.From, p.Kind, p.Linked, false), p.Cause ?? UnknownCause, quality), context);
    }
}
