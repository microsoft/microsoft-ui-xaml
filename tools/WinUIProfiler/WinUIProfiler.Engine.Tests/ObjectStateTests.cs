using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class ObjectStateTests
{
    [Fact]
    public async Task Late_type_metadata_preserves_exact_lifetime_property_and_type_history()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var key = f.Key(1);
        f.Commit((10, new ObjectLifetimePayload(key, LifetimeOperation.Birth, 7, 123, DataQuality.Complete)),
            (20, f.Property(key)), (30, new TypeResolvedPayload(f.Epoch, 7, "Button", ProjectionFixture.Element)));
        var rows = await f.History(key);
        Assert.Equal(new long[] { 10, 20, 30 }, rows.Select(r => r.TimestampQpc));
        Assert.Equal(new long[] { 1, 2, 3 }, rows.Select(r => r.IngestOrdinal));
        Assert.Equal(new LifetimeObservation(LifetimeOperation.Birth, null, 123), rows[0].Observation);
        Assert.Equal(new PropertyObservation(f.Property(key).Property, f.Property(key).Previous, f.Property(key).Current), rows[1].Observation);
        Assert.Equal(new TypeObservation(7, "Button", ProjectionFixture.Element), rows[2].Observation);
        Assert.All(rows, r => Assert.NotEqual(CoverageState.Complete, r.Quality.Coverage));
    }

    [Fact]
    public async Task Metadata_before_birth_and_epoch_qualified_address_reuse_do_not_merge_objects()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var first = f.Key(1);
        var second = f.Key(2);
        var otherEpoch = new ObjectKey(first.Process, first.Domain, Guid.NewGuid(), first.RuntimeObjectId);
        f.Commit((5, new TypeResolvedPayload(f.Epoch, 7, "Button", ProjectionFixture.Element)),
            (10, new ObjectLifetimePayload(first, LifetimeOperation.Birth, 7, 123, DataQuality.Complete)),
            (20, new ObjectLifetimePayload(first, LifetimeOperation.Destroy, null, 123, DataQuality.Complete)),
            (30, new ObjectLifetimePayload(second, LifetimeOperation.Birth, 7, 123, DataQuality.Complete)),
            (40, new ObjectLifetimePayload(otherEpoch, LifetimeOperation.BaselinePresent, 7, 123, ProjectionFixture.Partial)));
        Assert.Equal(3, (await f.History(first)).Count);
        Assert.Equal("Button", Assert.IsType<LifetimeObservation>((await f.History(second))[0].Observation).TypeName);
        Assert.Null(Assert.IsType<LifetimeObservation>((await f.History(otherEpoch))[0].Observation).TypeName);
    }

    [Fact]
    public async Task Unknown_birth_is_not_synthesized_from_property_or_membership()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var key = f.Key(2);
        f.Commit((10, f.Property(key)), (20, f.Edge(key, f.Key(1), RelationshipKind.Content)));
        var rows = await f.History(key);
        Assert.Collection(rows, r => Assert.IsType<PropertyObservation>(r.Observation),
            r => Assert.IsType<MembershipObservation>(r.Observation));
        Assert.All(rows, r => Assert.Equal(CoverageState.Partial, r.Quality.Coverage));
        Assert.All(rows, r => Assert.Equal(CauseKind.Unknown, r.Cause.Kind));
    }

    [Fact]
    public async Task History_orders_by_event_time_not_arrival_and_destroyed_objects_remain_queryable()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var key = f.Key(1);
        f.Commit((30, new ObjectLifetimePayload(key, LifetimeOperation.Destroy, 7, null, DataQuality.Complete)));
        f.Commit((10, new ObjectLifetimePayload(key, LifetimeOperation.Birth, 7, null, DataQuality.Complete)),
            (20, f.Property(key)), (15, new TypeResolvedPayload(f.Epoch, 7, "Button", ProjectionFixture.Element)));
        Assert.Equal(new long[] { 10, 15, 20, 30 }, (await f.History(key)).Select(r => r.TimestampQpc));
        Assert.Equal(new long[] { 2, 4, 3, 1 }, (await f.History(key)).Select(r => r.IngestOrdinal));
    }

    [Fact]
    public async Task Core_peer_associations_are_typed_bidirectional_history()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var core = f.Key(1);
        var peer = new ObjectKey(f.Scope.Process, IdentityDomain.XamlPeer, f.Epoch, 1);
        f.Commit((10, new AssociationPayload(core, peer, true)), (20, new AssociationPayload(core, peer, false)));
        Assert.Equal(new[] { true, false }, (await f.History(core)).Select(r => Assert.IsType<CorrelationObservation>(r.Observation).Linked));
        Assert.All(await f.History(peer), r => Assert.False(Assert.IsType<CorrelationObservation>(r.Observation).IsFrom));
    }

    [Fact]
    public async Task Capture_chunks_are_invisible_until_validated_end_and_never_invent_birth()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var id = Guid.NewGuid();
        var key = f.Key(1);
        f.Commit((10, f.Boundary(id, CapturePhase.Begin)), (20, f.Chunk(id, key)));
        Assert.Empty(await f.History(key));
        f.PublishBaseline(30, f.Boundary(id, CapturePhase.End), [f.Chunk(id, key)]);
        var rows = await f.History(key);
        Assert.Equal(2, rows.Count);
        var row = Assert.Single(rows, r => r.Observation is LifetimeObservation);
        Assert.Equal(30, row.TimestampQpc);
        Assert.Equal(LifetimeOperation.BaselinePresent, Assert.IsType<LifetimeObservation>(row.Observation).Operation);
        Assert.Equal(CoverageState.Partial, row.Quality.Coverage);
    }

    [Theory]
    [InlineData("missing")]
    [InlineData("duplicate")]
    [InlineData("scope")]
    [InlineData("loss")]
    public async Task Invalid_or_interrupted_baseline_never_publishes_staged_objects(string problem)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var id = Guid.NewGuid();
        var key = f.Key(1);
        f.Commit((10, f.Boundary(id, CapturePhase.Begin)));
        if (problem != "missing") f.Commit((20, f.Chunk(id, key)));
        if (problem == "duplicate") f.Commit((21, f.Chunk(id, key)));
        if (problem == "loss") f.Commit((22, new LossPayload(f.Epoch, new(15, 22), 1, "gap")));
        f.Commit((30, f.Boundary(id, CapturePhase.End) with { RootId = problem == "scope" ? Guid.NewGuid() : f.Root }));
        Assert.Empty(await f.History(key));
    }

    [Fact]
    public void Object_and_metadata_limits_stop_explicitly_instead_of_evicting()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor", 1);
        f.Commit((10, f.Property(f.Key(1))));
        Assert.Throws<ProcessingLimitException>(() => f.Commit((20, f.Property(f.Key(2)))));
        Assert.Equal(1, f.Store.Watermark.IngestOrdinal);
    }

    [Fact]
    public void Type_metadata_has_its_own_bounded_storage_and_immutable_identity()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor", 1);
        f.Commit((10, new TypeResolvedPayload(f.Epoch, 1, "First")));
        Assert.Throws<ProcessingLimitException>(() => f.Commit((20, new TypeResolvedPayload(f.Epoch, 2, "Second"))));
        Assert.Throws<InvalidDataException>(() => f.Commit((20, new TypeResolvedPayload(f.Epoch, 1, "Changed"))));
    }

    [Fact]
    public async Task Explicit_causal_attempt_and_unavailable_property_values_are_preserved()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var key = f.Key(1);
        var cause = new ChangeCause(CauseKind.ExplicitAttempt, new(f.Epoch, 1, f.Root, 1), null);
        var property = f.Property(key) with
        {
            Previous = new(ValueKind.Missing, ValueUnit.Scalar, null, null, null, null,
                new(MeasurementKind.Unavailable, CoverageState.Partial, "Previous value not observed.")),
            Cause = cause
        };
        f.Commit((10, property));
        var row = Assert.Single(await f.History(key));
        Assert.Equal(cause, row.Cause);
        Assert.Equal(property.Previous, Assert.IsType<PropertyObservation>(row.Observation).Previous);
    }

    [Theory]
    [InlineData("orphan")]
    [InlineData("cycle")]
    [InlineData("duplicate-object")]
    [InlineData("mutation")]
    public async Task Raw_capture_end_is_not_a_verified_baseline_even_when_all_chunks_arrived(string problem)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var id = Guid.NewGuid();
        var chunk = f.Chunk(id, f.Key(1));
        chunk = problem switch
        {
            "orphan" => chunk with { Tree = [f.Edge(f.Key(1), f.Key(2), RelationshipKind.Child)] },
            "cycle" => chunk with
            {
                Objects = [.. chunk.Objects, chunk.Objects[0] with { Key = f.Key(2) }],
                Tree = [f.Edge(f.Key(1), f.Key(2), RelationshipKind.Child), f.Edge(f.Key(2), f.Key(1), RelationshipKind.Template)]
            },
            "duplicate-object" => chunk with { Objects = [chunk.Objects[0], chunk.Objects[0]] },
            _ => chunk
        };
        f.Commit((10, f.Boundary(id, CapturePhase.Begin)), (20, chunk));
        if (problem == "mutation") f.Commit((25, f.Property(f.Key(1))));
        f.Commit((30, f.Boundary(id, CapturePhase.End)));
        Assert.DoesNotContain(await f.History(f.Key(1)), r => r.Observation is LifetimeObservation);
        Assert.Empty((await f.Tree(30)).Items);
    }

    [Fact]
    public async Task Worker_stops_at_processor_index_limit_and_keeps_the_previous_durable_boundary()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor", 1);
        await using var worker = new ProcessingWorker(f.Store, f.Info,
            new(BatchItems: 1), f.Processors);
        foreach (var source in new[] { f.Source(10, 1, f.Property(f.Key(1))), f.Source(20, 2, f.Property(f.Key(2))) })
            Assert.True(worker.TryEnqueue(new(source, new(f.Scope.Process, source.Envelope.TimestampQpc, null))));
        worker.Complete(SourceCompletion.FiniteEnd);
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        Assert.Equal(1, result.DurableBoundary.IngestOrdinal);
        Assert.Single(await f.History(f.Key(1)));
    }

    [Fact]
    public async Task Verified_baseline_preserves_End_quality_and_never_repersists_old_chunks()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var id = Guid.NewGuid();
        var end = f.Boundary(id, CapturePhase.End) with { Quality = StoreFixture.Partial };
        f.PublishBaseline(30, end, [f.Chunk(id, f.Key(1))]);
        Assert.Equal(1, f.Store.Watermark.IngestOrdinal);
        Assert.All(await f.History(f.Key(1)), r =>
        {
            Assert.Equal(1, r.IngestOrdinal);
            Assert.Equal(30, r.TimestampQpc);
            Assert.Equal(MeasurementKind.Estimated, r.Quality.Kind);
        });
        Assert.Equal(MeasurementKind.Estimated, (await f.Tree(30)).Items.Single().Quality.Kind);
    }

    [Theory]
    [InlineData("scope")]
    [InlineData("orphan")]
    [InlineData("cycle")]
    public void Verified_publication_defensively_checks_structure_before_yielding_any_candidate(string problem)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var id = Guid.NewGuid();
        var chunk = f.Chunk(id, f.Key(1));
        if (problem == "scope") chunk = chunk with { RootId = Guid.NewGuid(), Tree = [] };
        if (problem == "orphan") chunk = chunk with { Tree = [f.Edge(f.Key(1), f.Key(2), RelationshipKind.Child)] };
        if (problem == "cycle") chunk = chunk with
        {
            Objects = [.. chunk.Objects, chunk.Objects[0] with { Key = f.Key(2) }],
            Tree = [f.Edge(f.Key(1), f.Key(2), RelationshipKind.Child), f.Edge(f.Key(2), f.Key(1), RelationshipKind.Template)]
        };
        var source = f.Source(30, 1, f.Boundary(id, CapturePhase.End));
        foreach (var processor in f.Processors.Cast<IVerifiedBaselineProcessor>())
        {
            using var candidates = processor.PublishBaseline(source, [chunk],
                new(f.Scope, new(1, 30), ProjectionFixture.Partial, new(SessionWriter.MaximumBatchBytes))).GetEnumerator();
            Assert.Throws<ArgumentException>(() => candidates.MoveNext());
        }
        Assert.Equal(0, f.Store.Watermark.IngestOrdinal);
    }
}

internal sealed class ProjectionFixture : IDisposable
{
    internal static DataQuality Partial { get; } = new(MeasurementKind.Measured, CoverageState.Partial, "No complete historical baseline.");
    internal static XamlTypeClassification Element { get; } = new(XamlObjectKind.UIElement, ClassificationEvidence.RuntimeBridge, "runtime");
    internal SessionScope Scope { get; } = new(Guid.NewGuid(), new(42, 123456, Guid.NewGuid()));
    internal Guid Epoch { get; } = Guid.NewGuid();
    internal Guid Root { get; } = Guid.NewGuid();
    internal SessionInfo Info { get; } = new(SessionMode.Offline, SessionStatus.Recording, 1_000_000, null, [], null);
    internal SessionStore Store { get; }
    private SessionWriter? writer;
    internal IEventProcessor[] Processors { get; }
    private readonly string path = Path.GetFullPath(Path.Combine("TestResults", Guid.NewGuid() + ".sqlite"));
    internal ProjectionFixture(string processor, int maximumEntries = 65536)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        Store = SessionStore.Create(path, Scope, Info, Partial);
        Processors = processor.Split(',').Select(name =>
        {
            var type = typeof(IEventProcessor).Assembly.GetType("WinUIProfiler.Engine.Processing." + name);
            Assert.NotNull(type);
            return (IEventProcessor)Activator.CreateInstance(type, maximumEntries)!;
        }).ToArray();
    }
    internal ObjectKey Key(ulong id) => new(Scope.Process, IdentityDomain.XamlCore, Epoch, id);
    internal XamlPropertyChangedPayload Property(ObjectKey key) => new(key, new(1, "UIElement", "Opacity"),
        new(ValueKind.Number, ValueUnit.Scalar, null, 1, null, null, DataQuality.Complete),
        new(ValueKind.Number, ValueUnit.Scalar, null, .5, null, null, DataQuality.Complete), null);
    internal TreeEdgePayload Edge(ObjectKey child, ObjectKey? parent = null, RelationshipKind relationship = RelationshipKind.Root,
        TreeMembershipKind membership = TreeMembershipKind.Visual, bool connected = true) =>
        new(parent, child, relationship, 0, Root, 1, connected, Membership: membership);
    internal CaptureBoundaryPayload Boundary(Guid id, CapturePhase phase) =>
        new(id, CaptureKind.Baseline, phase, Root, 1, [], phase == CapturePhase.End ? 1u : null, Partial);
    internal CaptureChunkPayload Chunk(Guid id, ObjectKey key) => new(id, 0, Root, 1,
        [new(key, null, null, new(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null,
            new(MeasurementKind.Unavailable, CoverageState.Partial, "No object size observed.")), Element)],
        [], [Edge(key)], [], [], [], [], [], []);
    internal IReadOnlyList<ProjectionChange> Commit(params (long Qpc, IEventPayload Payload)[] input)
    {
        var first = Store.Watermark.IngestOrdinal;
        var events = input.Select((item, i) => Source(item.Qpc, first + i + 1, item.Payload)).ToArray();
        var context = new ProcessingContext(Scope, new(first + events.Length,
            Math.Max(Store.Watermark.TimestampQpc, input.Max(i => i.Qpc))), Partial, new(SessionWriter.MaximumBatchBytes));
        var changes = Processors.SelectMany(p => p.Process(events, context)).ToArray();
        (writer ??= Store.CreateWriter()).Commit(new(events, changes, Info, Partial));
        return changes;
    }
    internal NormalizedEvent Source(long qpc, long ordinal, IEventPayload payload)
    {
        var identity = EventCatalog.All.Single(d => d.PayloadType == payload.GetType() &&
            d.Identity.ProviderId == EventCatalog.RuntimeProviderId).Identity;
        var epoch = payload switch
        {
            ObjectLifetimePayload p => p.Object.ProducerEpoch,
            TypeResolvedPayload p => p.ProducerEpoch,
            LossPayload p => p.ProducerEpoch,
            _ => Epoch
        };
        return EventCodec.Normalize(new(Scope.SessionId, Scope.Process, 42, 1, qpc, ordinal, (ulong)ordinal, null,
            identity, EventCodec.EncodePayload(payload, Scope.Process)), new(identity.ProviderId, 42, epoch));
    }
    internal async Task<IReadOnlyList<ObjectHistoryEntry>> History(ObjectKey key) =>
        (await Store.Queries.GetObjectHistory(new(Scope, key, new(0, 1000), new(128)), default)).Items;
    internal async Task<QueryPage<TreeEntry>> Tree(long at, ObjectKey? root = null, CommittedWatermark? watermark = null) =>
        await Store.Queries.GetXamlTree(new(Scope, at, new(128, 0, watermark), root), default);
    internal SessionStore Reopen() => SessionStore.OpenReadOnly(path);
    internal void RecordHealth(HealthSnapshot health) => (writer ??= Store.CreateWriter()).CommitHealth(health);
    internal void PublishBaseline(long qpc, CaptureBoundaryPayload end, IReadOnlyList<CaptureChunkPayload> chunks)
    {
        var source = Source(qpc, Store.Watermark.IngestOrdinal + 1, end);
        var context = new ProcessingContext(Scope, new(source.Envelope.IngestOrdinal,
            Math.Max(qpc, Store.Watermark.TimestampQpc)), Partial, new(SessionWriter.MaximumBatchBytes));
        var changes = Processors.Cast<IVerifiedBaselineProcessor>()
            .SelectMany(p => p.PublishBaseline(source, chunks, context)).ToArray();
        (writer ??= Store.CreateWriter()).Commit(new([source], changes, Info, Partial));
    }
    public void Dispose()
    {
        Store.Dispose();
        foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
            if (File.Exists(path + suffix)) File.Delete(path + suffix);
    }
}
