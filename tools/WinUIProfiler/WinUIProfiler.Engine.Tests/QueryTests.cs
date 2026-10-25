using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Query;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class QueryTests
{
    [Fact]
    public async Task Frozen_pages_and_bound_deterministic_tokens_survive_new_commits()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 3));
        var request = f.Metrics(new(1));
        var first = await store.Queries.GetMetrics(request, default);
        var token = store.Queries.Continuation(request, first.Page, first.Context);
        Assert.NotNull(token);
        Assert.Equal(token, store.Queries.Continuation(request, first.Page, first.Context));
        writer.Commit(f.Batch(4, 5));
        var secondRequest = request with { Page = store.Queries.Resume(request, token) };
        var second = await store.Queries.GetMetrics(secondRequest, default);
        Assert.Equal(first.Context, second.Context);
        Assert.Equal(2, second.Items.Single().Value.Integer);
        Assert.Equal(5, (await store.Queries.GetMetrics(f.Metrics(), default)).Items.Count);
        Assert.Throws<ArgumentException>(() => store.Queries.Resume(request with { Metric = MetricKind.CpuUtilization }, token));
        Assert.Throws<ArgumentException>(() => store.Queries.Resume(request with { Scope = new(Guid.NewGuid(), f.Scope.Process) }, token));
        Assert.Throws<ArgumentException>(() => store.Queries.Resume(request with { Page = new(2) }, token));
        Assert.Throws<ArgumentException>(() => store.Queries.Resume(request, token + "x"));
        Assert.Throws<ArgumentException>(() => store.Queries.Resume(request, "garbage"));
    }

    [Fact]
    public async Task High_bit_identities_and_historical_versions_are_never_rewritten()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var key = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, ulong.MaxValue);
        var tree = new TreeEntry(key, null, RelationshipKind.Root, 0, true, Guid.NewGuid(), ulong.MaxValue,
            "Synthetic", StoreFixture.Partial, TreeMembershipKind.Visual,
            new(XamlObjectKind.UIElement, ClassificationEvidence.RuntimeType, "explicit bridge"));
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1) with { Changes = [new(1, tree)] });
        var query = new GetXamlTreeRequest(f.Scope, 10, new(1));
        var old = await store.Queries.GetXamlTree(query, default);
        writer.Commit(f.Batch(2, 2) with { Changes = [new(2, tree with { Connected = false })] });
        var frozen = await store.Queries.GetXamlTree(query with { Page = new(1, 0, old.Context.Watermark) }, default);
        Assert.Equal(tree, frozen.Items.Single());
        var current = await store.Queries.GetXamlTree(query with { AtQpc = 20 }, default);
        Assert.False(current.Items.Single().Connected);
        Assert.Equal(ulong.MaxValue, current.Items.Single().Object.RuntimeObjectId);
        Assert.Equal(ulong.MaxValue, current.Items.Single().DispatcherId);
        using var db = f.Raw();
        Assert.Equal("FFFFFFFFFFFFFFFF", StoreFixture.Scalar(db, "SELECT RuntimeId FROM Relationships LIMIT 1"));
        Assert.Equal("FFFFFFFFFFFFFFFF", StoreFixture.Scalar(db, "SELECT DispatcherId FROM Roots LIMIT 1"));
    }

    [Fact]
    public async Task Validation_cancellation_unobserved_quality_and_real_boundary_checks()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var q = store.Queries;
        var empty = await q.GetMetrics(f.Metrics(), default);
        Assert.Empty(empty.Items);
        Assert.Equal(CoverageState.Unavailable, empty.Context.Quality.Coverage);
        Assert.NotNull(empty.Context.Quality.Reason);
        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await q.GetMetrics(f.Metrics(), new(true)));
        await Assert.ThrowsAsync<ArgumentException>(async () => await q.GetMetrics(f.Metrics(new(4097)), default));
        await Assert.ThrowsAsync<ArgumentException>(async () => await q.GetMetrics(f.Metrics(new(1, 1)), default));
        await Assert.ThrowsAsync<ArgumentException>(async () => await q.GetMetrics(f.Metrics() with { Scope = new(Guid.NewGuid(), f.Scope.Process) }, default));
        store.CreateWriter().Commit(f.Batch(1, 2));
        await Assert.ThrowsAsync<ArgumentException>(async () => await q.GetMetrics(f.Metrics(new(1, 0, new(1, 10))), default));
        var result = await q.GetMetrics(f.Metrics(), default);
        Assert.Equal(StoreFixture.Partial, result.Context.Quality);
        Assert.All(result.Items, sample => Assert.Equal(StoreFixture.Partial, sample.Value.Quality));
        await Assert.ThrowsAsync<NotSupportedException>(async () => await q.CompareSnapshots(new(f.Scope, Guid.NewGuid(), [Guid.NewGuid()], new(1)), default));
    }

    [Fact]
    public async Task Deleted_projection_is_temporal_and_numeric_missing_values_round_trip()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        var missing = new DataQuality(MeasurementKind.Unavailable, CoverageState.Partial, "counter not available");
        var batch = f.Batch(1, 1);
        var sample = ((MetricSample)batch.Changes[0].Value) with { Value = new(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null, missing) };
        writer.Commit(batch with { Changes = [new(1, sample)] });
        var old = await store.Queries.GetMetrics(f.Metrics(), default);
        Assert.Null(old.Items.Single().Value.Integer);
        Assert.Equal(missing, old.Items.Single().Value.Quality);
        using var db = f.Raw();
        Assert.Null(StoreFixture.Scalar(db, "SELECT IntegerValue FROM MetricSamples") is DBNull ? null : StoreFixture.Scalar(db, "SELECT IntegerValue FROM MetricSamples"));
        Assert.Equal((long)ValueKind.Missing, StoreFixture.Scalar(db, "SELECT ValueKind FROM MetricSamples"));
    }

    [Fact]
    public async Task Explicit_frame_history_composition_correlation_resource_and_snapshot_queries_round_trip()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        ObjectKey Key(IdentityDomain domain, ulong id) => new(f.Scope.Process, domain, f.Epoch, id);
        var xaml = Key(IdentityDomain.XamlCore, ulong.MaxValue);
        var visual = Key(IdentityDomain.CompositionVisual, ulong.MaxValue);
        var resource = Key(IdentityDomain.Resource, ulong.MaxValue);
        var cause = new ChangeCause(CauseKind.OutsideAttempt, null, null);
        var history = new ObjectHistoryEntry(xaml, 10, 1, new LifetimeObservation(LifetimeOperation.BaselinePresent, "Synthetic", null,
            new(XamlObjectKind.UIElement, ClassificationEvidence.RuntimeType, "explicit bridge")), cause, StoreFixture.Partial);
        var attempt = new AttemptReference(f.Epoch, ulong.MaxValue, Guid.NewGuid(), ulong.MaxValue);
        var frame = new FrameInfo(attempt, new(0, 10), FrameAttemptStatus.Failed, new(ulong.MaxValue, 5, f.Epoch), -1, []);
        var topology = new TreeEntry(visual, null, RelationshipKind.VisualRoot, 0, true, Guid.NewGuid(), ulong.MaxValue, "Visual",
            StoreFixture.Partial, TreeMembershipKind.Composition, new(XamlObjectKind.NotApplicable, ClassificationEvidence.RuntimeType, "explicit type"));
        var composition = new CompositionTreeEntry(topology, [new(VisualProperty.Width,
            new(ValueKind.Number, ValueUnit.Pixels, null, 32, null, null, DataQuality.Complete), 10, cause)]);
        var correlation = new CorrelationEntry(xaml, visual, CorrelationKind.XamlVisual, new(0, 10), StoreFixture.Partial);
        var usage = new ResourceUsageEntry(resource, xaml, Guid.NewGuid(), Guid.NewGuid(),
            new(ValueKind.Integer, ValueUnit.Bytes, 100, null, null, null, DataQuality.Complete),
            SizeProvenance.ActualAllocation, new(ResourceKind.Texture2D, ResourceKindEvidence.CreationApi, "explicit CreateTexture"), DataQuality.Complete);
        var snapshot = new SnapshotMetadata(Guid.NewGuid(), f.Scope.SessionId, f.Scope.Process, SnapshotKind.Lightweight,
            new(10, null, CaptureMechanism.Synthetic, CaptureConsistency.NonAtomic, []), 1, f.Epoch,
            SnapshotState.Capturing, StoreFixture.Partial, null, null, 1, "synthetic", f.Info.QpcFrequency);
        var startup = new StartupEntry("observed phase", new(0, 10), ulong.MaxValue, StoreFixture.Partial);
        store.CreateWriter().Commit(f.Batch(1, 1) with { Changes = [
            new(1, history), new(1, frame), new(1, composition), new(1, correlation), new(1, usage),
            new(1, usage with { Owner = visual }), new(1, snapshot), new(1, startup)] });
        var q = store.Queries;
        Assert.Equal(history, (await q.GetObjectHistory(new(f.Scope, xaml, new(0, 11), new(1)), default)).Items.Single());
        Assert.Equal(frame.Attempt, (await q.GetFrames(new(f.Scope, new(0, 11), new(1)), default)).Items.Single().Attempt);
        Assert.Equal(composition.Topology, (await q.GetCompositionTree(new(f.Scope, 10, new(1)), default)).Items.Single().Topology);
        Assert.Equal(correlation, (await q.GetCorrelations(new(f.Scope, visual, 5, new(1)), default)).Items.Single());
        Assert.Equal(2, (await q.GetResourceUsage(new(f.Scope, new(0, 11), new(10)), default)).Items.Count);
        Assert.Single((await q.GetResourceUsage(new(f.Scope, new(0, 11), new(10), xaml), default)).Items);
        Assert.Equal(snapshot.SnapshotId, (await q.GetSnapshots(new(f.Scope, new(1), SnapshotKind.Lightweight), default)).Items.Single().SnapshotId);
        Assert.Equal(startup, (await q.GetStartupTimeline(new(f.Scope, new(1)), default)).Items.Single());
        using var db = f.Raw();
        Assert.Equal("FFFFFFFFFFFFFFFF", StoreFixture.Scalar(db, "SELECT FrameId FROM FrameSubmissions"));
        Assert.Equal(2L, StoreFixture.Scalar(db, "SELECT count(*) FROM ResourceOwners"));
        Assert.Equal(32d, StoreFixture.Scalar(db, "SELECT NumberValue FROM ObjectProperties"));
    }

    [Fact]
    public async Task Temporal_tombstones_root_selection_and_ties_are_stable()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var root = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, 1);
        var child = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, 2);
        var other = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, 3);
        var topology = new TreeEntry(root, null, RelationshipKind.Root, 0, true, Guid.NewGuid(), 1, null, StoreFixture.Partial,
            TreeMembershipKind.Visual, new(XamlObjectKind.Unknown, ClassificationEvidence.Unavailable, "no classification"));
        var childTree = topology with { Object = child, Parent = root, Relationship = RelationshipKind.Child };
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1) with { Changes = [new(1, topology), new(1, childTree), new(1, topology with { Object = other })] });
        var initial = await store.Queries.GetXamlTree(new(f.Scope, 10, new(10), root), default);
        Assert.Equal(2, initial.Items.Count);
        writer.Commit(f.Batch(2, 2) with { Changes = [new(2, childTree, true)] });
        Assert.Single((await store.Queries.GetXamlTree(new(f.Scope, 20, new(10), root), default)).Items);
        Assert.Equal(2, (await store.Queries.GetXamlTree(new(f.Scope, 10, new(10, 0, initial.Context.Watermark), root), default)).Items.Count);
    }

    [Fact]
    public async Task Materialization_has_a_byte_budget_in_addition_to_the_row_limit()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        for (int i = 1; i <= 18; i++)
        {
            var batch = f.Batch(i, i);
            var sample = ((MetricSample)batch.Changes.Single().Value) with { Source = new string('s', 950_000) };
            writer.Commit(batch with { Changes = [new(i, sample)] });
        }
        await Assert.ThrowsAsync<QueryMaterializationLimitException>(async () => await store.Queries.GetMetrics(f.Metrics(), default));
        Assert.Single((await store.Queries.GetMetrics(f.Metrics(new(1)), default)).Items);
    }

    [Fact]
    public async Task A_batch_retains_multiple_temporal_revisions_and_history_observations()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var key = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, ulong.MaxValue);
        var tree = new TreeEntry(key, null, RelationshipKind.Root, 0, true, Guid.NewGuid(), 1, null, StoreFixture.Partial,
            TreeMembershipKind.Visual, new(XamlObjectKind.Unknown, ClassificationEvidence.Unavailable, "no type evidence"));
        var cause = new ChangeCause(CauseKind.OutsideAttempt, null, null);
        var birth = new ObjectHistoryEntry(key, 10, 1, new LifetimeObservation(LifetimeOperation.BaselinePresent, null, null), cause, StoreFixture.Partial);
        var membership = birth with { Observation = new MembershipObservation(null, RelationshipKind.Root, 0, tree.RootId, 1, true, TreeMembershipKind.Visual) };
        store.CreateWriter().Commit(f.Batch(1, 2) with { Changes = [
            new(1, tree), new(2, tree with { Connected = false }), new(1, birth), new(1, membership)] });
        var before = await store.Queries.GetXamlTree(new(f.Scope, 10, new(1)), default);
        var after = await store.Queries.GetXamlTree(new(f.Scope, 20, new(1)), default);
        Assert.True(before.Items.Single().Connected);
        Assert.False(after.Items.Single().Connected);
        Assert.Equal(before.Context.Watermark, after.Context.Watermark);
        Assert.Equal(2, (await store.Queries.GetObjectHistory(new(f.Scope, key, new(0, 21), new(10)), default)).Items.Count);
    }

    [Fact]
    public async Task Correlation_validity_is_half_open_not_extended_by_a_query_timestamp()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var from = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, 1);
        var to = new ObjectKey(f.Scope.Process, IdentityDomain.CompositionVisual, f.Epoch, 2);
        var relation = new CorrelationEntry(from, to, CorrelationKind.XamlVisual, new(0, 10), StoreFixture.Partial);
        store.CreateWriter().Commit(f.Batch(1, 1) with { Changes = [new(1, relation)] });
        Assert.Single((await store.Queries.GetCorrelations(new(f.Scope, from, 9, new(10)), default)).Items);
        Assert.Empty((await store.Queries.GetCorrelations(new(f.Scope, from, 10, new(10)), default)).Items);
    }
}
