using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;
using static WinUIProfiler.Engine.Processing.ProjectionSupport;

namespace WinUIProfiler.Engine.Processing;

public sealed class CompositionTreeProcessor : IVerifiedBaselineProcessor
{
    private sealed record Fact<T>(long Qpc, long Ordinal, T Value);
    private sealed record Gap(Guid? Epoch, long Qpc, long Ordinal);
    private readonly Dictionary<(ObjectKey Object, TreeMembershipKind Membership), SortedDictionary<long, Fact<TreeEdgePayload>>> edges = [];
    private readonly Dictionary<(ObjectKey Object, VisualProperty Property), SortedDictionary<long, Fact<VisualDeltaPayload>>> properties = [];
    private readonly Dictionary<(ObjectKey From, ObjectKey To, CorrelationKind Kind, long Qpc), CorrelationPayload> correlations = [];
    private readonly Dictionary<ObjectKey, Fact<ObjectLifetimePayload>> destroyed = [];
    private readonly Dictionary<ObjectKey, Fact<ObjectLifetimePayload>> born = [];
    private readonly List<Gap> gaps = [];
    private readonly Dictionary<ObjectKey, SortedDictionary<long, SnapshotObject>> metadata = [];
    private readonly Dictionary<(ObjectKey Object, TreeMembershipKind Membership), SortedSet<long>> retirements = [];
    private readonly int maximumEntries;
    private readonly long maximumBytes;
    private SessionScope? scope;
    private int entries;
    private long bytes;
    private long failures;
    private bool invalidationPending;

    public CompositionTreeProcessor(int maximumEntries = 65536) : this(maximumEntries, 16 * 1024 * 1024) { }
    public CompositionTreeProcessor(int maximumEntries, long maximumBytes)
    {
        if (maximumEntries is < 1 or > 1_048_576) throw new ArgumentOutOfRangeException(nameof(maximumEntries));
        if (maximumBytes is < 1 or > 16 * 1024 * 1024) throw new ArgumentOutOfRangeException(nameof(maximumBytes));
        this.maximumEntries = maximumEntries;
        this.maximumBytes = maximumBytes;
    }

    public void Invalidate(HealthSnapshot health)
    {
        ArgumentNullException.ThrowIfNull(health);
        // Explicit Loss carries its own epoch and time. Worker health is batch-wide.
        var count = ContinuityFailures(health) - health.Count(IngestionIssue.ProducerLoss);
        if (count > failures) invalidationPending = true;
        failures = Math.Max(failures, count);
    }

    public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
    {
        var output = new Output(context);
        foreach (var source in events)
        {
            ValidateScope(source, context);
            if (scope is { } prior && prior != context.Scope)
                throw new ArgumentException("A composition processor belongs to one session.");
            scope = context.Scope;
            var e = source.Envelope;
            if (e.TimestampQpc > context.BatchBoundary.TimestampQpc || e.IngestOrdinal > context.BatchBoundary.IngestOrdinal)
                throw new ArgumentException("Composition source exceeds its transaction boundary.");
            if (invalidationPending)
            {
                foreach (var row in InvalidateRows(new(null, e.TimestampQpc, e.IngestOrdinal), source, context, output))
                    yield return row;
                invalidationPending = false;
            }
            var definition = EventCatalog.Get(e.Identity.ProviderId, e.Identity.EventId);
            if (definition.PayloadType == typeof(LossPayload))
            {
                var loss = EventCodec.ReadPayload<LossPayload>(e);
                if (e.Identity.ProviderId == EventCatalog.RuntimeProviderId &&
                    loss.Domains is { Count: > 0 } && !loss.Domains.Any(d => d.EventId is 2 or 4 or 5 or 6 or 7))
                    continue;
                Guid? epoch = e.Identity.ProviderId == EventCatalog.RuntimeProviderId ? loss.ProducerEpoch : null;
                if (loss.Interval.StartQpc != e.TimestampQpc &&
                    edges.Any(p => (epoch is null || p.Key.Object.ProducerEpoch == epoch) &&
                        p.Value.Values.Any(v => v.Qpc <= e.TimestampQpc)))
                    throw new InvalidDataException("Backdated composition loss needs query-time interval overlay; the current composition query cannot represent it.");
                foreach (var row in InvalidateRows(new(epoch, e.TimestampQpc, e.IngestOrdinal), source, context, output))
                    yield return row;
                continue;
            }
            if (e.Identity.ProviderId != EventCatalog.RuntimeProviderId) continue;
            switch (e.Identity.EventId)
            {
                case 2:
                    foreach (var row in Lifetime(EventCodec.ReadPayload<ObjectLifetimePayload>(e), source, context, output))
                        yield return row;
                    break;
                case 4:
                    var association = EventCodec.ReadPayload<AssociationPayload>(e);
                    foreach (var row in Correlate(new(association.Core, association.Peer, CorrelationKind.CorePeer,
                        association.Linked, association.Cause), source, context, output)) yield return row;
                    break;
                case 5:
                    var edge = EventCodec.ReadPayload<TreeEdgePayload>(e);
                    if (edge.Child.Domain is IdentityDomain.CompNode or IdentityDomain.CompositionVisual)
                        foreach (var row in Membership(edge, source, context, output)) yield return row;
                    break;
                case 6:
                    foreach (var row in Property(EventCodec.ReadPayload<VisualDeltaPayload>(e), source, context, output))
                        yield return row;
                    break;
                case 7:
                    foreach (var row in Correlate(EventCodec.ReadPayload<CorrelationPayload>(e), source, context, output))
                        yield return row;
                    break;
                // Capture chunks and End are not independent publication authority.
            }
        }
    }

    public IEnumerable<ProjectionChange> PublishBaseline(NormalizedEvent validatedEnd,
        IReadOnlyList<CaptureChunkPayload> chunks, ProcessingContext context)
    {
        ValidatePublication(validatedEnd, chunks, context);
        var e = validatedEnd.Envelope;
        if (scope is { } prior && prior != context.Scope || e.TimestampQpc > context.BatchBoundary.TimestampQpc)
            throw new ArgumentException("Baseline source belongs to another composition owner or transaction.");
        if (edges.Values.Any(t => t.Keys.Any(q => q >= e.TimestampQpc)) ||
            properties.Values.Any(t => t.Keys.Any(q => q >= e.TimestampQpc)))
            throw new InvalidDataException("Verified End must follow retained composition facts; same-tick/backdated snapshot repair is unsupported.");
        scope = context.Scope;
        var boundary = EventCodec.ReadPayload<CaptureBoundaryPayload>(e);
        context = context with { Quality = Partial(Combine(context.Quality, boundary.Quality),
            "Verified reconciled capture establishes present state, not birth or atomic history.") };
        var output = new Output(context);
        var captured = chunks.SelectMany(c => c.Tree).Where(t =>
            t.Child.Domain is IdentityDomain.CompNode or IdentityDomain.CompositionVisual).ToArray();
        if (captured.Select(t => (t.Child, t.Membership)).Distinct().Count() != captured.Length)
            throw new ArgumentException("Conflicting staged composition channels.");
        var deltas = chunks.SelectMany(c => c.Visuals).ToArray();
        var links = chunks.SelectMany(c => c.Correlations).Concat(chunks.SelectMany(c => c.Associations)
            .Select(a => new CorrelationPayload(a.Core, a.Peer, CorrelationKind.CorePeer, a.Linked, a.Cause))).ToArray();
        if (deltas.Select(p => (p.Visual, p.Property)).Distinct().Count() != deltas.Length ||
            links.Select(p => (p.From, p.To, p.Kind)).Distinct().Count() != links.Length)
            throw new ArgumentException("Duplicate staged composition property/correlation.");
        var present = captured.Select(t => (t.Child, t.Membership)).ToHashSet();
        var qualified = boundary.Quality.Coverage == CoverageState.Complete && boundary.Domains.Any(d =>
            d.Domain == new SequenceDomainKey(validatedEnd.Stream, context.Scope.Process, 5, EventCatalog.Version) &&
            d.Quality.Coverage == CoverageState.Complete);
        if (invalidationPending)
        {
            // Refreshed rows are emitted once below, not twice under the End source key.
            var gap = new Gap(null, e.TimestampQpc, e.IngestOrdinal);
            Reserve(gap);
            gaps.Add(gap);
            invalidationPending = false;
        }
        foreach (var obj in chunks.SelectMany(c => c.Objects).Where(o =>
            o.Key.Domain is IdentityDomain.CompNode or IdentityDomain.CompositionVisual))
        {
            CheckAlive(obj.Key, e.TimestampQpc);
            if (!metadata.TryGetValue(obj.Key, out var timeline)) metadata.Add(obj.Key, timeline = []);
            if (timeline.TryGetValue(e.TimestampQpc, out var old) && old != obj)
                throw new InvalidDataException("Conflicting staged object metadata.");
            if (!timeline.ContainsKey(e.TimestampQpc))
            {
                Reserve(new { e.TimestampQpc, Object = obj });
                timeline.Add(e.TimestampQpc, obj);
            }
        }
        foreach (var delta in deltas)
        {
            CheckAlive(delta.Visual, e.TimestampQpc);
            var key = (delta.Visual, delta.Property);
            if (!properties.TryGetValue(key, out var timeline)) properties.Add(key, timeline = []);
            if (timeline.TryGetValue(e.TimestampQpc, out var old) && old.Value != delta)
                throw new InvalidDataException("Conflicting staged visual property.");
            if (!timeline.ContainsKey(e.TimestampQpc))
            {
                var fact = new Fact<VisualDeltaPayload>(e.TimestampQpc, e.IngestOrdinal, delta);
                Reserve(fact);
                timeline.Add(e.TimestampQpc, fact);
            }
            yield return output.Emit(validatedEnd, History(delta.Visual,
                new VisualPropertyObservation(delta.Property, delta.Value), delta.Cause, validatedEnd, context));
        }
        foreach (var pair in edges)
        {
            var old = At(pair.Value, e.TimestampQpc);
            if (old is null || Retired(old, e.TimestampQpc) || present.Contains(pair.Key)) continue;
            var edge = old.Value;
            var absent = qualified && edge.Child.ProducerEpoch == validatedEnd.Stream.ProducerEpoch &&
                (boundary.RootId is null || boundary.RootId == edge.RootId) &&
                (boundary.DispatcherId is null || boundary.DispatcherId == edge.DispatcherId);
            if (absent)
            {
                Reserve(new { edge.Child, edge.Membership, e.TimestampQpc });
                if (!retirements.TryGetValue(pair.Key, out var times)) retirements.Add(pair.Key, times = []);
                times.Add(e.TimestampQpc);
            }
            if (absent || gaps.Count > 0 || metadata.ContainsKey(edge.Child) || deltas.Any(d => d.Visual == edge.Child))
                yield return output.Emit(validatedEnd, Row(old, e.TimestampQpc, context), absent);
        }
        foreach (var edge in captured)
            foreach (var row in Membership(edge, validatedEnd, context, output)) yield return row;
        foreach (var link in links)
            foreach (var row in Correlate(link, validatedEnd, context, output, link.Kind != CorrelationKind.CorePeer))
                yield return row;
    }

    private bool Retired(Fact<TreeEdgePayload> edge, long at) =>
        retirements.TryGetValue((edge.Value.Child, edge.Value.Membership), out var times) &&
        times.Any(t => t >= edge.Qpc && t <= at);

    private void Reserve(object fact)
    {
        var size = BatchAccounting.JsonBytes(fact, ContractVersions.MaximumPayloadBytes) + 256;
        if (entries == maximumEntries || size > maximumBytes - bytes) throw new ProcessingLimitException();
        entries++;
        bytes += size;
    }

    private static Fact<T>? At<T>(SortedDictionary<long, Fact<T>> timeline, long qpc) =>
        timeline.LastOrDefault(p => p.Key <= qpc).Value;
    private bool Dead(ObjectKey key, long qpc) => destroyed.TryGetValue(key, out var death) && death.Qpc <= qpc;
    private void CheckAlive(ObjectKey key, long qpc)
    {
        if (Dead(key, qpc)) throw new InvalidDataException("A destroyed full composition/correlation identity cannot be reused.");
    }
    private bool Uncertain(ObjectKey key, long qpc, long ordinal, long at) =>
        gaps.Any(g => (g.Epoch is null || g.Epoch == key.ProducerEpoch) && g.Qpc <= at &&
            (g.Qpc, g.Ordinal).CompareTo((qpc, ordinal)) > 0);

    private CompositionTreeEntry Row(Fact<TreeEdgePayload> fact, long at, ProcessingContext context)
    {
        var edge = fact.Value;
        var uncertain = Uncertain(edge.Child, fact.Qpc, fact.Ordinal, at);
        var quality = uncertain ? ProjectionSupport.Uncertain("Composition continuity failed; membership is unknown, not detached.")
            : Partial(context.Quality, "Observed membership does not establish historical birth or a complete composition baseline.");
        var values = edge.Child.Domain == IdentityDomain.CompositionVisual
            ? properties.Where(p => p.Key.Object == edge.Child).Select(p => At(p.Value, at)).OfType<Fact<VisualDeltaPayload>>()
                .OrderBy(p => p.Value.Property).Select(p => new ObservedVisualProperty(p.Value.Property,
                    Uncertain(edge.Child, p.Qpc, p.Ordinal, at)
                        ? new TypedValue(ValueKind.Missing, p.Value.Value.Unit, null, null, null, null,
                            ProjectionSupport.Uncertain("Visual property continuity is unknown across loss."))
                        : p.Value.Value, p.Qpc, p.Value.Cause ?? UnknownCause)).ToArray()
            : [];
        var obj = metadata.TryGetValue(edge.Child, out var timeline) ? timeline.LastOrDefault(p => p.Key <= at).Value : null;
        return new(new(edge.Child, edge.Parent, edge.Relationship, edge.Order, edge.Connected, edge.RootId,
            edge.DispatcherId, obj?.TypeName, quality, edge.Membership, obj?.Classification ?? UnknownType), values);
    }

    private static ObjectHistoryEntry History(ObjectKey key, ObjectObservation observation, ChangeCause? cause,
        NormalizedEvent source, ProcessingContext context) =>
        new(key, source.Envelope.TimestampQpc, source.Envelope.IngestOrdinal, observation, cause ?? UnknownCause,
            Partial(context.Quality, "Individual composition observation; unknown endpoints/birth remain explicit."));

    private IEnumerable<ProjectionChange> Membership(TreeEdgePayload edge, NormalizedEvent source, ProcessingContext context, Output output)
    {
        var e = source.Envelope;
        var key = (edge.Child, edge.Membership);
        edges.TryGetValue(key, out var timeline);
        if (timeline is not null && timeline.TryGetValue(e.TimestampQpc, out var same))
        {
            if (same.Value != edge) throw new InvalidDataException("Conflicting composition memberships at the same event time.");
            yield break;
        }
        CheckAlive(edge.Child, e.TimestampQpc);
        if (edge.Parent is { } parent) CheckAlive(parent, e.TimestampQpc);
        var before = timeline is null ? null : At(timeline, e.TimestampQpc);
        var after = timeline?.FirstOrDefault(p => p.Key > e.TimestampQpc).Value;
        if (after is not null && after.Value.Relationship != edge.Relationship)
            throw new InvalidDataException("Late relationship classification change needs backdated row-key repair.");
        if (properties.Any(p => p.Key.Object == edge.Child && p.Value.Keys.Any(t =>
            t > e.TimestampQpc && (after is null || t < after.Qpc))))
            throw new InvalidDataException("Late topology would revise a future visual-property snapshot; backdated repair is unsupported.");
        if (destroyed.Values.Any(d => d.Qpc > e.TimestampQpc &&
            (d.Value.Object == edge.Child || d.Value.Object == edge.Parent)) &&
            (before is null || before.Value.Relationship != edge.Relationship))
            throw new InvalidDataException("Late first membership cannot escape an already persisted destruction.");
        if (gaps.Any(g => (g.Epoch is null || g.Epoch == edge.Child.ProducerEpoch) && g.Qpc > e.TimestampQpc) &&
            (before is null || before.Value.Relationship != edge.Relationship))
            throw new InvalidDataException("Late new membership row key needs backdated loss repair.");
        var fact = new Fact<TreeEdgePayload>(e.TimestampQpc, e.IngestOrdinal, edge);
        ValidateCycles(key, fact);
        Reserve(fact);
        if (timeline is null) edges.Add(key, timeline = []);
        timeline.Add(fact.Qpc, fact);
        yield return output.Emit(source, History(edge.Child, new MembershipObservation(edge.Parent, edge.Relationship,
            edge.Order, edge.RootId, edge.DispatcherId, edge.Connected, edge.Membership), edge.Cause, source, context));
        if (before is not null && before.Value.Relationship != edge.Relationship)
            yield return output.Emit(source, Row(before, e.TimestampQpc, context), true);
        yield return output.Emit(source, Row(fact, e.TimestampQpc, context));
    }

    private void ValidateCycles((ObjectKey Object, TreeMembershipKind Membership) key, Fact<TreeEdgePayload> proposed)
    {
        // A late edge must also be checked against later topology, not just today's cache.
        var times = edges.Values.SelectMany(t => t.Keys).Where(t => t >= proposed.Qpc).Append(proposed.Qpc).Distinct();
        foreach (var time in times)
        {
            Fact<TreeEdgePayload>? Edge((ObjectKey Object, TreeMembershipKind Membership) k)
            {
                var actual = edges.TryGetValue(k, out var timeline) ? At(timeline, time) : null;
                return k == key && (actual is null || actual.Qpc <= proposed.Qpc) ? proposed : actual;
            }
            var visited = new HashSet<ObjectKey>();
            ObjectKey? current = key.Object;
            while (current is { } node)
            {
                if (Dead(node, time)) break;
                if (!visited.Add(node)) throw new InvalidDataException("A composition membership cycle was observed.");
                var next = Edge((node, key.Membership));
                current = next?.Value.Connected == true && !Retired(next, time) ? next.Value.Parent : null;
            }
        }
    }

    private IEnumerable<ProjectionChange> Property(VisualDeltaPayload delta, NormalizedEvent source, ProcessingContext context, Output output)
    {
        var e = source.Envelope;
        CheckAlive(delta.Visual, e.TimestampQpc);
        var key = (delta.Visual, delta.Property);
        properties.TryGetValue(key, out var timeline);
        if (timeline is not null && timeline.TryGetValue(e.TimestampQpc, out var same))
        {
            if (same.Value != delta) throw new InvalidDataException("Conflicting visual property facts at the same event time.");
            yield break;
        }
        var successor = timeline?.FirstOrDefault(p => p.Key > e.TimestampQpc).Value;
        bool BeforeSuccessor(long time) => time > e.TimestampQpc && (successor is null || time < successor.Qpc);
        if (edges.Any(p => p.Key.Object == delta.Visual && p.Value.Keys.Any(BeforeSuccessor)) ||
            properties.Any(p => p.Key.Object == delta.Visual && p.Key.Property != delta.Property && p.Value.Keys.Any(BeforeSuccessor)))
            throw new InvalidDataException("Late property changes a future materialized composition snapshot; backdated repair is unsupported.");
        var fact = new Fact<VisualDeltaPayload>(e.TimestampQpc, e.IngestOrdinal, delta);
        Reserve(fact);
        if (timeline is null) properties.Add(key, timeline = []);
        timeline.Add(fact.Qpc, fact);
        yield return output.Emit(source, History(delta.Visual, new VisualPropertyObservation(delta.Property, delta.Value),
            delta.Cause, source, context));
        foreach (var edge in edges.Where(p => p.Key.Object == delta.Visual).Select(p => At(p.Value, e.TimestampQpc)).OfType<Fact<TreeEdgePayload>>())
            if (!Retired(edge, e.TimestampQpc) && (edge.Value.Parent is not { } parent || !Dead(parent, e.TimestampQpc)))
                yield return output.Emit(source, Row(edge, e.TimestampQpc, context));
    }

    private IEnumerable<ProjectionChange> Lifetime(ObjectLifetimePayload lifetime, NormalizedEvent source, ProcessingContext context, Output output)
    {
        var e = source.Envelope;
        if (lifetime.Operation != LifetimeOperation.Destroy)
        {
            CheckAlive(lifetime.Object, e.TimestampQpc);
            if (lifetime.Operation == LifetimeOperation.Birth)
            {
                if (born.TryGetValue(lifetime.Object, out var birth))
                {
                    if (birth.Qpc != e.TimestampQpc || birth.Value != lifetime)
                        throw new InvalidDataException("A full identity cannot have multiple historical births.");
                    yield break;
                }
                var birthFact = new Fact<ObjectLifetimePayload>(e.TimestampQpc, e.IngestOrdinal, lifetime);
                Reserve(birthFact);
                born.Add(lifetime.Object, birthFact);
            }
            yield break;
        }
        if (destroyed.TryGetValue(lifetime.Object, out var previous))
        {
            if (previous.Qpc != e.TimestampQpc || previous.Value != lifetime)
                throw new InvalidDataException("Conflicting destruction for a full object identity.");
            yield break;
        }
        if (edges.Values.SelectMany(t => t.Values).Any(f => f.Qpc >= e.TimestampQpc &&
            (f.Value.Child == lifetime.Object || f.Value.Parent == lifetime.Object)) ||
            properties.Any(p => p.Key.Object == lifetime.Object && p.Value.Keys.Any(t => t >= e.TimestampQpc)) ||
            correlations.Any(p => (p.Key.From == lifetime.Object || p.Key.To == lifetime.Object) && p.Key.Qpc >= e.TimestampQpc && p.Value.Linked))
            throw new InvalidDataException("Destruction conflicts with later observations of the same full identity.");
        var fact = new Fact<ObjectLifetimePayload>(e.TimestampQpc, e.IngestOrdinal, lifetime);
        Reserve(fact);
        destroyed.Add(lifetime.Object, fact);
        foreach (var edge in edges.Values.Select(t => At(t, e.TimestampQpc)).OfType<Fact<TreeEdgePayload>>()
            .Where(f => f.Value.Child == lifetime.Object || f.Value.Parent == lifetime.Object))
            yield return output.Emit(source, Row(edge, e.TimestampQpc, context), true);
    }

    private IEnumerable<ProjectionChange> Correlate(CorrelationPayload link, NormalizedEvent source, ProcessingContext context, Output output, bool history = true)
    {
        var e = source.Envelope;
        CheckAlive(link.From, e.TimestampQpc);
        CheckAlive(link.To, e.TimestampQpc);
        // ResourceUsageProcessor owns these rows and both endpoint histories.
        if (link.Kind is CorrelationKind.VisualResource or CorrelationKind.ResourceBacking) yield break;
        var key = (link.From, link.To, link.Kind, e.TimestampQpc);
        if (correlations.TryGetValue(key, out var same))
        {
            if (same != link) throw new InvalidDataException("Conflicting correlation facts at the same event time.");
            yield break;
        }
        Reserve(new { e.TimestampQpc, Link = link });
        correlations.Add(key, link);
        var quality = Partial(context.Quality,
            "Instant-only correlation evidence; the current query/writer cannot encode an open temporal link.");
        yield return output.Emit(source, new CorrelationEntry(link.From, link.To, link.Kind,
            new(e.TimestampQpc, e.TimestampQpc), quality), !link.Linked);
        // P09 owns CorePeerAssociation history; emitting it twice violates writer keys.
        if (e.Identity.EventId == 4 || !history) yield break;
        yield return output.Emit(source, History(link.From,
            new CorrelationObservation(link.To, link.Kind, link.Linked, true), link.Cause, source, context));
        yield return output.Emit(source, History(link.To,
            new CorrelationObservation(link.From, link.Kind, link.Linked, false), link.Cause, source, context));
    }

    private IEnumerable<ProjectionChange> InvalidateRows(Gap gap, NormalizedEvent source, ProcessingContext context, Output output)
    {
        if (edges.Any(p => (gap.Epoch is null || p.Key.Object.ProducerEpoch == gap.Epoch) && p.Value.Keys.Any(t => t > gap.Qpc)) ||
            properties.Any(p => (gap.Epoch is null || p.Key.Object.ProducerEpoch == gap.Epoch) && p.Value.Keys.Any(t => t > gap.Qpc)))
            throw new InvalidDataException("Out-of-order invalidation needs backdated composition snapshot repair.");
        Reserve(gap);
        gaps.Add(gap);
        foreach (var edge in edges.Values.Select(t => At(t, gap.Qpc)).OfType<Fact<TreeEdgePayload>>()
            .Where(f => (gap.Epoch is null || f.Value.Child.ProducerEpoch == gap.Epoch) &&
                !Retired(f, gap.Qpc) && !Dead(f.Value.Child, gap.Qpc) && (f.Value.Parent is not { } parent || !Dead(parent, gap.Qpc)) &&
                !RevisedBySource(f.Value, source)))
            yield return output.Emit(source, Row(edge, gap.Qpc, context));
    }

    private static bool RevisedBySource(TreeEdgePayload edge, NormalizedEvent source)
    {
        var e = source.Envelope;
        if (e.Identity.ProviderId != EventCatalog.RuntimeProviderId) return false;
        if (e.Identity.EventId == 6)
            return EventCodec.ReadPayload<VisualDeltaPayload>(e).Visual == edge.Child;
        if (e.Identity.EventId == 5)
        {
            var fresh = EventCodec.ReadPayload<TreeEdgePayload>(e);
            return fresh.Child == edge.Child && fresh.Membership == edge.Membership;
        }
        if (e.Identity.EventId == 2)
        {
            var lifetime = EventCodec.ReadPayload<ObjectLifetimePayload>(e);
            return lifetime.Operation == LifetimeOperation.Destroy &&
                (lifetime.Object == edge.Child || lifetime.Object == edge.Parent);
        }
        return false;
    }

    private sealed class Output(ProcessingContext context)
    {
        private long bytes = context.OutputBudget.RemainingBytes;
        private int records = context.OutputBudget.RemainingRecords;
        internal ProjectionChange Emit(NormalizedEvent source, IQueryValue value, bool deleted = false)
        {
            var available = Math.Min(bytes, context.OutputBudget.RemainingBytes);
            if (records == 0 || context.OutputBudget.RemainingRecords == 0) throw new ProcessingLimitException();
            var size = BatchAccounting.JsonBytes(value, (int)Math.Min(ProjectionBudget.MaximumRecordBytes,
                available - BatchAccounting.ChangeHeaderBytes)) + BatchAccounting.ChangeHeaderBytes;
            bytes -= size;
            records--;
            // Worker owns shared-budget consumption. Local accounting also bounds direct enumeration.
            return new(source.Envelope.IngestOrdinal, value, deleted);
        }
    }
}
