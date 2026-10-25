using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class XamlTreeTests
{
    [Theory]
    [InlineData(RelationshipKind.Child)]
    [InlineData(RelationshipKind.Content)]
    [InlineData(RelationshipKind.Template)]
    [InlineData(RelationshipKind.Popup)]
    public async Task Root_and_relationships_have_exact_half_open_temporal_validity(RelationshipKind relationship)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        var other = f.Key(2);
        var child = f.Key(3);
        f.Commit((10, f.Edge(root)), (11, f.Edge(other)), (20, f.Edge(child, root, relationship)),
            (30, f.Edge(child, other, relationship)), (40, f.Edge(child, other, relationship, connected: false)));
        Assert.Equal(2, (await f.Tree(19)).Items.Count);
        Assert.Equal(root, (await f.Tree(20)).Items.Single(r => r.Object == child).Parent);
        Assert.Equal(root, (await f.Tree(29)).Items.Single(r => r.Object == child).Parent);
        Assert.Equal(other, (await f.Tree(30)).Items.Single(r => r.Object == child).Parent);
        Assert.False((await f.Tree(40)).Items.Single(r => r.Object == child).Connected);
        Assert.Equal(relationship, (await f.Tree(40)).Items.Single(r => r.Object == child).Relationship);
        Assert.Equal(3, (await f.History(child)).Count);
        Assert.DoesNotContain(await f.History(child), r => r.Observation is LifetimeObservation);
    }

    [Fact]
    public async Task Logical_and_visual_memberships_are_distinct_and_reparenting_is_not_birth()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        var child = f.Key(2);
        f.Commit((10, f.Edge(root)), (20, f.Edge(child, root, RelationshipKind.Child, TreeMembershipKind.Logical)),
            (21, f.Edge(child, root, RelationshipKind.Template)), (30, f.Edge(child, root, RelationshipKind.Child, TreeMembershipKind.Logical, false)));
        var rows = (await f.Tree(30)).Items.Where(r => r.Object == child).ToArray();
        Assert.Equal(2, rows.Length);
        Assert.False(rows.Single(r => r.Membership == TreeMembershipKind.Logical).Connected);
        Assert.True(rows.Single(r => r.Membership == TreeMembershipKind.Visual).Connected);
    }

    [Fact]
    public async Task Out_of_order_revisions_and_frozen_queries_do_not_resurrect_destroyed_objects()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        var child = f.Key(2);
        f.Commit((10, f.Edge(root)), (20, f.Edge(child, root, RelationshipKind.Child)),
            (40, new ObjectLifetimePayload(child, LifetimeOperation.Destroy, null, null, DataQuality.Complete)));
        var frozen = (await f.Tree(30)).Context.Watermark;
        f.Commit((25, f.Edge(child, root, RelationshipKind.Child, connected: false)),
            (15, f.Edge(child, root, RelationshipKind.Child)));
        Assert.Equal(2, (await f.Tree(30, watermark: frozen)).Items.Count);
        Assert.False((await f.Tree(30)).Items.Single(r => r.Object == child).Connected);
        Assert.DoesNotContain((await f.Tree(40)).Items, r => r.Object == child);
        Assert.Single(await f.History(child), r => r.Observation is LifetimeObservation);
    }

    [Fact]
    public async Task Late_type_metadata_enriches_tree_without_revising_old_topology_time()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var key = f.Key(1);
        f.Commit((10, new ObjectLifetimePayload(key, LifetimeOperation.Birth, 7, null, DataQuality.Complete)),
            (20, f.Edge(key)), (30, new TypeResolvedPayload(f.Epoch, 7, "Button", ProjectionFixture.Element)));
        Assert.Null((await f.Tree(20)).Items.Single().TypeName);
        var row = (await f.Tree(30)).Items.Single();
        Assert.Equal("Button", row.TypeName);
        Assert.Equal(ProjectionFixture.Element, row.Classification);
    }

    [Fact]
    public async Task Missing_baseline_is_partial_and_loss_does_not_preserve_stale_measured_certainty()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        f.Commit((10, f.Edge(root)));
        Assert.Equal(CoverageState.Partial, (await f.Tree(10)).Items.Single().Quality.Coverage);
        f.Commit((40, new LossPayload(f.Epoch, new(20, 30), 1, "explicit lost interval")));
        Assert.Equal(MeasurementKind.Measured, (await f.Tree(19)).Items.Single().Quality.Kind);
        Assert.Equal(MeasurementKind.Unavailable, (await f.Tree(20)).Items.Single().Quality.Kind);
        Assert.Equal(MeasurementKind.Unavailable, (await f.Tree(40)).Items.Single().Quality.Kind);
        f.Commit((50, f.Edge(root, connected: false)));
        Assert.Equal(MeasurementKind.Measured, (await f.Tree(50)).Items.Single().Quality.Kind);
    }

    [Fact]
    public async Task Health_invalidation_before_batch_degrades_existing_edges_without_fabricating_detachment()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        f.Commit((10, f.Edge(root)));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.SequenceGap, "missing domain sequence"));
        foreach (var p in f.Processors) p.Invalidate(health.Snapshot());
        f.Commit((20, f.Property(root)));
        var row = (await f.Tree(20)).Items.Single();
        Assert.True(row.Connected);
        Assert.Equal(MeasurementKind.Unavailable, row.Quality.Kind);
    }

    [Fact]
    public async Task Staged_baseline_tree_is_not_visible_before_end_or_before_observation_time()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var id = Guid.NewGuid();
        var key = f.Key(1);
        f.Commit((10, f.Boundary(id, CapturePhase.Begin)), (20, f.Chunk(id, key)));
        Assert.Empty((await f.Tree(20)).Items);
        f.PublishBaseline(30, f.Boundary(id, CapturePhase.End), [f.Chunk(id, key)]);
        Assert.Empty((await f.Tree(20)).Items);
        Assert.Equal(key, (await f.Tree(30)).Items.Single().Object);
    }

    [Fact]
    public void Relationship_index_is_bounded_and_output_budget_is_checked_incrementally()
    {
        using var f = new ProjectionFixture("XamlTreeProcessor", 1);
        f.Commit((10, f.Edge(f.Key(1))));
        Assert.Throws<ProcessingLimitException>(() => f.Commit((20, f.Edge(f.Key(2)))));
        using var other = new ProjectionFixture("XamlTreeProcessor");
        var source = other.Source(10, 1, other.Edge(other.Key(1)));
        Assert.Throws<ProcessingLimitException>(() => other.Processors[0].Process([source],
            new(other.Scope, new(1, 10), ProjectionFixture.Partial, new(16))).ToArray());
    }

    [Fact]
    public async Task Destruction_before_first_tree_arrival_and_parent_destruction_end_relationships()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        var child = f.Key(2);
        f.Commit((40, new ObjectLifetimePayload(child, LifetimeOperation.Destroy, null, null, DataQuality.Complete)),
            (10, f.Edge(root)), (20, f.Edge(child, root, RelationshipKind.Child)));
        Assert.Equal(2, (await f.Tree(30)).Items.Count);
        Assert.Single((await f.Tree(40)).Items);
        f.Commit((35, new ObjectLifetimePayload(root, LifetimeOperation.Destroy, null, null, DataQuality.Complete)));
        Assert.Empty((await f.Tree(35)).Items);
    }

    [Fact]
    public async Task Frozen_loss_coverage_is_temporal_epoch_qualified_and_survives_reopening()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var key = f.Key(1);
        f.Commit((10, f.Edge(key)), (50, f.Property(key)));
        var frozen = (await f.Tree(25)).Context.Watermark;
        f.Commit((60, new LossPayload(Guid.NewGuid(), new(20, 30), 1, "foreign epoch")));
        Assert.Equal(MeasurementKind.Measured, (await f.Tree(25)).Items.Single().Quality.Kind);
        f.Commit((70, new LossPayload(f.Epoch, new(20, 30), 1, "late loss receipt")));
        Assert.Equal(MeasurementKind.Measured, (await f.Tree(25, watermark: frozen)).Items.Single().Quality.Kind);
        Assert.Equal(MeasurementKind.Unavailable, (await f.Tree(25)).Items.Single().Quality.Kind);
        using var reopened = f.Reopen();
        Assert.Equal((await f.Tree(25)).Items.Single(),
            (await reopened.Queries.GetXamlTree(new(f.Scope, 25, new(128)), default)).Items.Single());
    }

    [Fact]
    public async Task Producer_loss_health_does_not_move_the_known_gap_to_the_first_event_of_a_batch()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        f.Commit((10, f.Edge(root)));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.ProducerLoss, "explicit interval follows in this batch"));
        foreach (var p in f.Processors) p.Invalidate(health.Snapshot());
        f.Commit((15, f.Property(root)), (40, new LossPayload(f.Epoch, new(20, 30), 1, "gap")));
        Assert.Equal(MeasurementKind.Measured, (await f.Tree(19)).Items.Single().Quality.Kind);
        Assert.Equal(MeasurementKind.Unavailable, (await f.Tree(20)).Items.Single().Quality.Kind);
    }

    [Fact]
    public async Task Selected_subtrees_retain_detached_boundaries_without_traversing_them_or_destroyed_intermediates()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        var child = f.Key(2);
        var grandchild = f.Key(3);
        f.Commit((10, f.Edge(root)), (11, f.Edge(child, root, RelationshipKind.Child)),
            (12, f.Edge(grandchild, child, RelationshipKind.Child)),
            (20, f.Edge(child, root, RelationshipKind.Child, connected: false)));
        Assert.Equal(3, (await f.Tree(19, root)).Items.Count);
        var detached = (await f.Tree(20, root)).Items;
        Assert.Equal(2, detached.Count);
        Assert.Contains(detached, row => row.Object == root);
        Assert.False(detached.Single(row => row.Object == child).Connected);
        Assert.DoesNotContain(detached, row => row.Object == grandchild);
        Assert.Equal(3, (await f.Tree(20)).Items.Count);
        f.Commit((30, new ObjectLifetimePayload(child, LifetimeOperation.Destroy, null, null, DataQuality.Complete)));
        Assert.Single((await f.Tree(30, root)).Items);
    }

    [Fact]
    public void Live_membership_cycles_stop_explicitly_instead_of_returning_a_valid_looking_tree()
    {
        using var f = new ProjectionFixture("XamlTreeProcessor");
        f.Commit((10, f.Edge(f.Key(2), f.Key(1), RelationshipKind.Child)));
        Assert.Throws<InvalidDataException>(() => f.Commit((20, f.Edge(f.Key(1), f.Key(2), RelationshipKind.Template))));
        Assert.Equal(1, f.Store.Watermark.IngestOrdinal);
    }

    [Fact]
    public async Task Terminal_health_without_another_source_cannot_preserve_measured_tree_certainty()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        f.Commit((10, f.Edge(f.Key(1))), (50, f.Property(f.Key(1))));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.EtwEventsLost, "late transport loss", timestampQpc: 20));
        foreach (var p in f.Processors) p.Invalidate(health.Snapshot());
        f.RecordHealth(health.Snapshot());
        Assert.Equal(MeasurementKind.Measured, (await f.Tree(19)).Items.Single().Quality.Kind);
        Assert.Equal(MeasurementKind.Unavailable, (await f.Tree(20)).Items.Single().Quality.Kind);
    }

    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public async Task Only_a_complete_verified_scope_can_end_absent_memberships_without_destroying_objects(bool complete)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var root = f.Key(1);
        var child = f.Key(2);
        var other = f.Key(3);
        f.Commit((10, f.Edge(root)), (20, f.Edge(child, root, RelationshipKind.Content)),
            (21, f.Edge(other) with { RootId = Guid.NewGuid() }));
        var id = Guid.NewGuid();
        var end = f.Boundary(id, CapturePhase.End);
        if (complete)
        {
            var domain = new SequenceDomainKey(new(EventCatalog.RuntimeProviderId, 42, f.Epoch), f.Scope.Process, 5, 1);
            end = end with
            {
                Quality = DataQuality.Complete,
                Domains = [new(domain, 1, 10, DataQuality.Complete, new(Guid.NewGuid(), domain, 10, 10))]
            };
        }
        f.PublishBaseline(30, end, [f.Chunk(id, root)]);
        Assert.Equal(3, (await f.Tree(29)).Items.Count);
        Assert.Equal(complete ? 2 : 3, (await f.Tree(30)).Items.Count);
        Assert.DoesNotContain(await f.History(child), r => r.Observation is LifetimeObservation);
    }

    [Fact]
    public async Task Baseline_classification_without_a_name_does_not_erase_a_resolved_type_name()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var key = f.Key(1);
        f.Commit((10, new ObjectLifetimePayload(key, LifetimeOperation.Birth, 7, null, DataQuality.Complete)),
            (20, f.Edge(key)), (30, new TypeResolvedPayload(f.Epoch, 7, "Button", ProjectionFixture.Element)));
        var id = Guid.NewGuid();
        f.PublishBaseline(40, f.Boundary(id, CapturePhase.End), [f.Chunk(id, key)]);
        Assert.Equal("Button", (await f.Tree(40)).Items.Single().TypeName);
        Assert.Equal(ProjectionFixture.Element, (await f.Tree(40)).Items.Single().Classification);
    }
}
