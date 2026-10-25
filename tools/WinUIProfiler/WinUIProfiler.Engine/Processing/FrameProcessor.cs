using System.Text;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;
using static WinUIProfiler.Engine.Processing.ProjectionSupport;

namespace WinUIProfiler.Engine.Processing;

public sealed class FrameProcessor : IEventProcessor
{
    private const long MaximumStateBytes = 16 * 1024 * 1024;
    private readonly int maximumEntries;
    private readonly MetricProcessor metrics = new();
    private readonly List<Fact> facts = [];
    private readonly Dictionary<string, string> published = [];
    private readonly Dictionary<AttemptReference, List<Fact>> attempts = [];
    private readonly Dictionary<AttemptReference, List<FrameChange>> causal = [];
    private readonly Dictionary<Guid, SortedList<(long Qpc, long Ordinal), FrameChange>> unknown = [];
    private readonly Dictionary<AttemptReference, TimeRange> intervals = [];
    private readonly Dictionary<Guid, long> latestRuntime = [];
    private readonly HashSet<AttemptReference> open = [];
    private readonly HashSet<AttemptReference> dirty = [];
    private readonly List<Fact> losses = [];
    private DataQuality? previousQuality;
    private bool timelineDirty;
    private long retainedBytes;
    private long? unknownAnchor;
    private SessionScope? scope;
    private bool continuityLost;
    private bool invalidated;

    public FrameProcessor(int maximumEntries = 65536)
    {
        this.maximumEntries = maximumEntries is >= 1 and <= 1_048_576
            ? maximumEntries : throw new ArgumentOutOfRangeException(nameof(maximumEntries));
    }

    public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
    {
        if (scope is { } previous && previous != context.Scope)
            throw new ArgumentException("A frame processor belongs to one canonical session/process.");
        scope = context.Scope;
        var qualityChanged = previousQuality != context.Quality;
        if (invalidated || qualityChanged)
        {
            dirty.UnionWith(attempts.Keys);
            timelineDirty = true;
        }
        previousQuality = context.Quality;
        var changed = invalidated || qualityChanged;
        var revisionSource = events.Count == 0 ? 0 : events[^1].Envelope.IngestOrdinal;
        foreach (var source in events)
        {
            ValidateScope(source, context);
            var definition = EventCatalog.Get(source.Envelope.Identity.ProviderId, source.Envelope.Identity.EventId);
            if (definition.PayloadType == typeof(MetricPayload))
            {
                foreach (var metric in metrics.Process([source], context))
                    yield return Candidate(metric.SourceOrdinal, metric.Value, context);
                continue;
            }
            var payload = Read(source);
            if (payload is null) continue;
            var fact = new Fact(source, payload);
            var observations = Changes(fact).Select(o => o.Change).ToArray();
            var bytes = BatchAccounting.JsonBytes(source, ContractVersions.MaximumPayloadBytes * 2) +
                BatchAccounting.JsonBytes(payload, ContractVersions.MaximumPayloadBytes) + 128 +
                observations.Sum(o => BatchAccounting.JsonBytes(o, ContractVersions.MaximumPayloadBytes) + 128);
            if (facts.Count == maximumEntries || retainedBytes + bytes > MaximumStateBytes)
                throw new ProcessingLimitException();
            facts.Add(fact);
            retainedBytes += bytes;
            Index(fact, observations);
            unknownAnchor ??= source.Envelope.TimestampQpc;
            revisionSource = source.Envelope.IngestOrdinal;
            changed = true;
        }
        if (!changed || events.Count == 0 || facts.Count == 0) yield break;
        foreach (var row in Project(context))
        {
            var key = row switch
            {
                FrameInfo f => $"Frame:{f.Attempt}",
                StartupEntry s => $"Startup:{s.Phase}:{s.Range.StartQpc}",
                _ => throw new InvalidOperationException()
            };
            _ = BatchAccounting.JsonBytes(row, ProjectionBudget.MaximumRecordBytes);
            var json = JsonSerializer.Serialize(row, row.GetType(), WireJson.Options);
            if (published.TryGetValue(key, out var old) && old == json) continue;
            var candidate = Candidate(revisionSource, row, context);
            var delta = Encoding.UTF8.GetByteCount(json) - (old is null ? 0 : Encoding.UTF8.GetByteCount(old));
            if (retainedBytes + delta > MaximumStateBytes) throw new ProcessingLimitException();
            retainedBytes += delta;
            published[key] = json;
            yield return candidate;
        }
        invalidated = false;
        dirty.Clear();
        timelineDirty = false;
    }

    private void Index(Fact fact, IReadOnlyList<FrameChange> observations)
    {
        if (fact.Source.Envelope.Identity.ProviderId == EventCatalog.RuntimeProviderId)
        {
            if (!latestRuntime.TryGetValue(fact.Epoch, out var latest) || fact.Qpc > latest)
            {
                latestRuntime[fact.Epoch] = fact.Qpc;
                foreach (var attempt in open)
                    if (attempt.ProducerEpoch == fact.Epoch) dirty.Add(attempt);
            }
        }
        if (fact.Payload is FramePayload frame)
        {
            timelineDirty = true;
            if (frame.Attempt is { } attempt && frame.Outcome != FrameOutcome.StandaloneCommit)
            {
                if (!attempts.TryGetValue(attempt, out var group)) attempts.Add(attempt, group = []);
                group.Add(fact);
                dirty.Add(attempt);
                if (group.Any(f => ((FramePayload)f.Payload).Outcome is FrameOutcome.Failed or FrameOutcome.AttemptCompleted))
                    open.Remove(attempt);
                else open.Add(attempt);
            }
        }
        if (fact.Payload is LossPayload)
        {
            losses.Add(fact);
            dirty.UnionWith(attempts.Keys);
        }
        if (fact.Payload is ProcessPayload or BoundaryPayload or LossPayload) timelineDirty = true;
        foreach (var change in observations)
        {
            if (change.Cause is { Kind: CauseKind.ExplicitAttempt, Attempt: { } attempt })
            {
                if (attempt.ProducerEpoch != fact.Epoch) continue;
                if (!causal.TryGetValue(attempt, out var list)) causal.Add(attempt, list = []);
                list.Add(change);
                dirty.Add(attempt);
            }
            else if (change.Cause.Kind == CauseKind.Unknown)
            {
                if (!unknown.TryGetValue(fact.Epoch, out var list)) unknown.Add(fact.Epoch, list = []);
                list.Add((change.TimestampQpc, change.IngestOrdinal), change);
                foreach (var pair in intervals)
                    if (pair.Key.ProducerEpoch == fact.Epoch && change.TimestampQpc >= pair.Value.StartQpc &&
                        change.TimestampQpc <= pair.Value.EndQpc)
                        dirty.Add(pair.Key);
            }
        }
    }

    private IEnumerable<FrameChange> UnknownWithin(Guid epoch, long start, long end)
    {
        if (!unknown.TryGetValue(epoch, out var list)) yield break;
        var low = 0;
        var high = list.Count;
        while (low < high)
        {
            var middle = low + (high - low) / 2;
            if (list.Keys[middle].Qpc < start) low = middle + 1;
            else high = middle;
        }
        for (var i = low; i < list.Count && list.Keys[i].Qpc <= end; i++)
            yield return list.Values[i];
    }

    private static IEventPayload? Read(NormalizedEvent source)
    {
        var e = source.Envelope;
        if (e.Identity.ProviderId == EventCatalog.KernelProcessProviderId ||
            e.Identity.ProviderId == EventCatalog.CollectorProviderId && e.Identity.EventId == 1)
            return EventCodec.ReadPayload<ProcessPayload>(e);
        if (e.Identity.ProviderId == EventCatalog.CollectorProviderId && e.Identity.EventId == 3)
            return EventCodec.ReadPayload<LossPayload>(e);
        if (e.Identity.ProviderId != EventCatalog.RuntimeProviderId) return null;
        return e.Identity.EventId switch
        {
            2 => EventCodec.ReadPayload<ObjectLifetimePayload>(e),
            4 => EventCodec.ReadPayload<AssociationPayload>(e),
            5 => EventCodec.ReadPayload<TreeEdgePayload>(e),
            6 => EventCodec.ReadPayload<VisualDeltaPayload>(e),
            7 => EventCodec.ReadPayload<CorrelationPayload>(e),
            8 => EventCodec.ReadPayload<BoundaryPayload>(e),
            9 => EventCodec.ReadPayload<FramePayload>(e),
            11 => EventCodec.ReadPayload<ResourcePayload>(e),
            12 => EventCodec.ReadPayload<AllocationPayload>(e),
            15 => EventCodec.ReadPayload<LossPayload>(e),
            16 => EventCodec.ReadPayload<XamlPropertyChangedPayload>(e),
            _ => null
        };
    }

    private IEnumerable<IQueryValue> Project(ProcessingContext context)
    {
        foreach (var attempt in dirty)
        {
            if (!attempts.TryGetValue(attempt, out var attemptFacts)) continue;
            var group = attemptFacts.OrderBy(f => f.Qpc).ThenBy(f => f.Ordinal).ToArray();
            var begin = group.FirstOrDefault(f => ((FramePayload)f.Payload).Outcome == FrameOutcome.Attempt);
            var terminal = group.LastOrDefault(f => ((FramePayload)f.Payload).Outcome is FrameOutcome.AttemptCompleted or FrameOutcome.Failed);
            var submissions = group.Where(f => ((FramePayload)f.Payload).Outcome == FrameOutcome.Submitted).ToArray();
            var submission = submissions.FirstOrDefault();
            if (submissions.Any(s => s.Qpc != submission!.Qpc ||
                ((FramePayload)s.Payload).SubmittedFrameId != ((FramePayload)submission.Payload).SubmittedFrameId))
                throw new InvalidDataException("An attempt has conflicting physical submission identities.");
            var start = begin?.Qpc ?? group.Min(f => f.Qpc);
            var end = group.Max(f => f.Qpc);
            if (terminal is null)
                end = Math.Max(end, latestRuntime.GetValueOrDefault(attempt.ProducerEpoch));
            var intervalEnd = end;
            if (begin is not null) intervals[attempt] = new(begin.Qpc, intervalEnd);
            var exact = causal.GetValueOrDefault(attempt) ?? [];
            if (exact.Count > 0)
            {
                start = Math.Min(start, exact.Min(o => o.TimestampQpc));
                end = Math.Max(end, exact.Max(o => o.TimestampQpc));
            }
            var range = new TimeRange(start, end);
            var lost = continuityLost || losses.Any(f => Affects(f, attempt.ProducerEpoch, range));
            var temporal = !lost && begin is not null
                ? UnknownWithin(attempt.ProducerEpoch, begin.Qpc, intervalEnd) : [];
            var changes = exact.Concat(temporal).OrderBy(o => o.TimestampQpc).ThenBy(o => o.IngestOrdinal)
                .Select(o => o with
                {
                    Relation = o.Cause.Kind == CauseKind.ExplicitAttempt
                        ? FrameChangeRelation.Causal : FrameChangeRelation.TemporalOverlap,
                    Quality = ChangeQuality(o, context.Quality, lost)
                }).ToArray();
            if (changes.Length > 4096) throw new ProcessingLimitException();
            var result = terminal is null ? null : ((FramePayload)terminal.Payload).HResult;
            yield return new FrameInfo(attempt, range, terminal is null ? FrameAttemptStatus.InProgress :
                result < 0 ? FrameAttemptStatus.Failed : FrameAttemptStatus.Completed,
                submission is null ? null : new(((FramePayload)submission.Payload).SubmittedFrameId!.Value,
                    submission.Qpc, attempt.ProducerEpoch), result, changes);
        }

        if (!timelineDirty) yield break;
        var ordered = facts.OrderBy(f => f.Qpc).ThenBy(f => f.Ordinal).ToArray();
        var physicalSubmissions = ordered.Where(f => f.Payload is FramePayload { Outcome: FrameOutcome.Submitted })
            .GroupBy(f => (f.Epoch, ((FramePayload)f.Payload).SubmittedFrameId))
            .Select(g =>
            {
                if (g.Any(f => f.Qpc != g.First().Qpc))
                    throw new InvalidDataException("A physical submission ID has conflicting commit timestamps.");
                return g.First();
            }).ToArray();
        foreach (var group in ordered.Where(f => f.Payload is BoundaryPayload).GroupBy(f =>
        {
            var b = (BoundaryPayload)f.Payload;
            return (f.Epoch, f.Source.Stream.ProducerProcessId, f.Source.Envelope.ThreadId,
                f.Source.Envelope.ActivityId, b.Kind, b.OperationId);
        }))
        {
            var begins = group.Where(f => ((BoundaryPayload)f.Payload).Phase == BoundaryPhase.Begin).ToArray();
            var ends = group.Where(f => ((BoundaryPayload)f.Payload).Phase == BoundaryPhase.End).ToArray();
            if (begins.Length > 1 || ends.Length > 1)
                throw new InvalidDataException("A boundary reused its epoch/thread/activity/operation identity.");
            if (begins.Length == 0 || ends.Length == 0 || ends[0].Qpc < begins[0].Qpc) continue;
            var range = new TimeRange(begins[0].Qpc, ends[0].Qpc);
            var quality = continuityLost || losses.Any(f => Affects(f, group.Key.Epoch, range))
                ? Partial(context.Quality, "Boundary interval crosses lost or invalidated evidence.") : context.Quality;
            yield return new StartupEntry(
                $"Phase:{group.Key.Kind}:{group.Key.Epoch:N}:{group.Key.ProducerProcessId}:{group.Key.ThreadId}:{group.Key.ActivityId:N}:{group.Key.OperationId}",
                range, null, quality);
        }

        var creation = ordered.Where(f => f.Payload is ProcessPayload { Operation: ProcessOperation.Start, CreationQpc: not null })
            .ToArray();
        if (creation.Length == 0)
        {
            yield return new StartupEntry("StartupUnknown", new(unknownAnchor!.Value, unknownAnchor.Value), null,
                Uncertain("Exact process creation QPC was not observed; attach/recording clocks are not startup evidence."));
            yield break;
        }
        var creationQpc = ((ProcessPayload)creation[0].Payload).CreationQpc!.Value;
        if (creation.Any(f => ((ProcessPayload)f.Payload).CreationQpc != creationQpc))
            throw new InvalidDataException("Canonical process creation QPC changed.");
        var startupSubmissions = physicalSubmissions.Where(f => f.Qpc >= creationQpc)
            .OrderBy(f => f.Qpc).ThenBy(f => f.Ordinal).Take(10).ToArray();
        var exit = ordered.LastOrDefault(f => f.Payload is ProcessPayload { Operation: ProcessOperation.Exit });
        var windowEnd = startupSubmissions.Length > 0 ? startupSubmissions[^1].Qpc : creationQpc;
        var startupRange = new TimeRange(creationQpc, windowEnd);
        var startupQuality = Combine(context.Quality, ((ProcessPayload)creation[0].Payload).Quality);
        var epochs = ordered.Where(f => f.Payload is FramePayload &&
            f.Qpc >= creationQpc && f.Qpc <= windowEnd).Select(f => f.Epoch).Distinct().Count();
        if (continuityLost || losses.Any(f => ((LossPayload)f.Payload).Interval.StartQpc <= windowEnd &&
            ((LossPayload)f.Payload).Interval.EndQpc >= creationQpc) || epochs > 1)
            startupQuality = Partial(startupQuality, "Loss/epoch discontinuity prevents proving the first ten physical submissions.");
        if (startupSubmissions.Length < 10)
            startupQuality = Partial(startupQuality,
                $"Startup incomplete: {startupSubmissions.Length}/10 successful primary physical submissions observed{(exit is null ? "." : " before target exit.")}");
        yield return new StartupEntry("ProcessCreation", new(creationQpc, creationQpc), null,
            ((ProcessPayload)creation[0].Payload).Quality);
        for (var i = 0; i < startupSubmissions.Length; i++)
            yield return new StartupEntry($"StartupFrame:{i + 1:D2}", new(creationQpc, startupSubmissions[i].Qpc),
                ((FramePayload)startupSubmissions[i].Payload).SubmittedFrameId, startupQuality);
        yield return new StartupEntry("StartupWindow", startupRange, null, startupQuality);
    }

    private static bool Affects(Fact fact, Guid epoch, TimeRange range)
    {
        var loss = (LossPayload)fact.Payload;
        return (fact.Source.Envelope.Identity.ProviderId == EventCatalog.CollectorProviderId || loss.ProducerEpoch == epoch) &&
            loss.Interval.StartQpc <= range.EndQpc && loss.Interval.EndQpc >= range.StartQpc;
    }

    private static DataQuality ChangeQuality(FrameChange change, DataQuality context, bool lost)
    {
        var quality = Combine(change.Quality, context);
        if (lost) quality = Partial(quality, "Loss invalidated interval coverage; explicit attribution is retained as reported.");
        if (change.Cause.Kind == CauseKind.Unknown)
            quality = Partial(quality, "Temporal overlap is not explicit causal attribution.");
        return quality;
    }

    private static IEnumerable<(Guid Epoch, FrameChange Change)> Changes(Fact fact)
    {
        ObjectKey? key;
        ObjectObservation observation;
        ChangeCause? cause;
        var quality = DataQuality.Complete;
        switch (fact.Payload)
        {
            case ObjectLifetimePayload p:
                key = p.Object; observation = new LifetimeObservation(p.Operation, null, p.Address, p.Classification);
                cause = p.Cause; quality = p.Quality; break;
            case AssociationPayload p:
                key = p.Core; observation = new CorrelationObservation(p.Peer, CorrelationKind.CorePeer, p.Linked, true);
                cause = p.Cause; break;
            case TreeEdgePayload p:
                key = p.Child; observation = new MembershipObservation(p.Parent, p.Relationship, p.Order,
                    p.RootId, p.DispatcherId, p.Connected, p.Membership); cause = p.Cause; break;
            case VisualDeltaPayload p:
                key = p.Visual; observation = new VisualPropertyObservation(p.Property, p.Value);
                cause = p.Cause; quality = p.Value.Quality; break;
            case CorrelationPayload p:
                key = p.From; observation = new CorrelationObservation(p.To, p.Kind, p.Linked, true);
                cause = p.Cause; break;
            case BoundaryPayload p:
                key = null; observation = new BoundaryObservation(p.Kind, p.Phase, p.OperationId, p.HResult);
                cause = p.Cause; break;
            case ResourcePayload p:
                key = p.Resource;
                var size = p.AllocationBytes is { } bytes && bytes <= long.MaxValue
                    ? new TypedValue(ValueKind.Integer, ValueUnit.Bytes, (long)bytes, null, null, null, p.Quality)
                    : new(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null, Uncertain("Resource size is unknown or exceeds signed query storage."));
                observation = new ResourceObservation(p.Operation, p.Classification, p.DeviceEpoch, p.BackingAllocationId,
                    size, size.Kind == ValueKind.Missing ? SizeProvenance.Unknown : p.SizeProvenance, p.Owner);
                cause = p.Cause; quality = Combine(p.Quality, size.Quality); break;
            case AllocationPayload p:
                key = null; observation = new AllocationObservation(p.AllocationId, p.Operation, p.PreviousAllocationId,
                    p.RequestedBytes, p.ActualBytes, p.AllocatorEpoch); cause = p.Cause; quality = p.Quality; break;
            case XamlPropertyChangedPayload p:
                key = p.Object; observation = new PropertyObservation(p.Property, p.Previous, p.Current);
                cause = p.Cause; quality = Combine(p.Previous.Quality, p.Current.Quality); break;
            default: yield break;
        }
        yield return (fact.Epoch, new(fact.Qpc, fact.Ordinal, key, observation, cause ?? UnknownCause,
            FrameChangeRelation.TemporalOverlap, quality));
    }

    private sealed record Fact(NormalizedEvent Source, IEventPayload Payload)
    {
        internal long Qpc => Source.Envelope.TimestampQpc;
        internal long Ordinal => Source.Envelope.IngestOrdinal;
        internal Guid Epoch => Source.Stream.ProducerEpoch;
    }

    public void Invalidate(HealthSnapshot health)
    {
        metrics.Invalidate(health);
        if (health.HasContinuityFailure && !continuityLost)
        {
            continuityLost = true;
            invalidated = true;
        }
    }
}
