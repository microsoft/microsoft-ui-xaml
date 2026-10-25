using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class StorageTests
{
    [Fact]
    public async Task Commit_is_atomic_and_reopens_with_durable_boundary()
    {
        using var f = new StoreFixture();
        using (var store = f.Create())
        {
            Assert.Equal(new(0, 0), store.Watermark);
            var writer = store.CreateWriter();
            Assert.Equal(new(2, 20), writer.Commit(f.Batch(1, 2)));
            using var reader = SessionStore.OpenReadOnly(f.Path);
            Assert.Equal(new(2, 20), reader.Watermark);
            var rows = await reader.Queries.GetMetrics(f.Metrics(), default);
            Assert.Equal(2, rows.Items.Count);
            var invalid = f.Batch(3, 4);
            invalid = invalid with { Changes = [.. invalid.Changes, new(4, invalid.Changes[0].Value)] };
            Assert.Throws<ArgumentException>(() => writer.Commit(invalid));
            Assert.Equal(new(2, 20), store.Watermark);
            Assert.Equal(2, (await reader.Queries.GetMetrics(f.Metrics(), default)).Items.Count);
        }
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(new(2, 20), reopened.Watermark);
        Assert.True(reopened.UnfinishedRecording);
        Assert.Equal(SessionStatus.Recording, (await reopened.Queries.GetSession(new(f.Scope), default)).Value.Status);
    }

    [Fact]
    public void Read_only_never_creates_or_migrates_and_rejects_future_or_corrupt_databases()
    {
        using var f = new StoreFixture();
        Assert.Throws<FileNotFoundException>(() => SessionStore.OpenReadOnly(f.Path));
        Assert.False(File.Exists(f.Path));
        using (f.Create()) { }
        using (var read = SessionStore.OpenReadOnly(f.Path))
            Assert.Throws<InvalidOperationException>(() => read.CreateWriter());
        using (var db = f.Raw()) StoreFixture.Sql(db, "PRAGMA user_version=999");
        Assert.Throws<NotSupportedException>(() => SessionStore.OpenReadOnly(f.Path));
        Assert.Throws<NotSupportedException>(() => SessionStore.OpenWritable(f.Path));
        using (var db = f.Raw()) StoreFixture.Sql(db, "PRAGMA user_version=1; DROP TABLE Relationships;");
        Assert.Throws<InvalidDataException>(() => SessionStore.OpenReadOnly(f.Path));
    }

    [Fact]
    public void Interrupted_sqlite_transaction_rolls_back_and_one_writer_is_enforced()
    {
        using var f = new StoreFixture();
        using (var store = f.Create())
        {
            store.CreateWriter().Commit(f.Batch(1, 1));
            Assert.Throws<IOException>(() => SessionStore.OpenWritable(f.Path));
        }
        using (var db = f.Raw())
        {
            using var transaction = db.BeginTransaction();
            StoreFixture.Sql(db, "UPDATE SourceEvents SET Envelope=X'00'", transaction);
        }
        using var reopened = SessionStore.OpenWritable(f.Path);
        Assert.Equal(new(1, 10), reopened.Watermark);
        using var check = f.Raw();
        Assert.Equal(1L, StoreFixture.Scalar(check, "SELECT count(*) FROM SourceEvents"));
    }

    [Fact]
    public async Task Mid_batch_database_failure_rolls_back_and_records_failure_outside_database()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        using (var db = f.Raw())
            StoreFixture.Sql(db, "CREATE TRIGGER fail_projection BEFORE INSERT ON MetricSamples BEGIN SELECT RAISE(ABORT, 'controlled writer failure'); END");
        Assert.Throws<SessionWriteException>(() => writer.Commit(f.Batch(2, 3)));
        Assert.Equal(new(1, 10), store.Watermark);
        Assert.True(File.Exists(f.Path + ".failure.json"));
        Assert.InRange(new FileInfo(f.Path + ".failure.json").Length, 1, 4096);
        Assert.Throws<InvalidOperationException>(() => writer.Commit(f.Batch(2, 2)));
        using var reader = SessionStore.OpenReadOnly(f.Path);
        var info = await reader.Queries.GetSession(new(f.Scope), default);
        Assert.Equal(SessionStatus.Failed, info.Value.Status);
        Assert.NotNull(info.Value.StopReason);
        Assert.Single((await reader.Queries.GetMetrics(f.Metrics(), default)).Items);
        using var raw = f.Raw();
        Assert.Equal(1L, StoreFixture.Scalar(raw, "SELECT count(*) FROM SourceEvents"));
    }

    [Fact]
    public async Task Actual_sqlite_full_preserves_last_committed_history()
    {
        using var f = new StoreFixture();
        using var store = f.Create(new(1024));
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        // Exercise SQLite's real page quota, not a synthetic exception.
        using (var db = f.Raw())
        {
            StoreFixture.Sql(db, "CREATE TRIGGER fill_disk BEFORE INSERT ON MetricSamples BEGIN UPDATE SourceEvents SET Envelope=zeroblob(8388608); END;");
        }
        var error = Assert.Throws<SessionWriteException>(() => writer.Commit(f.Batch(2, 2)));
        Assert.Equal(13, error.SqliteErrorCode);
        Assert.Equal(new(1, 10), store.Watermark);
        Assert.Single((await store.Queries.GetMetrics(f.Metrics(), default)).Items);
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(SessionStatus.Failed, (await reopened.Queries.GetSession(new(f.Scope), default)).Value.Status);
    }

    [Fact]
    public void Incompatible_process_metadata_and_boundary_state_are_rejected()
    {
        using var f = new StoreFixture();
        using (var store = f.Create()) store.CreateWriter().Commit(f.Batch(1, 1));
        using (var db = f.Raw()) StoreFixture.Sql(db, "UPDATE ProcessInstances SET ProcessId=43");
        Assert.Throws<InvalidDataException>(() => SessionStore.OpenReadOnly(f.Path));
        using (var db = f.Raw()) StoreFixture.Sql(db, "UPDATE ProcessInstances SET ProcessId=42; UPDATE Checkpoints SET TimestampQpc=0 WHERE Ordinal=1");
        Assert.Throws<InvalidDataException>(() => SessionStore.OpenReadOnly(f.Path));
    }

    [Fact]
    public void Cancellation_wrong_scope_and_terminal_writes_cannot_advance_the_boundary()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        Assert.ThrowsAny<OperationCanceledException>(() => writer.Commit(f.Batch(1, 1), new(true)));
        var wrong = f.Batch(1, 1);
        var e = wrong.Sources[0].Envelope;
        wrong = wrong with { Sources = [new(wrong.Sources[0].Stream, new(Guid.NewGuid(), e.Process, e.ProducerProcessId,
            e.ThreadId, e.TimestampQpc, e.IngestOrdinal, e.SourceSequence, e.ActivityId, e.Identity, e.OwnedPayload))] };
        Assert.Throws<ArgumentException>(() => writer.Commit(wrong));
        Assert.Equal(new(0, 0), store.Watermark);
        writer.Commit(f.Batch(1, 1) with { Info = f.Info with { Status = SessionStatus.Completed } });
        Assert.False(store.UnfinishedRecording);
        Assert.Throws<InvalidOperationException>(() => writer.Commit(f.Batch(2, 2)));
    }

    [Fact]
    public async Task Wal_reader_cannot_observe_an_in_flight_checkpoint_or_projection()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        store.CreateWriter().Commit(f.Batch(1, 1));
        using var reader = SessionStore.OpenReadOnly(f.Path);
        using var db = f.Raw();
        using var transaction = db.BeginTransaction();
        StoreFixture.Sql(db, "UPDATE Checkpoints SET TimestampQpc=999 WHERE Ordinal=1; DELETE FROM MetricSamples; DELETE FROM ProjectionRows;", transaction);
        Assert.Equal(new(1, 10), reader.Watermark);
        var page = await reader.Queries.GetMetrics(f.Metrics(), default);
        Assert.Single(page.Items);
        Assert.Equal(new(1, 10), page.Context.Watermark);
        transaction.Rollback();
    }

    [Fact]
    public async Task Source_capabilities_capture_epochs_and_unsigned_allocation_values_are_typed()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var info = new RuntimeCapabilitiesPayload(f.Epoch, "synthetic/1", HostArchitecture.X64, f.Scope.Process, f.Info.QpcFrequency,
            [new(Capability.TracedAllocator, 1, StoreFixture.Partial)]);
        var capture = new CaptureBoundaryPayload(Guid.NewGuid(), CaptureKind.LightweightSnapshot, CapturePhase.Begin,
            Guid.NewGuid(), ulong.MaxValue, [], null, StoreFixture.Partial);
        NormalizedEvent Source(IEventPayload payload, ushort id, long ordinal)
        {
            var identity = EventCatalog.Get(EventCatalog.RuntimeProviderId, id).Identity;
            return EventCodec.Normalize(new(f.Scope.SessionId, f.Scope.Process, f.Scope.Process.ProcessId, 7, ordinal * 10, ordinal,
                ulong.MaxValue, null, identity, EventCodec.EncodePayload(payload, f.Scope.Process)), new(identity.ProviderId, f.Scope.Process.ProcessId, f.Epoch));
        }
        var key = new ObjectKey(f.Scope.Process, IdentityDomain.Resource, f.Epoch, ulong.MaxValue);
        var allocation = new ObjectHistoryEntry(key, 30, 3,
            new AllocationObservation(ulong.MaxValue, AllocationOperation.Allocate, null, ulong.MaxValue, ulong.MaxValue, f.Epoch),
            new(CauseKind.OutsideAttempt, null, null), StoreFixture.Partial);
        var batch = f.Batch(3, 3);
        store.CreateWriter().Commit(new([Source(info, 1, 1), Source(capture, 13, 2), batch.Sources.Single()],
            [new(3, allocation)], f.Info, StoreFixture.Partial));
        Assert.Equal(allocation, (await store.Queries.GetObjectHistory(new(f.Scope, key, new(0, 31), new(10)), default)).Items.Single());
        using var db = f.Raw();
        Assert.Equal(1L, StoreFixture.Scalar(db, "SELECT count(*) FROM Capabilities"));
        Assert.Equal("FFFFFFFFFFFFFFFF", StoreFixture.Scalar(db, "SELECT DispatcherId FROM CaptureEpochs"));
        Assert.Equal(4294967295L, StoreFixture.Scalar(db, "SELECT RequestedBytesHigh FROM ResourceAllocations"));
        Assert.Equal(4294967295L, StoreFixture.Scalar(db, "SELECT ActualBytesLow FROM ResourceAllocations"));
    }

    [Fact]
    public void Explicit_snapshot_attributes_and_edges_are_atomic_typed_and_snapshot_scoped()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var snapshot = new SnapshotMetadata(Guid.NewGuid(), f.Scope.SessionId, f.Scope.Process, SnapshotKind.Lightweight,
            new(10, null, CaptureMechanism.Synthetic, CaptureConsistency.NonAtomic, []), 1, f.Epoch,
            SnapshotState.Capturing, StoreFixture.Partial, null, null, 1, "synthetic", f.Info.QpcFrequency);
        var from = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, ulong.MaxValue);
        var to = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, 1);
        var attribute = new SnapshotAttributeChange(1, snapshot.SnapshotId,
            new(SnapshotAttribute.ObjectCount, ComparisonGrouping.Session, null, null, null, null),
            new(ValueKind.Integer, ValueUnit.Count, 2, null, null, null, StoreFixture.Partial));
        var edge = new RetentionEdgeChange(1, snapshot.SnapshotId, new(from, to, "explicit ownership", OwnershipEvidence.Proven, StoreFixture.Partial));
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1) with { Changes = [new(1, snapshot)], SnapshotAttributes = [attribute], RetentionEdges = [edge] });
        using var db = f.Raw();
        Assert.Equal(1L, StoreFixture.Scalar(db, "SELECT count(*) FROM SnapshotAttributes"));
        Assert.Equal(1L, StoreFixture.Scalar(db, "SELECT count(*) FROM RetentionEdges"));
        Assert.Equal(2L, StoreFixture.Scalar(db, "SELECT IntegerValue FROM SnapshotAttributes"));
        Assert.Equal(1L, StoreFixture.Scalar(db, "SELECT count(*) FROM ProjectionRows WHERE Dataset='RetentionEdge' AND SnapshotId IS NOT NULL"));
        Assert.Throws<ArgumentException>(() => writer.Commit(f.Batch(2, 2) with {
            SnapshotAttributes = [attribute with { SourceOrdinal = 2, SnapshotId = Guid.NewGuid() }] }));
        Assert.Equal(new(1, 10), store.Watermark);
    }

    [Fact]
    public async Task Fallback_write_failure_is_exposed_without_hiding_the_known_failed_state()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        using (var db = f.Raw())
            StoreFixture.Sql(db, "CREATE TRIGGER fail_projection BEFORE INSERT ON MetricSamples BEGIN SELECT RAISE(ABORT, 'controlled failure'); END");
        File.Delete(f.Path + ".failure.json");
        Directory.CreateDirectory(f.Path + ".failure.json");
        try
        {
            var error = Assert.Throws<SessionWriteException>(() => writer.Commit(f.Batch(2, 2)));
            Assert.NotNull(error.FailureRecordError);
            Assert.Equal(SessionStatus.Failed, (await store.Queries.GetSession(new(f.Scope), default)).Value.Status);
            Assert.Equal(new(1, 10), store.Watermark);
        }
        finally { Directory.Delete(f.Path + ".failure.json"); }
    }

    [Fact]
    public void Session_metadata_is_bounded_before_creation_or_commit()
    {
        using var f = new StoreFixture();
        var oversized = f.Info with { Producers = [new(EventCatalog.RuntimeProviderId, f.Epoch, 1,
            new string('x', ContractVersions.MaximumPayloadBytes), HostArchitecture.X64, [])] };
        Directory.CreateDirectory(System.IO.Path.GetDirectoryName(f.Path)!);
        Assert.Throws<ArgumentException>(() => { using var unexpected = SessionStore.Create(f.Path, f.Scope, oversized, StoreFixture.Partial); });
        Assert.False(File.Exists(f.Path));
        using var store = f.Create();
        var writer = store.CreateWriter();
        Assert.Throws<ArgumentException>(() => writer.Commit(f.Batch(1, 1) with { Info = oversized }));
        Assert.Equal(new(0, 0), store.Watermark);
    }
}

internal sealed class StoreFixture : IDisposable
{
    internal string Path { get; } = System.IO.Path.GetFullPath(System.IO.Path.Combine("TestResults", Guid.NewGuid() + ".sqlite"));
    internal SessionScope Scope { get; } = new(Guid.NewGuid(), new(42, null, Guid.NewGuid()));
    internal Guid Epoch { get; } = Guid.NewGuid();
    internal SessionInfo Info { get; } = new(SessionMode.LiveAttach, SessionStatus.Recording, 1_000_000, null, [], null);
    internal static DataQuality Partial { get; } = new(MeasurementKind.Estimated, CoverageState.Partial, "explicit synthetic estimate");
    internal SessionStore Create(SessionStoreOptions? options = null)
    {
        Directory.CreateDirectory(System.IO.Path.GetDirectoryName(Path)!);
        return SessionStore.Create(Path, Scope, Info, Partial, options);
    }
    internal GetMetricsRequest Metrics(PageRequest? page = null) => new(Scope, new(0, 10000), page ?? new(128));
    internal SessionBatch Batch(int first, int last)
    {
        var sources = new List<NormalizedEvent>();
        var changes = new List<ProjectionChange>();
        for (var i = first; i <= last; i++)
        {
            var sample = new MetricSample(i * 10, MetricKind.ProcessPrivateCommit,
                new(ValueKind.Integer, ValueUnit.Bytes, i, null, null, null, Partial),
                new(i * 10 - 1, i * 10), "synthetic", "1");
            var payload = new MetricPayload(sample.Metric, sample.Value, sample.Interval, sample.Source, sample.SourceVersion, null);
            var identity = EventCatalog.Get(EventCatalog.CollectorProviderId, 2).Identity;
            var envelope = new EventEnvelope(Scope.SessionId, Scope.Process, 99, 7, sample.TimestampQpc, i,
                (ulong)i, null, identity, EventCodec.EncodePayload(payload, Scope.Process));
            sources.Add(EventCodec.Normalize(envelope, new(identity.ProviderId, 99, Epoch)));
            changes.Add(new(i, sample));
        }
        return new(sources, changes, Info, Partial);
    }
    internal SqliteConnection Raw()
    {
        var db = new SqliteConnection(new SqliteConnectionStringBuilder { DataSource = Path, Pooling = false }.ToString());
        db.Open(); return db;
    }
    internal static void Sql(SqliteConnection db, string text, SqliteTransaction? transaction = null)
    {
        using var command = db.CreateCommand(); command.CommandText = text; command.Transaction = transaction; command.ExecuteNonQuery();
    }
    internal static object? Scalar(SqliteConnection db, string text)
    {
        using var command = db.CreateCommand(); command.CommandText = text; return command.ExecuteScalar();
    }
    public void Dispose()
    {
        foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
            if (File.Exists(Path + suffix)) File.Delete(Path + suffix);
    }
}
