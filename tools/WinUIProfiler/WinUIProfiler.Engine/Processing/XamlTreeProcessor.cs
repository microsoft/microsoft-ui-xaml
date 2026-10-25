using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;
using static WinUIProfiler.Engine.Processing.ProjectionSupport;

namespace WinUIProfiler.Engine.Processing;

public sealed class XamlTreeProcessor : IVerifiedBaselineProcessor
{
    private readonly ProjectionIndexBudget budget;
    private readonly Dictionary<(ObjectKey Object, RelationshipKind Relationship, TreeMembershipKind Membership),
        (TreeEntry Row, long Qpc, long Ordinal, bool Deleted)> edges = [];
    private long failures;
    private bool invalidated;
    public XamlTreeProcessor(int maximumEntries = 65536)
    {
        budget = new(maximumEntries);
    }
    public void Invalidate(HealthSnapshot health)
    {
        // Explicit producer loss has an exact persisted interval, applied by the query.
        // The worker reports it before the entire containing batch; using its first
        // source timestamp would move the gap backward and invalidate earlier state.
        var count = ContinuityFailures(health) - health.Count(IngestionIssue.ProducerLoss);
        if (count > failures) invalidated = true;
        failures = Math.Max(count, failures);
    }
    private ProjectionChange Revision(TreeEntry row, NormalizedEvent source, ProcessingContext context, bool deleted = false)
    {
        var key = (row.Object, row.Relationship, row.Membership);
        if (!deleted && row.Connected && row.Parent is { } parent)
        {
            var pending = new Stack<ObjectKey>();
            var visited = new HashSet<ObjectKey>();
            pending.Push(parent);
            while (pending.TryPop(out var node))
            {
                if (node == row.Object) throw new InvalidDataException("A XAML membership cycle was observed.");
                if (!visited.Add(node)) continue;
                foreach (var ancestor in edges.Values.Where(v => !v.Deleted && v.Row.Object == node && v.Row.Connected &&
                    v.Row.Membership == row.Membership && v.Qpc <= source.Envelope.TimestampQpc))
                    if (ancestor.Row.Parent is { } next) pending.Push(next);
            }
        }
        if (!edges.TryGetValue(key, out var old))
        {
            budget.Add(row); edges.Add(key, (row, source.Envelope.TimestampQpc, source.Envelope.IngestOrdinal, deleted));
        }
        else if ((source.Envelope.TimestampQpc, source.Envelope.IngestOrdinal).CompareTo((old.Qpc, old.Ordinal)) > 0)
        {
            budget.Remove(old.Row); budget.Add(row);
            edges[key] = (row, source.Envelope.TimestampQpc, source.Envelope.IngestOrdinal, deleted);
        }
        return Candidate(source.Envelope.IngestOrdinal, row, context, deleted);
    }
    public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
    {
        foreach (var source in events)
        {
            ValidateScope(source, context);
            var e = source.Envelope;
            var runtime = e.Identity.ProviderId == EventCatalog.RuntimeProviderId;
            var edge = runtime && e.Identity.EventId == 5 ? EventCodec.ReadPayload<TreeEdgePayload>(e) : null;
            var lifetime = runtime && e.Identity.EventId == 2 ? EventCodec.ReadPayload<ObjectLifetimePayload>(e) : null;
            if (invalidated)
            {
                invalidated = false;
                foreach (var old in edges.Values.Where(v => !v.Deleted).ToArray())
                {
                    if (edge is not null && (edge.Child, edge.Relationship, edge.Membership) == (old.Row.Object, old.Row.Relationship, old.Row.Membership) ||
                        lifetime?.Operation == LifetimeOperation.Destroy && (old.Row.Object == lifetime.Object || old.Row.Parent == lifetime.Object))
                        continue;
                    yield return Revision(old.Row with { Quality = Uncertain("Continuity failed; no fresh membership observation covers this state.") }, source, context);
                }
            }
            if (edge?.Child.Domain == IdentityDomain.XamlCore) yield return Project(edge);
            if (lifetime?.Operation == LifetimeOperation.Destroy)
                foreach (var old in edges.Values.Where(v => !v.Deleted && (v.Row.Object == lifetime.Object || v.Row.Parent == lifetime.Object)).ToArray())
                    yield return Revision(old.Row, source, context, true);

            ProjectionChange Project(TreeEdgePayload observed) => Revision(new(observed.Child, observed.Parent,
                observed.Relationship, observed.Order, observed.Connected, observed.RootId, observed.DispatcherId, null,
                Partial(context.Quality, "No complete historical tree baseline has been established."),
                observed.Membership, UnknownType), source, context);
        }
    }
    public IEnumerable<ProjectionChange> PublishBaseline(NormalizedEvent validatedEnd,
        IReadOnlyList<CaptureChunkPayload> chunks, ProcessingContext context)
    {
        ValidatePublication(validatedEnd, chunks, context);
        var boundary = EventCodec.ReadPayload<CaptureBoundaryPayload>(validatedEnd.Envelope);
        var quality = Combine(context.Quality, boundary.Quality);
        var metadata = chunks.SelectMany(c => c.Objects).ToDictionary(o => o.Key);
        var captured = chunks.SelectMany(c => c.Tree).Where(t => t.Child.Domain == IdentityDomain.XamlCore).ToArray();
        if (boundary.Quality.Coverage == CoverageState.Complete &&
            boundary.Domains.Any(d => d.Domain.EventId == 5 && d.Quality.Coverage == CoverageState.Complete))
        {
            var present = captured.Select(e => (e.Child, e.Relationship, e.Membership)).ToHashSet();
            foreach (var old in edges.Values.Where(v => !v.Deleted &&
                v.Row.Object.ProducerEpoch == validatedEnd.Stream.ProducerEpoch &&
                (boundary.RootId is null || v.Row.RootId == boundary.RootId) &&
                (boundary.DispatcherId is null || v.Row.DispatcherId == boundary.DispatcherId)).ToArray())
                if (!present.Contains((old.Row.Object, old.Row.Relationship, old.Row.Membership)))
                    yield return Revision(old.Row, validatedEnd, context, true);
        }
        foreach (var edge in captured)
        {
            var obj = metadata[edge.Child];
            yield return Revision(new(edge.Child, edge.Parent, edge.Relationship, edge.Order, edge.Connected,
                edge.RootId, edge.DispatcherId, obj.TypeName,
                Partial(quality, "Validated non-atomic capture establishes observed membership, not historical birth."),
                edge.Membership, obj.Classification ?? UnknownType), validatedEnd, context);
        }
    }
}
