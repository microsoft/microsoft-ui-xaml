using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class AtomicBaselineTests
{
    [Fact]
    public void MetricSample_remains_rejected_for_baseline_End_and_preserves_actual_prior_sources()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        var metric = new MetricSample(20, MetricKind.ProcessPrivateWorkingSet,
            new(ValueKind.Integer, ValueUnit.Bytes, 1, null, null, null, DataQuality.Complete),
            new(20, 20), "fixture", "1");
        Assert.Throws<ArgumentException>(() =>
            writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial,
                [Rows(f, 1, 1), new ProjectionChange[] { new(2, metric) }]));
        AssertOldBoundary(f, store);
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(new(1, 10), reopened.Watermark);
        using var raw = f.Raw();
        Assert.Equal(1L, StoreFixture.Scalar(raw, "SELECT count(*) FROM SourceEvents"));
    }

    [Fact]
    public void Original_End_identity_and_clock_are_committed_with_the_baseline_and_survive_reopen()
    {
        using var f = new StoreFixture();
        var clock = new SourceClockMetadata(Guid.NewGuid(), 1, new(f.Info.QpcFrequency * 2, f.Info.QpcFrequency, 500));
        var provenance = new SourceProvenance(new(f.Scope.Process.ProcessId, f.Scope.Process.CreationTimeFileTime, Guid.NewGuid()), 540, clock);
        using (var store = f.Create())
        {
            var writer = store.CreateWriter();
            writer.Commit(f.Batch(1, 1));
            writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial, [Rows(f, 1, 20)], provenance: provenance);
        }
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(new(2, 20), reopened.Watermark);
        Assert.Equal(provenance, reopened.ReadSourceProvenance(2));
        Assert.Equal(clock, Assert.Single(reopened.ReadSourceClocks()));
    }

    [Theory]
    [InlineData("target")]
    [InlineData("timestamp")]
    [InlineData("frequency")]
    [InlineData("mapping")]
    public void Invalid_original_End_provenance_is_rejected_before_publication(string invalid)
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        var original = invalid == "target"
            ? new(f.Scope.Process.ProcessId + 1, f.Scope.Process.CreationTimeFileTime, f.Scope.Process.InstanceId)
            : f.Scope.Process;
        var clock = new SourceClockMetadata(Guid.NewGuid(), 1,
            new(f.Info.QpcFrequency * 2, f.Info.QpcFrequency + (invalid == "frequency" ? 1 : 0), 500));
        var provenance = new SourceProvenance(original, invalid == "timestamp" ? -1 : invalid == "mapping" ? 550 : 540, clock);
        Assert.ThrowsAny<ArgumentException>(() =>
            writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial, [Rows(f, 1, 20)], provenance: provenance));
        AssertOldBoundary(f, store);
        Assert.Null(store.ReadSourceProvenance(2));
        Assert.Empty(store.ReadSourceClocks());
    }

    [Fact]
    public void Page_producer_failure_rolls_back_End_provenance_and_new_clock_with_the_projection()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        var provenance = new SourceProvenance(f.Scope.Process, 540,
            new(Guid.NewGuid(), 1, new(f.Info.QpcFrequency * 2, f.Info.QpcFrequency, 500)));
        IEnumerable<IReadOnlyList<ProjectionChange>> Pages()
        {
            yield return Rows(f, 1, 20);
            using var reader = SessionStore.OpenReadOnly(f.Path);
            Assert.Null(reader.ReadSourceProvenance(2));
            Assert.Empty(reader.ReadSourceClocks());
            throw new InvalidOperationException("Controlled baseline producer failure after clock staging.");
        }
        Assert.Throws<InvalidOperationException>(() =>
            writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial, Pages(), provenance: provenance));
        AssertOldBoundary(f, store);
        Assert.Null(store.ReadSourceProvenance(2));
        Assert.Empty(store.ReadSourceClocks());
    }

    [Fact]
    public async Task More_than_one_normal_batch_is_invisible_until_one_durable_baseline_commit()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        IEnumerable<IReadOnlyList<ProjectionChange>> Pages()
        {
            yield return Rows(f, 1, 2600);
            using var reader = SessionStore.OpenReadOnly(f.Path);
            Assert.Equal(new(1, 10), reader.Watermark);
            using var raw = f.Raw();
            Assert.Equal(0L, StoreFixture.Scalar(raw, "SELECT count(*) FROM ProjectionRows WHERE Dataset='History'"));
            yield return Rows(f, 2601, 2600);
        }
        Assert.Equal(new(2, 20), writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial, Pages()));
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(new(2, 20), reopened.Watermark);
        var row = Assert.Single((await reopened.Queries.GetObjectHistory(Request(f, 5200), default)).Items);
        Assert.Equal(2, row.IngestOrdinal);
        Assert.Equal(20, row.TimestampQpc);
        Assert.Equal(LifetimeOperation.BaselinePresent, Assert.IsType<LifetimeObservation>(row.Observation).Operation);
        using var raw = f.Raw();
        Assert.Equal(5200L, StoreFixture.Scalar(raw, "SELECT count(*) FROM ProjectionRows WHERE Dataset='History'"));
        Assert.Equal(2L, StoreFixture.Scalar(raw, "SELECT count(*) FROM SourceEvents"));
    }

    [Fact]
    public void Producer_failure_after_an_inserted_page_rolls_back_every_page_and_End()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        IEnumerable<IReadOnlyList<ProjectionChange>> Pages()
        {
            yield return Rows(f, 1, 20);
            throw new InvalidOperationException("Controlled baseline page producer failure.");
        }
        Assert.Throws<InvalidOperationException>(() => writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial, Pages()));
        AssertOldBoundary(f, store);
    }

    [Fact]
    public void Cancellation_between_pages_cannot_publish_a_partial_baseline()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        using var stop = new CancellationTokenSource();
        IEnumerable<IReadOnlyList<ProjectionChange>> Pages()
        {
            yield return Rows(f, 1, 20);
            stop.Cancel();
            yield return Rows(f, 21, 20);
        }
        Assert.ThrowsAny<OperationCanceledException>(() =>
            writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial, Pages(), cancellationToken: stop.Token));
        AssertOldBoundary(f, store);
    }

    [Fact]
    public async Task Duplicate_projection_keys_across_pages_are_rejected_without_marking_storage_failed()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        Assert.Throws<ArgumentException>(() =>
            writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial, [Rows(f, 1, 20), Rows(f, 20, 20)]));
        AssertOldBoundary(f, store);
        Assert.Equal(SessionStatus.Recording, (await store.Queries.GetSession(new(f.Scope), default)).Value.Status);
    }

    [Theory]
    [InlineData("page")]
    [InlineData("source")]
    [InlineData("old-end")]
    [InlineData("not-end")]
    [InlineData("missing-sequence")]
    [InlineData("unavailable")]
    public void Atomic_publication_does_not_relax_normal_page_or_actual_End_source_bounds(string invalid)
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        var end = invalid switch
        {
            "old-end" => End(f, ordinal: 1),
            "not-end" => f.Batch(2, 2).Sources[0],
            "missing-sequence" => End(f, sequence: null),
            "unavailable" => End(f, unavailable: true),
            _ => End(f)
        };
        var page = Rows(f, 1, invalid == "page" ? SessionWriter.MaximumBatchRecords + 1 : 1);
        if (invalid == "source") page[0] = page[0] with { SourceOrdinal = 1 };
        Assert.Throws<ArgumentException>(() => writer.CommitBaseline(end, f.Info, StoreFixture.Partial, [page]));
        AssertOldBoundary(f, store);
    }

    [Fact]
    public async Task Real_sqlite_page_quota_failure_preserves_old_history_and_durable_failure_status()
    {
        using var f = new StoreFixture();
        using var store = f.Create(new(1024));
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        using (var db = f.Raw())
            StoreFixture.Sql(db, "CREATE TRIGGER fill_baseline BEFORE INSERT ON ObjectLifetimes BEGIN UPDATE SourceEvents SET Envelope=zeroblob(8388608); END;");
        var error = Assert.Throws<SessionWriteException>(() =>
            writer.CommitBaseline(End(f), f.Info, StoreFixture.Partial, [Rows(f, 1, 20)]));
        Assert.Equal(13, error.SqliteErrorCode);
        AssertOldBoundary(f, store);
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(SessionStatus.Failed, (await reopened.Queries.GetSession(new(f.Scope), default)).Value.Status);
        Assert.Single((await reopened.Queries.GetMetrics(f.Metrics(), default)).Items);
    }

    [Fact]
    public void Aggregate_serialized_byte_limit_is_enforced_at_the_exact_record_without_partial_publication()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        writer.Commit(f.Batch(1, 1));
        var end = End(f);
        var context = new ResultContext(f.Scope, new(2, 20), StoreFixture.Partial);
        var info = QueryResultValidation.Freeze(f.Info, context);
        long bytes = 32 + EventCodec.Encode(end.Envelope).LongLength +
            JsonSerializer.SerializeToUtf8Bytes(info, WireJson.Options).Length +
            JsonSerializer.SerializeToUtf8Bytes(StoreFixture.Partial, WireJson.Options).Length;
        long previousBytes = bytes;
        var name = new string('s', 50_000);
        IEnumerable<IReadOnlyList<ProjectionChange>> Pages()
        {
            for (var id = 1; id < 2000; id++)
            {
                var value = (ObjectHistoryEntry)Rows(f, id, 1)[0].Value;
                value = value with { Observation = new LifetimeObservation(LifetimeOperation.BaselinePresent, name, null) };
                previousBytes = bytes;
                bytes += 16 + JsonSerializer.SerializeToUtf8Bytes(value, WireJson.Options).Length;
                yield return [new(2, value)];
            }
        }
        var error = Record.Exception(() => writer.CommitBaseline(end, f.Info, StoreFixture.Partial, Pages()));
        Assert.NotNull(error);
        Assert.Equal("ProcessingLimitException", error.GetType().Name);
        Assert.True(previousBytes <= SessionWriter.MaximumBaselineBytes);
        Assert.True(bytes > SessionWriter.MaximumBaselineBytes);
        AssertOldBoundary(f, store);
    }

    private static void AssertOldBoundary(StoreFixture f, SessionStore store)
    {
        Assert.Equal(new(1, 10), store.Watermark);
        using var raw = f.Raw();
        Assert.Equal(0L, StoreFixture.Scalar(raw, "SELECT count(*) FROM ProjectionRows WHERE Dataset='History'"));
        Assert.Equal(1L, StoreFixture.Scalar(raw, "SELECT count(*) FROM SourceEvents"));
        Assert.Equal(0L, StoreFixture.Scalar(raw, "SELECT count(*) FROM CaptureEpochs"));
    }
    private static GetObjectHistoryRequest Request(StoreFixture f, ulong id) =>
        new(f.Scope, new(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, id), new(0, 21), new(10));
    private static ProjectionChange[] Rows(StoreFixture f, int first, int count) =>
        Enumerable.Range(first, count).Select(id => new ProjectionChange(2,
            new ObjectHistoryEntry(new(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, (ulong)id), 20, 2,
                new LifetimeObservation(LifetimeOperation.BaselinePresent, "Synthetic baseline element", null),
                new(CauseKind.Unknown, null, "Synthetic present-state capture, not a birth."), StoreFixture.Partial))).ToArray();
    private static NormalizedEvent End(StoreFixture f, long ordinal = 2, ulong? sequence = 1, bool unavailable = false)
    {
        var stream = new ProducerStreamKey(EventCatalog.RuntimeProviderId, f.Scope.Process.ProcessId, f.Epoch);
        var domain = new SequenceDomainKey(stream, f.Scope.Process, 2, EventCatalog.Version);
        var payload = new CaptureBoundaryPayload(Guid.NewGuid(), CaptureKind.Baseline, CapturePhase.End, CoreTests.Session, 1,
            [new(domain, 0, 0, DataQuality.Complete, new(Guid.NewGuid(), domain, 0, 0))], 1,
            unavailable ? new(MeasurementKind.Unavailable, CoverageState.Unavailable, "Controlled unsupported capture.") : DataQuality.Complete);
        return EventCodec.Normalize(new(f.Scope.SessionId, f.Scope.Process, f.Scope.Process.ProcessId, 1, 20, ordinal, sequence, null,
            EventCatalog.Get(EventCatalog.RuntimeProviderId, 13).Identity, EventCodec.EncodePayload(payload, f.Scope.Process)), stream);
    }
}
