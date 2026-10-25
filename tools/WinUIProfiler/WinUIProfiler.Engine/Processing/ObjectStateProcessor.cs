using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Processing.ProjectionSupport;

namespace WinUIProfiler.Engine.Processing;

public sealed class ObjectStateProcessor : IVerifiedBaselineProcessor
{
    private readonly ProjectionIndexBudget budget;
    private readonly Dictionary<ObjectKey, ulong?> objects = [];
    private readonly Dictionary<(Guid Epoch, ulong Type), TypeResolvedPayload> types = [];

    public ObjectStateProcessor(int maximumEntries = 65536)
    {
        budget = new(maximumEntries);
    }
    public void Invalidate(HealthSnapshot health)
    {
        // Rows describe individual observations, not a carried-forward measured state.
        // The owner retains health/coverage; raw capture chunks are never deltas.
    }
    private void Remember(ObjectKey key, ulong? typeId = null)
    {
        if (!objects.TryGetValue(key, out var old))
        {
            budget.Add(key); objects.Add(key, typeId);
        }
        else if (typeId is { } type)
        {
            if (old is { } known && known != type) throw new InvalidDataException("An object changed its immutable runtime type identity.");
            objects[key] = type;
        }
    }
    private void Remember(TypeResolvedPayload type)
    {
        var key = (type.ProducerEpoch, type.TypeId);
        if (types.TryGetValue(key, out var old))
        {
            if (old != type) throw new InvalidDataException("A producer reused an immutable runtime type identity.");
            return;
        }
        budget.Add(type); types.Add(key, type);
    }
    public IEnumerable<Storage.ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
    {
        foreach (var source in events)
        {
            ValidateScope(source, context);
            var e = source.Envelope;
            Storage.ProjectionChange History(ObjectKey key, ObjectObservation observation, ChangeCause? cause = null, DataQuality? quality = null) =>
                Candidate(e.IngestOrdinal, new ObjectHistoryEntry(key, e.TimestampQpc, e.IngestOrdinal, observation,
                    cause ?? UnknownCause, Partial(Combine(quality ?? context.Quality, context.Quality),
                        "Historical birth and complete baseline coverage are not implied by this observation.")), context);
            if (e.Identity.ProviderId != EventCatalog.RuntimeProviderId) continue;
            switch (e.Identity.EventId)
            {
                case 2:
                    var lifetime = EventCodec.ReadPayload<ObjectLifetimePayload>(e);
                    Remember(lifetime.Object, lifetime.TypeId);
                    var typeId = objects[lifetime.Object];
                    var resolved = typeId is { } id ? types.GetValueOrDefault((lifetime.Object.ProducerEpoch, id)) : null;
                    yield return History(lifetime.Object, new LifetimeObservation(lifetime.Operation, resolved?.Name,
                        lifetime.Address, lifetime.Classification ?? resolved?.Classification), lifetime.Cause, lifetime.Quality);
                    if (resolved is not null && lifetime.Operation != LifetimeOperation.Destroy)
                        yield return History(lifetime.Object, new TypeObservation(resolved.TypeId, resolved.Name, resolved.Classification));
                    break;
                case 3:
                    var type = EventCodec.ReadPayload<TypeResolvedPayload>(e);
                    Remember(type);
                    foreach (var pair in objects.Where(p => p.Key.ProducerEpoch == type.ProducerEpoch && p.Value == type.TypeId))
                        yield return History(pair.Key, new TypeObservation(type.TypeId, type.Name, type.Classification));
                    break;
                case 4:
                    var association = EventCodec.ReadPayload<AssociationPayload>(e);
                    Remember(association.Core); Remember(association.Peer);
                    yield return History(association.Core, new CorrelationObservation(association.Peer, CorrelationKind.CorePeer, association.Linked, true), association.Cause);
                    yield return History(association.Peer, new CorrelationObservation(association.Core, CorrelationKind.CorePeer, association.Linked, false), association.Cause);
                    break;
                case 5:
                    var edge = EventCodec.ReadPayload<TreeEdgePayload>(e);
                    if (edge.Child.Domain != IdentityDomain.XamlCore) break;
                    Remember(edge.Child);
                    yield return History(edge.Child, Membership(edge), edge.Cause);
                    break;
                case 16:
                    var property = EventCodec.ReadPayload<XamlPropertyChangedPayload>(e);
                    Remember(property.Object);
                    yield return History(property.Object, new PropertyObservation(property.Property, property.Previous, property.Current), property.Cause);
                    break;
            }
        }
    }
    public IEnumerable<Storage.ProjectionChange> PublishBaseline(NormalizedEvent validatedEnd,
        IReadOnlyList<CaptureChunkPayload> chunks, ProcessingContext context)
    {
        ValidatePublication(validatedEnd, chunks, context);
        var e = validatedEnd.Envelope;
        var quality = Combine(context.Quality, EventCodec.ReadPayload<CaptureBoundaryPayload>(e).Quality);
        Storage.ProjectionChange History(ObjectKey key, ObjectObservation observation, ChangeCause? cause = null) =>
            Candidate(e.IngestOrdinal, new ObjectHistoryEntry(key, e.TimestampQpc, e.IngestOrdinal, observation,
                cause ?? UnknownCause, Partial(quality,
                    "Validated capture establishes presence, not historical birth or an atomic lifetime baseline.")), context);
        foreach (var type in chunks.SelectMany(c => c.Types))
        {
            Remember(type);
            foreach (var pair in objects.Where(p => p.Key.ProducerEpoch == type.ProducerEpoch && p.Value == type.TypeId))
                yield return History(pair.Key, new TypeObservation(type.TypeId, type.Name, type.Classification));
        }
        foreach (var obj in chunks.SelectMany(c => c.Objects))
        {
            Remember(obj.Key);
            yield return History(obj.Key, new LifetimeObservation(LifetimeOperation.BaselinePresent,
                obj.TypeName, obj.Address, obj.Classification));
        }
        foreach (var association in chunks.SelectMany(c => c.Associations))
        {
            Remember(association.Core); Remember(association.Peer);
            yield return History(association.Core, new CorrelationObservation(association.Peer, CorrelationKind.CorePeer, association.Linked, true), association.Cause);
            yield return History(association.Peer, new CorrelationObservation(association.Core, CorrelationKind.CorePeer, association.Linked, false), association.Cause);
        }
        foreach (var edge in chunks.SelectMany(c => c.Tree).Where(t => t.Child.Domain == IdentityDomain.XamlCore))
            yield return History(edge.Child, Membership(edge), edge.Cause);
    }
    private static MembershipObservation Membership(TreeEdgePayload edge) =>
        new(edge.Parent, edge.Relationship, edge.Order, edge.RootId, edge.DispatcherId, edge.Connected, edge.Membership);
}
