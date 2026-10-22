using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Query;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class P02ReviewRegressionTests
{
    [Fact]
    public void Open_validation_uses_one_snapshot_across_an_interleaved_Wal_commit()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        using var reader = SessionStore.Connect(f.Path, true);
        bool committed = false;
        Exception? callbackError = null;
        SQLitePCL.strdelegate_trace trace = (_, sql) =>
        {
            if (committed || !sql.Contains("SELECT count(*),coalesce(max(Ordinal)", StringComparison.Ordinal)) return;
            committed = true;
            try { writer.Commit(f.Batch(2, 2)); }
            catch (Exception error) { callbackError = error; }
        };
        SQLitePCL.raw.sqlite3_trace(reader.Handle, trace, null);
        try { SessionStore.ValidateDatabase(reader); }
        finally { SQLitePCL.raw.sqlite3_trace(reader.Handle, (SQLitePCL.strdelegate_trace)null!, null); }
        Assert.True(committed);
        Assert.Null(callbackError);
        Assert.Equal(new(2, 20), store.Watermark);
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(new(2, 20), reopened.Watermark);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Point_state_uses_effective_time_for_late_reparenting_resources_and_tombstones(bool composition)
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        var rootA = Tree(f, 1, composition);
        var rootB = Tree(f, 2, composition);
        var child = Tree(f, 3, composition) with { Parent = rootA.Object, Relationship = composition ? RelationshipKind.VisualChild : RelationshipKind.Child };
        ProjectionChange Change(long source, TreeEntry row, bool deleted = false) =>
            new(source, composition ? new CompositionTreeEntry(row, []) : row, deleted);
        async Task<QueryPage<TreeEntry>> Read(long at, ObjectKey? root = null, CommittedWatermark? boundary = null)
        {
            if (!composition) return await store.Queries.GetXamlTree(new(f.Scope, at, new(10, 0, boundary), root), default);
            var page = await store.Queries.GetCompositionTree(new(f.Scope, at, new(10, 0, boundary), root), default);
            return new(page.Context, page.Items.Select(row => row.Topology).ToArray(), page.Page);
        }
        var resource = new ResourceUsageEntry(Key(f, IdentityDomain.Resource, 4), rootA.Object, Guid.NewGuid(), Guid.NewGuid(),
            Bytes(20), SizeProvenance.ActualAllocation, new(ResourceKind.Texture2D, ResourceKindEvidence.CreationApi, "synthetic"), DataQuality.Complete);
        writer.Commit(At(f, 1, 0) with { Changes = [Change(1, rootA), Change(1, rootB)] });
        writer.Commit(At(f, 2, 20) with { Changes = [Change(2, child), new(2, resource)] });
        var originalBoundary = store.Watermark;
        writer.Commit(At(f, 3, 10) with { Changes = [
            Change(3, child with { Parent = rootB.Object }), new(3, resource with { Size = Bytes(10) })] });
        var before = await Read(10, rootB.Object);
        Assert.Contains(before.Items, row => row.Object == child.Object);
        var after = await Read(20, rootA.Object);
        Assert.Contains(after.Items, row => row.Object == child.Object && row.Parent == rootA.Object);
        Assert.Equal(20, (await store.Queries.GetResourceUsage(new(f.Scope, new(20, 20), new(10)), default)).Items.Single().Size.Integer);
        var frozen = await Read(20, rootA.Object, originalBoundary);
        Assert.Contains(frozen.Items, row => row.Object == child.Object);
        writer.Commit(At(f, 4, 15) with { Changes = [Change(4, child, true), new(4, resource, true)] });
        Assert.DoesNotContain((await Read(15)).Items, row => row.Object == child.Object);
        Assert.Empty((await store.Queries.GetResourceUsage(new(f.Scope, new(15, 15), new(10)), default)).Items);
        Assert.Contains((await Read(20, rootA.Object)).Items, row => row.Object == child.Object);
        Assert.Equal(20, (await store.Queries.GetResourceUsage(new(f.Scope, new(20, 20), new(10)), default)).Items.Single().Size.Integer);
        writer.Commit(At(f, 5, 20) with { Changes = [Change(5, child with { Connected = false }), new(5, resource with { Size = Bytes(21) })] });
        Assert.False((await Read(20, rootA.Object)).Items.Single(row => row.Object == child.Object).Connected);
        Assert.Equal(21, (await store.Queries.GetResourceUsage(new(f.Scope, new(20, 20), new(10)), default)).Items.Single().Size.Integer);
        Assert.Contains((await Read(20, rootA.Object, originalBoundary)).Items, row => row.Object == child.Object && row.Connected);
        Assert.Equal(20, (await store.Queries.GetResourceUsage(new(f.Scope, new(20, 20), new(10, 0, originalBoundary)), default)).Items.Single().Size.Integer);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Root_availability_is_selection_time_and_checkpoint_aware(bool composition)
    {
        using var f = new StoreFixture();
        Directory.CreateDirectory(System.IO.Path.GetDirectoryName(f.Path)!);
        using var store = SessionStore.Create(f.Path, f.Scope, f.Info, DataQuality.Complete);
        var writer = store.CreateWriter();
        var known = Tree(f, 1, composition);
        var later = Tree(f, 2, composition);
        var unknown = Tree(f, 3, composition);
        ProjectionChange Change(long source, TreeEntry row, bool deleted = false) =>
            new(source, composition ? new CompositionTreeEntry(row, []) : row, deleted);
        async Task<(int Count, DataQuality Quality)> Read(ObjectKey root, long at, CommittedWatermark? boundary = null)
        {
            if (composition)
            {
                var result = await store.Queries.GetCompositionTree(new(f.Scope, at, new(10, 0, boundary), root), default);
                return (result.Items.Count, result.Context.Quality);
            }
            var xaml = await store.Queries.GetXamlTree(new(f.Scope, at, new(10, 0, boundary), root), default);
            return (xaml.Items.Count, xaml.Context.Quality);
        }
        writer.Commit(At(f, 1, 10) with { Changes = [Change(1, known)], Quality = DataQuality.Complete });
        var frozen = store.Watermark;
        Assert.Equal(CoverageState.Unavailable, (await Read(unknown.Object, 10)).Quality.Coverage);
        Assert.Equal(CoverageState.Unavailable, (await Read(known.Object, 0)).Quality.Coverage);
        writer.Commit(At(f, 2, 20) with { Changes = [Change(2, known, true), Change(2, later)], Quality = DataQuality.Complete });
        var observedEmpty = await Read(known.Object, 20);
        Assert.Equal(0, observedEmpty.Count);
        Assert.Equal(DataQuality.Complete, observedEmpty.Quality);
        Assert.Equal(CoverageState.Unavailable, (await Read(later.Object, 10, frozen)).Quality.Coverage);
        Assert.Equal(CoverageState.Unavailable, (await Read(unknown.Object, 10, frozen)).Quality.Coverage);
        Assert.Equal(1, (await Read(known.Object, 10, frozen)).Count);
    }

    [Theory]
    [InlineData(0, 1)]
    [InlineData(5, 1)]
    [InlineData(10, 0)]
    [InlineData(15, 1)]
    [InlineData(14, 0)]
    [InlineData(16, 0)]
    public async Task Frame_points_obey_half_open_containment_and_keep_zero_duration(long point, int expected)
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        FrameInfo Frame(ulong id, TimeRange range) => new(new(f.Epoch, id, Guid.NewGuid(), 1), range, FrameAttemptStatus.Completed, null, 0, []);
        store.CreateWriter().Commit(At(f, 1, 20) with { Changes = [new(1, Frame(1, new(0, 10))), new(1, Frame(2, new(15, 15)))] });
        Assert.Equal(expected, (await store.Queries.GetFrames(new(f.Scope, new(point, point), new(10)), default)).Items.Count);
    }

    [Fact]
    public async Task Native_sql_cancellation_stops_running_work_and_keeps_connection_usable()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        store.CreateWriter().Commit(f.Batch(1, 1));
        using var db = SessionStore.Connect(f.Path, true);
        using var cancellation = new CancellationTokenSource();
        using var entered = new ManualResetEventSlim();
        using var resume = new ManualResetEventSlim();
        int visited = 0;
        db.CreateFunction<long, long>("p02_observe", value =>
        {
            if (Interlocked.Increment(ref visited) == 1)
            {
                entered.Set();
                if (!resume.Wait(TimeSpan.FromSeconds(30))) throw new TimeoutException("Cancellation test coordinator did not release native work.");
            }
            return value;
        });
        var running = Task.Run(() => SqliteReadExecution.Run(db, cancellation.Token, () =>
        {
            using var command = db.CreateCommand();
            command.CommandText = """
                WITH RECURSIVE work(x) AS (VALUES(1) UNION ALL SELECT x+1 FROM work WHERE x<1000000)
                SELECT sum(p02_observe(x)) FROM work
                """;
            return command.ExecuteScalar();
        }));
        try
        {
            Assert.True(entered.Wait(TimeSpan.FromSeconds(30)));
            cancellation.Cancel();
        }
        finally { resume.Set(); }
        var error = await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await running);
        Assert.Equal(cancellation.Token, error.CancellationToken);
        Assert.Equal(9, Assert.IsType<SqliteException>(error.InnerException).SqliteErrorCode);
        Assert.InRange(visited, 1, 999999);
        Assert.Equal(1L, StoreFixture.Scalar(db, "SELECT count(*) FROM SourceEvents"));
        Assert.Equal(new(1, 10), store.Watermark);
        Assert.Single((await store.Queries.GetMetrics(f.Metrics(), default)).Items);
        Assert.Equal(SessionStatus.Recording, (await store.Queries.GetSession(new(f.Scope), default)).Value.Status);
    }

    [Theory]
    [InlineData("SELECT EXISTS")]
    [InlineData("SELECT r.PayloadJson")]
    public async Task Public_queries_cancel_native_availability_and_result_steps(string stage)
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        store.CreateWriter().Commit(f.Batch(1, 128));
        using var cancellation = new CancellationTokenSource();
        bool started = false;
        SQLitePCL.strdelegate_trace trace = (_, sql) =>
        {
            if (!sql.StartsWith(stage, StringComparison.Ordinal)) return;
            started = true;
            cancellation.Cancel();
        };
        var queries = new ProfilerQueryService(store, () =>
        {
            var db = SessionStore.Connect(f.Path, true);
            SQLitePCL.raw.sqlite3_trace(db.Handle, trace, null);
            return db;
        });
        var error = await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await queries.GetMetrics(f.Metrics(), cancellation.Token));
        Assert.True(started);
        Assert.Equal(9, Assert.IsType<SqliteException>(error.InnerException).SqliteErrorCode);
        Assert.Equal(new(128, 1280), store.Watermark);
        Assert.Equal(128, (await store.Queries.GetMetrics(f.Metrics(), default)).Items.Count);
        Assert.Equal(SessionStatus.Recording, (await store.Queries.GetSession(new(f.Scope), default)).Value.Status);
        GC.KeepAlive(trace);
    }

    [Fact]
    public void Cancellation_just_before_a_native_step_cannot_be_lost_while_sqlite_is_idle()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        using var db = SessionStore.Connect(f.Path, true);
        using var cancellation = new CancellationTokenSource();
        int visited = 0;
        db.CreateFunction<long, long>("p02_observe", value => { visited++; return value; });
        var error = Assert.ThrowsAny<OperationCanceledException>(() => SqliteReadExecution.Run(db, cancellation.Token, () =>
        {
            cancellation.Cancel();
            using var command = db.CreateCommand();
            command.CommandText = """
                WITH RECURSIVE work(x) AS (VALUES(1) UNION ALL SELECT x+1 FROM work WHERE x<1000000)
                SELECT sum(p02_observe(x)) FROM work
                """;
            return command.ExecuteScalar();
        }));
        Assert.Equal(9, Assert.IsType<SqliteException>(error.InnerException).SqliteErrorCode);
        Assert.InRange(visited, 0, 999999);
        Assert.Equal(1L, StoreFixture.Scalar(db, "SELECT count(*) FROM Sessions"));
    }

    [Fact]
    public async Task Non_point_frame_revisions_still_follow_source_ordinal_not_interval_start()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var attempt = new AttemptReference(f.Epoch, 1, Guid.NewGuid(), 1);
        var original = new FrameInfo(attempt, new(5, 20), FrameAttemptStatus.Completed, null, 0, []);
        var correction = original with { Range = new(0, 10) };
        var writer = store.CreateWriter();
        writer.Commit(At(f, 1, 20) with { Changes = [new(1, original)] });
        var frozen = store.Watermark;
        writer.Commit(At(f, 2, 10) with { Changes = [new(2, correction)] });
        Assert.Equal(correction.Range, (await store.Queries.GetFrames(new(f.Scope, new(0, 21), new(10)), default)).Items.Single().Range);
        Assert.Equal(original.Range, (await store.Queries.GetFrames(new(f.Scope, new(0, 21), new(10, 0, frozen)), default)).Items.Single().Range);
    }

    private static ObjectKey Key(StoreFixture f, IdentityDomain domain, ulong id) => new(f.Scope.Process, domain, f.Epoch, id);
    private static TypedValue Bytes(long value) => new(ValueKind.Integer, ValueUnit.Bytes, value, null, null, null, DataQuality.Complete);
    private static TreeEntry Tree(StoreFixture f, ulong id, bool composition = false) => new(
        Key(f, composition ? IdentityDomain.CompositionVisual : IdentityDomain.XamlCore, id), null,
        composition ? RelationshipKind.VisualRoot : RelationshipKind.Root, 0, true, Guid.NewGuid(), 1, null,
        DataQuality.Complete, composition ? TreeMembershipKind.Composition : TreeMembershipKind.Visual,
        new(composition ? XamlObjectKind.NotApplicable : XamlObjectKind.UIElement, ClassificationEvidence.RuntimeType, "explicit synthetic type"));
    private static SessionBatch At(StoreFixture f, int ordinal, long timestamp)
    {
        var batch = f.Batch(ordinal, ordinal);
        var source = batch.Sources.Single();
        var e = source.Envelope;
        var payload = EventCodec.ReadPayload<MetricPayload>(e) with { Interval = new(Math.Max(0, timestamp - 1), timestamp) };
        return batch with { Sources = [EventCodec.Normalize(new(e.SessionId, e.Process, e.ProducerProcessId, e.ThreadId, timestamp,
            e.IngestOrdinal, e.SourceSequence, e.ActivityId, e.Identity, EventCodec.EncodePayload(payload, e.Process)), source.Stream)] };
    }
}
