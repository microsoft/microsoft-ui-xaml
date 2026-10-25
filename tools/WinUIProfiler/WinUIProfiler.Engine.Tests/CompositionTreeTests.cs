using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class CompositionTreeTests
{
    [Fact]
    public async Task Core_compnodes_canonical_visuals_and_shared_resources_keep_many_to_many_reverse_links()
    {
        using var f = new Fixture(includeResources: true);
        var core = f.Key(1, IdentityDomain.XamlCore);
        var otherCore = f.Key(2, IdentityDomain.XamlCore);
        var peer = f.Key(1, IdentityDomain.XamlPeer);
        var node = f.Key(1, IdentityDomain.CompNode);
        var a = f.Key(1);
        var b = f.Key(2);
        var resource = f.Key(1, IdentityDomain.Resource);
        f.Commit((10, f.Edge(node)), (11, f.Edge(a)), (12, f.Edge(b)),
            (20, new AssociationPayload(core, peer, true)),
            (20, new CorrelationPayload(core, node, CorrelationKind.XamlCompNode, true)),
            (20, new CorrelationPayload(node, a, CorrelationKind.CompNodeVisual, true)),
            (20, new CorrelationPayload(node, b, CorrelationKind.CompNodeVisual, true)),
            (20, new CorrelationPayload(core, a, CorrelationKind.XamlVisual, true)),
            (20, new CorrelationPayload(core, b, CorrelationKind.XamlVisual, true)),
            (20, new CorrelationPayload(otherCore, a, CorrelationKind.XamlVisual, true)),
            (20, new CorrelationPayload(a, resource, CorrelationKind.VisualResource, true)),
            (20, new CorrelationPayload(b, resource, CorrelationKind.VisualResource, true)));
        Assert.Equal(3, (await f.Tree(20)).Items.Count);
        Assert.Equal(2, (await f.Links(core, 20)).Items.Count(r => r.Kind == CorrelationKind.XamlVisual));
        Assert.Equal(2, (await f.Links(resource, 20)).Items.Count);
        Assert.Equal(2, (await f.Links(a, 20)).Items.Count(r => r.Kind == CorrelationKind.XamlVisual));
        Assert.Contains((await f.Links(peer, 20)).Items, r => r.From == core && r.To == peer);
        var reverse = await f.History(resource);
        Assert.Equal(2, reverse.Count);
        Assert.All(reverse, r => Assert.False(Assert.IsType<CorrelationObservation>(r.Observation).IsFrom));
        Assert.All((await f.Tree(20)).Items, r => Assert.Equal(CoverageState.Partial, r.Topology.Quality.Coverage));
        using var reopened = f.Reopen();
        Assert.Equal(2, (await reopened.Queries.GetCorrelations(new(f.Scope, resource, 20, new(128)), default)).Items.Count);
    }

    [Fact]
    public async Task Property_snapshots_preserve_units_cause_and_exact_source_without_inventing_topology()
    {
        using var f = new Fixture();
        var visual = f.Key(1);
        var cause = new ChangeCause(CauseKind.OutsideAttempt, null, null);
        f.Commit((5, f.Delta(visual, .2) with { Cause = cause }));
        Assert.Empty((await f.Tree(5)).Items);
        f.Commit((10, f.Edge(visual)), (20, f.Delta(visual, .7)));
        Assert.Equal(.2, (await f.Tree(10)).Items.Single().Properties.Single().Value.Number);
        var row = (await f.Tree(20)).Items.Single();
        Assert.Equal(.7, row.Properties.Single().Value.Number);
        Assert.Equal(ValueUnit.Scalar, row.Properties.Single().Value.Unit);
        var history = await f.History(visual);
        Assert.Equal(cause, history.Single(r => r.TimestampQpc == 5).Cause);
        Assert.All(history, r => Assert.True(r.IngestOrdinal > 0));
        Assert.DoesNotContain(history, r => r.Observation is LifetimeObservation);
        using var reopened = f.Reopen();
        Assert.Equal(row, (await reopened.Queries.GetCompositionTree(new(f.Scope, 20, new(128)), default)).Items.Single(),
            new CompositionComparer());
    }

    [Fact]
    public async Task Reparent_and_removal_are_membership_observations_not_destruction()
    {
        using var f = new Fixture();
        var a = f.Key(1);
        var b = f.Key(2);
        var child = f.Key(3);
        f.Commit((10, f.Edge(a)), (11, f.Edge(b)), (20, f.Edge(child, a)),
            (30, f.Edge(child, b)), (40, f.Edge(child, b) with { Connected = false }));
        Assert.Equal(a, (await f.Tree(29)).Items.Single(r => r.Topology.Object == child).Topology.Parent);
        Assert.Equal(b, (await f.Tree(30)).Items.Single(r => r.Topology.Object == child).Topology.Parent);
        Assert.False((await f.Tree(40)).Items.Single(r => r.Topology.Object == child).Topology.Connected);
        Assert.DoesNotContain(await f.History(child), r => r.Observation is LifetimeObservation);
        f.Commit((50, new ObjectLifetimePayload(child, LifetimeOperation.Destroy, null, null, DataQuality.Complete)));
        Assert.DoesNotContain((await f.Tree(50)).Items, r => r.Topology.Object == child);
    }

    [Fact]
    public async Task Late_same_channel_membership_and_properties_are_event_ordered_and_frozen()
    {
        using var f = new Fixture();
        var root = f.Key(1);
        var child = f.Key(2);
        f.Commit((10, f.Edge(root)), (20, f.Edge(child, root)), (40, f.Edge(child, root) with { Connected = false }));
        var frozen = (await f.Tree(30)).Context.Watermark;
        f.Commit((25, f.Edge(child, root) with { Connected = false }));
        Assert.True((await f.Tree(30, frozen)).Items.Single(r => r.Topology.Object == child).Topology.Connected);
        Assert.False((await f.Tree(30)).Items.Single(r => r.Topology.Object == child).Topology.Connected);
        f.Commit((50, f.Delta(root, .9)), (45, f.Delta(root, .4)));
        Assert.Equal(.4, (await f.Tree(45)).Items.Single(r => r.Topology.Object == root).Properties.Single().Value.Number);
        Assert.Equal(.9, (await f.Tree(50)).Items.Single(r => r.Topology.Object == root).Properties.Single().Value.Number);
    }

    [Fact]
    public async Task Epochs_domains_and_unknown_endpoints_are_not_collapsed()
    {
        using var f = new Fixture();
        var visual = f.Key(1);
        var next = new ObjectKey(f.Scope.Process, IdentityDomain.CompositionVisual, Guid.NewGuid(), 1);
        var node = f.Key(1, IdentityDomain.CompNode);
        var unknownCore = f.Key(9, IdentityDomain.XamlCore);
        f.Commit((10, f.Edge(visual)), (11, f.Edge(next)), (12, f.Edge(node)),
            (20, new CorrelationPayload(unknownCore, visual, CorrelationKind.XamlVisual, true)));
        Assert.Equal(3, (await f.Tree(20)).Items.Count);
        Assert.Equal(unknownCore, (await f.Links(visual, 20)).Items.Single().From);
        Assert.Null((await f.Tree(20)).Items.Single(r => r.Topology.Object == visual).Topology.TypeName);
        f.Commit((30, new ObjectLifetimePayload(visual, LifetimeOperation.Destroy, null, null, DataQuality.Complete)));
        Assert.Contains((await f.Tree(30)).Items, r => r.Topology.Object == next);
    }

    [Fact]
    public async Task Relationship_classification_switch_tombstones_the_old_row_key()
    {
        using var f = new Fixture();
        var root = f.Key(1);
        var child = f.Key(2);
        f.Commit((10, f.Edge(root)), (20, f.Edge(child)), (30, f.Edge(child, root)));
        Assert.Single((await f.Tree(30)).Items, r => r.Topology.Object == child);
        Assert.Equal(RelationshipKind.VisualChild, (await f.Tree(30)).Items.Single(r => r.Topology.Object == child).Topology.Relationship);
    }

    [Fact]
    public async Task On_time_loss_degrades_without_detachment_and_fresh_observation_recovers_only_its_channel()
    {
        using var f = new Fixture();
        var a = f.Key(1);
        var b = f.Key(2);
        f.Commit((10, f.Edge(a)), (11, f.Edge(b)));
        var frozen = (await f.Tree(11)).Context.Watermark;
        f.Commit((20, new LossPayload(f.Epoch, new(20, 20), 1, "gap")));
        Assert.All((await f.Tree(20)).Items, r =>
        {
            Assert.True(r.Topology.Connected);
            Assert.Equal(MeasurementKind.Unavailable, r.Topology.Quality.Kind);
        });
        Assert.All((await f.Tree(11, frozen)).Items, r => Assert.Equal(MeasurementKind.Measured, r.Topology.Quality.Kind));
        f.Commit((30, f.Edge(a)));
        Assert.Equal(MeasurementKind.Measured, (await f.Tree(30)).Items.Single(r => r.Topology.Object == a).Topology.Quality.Kind);
        Assert.Equal(MeasurementKind.Unavailable, (await f.Tree(30)).Items.Single(r => r.Topology.Object == b).Topology.Quality.Kind);
    }

    [Fact]
    public void Unrepresentable_backdated_loss_and_property_repairs_fail_explicitly()
    {
        using var f = new Fixture();
        var v = f.Key(1);
        f.Commit((10, f.Edge(v)), (30, f.Delta(v, .8)));
        Assert.Throws<InvalidDataException>(() => f.Commit((20, f.Delta(v, .2, VisualProperty.TransformM11))));
        Assert.Throws<InvalidDataException>(() => f.Commit((40, new LossPayload(f.Epoch, new(15, 25), 1, "late gap"))));
        Assert.Equal(2, f.Store.Watermark.IngestOrdinal);
    }

    [Fact]
    public async Task Correlation_observations_are_instant_not_fabricated_open_lifetimes()
    {
        using var f = new Fixture();
        var core = f.Key(1, IdentityDomain.XamlCore);
        var v = f.Key(1);
        f.Commit((10, new CorrelationPayload(core, v, CorrelationKind.XamlVisual, true)), (20, f.Edge(v)));
        Assert.Single((await f.Links(core, 10)).Items);
        Assert.Empty((await f.Links(core, 19)).Items);
        f.Commit((30, new CorrelationPayload(core, v, CorrelationKind.XamlVisual, false)));
        Assert.Empty((await f.Links(core, 30)).Items);
        Assert.Equal(2, (await f.History(core)).Count);
    }

    [Fact]
    public void Entry_byte_and_incremental_output_limits_are_explicit_without_silent_eviction()
    {
        using var f = new Fixture(new CompositionTreeProcessor(1));
        f.Commit((10, f.Edge(f.Key(1))));
        Assert.Throws<ProcessingLimitException>(() => f.Commit((20, f.Edge(f.Key(2)))));
        using var bytes = new Fixture(new CompositionTreeProcessor(16, 128));
        Assert.Throws<ProcessingLimitException>(() => bytes.Commit((10, bytes.Edge(bytes.Key(1)))));
        using var output = new Fixture();
        var source = output.Source(10, 1, new CorrelationPayload(output.Key(1, IdentityDomain.XamlCore),
            output.Key(1), CorrelationKind.XamlVisual, true));
        var budget = new ProjectionBudget(1300);
        Assert.Throws<ProcessingLimitException>(() => output.Processor.Process([source],
            new(output.Scope, new(1, 10), ProjectionFixture.Partial, budget)).ToArray());
        Assert.Equal(1300, budget.RemainingBytes);
    }

    [Fact]
    public void Cycles_conflicting_facts_and_reuse_of_a_destroyed_full_identity_are_rejected()
    {
        using var f = new Fixture();
        var a = f.Key(1);
        var b = f.Key(2);
        f.Commit((10, f.Edge(b, a)));
        Assert.Throws<InvalidDataException>(() => f.Commit((20, f.Edge(a, b))));
        Assert.Throws<InvalidDataException>(() => f.Commit((10, f.Edge(b))));
        f.Commit((30, new ObjectLifetimePayload(b, LifetimeOperation.Destroy, null, null, DataQuality.Complete)));
        Assert.Throws<InvalidDataException>(() => f.Commit((40, f.Edge(b))));
    }

    [Fact]
    public async Task Raw_chunks_and_end_never_publish_a_baseline()
    {
        using var f = new Fixture();
        var id = Guid.NewGuid();
        f.Commit((10, new CaptureChunkPayload(id, 0, f.Root, 1, [], [], [f.Edge(f.Key(1))],
                [], [], [f.Delta(f.Key(1), .5)], [], [], [])),
            (20, new CaptureBoundaryPayload(id, CaptureKind.Baseline, CapturePhase.End, f.Root, 1, [], 1, ProjectionFixture.Partial)));
        Assert.Empty((await f.Tree(20)).Items);
    }

    [Fact]
    public void Wrong_session_and_invalid_typed_identity_observations_do_not_produce_rows()
    {
        using var f = new Fixture();
        Assert.Throws<ArgumentException>(() => f.Source(10, 1,
            new VisualDeltaPayload(f.Key(1, IdentityDomain.CompNode), VisualProperty.Opacity, Fixture.Number(.5))));
        var source = f.Source(10, 1, f.Edge(f.Key(1)));
        Assert.Throws<ArgumentException>(() => f.Processor.Process([source],
            new(new(Guid.NewGuid(), f.Scope.Process), new(1, 10), ProjectionFixture.Partial, new(10000))).ToArray());
    }

    [Fact]
    public async Task Health_invalidation_is_uncertainty_not_a_synthetic_remove()
    {
        using var f = new Fixture();
        var v = f.Key(1);
        f.Commit((10, f.Edge(v)));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.SequenceGap, "gap"));
        f.Processor.Invalidate(health.Snapshot());
        f.Commit((20, f.Delta(v, .6)));
        var row = (await f.Tree(20)).Items.Single();
        Assert.True(row.Topology.Connected);
        Assert.Equal(MeasurementKind.Unavailable, row.Topology.Quality.Kind);
    }

    [Fact]
    public async Task Property_loss_keeps_original_history_but_exposes_missing_current_value_with_units()
    {
        using var f = new Fixture();
        var v = f.Key(1);
        f.Commit((10, f.Edge(v)), (11, f.Delta(v, .4)),
            (20, new LossPayload(f.Epoch, new(20, 20), 1, "gap")));
        var property = (await f.Tree(20)).Items.Single().Properties.Single();
        Assert.Equal(ValueKind.Missing, property.Value.Kind);
        Assert.Equal(ValueUnit.Scalar, property.Value.Unit);
        Assert.Equal(11, property.TimestampQpc);
        Assert.Equal(.4, Assert.IsType<VisualPropertyObservation>((await f.History(v))
            .Single(h => h.TimestampQpc == 11).Observation).Value.Number);
        f.Commit((30, f.Delta(v, .8)));
        var current = (await f.Tree(30)).Items.Single();
        Assert.Equal(.8, current.Properties.Single().Value.Number);
        Assert.Equal(MeasurementKind.Unavailable, current.Topology.Quality.Kind);
    }

    [Fact]
    public void Late_parent_destruction_cannot_leave_a_future_child_membership_alive()
    {
        using var f = new Fixture();
        var root = f.Key(1);
        f.Commit((10, f.Edge(root)), (30, f.Edge(f.Key(2), root)));
        Assert.Throws<InvalidDataException>(() => f.Commit((20,
            new ObjectLifetimePayload(root, LifetimeOperation.Destroy, null, null, DataQuality.Complete))));
        Assert.Equal(2, f.Store.Watermark.IngestOrdinal);
    }

    [Fact]
    public async Task Foreign_epoch_and_unrelated_domain_loss_do_not_degrade_composition()
    {
        using var f = new Fixture();
        var v = f.Key(1);
        f.Commit((10, f.Edge(v)));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.ProducerLoss, "batch-wide notification is not an interval"));
        f.Processor.Invalidate(health.Snapshot());
        f.Commit((20, new LossPayload(Guid.NewGuid(), new(20, 20), 1, "other epoch")),
            (30, new LossPayload(f.Epoch, new(30, 30), 1, "allocation only",
                [new(new(EventCatalog.RuntimeProviderId, 42, f.Epoch), f.Scope.Process, 12, 1)])));
        Assert.Equal(MeasurementKind.Measured, (await f.Tree(30)).Items.Single().Topology.Quality.Kind);
    }

    [Fact]
    public void Enumeration_respects_remaining_record_budget_even_without_worker_consumption()
    {
        using var f = new Fixture();
        var source = f.Source(10, 1, f.Edge(f.Key(1)));
        var budget = new ProjectionBudget(SessionWriter.MaximumBatchBytes);
        for (var i = 1; i < SessionWriter.MaximumBatchRecords; i++) budget.Consume(0);
        using var rows = f.Processor.Process([source],
            new(f.Scope, new(1, 10), ProjectionFixture.Partial, budget)).GetEnumerator();
        Assert.True(rows.MoveNext());
        Assert.IsType<ObjectHistoryEntry>(rows.Current.Value);
        Assert.Throws<ProcessingLimitException>(() => rows.MoveNext());
    }

    [Fact]
    public void Repeated_birth_cannot_reuse_the_same_full_identity_before_destruction()
    {
        using var f = new Fixture();
        var birth = new ObjectLifetimePayload(f.Key(1), LifetimeOperation.Birth, null, null, DataQuality.Complete);
        f.Commit((10, birth));
        Assert.Throws<InvalidDataException>(() => f.Commit((20, birth)));
    }

    [Fact]
    public async Task Object_state_owns_association_history_while_composition_supplies_the_reverse_query_index()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,CompositionTreeProcessor");
        var core = f.Key(1);
        var peer = new ObjectKey(f.Scope.Process, IdentityDomain.XamlPeer, f.Epoch, 1);
        f.Commit((10, new AssociationPayload(core, peer, true)));
        Assert.Single(await f.History(core));
        Assert.Single(await f.History(peer));
        Assert.Single((await f.Store.Queries.GetCorrelations(new(f.Scope, peer, 10, new(128)), default)).Items);
    }

    [Theory]
    [InlineData(CorrelationKind.VisualResource)]
    [InlineData(CorrelationKind.ResourceBacking)]
    public async Task Resource_correlations_have_one_projection_owner_when_components_share_a_writer(CorrelationKind kind)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor,CompositionTreeProcessor,ResourceUsageProcessor");
        var from = new ObjectKey(f.Scope.Process,
            kind == CorrelationKind.VisualResource ? IdentityDomain.CompositionVisual : IdentityDomain.Resource, f.Epoch, 1);
        var to = new ObjectKey(f.Scope.Process, IdentityDomain.Resource, f.Epoch, 2);
        var link = new CorrelationPayload(from, to, kind, true);
        var changes = f.Commit((10, link));
        Assert.Single(changes, c => c.Value is CorrelationEntry);
        Assert.Single(await f.History(from));
        Assert.Single(await f.History(to));
        Assert.Single((await f.Store.Queries.GetCorrelations(new(f.Scope, from, 10, new(128)), default)).Items);
        Assert.Single((await f.Store.Queries.GetCorrelations(new(f.Scope, to, 10, new(128)), default)).Items);
        f.Commit((20, link with { Linked = false }));
        Assert.Equal(2, (await f.History(from)).Count);
        Assert.Equal(2, (await f.History(to)).Count);
        Assert.Empty((await f.Store.Queries.GetCorrelations(new(f.Scope, from, 20, new(128)), default)).Items);
    }

    [Theory]
    [InlineData(CorrelationKind.VisualResource)]
    [InlineData(CorrelationKind.ResourceBacking)]
    public async Task Verified_resource_links_have_one_owner_under_the_actual_End_source(CorrelationKind kind)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor,CompositionTreeProcessor,ResourceUsageProcessor");
        var from = new ObjectKey(f.Scope.Process,
            kind == CorrelationKind.VisualResource ? IdentityDomain.CompositionVisual : IdentityDomain.Resource, f.Epoch, 1);
        var to = new ObjectKey(f.Scope.Process, IdentityDomain.Resource, f.Epoch, 2);
        var id = Guid.NewGuid();
        var chunk = f.Chunk(id, f.Key(1)) with { Correlations = [new(from, to, kind, true)] };
        f.PublishBaseline(20, f.Boundary(id, CapturePhase.End), [chunk]);
        var history = Assert.Single(await f.History(from));
        Assert.Equal(1, history.IngestOrdinal);
        Assert.Equal(20, history.TimestampQpc);
        Assert.Single(await f.History(to));
        using var reopened = f.Reopen();
        Assert.Single((await reopened.Queries.GetCorrelations(new(f.Scope, to, 20, new(128)), default)).Items);
    }

    [Fact]
    public void Same_time_property_and_link_conflicts_are_not_silently_overwritten()
    {
        using var f = new Fixture();
        var v = f.Key(1);
        f.Commit((10, f.Delta(v, .4)));
        Assert.Throws<InvalidDataException>(() => f.Commit((10, f.Delta(v, .6))));
        var link = new CorrelationPayload(f.Key(1, IdentityDomain.XamlCore), v, CorrelationKind.XamlVisual, true);
        f.Commit((20, link));
        Assert.Throws<InvalidDataException>(() => f.Commit((20, link with { Linked = false })));
    }

    [Fact]
    public void Every_emission_has_the_actual_current_source_ordinal_and_observation_time()
    {
        using var f = new Fixture();
        var source = f.Source(10, 7, f.Edge(f.Key(1)));
        var rows = f.Processor.Process([source],
            new(f.Scope, new(7, 10), ProjectionFixture.Partial, new(10000))).ToArray();
        Assert.NotEmpty(rows);
        Assert.All(rows, row => Assert.Equal(7, row.SourceOrdinal));
        var history = Assert.IsType<ObjectHistoryEntry>(rows.Single(r => r.Value is ObjectHistoryEntry).Value);
        Assert.Equal(10, history.TimestampQpc);
        Assert.Equal(7, history.IngestOrdinal);
    }

    [Fact]
    public async Task Verified_baseline_uses_actual_end_for_properties_metadata_links_and_reopen()
    {
        using var f = new Fixture(includeResources: true);
        var node = f.Key(1, IdentityDomain.CompNode);
        var visual = f.Key(1);
        var core = f.Key(1, IdentityDomain.XamlCore);
        var peer = f.Key(1, IdentityDomain.XamlPeer);
        var resource = f.Key(1, IdentityDomain.Resource);
        f.Commit((5, f.Delta(visual, .1)));
        var frozen = (await f.Tree(5)).Context.Watermark;
        var id = Guid.NewGuid();
        var chunk = f.Chunk(id, [f.Edge(node), f.Edge(visual)]) with
        {
            Visuals = [f.Delta(visual, .7)],
            Associations = [new(core, peer, true)],
            Correlations = [new(core, visual, CorrelationKind.XamlVisual, true),
                new(node, visual, CorrelationKind.CompNodeVisual, true),
                new(visual, resource, CorrelationKind.VisualResource, true)]
        };
        var rows = f.Publish(20, f.End(id), [chunk]);
        Assert.All(rows, row => Assert.Equal(2, row.SourceOrdinal));
        Assert.Empty((await f.Tree(5, frozen)).Items);
        Assert.Empty((await f.Tree(19)).Items);
        var tree = (await f.Tree(20)).Items;
        Assert.Equal(2, tree.Count);
        Assert.All(tree, row => Assert.Equal("observed", row.Topology.TypeName));
        Assert.Equal(.7, tree.Single(r => r.Topology.Object == visual).Properties.Single().Value.Number);
        Assert.Equal(20, tree.Single(r => r.Topology.Object == visual).Properties.Single().TimestampQpc);
        Assert.Single((await f.Links(peer, 20)).Items);
        Assert.Single((await f.Links(resource, 20)).Items);
        Assert.DoesNotContain(await f.History(visual), h => h.Observation is LifetimeObservation { Operation: LifetimeOperation.Birth });
        using var reopened = f.Reopen();
        Assert.Equal(2, (await reopened.Queries.GetCompositionTree(new(f.Scope, 20, new(128)), default)).Items.Count);
        f.Commit((30, f.Delta(visual, .8)));
        Assert.Equal("observed", (await f.Tree(30)).Items.Single(r => r.Topology.Object == visual).Topology.TypeName);
    }

    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public async Task Only_complete_qualified_baseline_retires_absent_scope_memberships(bool complete)
    {
        using var f = new Fixture();
        var present = f.Key(1);
        var absent = f.Key(2);
        var other = f.Key(3);
        f.Commit((10, f.Edge(present)), (11, f.Edge(absent)), (12, f.Edge(other) with { RootId = Guid.NewGuid() }));
        var id = Guid.NewGuid();
        f.Publish(20, f.End(id, complete), [f.Chunk(id, [f.Edge(present)])]);
        Assert.Equal(complete ? 2 : 3, (await f.Tree(20)).Items.Count);
        f.Commit((30, f.Delta(absent, .5)));
        Assert.Equal(complete ? 2 : 3, (await f.Tree(30)).Items.Count);
        Assert.Equal(3, (await f.Tree(19)).Items.Count);
    }

    [Fact]
    public void Verified_baseline_rejects_bad_scope_and_obeys_retained_and_output_limits()
    {
        using var f = new Fixture();
        var id = Guid.NewGuid();
        Assert.Throws<ArgumentException>(() => f.Publish(20, f.End(id), [f.Chunk(Guid.NewGuid(), [f.Edge(f.Key(1))])]));
        using var bounded = new Fixture(new CompositionTreeProcessor(1));
        var bid = Guid.NewGuid();
        Assert.Throws<ProcessingLimitException>(() => bounded.Publish(20, bounded.End(bid),
            [bounded.Chunk(bid, [bounded.Edge(bounded.Key(1))])]));
        var source = f.Source(20, 1, f.End(id));
        var processor = Assert.IsAssignableFrom<IVerifiedBaselineProcessor>(f.Processor);
        Assert.Throws<ProcessingLimitException>(() => processor.PublishBaseline(source,
            [f.Chunk(id, [f.Edge(f.Key(1))])], new(f.Scope, new(1, 20), ProjectionFixture.Partial, new(16))).ToArray());
        using var late = new Fixture();
        late.Commit((30, late.Edge(late.Key(1))));
        var lid = Guid.NewGuid();
        Assert.Throws<InvalidDataException>(() => late.Publish(20, late.End(lid), [late.Chunk(lid, [late.Edge(late.Key(1))])]));
    }

    private sealed class CompositionComparer : IEqualityComparer<CompositionTreeEntry>
    {
        public bool Equals(CompositionTreeEntry? x, CompositionTreeEntry? y) =>
            x is not null && y is not null && x.Topology == y.Topology && x.Properties.SequenceEqual(y.Properties);
        public int GetHashCode(CompositionTreeEntry obj) => obj.Topology.GetHashCode();
    }

    private sealed class Fixture : IDisposable
    {
        internal SessionScope Scope { get; } = new(Guid.NewGuid(), new(42, 123456, Guid.NewGuid()));
        internal Guid Epoch { get; } = Guid.NewGuid();
        internal Guid Root { get; } = Guid.NewGuid();
        internal IEventProcessor Processor { get; }
        internal SessionStore Store { get; }
        private readonly SessionInfo info = new(SessionMode.Offline, SessionStatus.Recording, 1_000_000, null, [], null);
        private readonly string path = Path.GetFullPath(Path.Combine("TestResults", Guid.NewGuid() + ".sqlite"));
        private readonly ResourceUsageProcessor? resources;
        private SessionWriter? writer;
        internal Fixture(IEventProcessor? processor = null, bool includeResources = false)
        {
            Processor = processor ?? new CompositionTreeProcessor();
            resources = includeResources ? new ResourceUsageProcessor() : null;
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            Store = SessionStore.Create(path, Scope, info, ProjectionFixture.Partial);
        }
        internal ObjectKey Key(ulong id, IdentityDomain domain = IdentityDomain.CompositionVisual) => new(Scope.Process, domain, Epoch, id);
        internal TreeEdgePayload Edge(ObjectKey child, ObjectKey? parent = null) =>
            new(parent, child, child.Domain == IdentityDomain.CompNode
                ? parent is null ? RelationshipKind.CompRoot : RelationshipKind.CompChild
                : parent is null ? RelationshipKind.VisualRoot : RelationshipKind.VisualChild,
                0, Root, 1, true, Membership: TreeMembershipKind.Composition);
        internal static TypedValue Number(double n) => new(ValueKind.Number, ValueUnit.Scalar, null, n, null, null, DataQuality.Complete);
        internal VisualDeltaPayload Delta(ObjectKey v, double value, VisualProperty property = VisualProperty.Opacity) => new(v, property, Number(value));
        internal NormalizedEvent Source(long qpc, long ordinal, IEventPayload payload)
        {
            var identity = EventCatalog.All.Single(d => d.PayloadType == payload.GetType() &&
                d.Identity.ProviderId == EventCatalog.RuntimeProviderId).Identity;
            var epoch = payload switch
            {
                TreeEdgePayload p => p.Child.ProducerEpoch,
                VisualDeltaPayload p => p.Visual.ProducerEpoch,
                CorrelationPayload p => p.From.ProducerEpoch,
                ObjectLifetimePayload p => p.Object.ProducerEpoch,
                LossPayload p => p.ProducerEpoch,
                _ => Epoch
            };
            return EventCodec.Normalize(new(Scope.SessionId, Scope.Process, 42, 1, qpc, ordinal, (ulong)ordinal, null,
                identity, EventCodec.EncodePayload(payload, Scope.Process)), new(identity.ProviderId, 42, epoch));
        }
        internal void Commit(params (long Qpc, IEventPayload Payload)[] input)
        {
            var first = Store.Watermark.IngestOrdinal;
            var events = input.Select((item, i) => Source(item.Qpc, first + i + 1, item.Payload)).ToArray();
            var context = new ProcessingContext(Scope, new(first + events.Length,
                Math.Max(Store.Watermark.TimestampQpc, input.Max(i => i.Qpc))), ProjectionFixture.Partial,
                new(SessionWriter.MaximumBatchBytes));
            var changes = Processor.Process(events, context).Concat(resources?.Process(events, context) ?? []).ToArray();
            (writer ??= Store.CreateWriter()).Commit(new(events, changes, info, ProjectionFixture.Partial));
        }
        internal Task<QueryPage<CompositionTreeEntry>> Tree(long at, CommittedWatermark? watermark = null) =>
            Store.Queries.GetCompositionTree(new(Scope, at, new(128, 0, watermark)), default).AsTask();
        internal Task<QueryPage<CorrelationEntry>> Links(ObjectKey key, long at) =>
            Store.Queries.GetCorrelations(new(Scope, key, at, new(128)), default).AsTask();
        internal async Task<IReadOnlyList<ObjectHistoryEntry>> History(ObjectKey key) =>
            (await Store.Queries.GetObjectHistory(new(Scope, key, new(0, 1000), new(128)), default)).Items;
        internal SessionStore Reopen() => SessionStore.OpenReadOnly(path);
        internal CaptureBoundaryPayload End(Guid id, bool complete = false)
        {
            var domain = new SequenceDomainKey(new(EventCatalog.RuntimeProviderId, 42, Epoch), Scope.Process, 5, 1);
            return new(id, CaptureKind.Baseline, CapturePhase.End, Root, 1,
                complete ? [new(domain, 0, 10, DataQuality.Complete, new(Guid.NewGuid(), domain, 10, 10))] : [],
                1, complete ? DataQuality.Complete : ProjectionFixture.Partial);
        }
        internal CaptureChunkPayload Chunk(Guid id, TreeEdgePayload[] tree) => new(id, 0, Root, 1,
            tree.Select(t => new SnapshotObject(t.Child, "observed", null,
                new(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null,
                    new(MeasurementKind.Unavailable, CoverageState.Partial, "unknown size")))).ToArray(),
            [], tree, [], [], [], [], [], []);
        internal ProjectionChange[] Publish(long qpc, CaptureBoundaryPayload end, CaptureChunkPayload[] chunks)
        {
            var source = Source(qpc, Store.Watermark.IngestOrdinal + 1, end);
            var context = new ProcessingContext(Scope, new(source.Envelope.IngestOrdinal,
                Math.Max(qpc, Store.Watermark.TimestampQpc)), ProjectionFixture.Partial, new(SessionWriter.MaximumBatchBytes));
            var processor = Assert.IsAssignableFrom<IVerifiedBaselineProcessor>(Processor);
            var rows = processor.PublishBaseline(source, chunks, context)
                .Concat(resources?.PublishBaseline(source, chunks, context) ?? []).ToArray();
            (writer ??= Store.CreateWriter()).Commit(new([source], rows, info, ProjectionFixture.Partial));
            return rows;
        }
        public void Dispose()
        {
            Store.Dispose();
            foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
                if (File.Exists(path + suffix)) File.Delete(path + suffix);
        }
    }
}
