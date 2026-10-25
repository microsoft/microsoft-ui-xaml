using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class BaselinePagePreparationTests
{
    [Fact]
    public async Task Real_baseline_output_is_frozen_into_multiple_pages_on_owner_before_sqlite_writer_runs()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var chunks = Chunks(f, 1800);
        var end = End(f, chunks);
        BaselineProjectionPages? prepared = null;
        Exception? failure = null;
        var ownerId = 0;
        var writerId = 0;
        var provenance = new SourceProvenance(f.Scope.Process, 540,
            new(Guid.NewGuid(), 1, new(f.Info.QpcFrequency * 2, f.Info.QpcFrequency, 500)));
        var owner = new Thread(() =>
        {
            try
            {
                ownerId = Environment.CurrentManagedThreadId;
                prepared = new BaselinePagePreparer(f.Scope, f.Store.Watermark, f.Processors)
                    .Prepare(end, f.Info, ProjectionFixture.Partial, chunks, provenance);
            }
            catch (Exception error) { failure = error; }
        });
        owner.Start(); owner.Join();
        Assert.Null(failure);
        Assert.NotNull(prepared);
        Assert.Equal(5400, prepared.RecordCount);
        Assert.Equal([4096, 1304], prepared.Pages.Select(p => p.Count));
        foreach (var chunk in chunks)
        {
            Array.Clear((SnapshotObject[])chunk.Objects);
            Array.Clear((TreeEdgePayload[])chunk.Tree);
        }
        var writer = new Thread(() =>
        {
            try
            {
                writerId = Environment.CurrentManagedThreadId;
                f.Store.CreateWriter().CommitBaseline(prepared.End, prepared.Info, prepared.Quality,
                    prepared.Pages, prepared.Provenance);
            }
            catch (Exception error) { failure = error; }
        });
        writer.Start(); writer.Join();
        Assert.Null(failure);
        Assert.NotEqual(ownerId, writerId);
        using var reopened = f.Reopen();
        Assert.Equal(new(1, 20), reopened.Watermark);
        Assert.Equal(provenance, reopened.ReadSourceProvenance(1));
        Assert.Equal(provenance.Clock, Assert.Single(reopened.ReadSourceClocks()));
        Assert.Equal(128, (await reopened.Queries.GetXamlTree(new(f.Scope, 20, new(128)), default)).Items.Count);
        Assert.Throws<NotSupportedException>(() => ((IList<IReadOnlyList<ProjectionChange>>)prepared.Pages).Clear());
        Assert.Throws<NotSupportedException>(() => ((IList<ProjectionChange>)prepared.Pages[0]).Clear());
    }

    [Fact]
    public void Two_real_processors_share_one_aggregate_allowance_and_failure_is_terminal()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var chunks = Chunks(f, 2);
        var preparer = new BaselinePagePreparer(f.Scope, f.Store.Watermark, f.Processors, maximumRecords: 5);
        Assert.Throws<ProcessingLimitException>(() => preparer.Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks));
        Assert.Throws<InvalidOperationException>(() => preparer.Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks));
        Assert.Equal(new(0, 0), f.Store.Watermark);
    }

    [Fact]
    public void Exact_shared_count_and_byte_allowances_are_accepted_with_End_metadata_provenance_and_clock()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,XamlTreeProcessor");
        var chunks = Chunks(f, 2);
        var end = End(f, chunks);
        var provenance = new SourceProvenance(f.Scope.Process, 540,
            new(Guid.NewGuid(), 1, new(f.Info.QpcFrequency * 2, f.Info.QpcFrequency, 500)));
        var prepared = new BaselinePagePreparer(f.Scope, default, f.Processors, maximumRecords: 6)
            .Prepare(end, f.Info, ProjectionFixture.Partial, chunks, provenance);
        long exact = 32 + EventCodec.Encode(prepared.End.Envelope).LongLength +
            JsonSerializer.SerializeToUtf8Bytes(prepared.Info, WireJson.Options).Length +
            JsonSerializer.SerializeToUtf8Bytes(prepared.Quality, WireJson.Options).Length +
            JsonSerializer.SerializeToUtf8Bytes(provenance, WireJson.Options).Length +
            prepared.Pages.SelectMany(p => p).Sum(c => 16L + JsonSerializer.SerializeToUtf8Bytes(c.Value, c.Value.GetType(), WireJson.Options).Length);
        Assert.Equal(6, prepared.RecordCount);
        Assert.Equal(exact, prepared.AccountedBytes);
        var processors = new IEventProcessor[] { new ObjectStateProcessor(), new XamlTreeProcessor() };
        var exactPages = new BaselinePagePreparer(f.Scope, default, processors, 6, exact)
            .Prepare(end, f.Info, ProjectionFixture.Partial, chunks, provenance);
        f.Store.CreateWriter().CommitBaseline(exactPages.End, exactPages.Info, exactPages.Quality, exactPages.Pages, exactPages.Provenance);
        Assert.Equal(new(1, 20), f.Store.Watermark);
        Assert.Throws<ProcessingLimitException>(() =>
            new BaselinePagePreparer(f.Scope, default, [new ObjectStateProcessor(), new XamlTreeProcessor()], 6, exact - 1)
                .Prepare(end, f.Info, ProjectionFixture.Partial, chunks, provenance));
    }

    [Fact]
    public async Task Real_composition_local_allowance_does_not_double_consume_shared_budget_and_nested_values_are_frozen()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor,CompositionTreeProcessor");
        var chunks = Chunks(f, 2);
        var objects = chunks[0].Objects.Select(o => o with
        {
            Key = new(f.Scope.Process, IdentityDomain.CompositionVisual, f.Epoch, o.Key.RuntimeObjectId),
            Classification = null
        }).ToArray();
        var visuals = objects.Select(o => new VisualDeltaPayload(o.Key, VisualProperty.Opacity,
            new(ValueKind.Number, ValueUnit.Scalar, null, .5, null, null, DataQuality.Complete))).ToArray();
        chunks[0] = chunks[0] with
        {
            Objects = objects,
            Tree = objects.Select(o => f.Edge(o.Key, relationship: RelationshipKind.VisualRoot,
                membership: TreeMembershipKind.Composition)).ToArray(),
            Visuals = visuals
        };
        var capabilities = new[] { new CapabilityInfo(Capability.ObjectLifetimes, 1, DataQuality.Complete) };
        var producers = new[]
        {
            new ProducerInfo(EventCatalog.RuntimeProviderId, f.Epoch, 1, "fixture", HostArchitecture.X64, capabilities)
        };
        var info = f.Info with { Producers = producers };
        var prepared = new BaselinePagePreparer(f.Scope, default, f.Processors, maximumRecords: 8)
            .Prepare(End(f, chunks), info, ProjectionFixture.Partial, chunks);
        Assert.Equal(8, prepared.RecordCount);
        var composition = prepared.Pages.SelectMany(p => p).Select(c => c.Value).OfType<CompositionTreeEntry>().ToArray();
        Assert.Equal(2, composition.Length);
        Assert.All(composition, row =>
            Assert.Throws<NotSupportedException>(() => ((IList<ObservedVisualProperty>)row.Properties).Clear()));
        Array.Clear(objects); Array.Clear(visuals); Array.Clear(producers); Array.Clear(capabilities);
        var source = f.Source(30, 2, new VisualDeltaPayload(composition[0].Topology.Object, VisualProperty.Opacity,
            new(ValueKind.Number, ValueUnit.Scalar, null, .9, null, null, DataQuality.Complete)));
        _ = f.Processors[1].Process([source], new(f.Scope, new(2, 30), ProjectionFixture.Partial,
            new(SessionWriter.MaximumBatchBytes))).ToArray();
        Assert.All(composition, row => Assert.Equal(.5, Assert.Single(row.Properties).Value.Number));
        Assert.Equal(Capability.ObjectLifetimes, Assert.Single(Assert.Single(prepared.Info.Producers).Capabilities).Capability);
        Assert.Throws<NotSupportedException>(() => ((IList<CapabilityInfo>)prepared.Info.Producers[0].Capabilities).Clear());
        Assert.Throws<NotSupportedException>(() => ((IList<ProducerInfo>)prepared.Info.Producers).Clear());
        f.Store.CreateWriter().CommitBaseline(prepared.End, prepared.Info, prepared.Quality, prepared.Pages);
        using var reopened = f.Reopen();
        Assert.All((await reopened.Queries.GetCompositionTree(new(f.Scope, 20, new(128)), default)).Items,
            row => Assert.Equal(.5, Assert.Single(row.Properties).Value.Number));
    }

    [Fact]
    public void A_value_at_exactly_64KiB_is_accepted_and_persists_without_changing_accounting()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var initial = History(f, 1, "x");
        var initialBytes = JsonSerializer.SerializeToUtf8Bytes(initial.Value, initial.Value.GetType(), WireJson.Options).Length;
        var row = History(f, 1, new string('x', ProjectionBudget.MaximumRecordBytes - initialBytes + 1));
        Assert.Equal(ProjectionBudget.MaximumRecordBytes,
            JsonSerializer.SerializeToUtf8Bytes(row.Value, row.Value.GetType(), WireJson.Options).Length);
        IEnumerable<ProjectionChange> Output() { yield return row; }
        var prepared = new BaselinePagePreparer(f.Scope, default, [new OutputProcessor(Output)])
            .Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks);
        f.Store.CreateWriter().CommitBaseline(prepared.End, prepared.Info, prepared.Quality, prepared.Pages);
        using var reopened = f.Reopen();
        Assert.Equal(new(1, 20), reopened.Watermark);
    }

    [Fact]
    public void Baseline_aggregate_count_can_exceed_ordinary_budget_without_weakening_ordinary_default()
    {
        var budget = new ProjectionBudget(SessionWriter.MaximumBaselineBytes, SessionWriter.MaximumBaselineRecords);
        for (var i = 0; i < 131072; i++) budget.Consume(1);
        Assert.Equal(0, budget.RemainingRecords);
        Assert.Throws<ProcessingLimitException>(() => budget.Consume(1));
        Assert.Equal(4096, new ProjectionBudget(SessionWriter.MaximumBaselineBytes).RemainingRecords);
    }

    [Theory]
    [InlineData(131073, 67108864)]
    [InlineData(131072, 67108865)]
    [InlineData(0, 67108864)]
    [InlineData(131072, 0)]
    public void Configured_aggregate_allowance_cannot_exceed_atomic_storage_hard_limits(int records, long bytes)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new BaselinePagePreparer(f.Scope, default, f.Processors, records, bytes));
    }

    [Theory]
    [InlineData(4096, true)]
    [InlineData(4097, false)]
    public void Ordinary_commit_retains_its_exact_4096_change_bound(int count, bool accepted)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var end = End(f, chunks);
        var changes = Enumerable.Range(1, count).Select(i => History(f, i)).ToArray();
        var writer = f.Store.CreateWriter();
        if (accepted)
        {
            writer.Commit(new([end], changes, f.Info, ProjectionFixture.Partial));
            Assert.Equal(new(1, 20), f.Store.Watermark);
        }
        else
        {
            Assert.Throws<ArgumentException>(() => writer.Commit(new([end], changes, f.Info, ProjectionFixture.Partial)));
            Assert.Equal(default, f.Store.Watermark);
        }
        Assert.Equal(4096, new ProjectionBudget(SessionWriter.MaximumBatchBytes).RemainingRecords);
    }

    [Fact]
    public void Pages_split_at_serialized_bytes_before_the_row_bound()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var text = new string('x', 50_000);
        var emitted = 0;
        IEnumerable<ProjectionChange> Output()
        {
            for (var i = 1; i <= 400; i++) { emitted++; yield return History(f, i, text); }
        }
        var prepared = new BaselinePagePreparer(f.Scope, default, [new OutputProcessor(Output)])
            .Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks);
        Assert.Equal(400, emitted);
        Assert.Equal(2, prepared.Pages.Count);
        Assert.All(prepared.Pages, page => Assert.InRange(
            page.Sum(c => 16L + JsonSerializer.SerializeToUtf8Bytes(c.Value, c.Value.GetType(), WireJson.Options).Length),
            1, SessionWriter.MaximumBatchBytes));
        var firstBytes = prepared.Pages[0].Sum(c => 16L + JsonSerializer.SerializeToUtf8Bytes(c.Value, c.Value.GetType(), WireJson.Options).Length);
        var nextBytes = 16L + JsonSerializer.SerializeToUtf8Bytes(prepared.Pages[1][0].Value,
            prepared.Pages[1][0].Value.GetType(), WireJson.Options).Length;
        Assert.True(firstBytes + nextBytes > SessionWriter.MaximumBatchBytes);
    }

    [Fact]
    public void Aggregate_64MiB_overflow_stops_at_first_over_budget_candidate_and_disposes_output()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var end = End(f, chunks);
        var emitted = 0;
        var disposed = false;
        var laterCalled = false;
        var text = new string('x', 50_000);
        long bytes = SessionWriter.PrepareBaselineHeader(end, f.Scope, default, f.Info, ProjectionFixture.Partial, null).Bytes;
        long before = bytes;
        IEnumerable<ProjectionChange> Output()
        {
            try
            {
                while (true)
                {
                    var row = History(f, ++emitted, text);
                    before = bytes;
                    bytes += 16L + JsonSerializer.SerializeToUtf8Bytes(row.Value, row.Value.GetType(), WireJson.Options).Length;
                    yield return row;
                }
            }
            finally { disposed = true; }
        }
        IEnumerable<ProjectionChange> Later() { laterCalled = true; yield break; }
        var preparer = new BaselinePagePreparer(f.Scope, default, [new OutputProcessor(Output), new OutputProcessor(Later)]);
        Assert.Throws<ProcessingLimitException>(() => preparer.Prepare(end, f.Info, ProjectionFixture.Partial, chunks));
        Assert.True(before <= SessionWriter.MaximumBaselineBytes);
        Assert.True(bytes > SessionWriter.MaximumBaselineBytes);
        Assert.True(disposed);
        Assert.False(laterCalled);
        Assert.Equal(default, f.Store.Watermark);
    }

    [Fact]
    public void Single_serialized_value_limit_is_64KiB_not_the_page_or_contract_limit()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var emitted = 0;
        IEnumerable<ProjectionChange> Output()
        {
            emitted++; yield return History(f, 1, new string('x', 65_536));
            emitted++; yield return History(f, 2);
        }
        Assert.Throws<ProcessingLimitException>(() =>
            new BaselinePagePreparer(f.Scope, default, [new OutputProcessor(Output)])
                .Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks));
        Assert.Equal(1, emitted);
        Assert.Equal(default, f.Store.Watermark);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void Cancellation_and_producer_failure_return_no_pages_do_not_publish_and_are_not_retryable(bool cancellation)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        using var stop = new CancellationTokenSource();
        var disposed = false;
        var emitted = 0;
        IEnumerable<ProjectionChange> Output()
        {
            try
            {
                for (var i = 1; i <= 4097; i++) { emitted++; yield return History(f, i); }
                if (!cancellation) throw new InvalidDataException("Controlled owner projection failure.");
                stop.Cancel();
                emitted++; yield return History(f, 4098);
            }
            finally { disposed = true; }
        }
        var preparer = new BaselinePagePreparer(f.Scope, default, [new OutputProcessor(Output)]);
        var error = Record.Exception(() => preparer.Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks,
            cancellationToken: stop.Token));
        if (cancellation) Assert.IsAssignableFrom<OperationCanceledException>(error);
        else Assert.IsType<InvalidDataException>(error);
        Assert.True(disposed);
        Assert.Equal(cancellation ? 4098 : 4097, emitted);
        Assert.Throws<InvalidOperationException>(() => preparer.Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks));
        using var reopened = f.Reopen();
        Assert.Equal(default, reopened.Watermark);
    }

    [Fact]
    public void Pre_cancelled_attempt_invokes_no_processor_and_is_terminal()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var called = false;
        IEnumerable<ProjectionChange> Output() { called = true; yield break; }
        using var stop = new CancellationTokenSource();
        stop.Cancel();
        var preparer = new BaselinePagePreparer(f.Scope, default, [new OutputProcessor(Output)]);
        Assert.ThrowsAny<OperationCanceledException>(() => preparer.Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial,
            chunks, cancellationToken: stop.Token));
        Assert.False(called);
        Assert.Throws<InvalidOperationException>(() => preparer.Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks));
    }

    [Theory]
    [InlineData("end")]
    [InlineData("provenance")]
    [InlineData("chunks")]
    [InlineData("source")]
    [InlineData("duplicate")]
    [InlineData("unsupported")]
    [InlineData("metric")]
    public void Invalid_inputs_or_output_cannot_produce_a_publishable_result(string invalid)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var end = invalid == "end" ? f.Source(20, 2, f.Boundary(chunks[0].CaptureId, CapturePhase.End)) : End(f, chunks);
        var provenance = invalid == "provenance" ? new SourceProvenance(f.Scope.Process, -1) : null;
        if (invalid == "chunks") chunks[0] = chunks[0] with { CaptureId = Guid.NewGuid() };
        IEnumerable<ProjectionChange> Output()
        {
            yield return invalid switch
            {
                "source" => History(f, 1) with { SourceOrdinal = 2 },
                "unsupported" => new(1, f.Info),
                "metric" => new(1, new MetricSample(20, MetricKind.ProcessPrivateWorkingSet,
                    new(ValueKind.Integer, ValueUnit.Bytes, 1, null, null, null, DataQuality.Complete),
                    new(20, 20), "fixture", "1")),
                _ => History(f, 1)
            };
            if (invalid == "duplicate") yield return History(f, 1);
        }
        Assert.ThrowsAny<ArgumentException>(() =>
            new BaselinePagePreparer(f.Scope, default, [new OutputProcessor(Output)])
                .Prepare(end, f.Info, ProjectionFixture.Partial, chunks, provenance));
        Assert.Equal(default, f.Store.Watermark);
    }

    [Theory]
    [InlineData("source-id")]
    [InlineData("clock-type")]
    public void Invalid_clock_metadata_with_valid_conversion_is_rejected_before_invoking_processors(string invalid)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var end = End(f, chunks);
        var clock = new SourceClockMetadata(invalid == "source-id" ? Guid.Empty : Guid.NewGuid(),
            invalid == "clock-type" ? 2 : 1, new(f.Info.QpcFrequency * 2, f.Info.QpcFrequency, 500));
        var provenance = new SourceProvenance(f.Scope.Process, 540, clock);
        Assert.Equal(end.Envelope.TimestampQpc, clock.Mapping.Convert(provenance.OriginalTimestampQpc));
        var called = false;
        IEnumerable<ProjectionChange> Output()
        {
            called = true;
            return ((IVerifiedBaselineProcessor)f.Processors[0]).PublishBaseline(end, chunks,
                new(f.Scope, new(1, 20), ProjectionFixture.Partial, new(SessionWriter.MaximumBaselineBytes)));
        }
        BaselineProjectionPages? prepared = null;
        var error = Record.Exception(() =>
            prepared = new BaselinePagePreparer(f.Scope, default, [new OutputProcessor(Output)])
                .Prepare(end, f.Info, ProjectionFixture.Partial, chunks, provenance));
        if (invalid == "source-id") Assert.IsType<ArgumentException>(error);
        else Assert.IsType<NotSupportedException>(error);
        Assert.False(called);
        Assert.Null(prepared);
        using var reopened = f.Reopen();
        Assert.Equal(default, reopened.Watermark);
        Assert.Null(reopened.ReadSourceProvenance(1));
        Assert.Empty(reopened.ReadSourceClocks());
    }

    private const string DefaultProcessors =
        "ObjectStateProcessor,FrameProcessor,XamlTreeProcessor,CompositionTreeProcessor,ResourceUsageProcessor";

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task All_five_prepare_real_End_revisions_atomically_and_preserve_historical_facts_on_reopen(bool invalidate)
    {
        using var f = new ProjectionFixture(DefaultProcessors);
        var writer = SeedFrame(f);
        var previous = f.Store.Watermark;
        var before = Assert.Single((await f.Store.Queries.GetFrames(new(f.Scope, new(0, 30), new(128)), default)).Items);
        var quality = invalidate ? ProjectionFixture.Partial : ChangedQuality;
        if (invalidate)
            foreach (var processor in f.Processors)
                processor.Invalidate(new([new(IngestionIssue.SequenceGap, 1,
                    new(IngestionIssue.SequenceGap, "Continuity changed before the real End."))]));
        var chunks = FullChunks(f);
        var end = f.Source(20, previous.IngestOrdinal + 1, f.Boundary(chunks[0].CaptureId, CapturePhase.End));
        var provenance = new SourceProvenance(f.Scope.Process, 540,
            new(Guid.NewGuid(), 1, new(f.Info.QpcFrequency * 2, f.Info.QpcFrequency, 500)));
        var prepared = new BaselinePagePreparer(f.Scope, previous, f.Processors)
            .Prepare(end, f.Info, quality, chunks, provenance);
        var changes = prepared.Pages.SelectMany(p => p).ToArray();
        Assert.All(changes, change => Assert.Equal(end.Envelope.IngestOrdinal, change.SourceOrdinal));
        var frame = Assert.Single(changes.Select(c => c.Value).OfType<FrameInfo>());
        Assert.Equal(before.Attempt, frame.Attempt);
        Assert.Equal(before.Range, frame.Range);
        Assert.Equal(before.Submission, frame.Submission);
        Assert.Equal(before.Changes.Select(c => (c.TimestampQpc, c.IngestOrdinal, c.Cause, c.Observation)),
            frame.Changes.Select(c => (c.TimestampQpc, c.IngestOrdinal, c.Cause, c.Observation)));
        Assert.All(frame.Changes, change =>
        {
            if (invalidate) Assert.Contains("invalidated", change.Quality.Reason);
            else Assert.Equal(MeasurementKind.Estimated, change.Quality.Kind);
        });
        var startup = Assert.Single(changes.Select(c => c.Value).OfType<StartupEntry>());
        Assert.StartsWith("Phase:", startup.Phase);
        Assert.Equal(new(11, 13), startup.Range);
        Assert.Null(startup.SubmittedFrameId);
        Assert.Contains(changes, c => c.Value is ObjectHistoryEntry);
        Assert.Contains(changes, c => c.Value is TreeEntry);
        Assert.Contains(changes, c => c.Value is CompositionTreeEntry);
        Assert.Contains(changes, c => c.Value is ResourceUsageEntry);
        Assert.Single(changes.Select(c => c.Value).OfType<CorrelationEntry>());
        Assert.DoesNotContain(changes, c => c.Value is MetricSample);
        Assert.Equal(changes.Length, prepared.RecordCount);
        var exact = SessionWriter.PrepareBaselineHeader(end, f.Scope, previous, f.Info, quality, provenance).Bytes +
            changes.Sum(c => 16L + JsonSerializer.SerializeToUtf8Bytes(c.Value, c.Value.GetType(), WireJson.Options).Length);
        Assert.Equal(exact, prepared.AccountedBytes);
        Assert.Throws<NotSupportedException>(() => ((IList<FrameChange>)frame.Changes).Clear());
        Assert.Equal(previous, f.Store.Watermark);
        Assert.Equal(before.Changes, Assert.Single((await f.Store.Queries.GetFrames(new(f.Scope, new(0, 30), new(128)), default)).Items).Changes);

        IEnumerable<IReadOnlyList<ProjectionChange>> Pages()
        {
            foreach (var page in prepared.Pages) yield return page;
            using var concurrent = f.Reopen();
            Assert.Equal(previous, concurrent.Watermark);
            Assert.Null(concurrent.ReadSourceProvenance(end.Envelope.IngestOrdinal));
            Assert.Equal(before.Changes.Select(c => c.Quality),
                Assert.Single((concurrent.Queries.GetFrames(new(f.Scope, new(0, 30), new(128)), default)
                    .AsTask().GetAwaiter().GetResult()).Items).Changes.Select(c => c.Quality));
        }
        writer.CommitBaseline(prepared.End, prepared.Info, prepared.Quality, Pages(), prepared.Provenance);
        using var reopened = f.Reopen();
        Assert.Equal(new(end.Envelope.IngestOrdinal, 20), reopened.Watermark);
        Assert.Equal(provenance, reopened.ReadSourceProvenance(end.Envelope.IngestOrdinal));
        Assert.Equal(provenance.Clock, Assert.Single(reopened.ReadSourceClocks()));
        var durable = Assert.Single((await reopened.Queries.GetFrames(new(f.Scope, new(0, 30), new(128)), default)).Items);
        Assert.Equal(frame.Range, durable.Range);
        Assert.Equal(frame.Changes, durable.Changes);
        Assert.Contains(startup, (await reopened.Queries.GetStartupTimeline(new(f.Scope, new(128)), default)).Items);
        Assert.Equal(2, (await reopened.Queries.GetXamlTree(new(f.Scope, 20, new(128)), default)).Items.Count);
        Assert.Single((await reopened.Queries.GetCompositionTree(new(f.Scope, 20, new(128)), default)).Items);
        Assert.Single((await reopened.Queries.GetResourceUsage(new(f.Scope, new(20, 20), new(128)), default)).Items);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void All_five_End_does_not_invent_Frame_facts_when_empty_or_quality_is_unchanged(bool seed)
    {
        using var f = new ProjectionFixture(DefaultProcessors);
        if (seed) SeedFrame(f);
        var chunks = Chunks(f, 1);
        var end = f.Source(20, f.Store.Watermark.IngestOrdinal + 1, f.Boundary(chunks[0].CaptureId, CapturePhase.End));
        var prepared = new BaselinePagePreparer(f.Scope, f.Store.Watermark, f.Processors)
            .Prepare(end, f.Info, ProjectionFixture.Partial, chunks);
        Assert.DoesNotContain(prepared.Pages.SelectMany(p => p), c => c.Value is FrameInfo or StartupEntry);
        Assert.Equal(3, prepared.RecordCount);
    }

    [Theory]
    [InlineData("exact")]
    [InlineData("records")]
    [InlineData("bytes")]
    public void Real_Frame_and_baseline_routes_share_exact_aggregate_bounds_and_failure_discards_the_attempt(string limit)
    {
        using var f = new ProjectionFixture(DefaultProcessors);
        SeedFrame(f);
        var chunks = FullChunks(f);
        var previous = f.Store.Watermark;
        var end = f.Source(20, previous.IngestOrdinal + 1, f.Boundary(chunks[0].CaptureId, CapturePhase.End));
        var referenceProcessors = new IEventProcessor[]
        {
            new ObjectStateProcessor(), new FrameProcessor(), new XamlTreeProcessor(),
            new CompositionTreeProcessor(), new ResourceUsageProcessor()
        };
        var sources = FrameSources(f);
        var context = new ProcessingContext(f.Scope, previous, ProjectionFixture.Partial, new(SessionWriter.MaximumBatchBytes));
        foreach (var processor in referenceProcessors) _ = processor.Process(sources, context).ToArray();
        var expected = new BaselinePagePreparer(f.Scope, previous, referenceProcessors)
            .Prepare(end, f.Info, ChangedQuality, chunks);
        var preparer = new BaselinePagePreparer(f.Scope, previous, f.Processors,
            expected.RecordCount - (limit == "records" ? 1 : 0), expected.AccountedBytes - (limit == "bytes" ? 1 : 0));
        if (limit == "exact")
        {
            var prepared = preparer.Prepare(end, f.Info, ChangedQuality, chunks);
            Assert.Equal(expected.RecordCount, prepared.RecordCount);
            Assert.Equal(expected.AccountedBytes, prepared.AccountedBytes);
        }
        else
            Assert.Throws<ProcessingLimitException>(() => preparer.Prepare(end, f.Info, ChangedQuality, chunks));
        Assert.Throws<InvalidOperationException>(() => preparer.Prepare(end, f.Info, ChangedQuality, chunks));
        using var reopened = f.Reopen();
        Assert.Equal(previous, reopened.Watermark);
        Assert.Null(reopened.ReadSourceProvenance(end.Envelope.IngestOrdinal));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Failure_or_cancellation_after_real_Frame_mutation_discards_owner_state_and_preserves_durable_sources(bool cancellation)
    {
        using var f = new ProjectionFixture(DefaultProcessors);
        SeedFrame(f);
        var previous = f.Store.Watermark;
        var chunks = FullChunks(f);
        var end = f.Source(20, previous.IngestOrdinal + 1, f.Boundary(chunks[0].CaptureId, CapturePhase.End));
        using var stop = new CancellationTokenSource();
        var processors = f.Processors.ToArray();
        var frame = processors[1];
        var disposed = false;
        IEnumerable<ProjectionChange> Interrupted(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
        {
            Assert.Equal(EventCodec.Encode(end.Envelope), EventCodec.Encode(Assert.Single(events).Envelope));
            try
            {
                foreach (var row in frame.Process(events, context))
                {
                    yield return row;
                    if (cancellation) stop.Cancel();
                    else throw new InvalidDataException("Controlled failure after retained Frame mutation.");
                }
            }
            finally { disposed = true; }
        }
        processors[1] = new OrdinaryOutputProcessor(Interrupted);
        var preparer = new BaselinePagePreparer(f.Scope, previous, processors);
        var error = Record.Exception(() =>
            preparer.Prepare(end, f.Info, ChangedQuality, chunks, cancellationToken: stop.Token));
        if (cancellation) Assert.IsAssignableFrom<OperationCanceledException>(error);
        else Assert.IsType<InvalidDataException>(error);
        Assert.True(disposed);
        Assert.Throws<InvalidOperationException>(() => preparer.Prepare(end, f.Info, ChangedQuality, chunks));
        using var reopened = f.Reopen();
        Assert.Equal(previous, reopened.Watermark);
        var durable = Assert.Single((await reopened.Queries.GetFrames(new(f.Scope, new(0, 30), new(128)), default)).Items);
        Assert.All(durable.Changes, c => Assert.Equal(MeasurementKind.Measured, c.Quality.Kind));
        Assert.Equal(new long[] { 2, 3, 4 }, durable.Changes.Select(c => c.IngestOrdinal));
        Assert.Null(reopened.ReadSourceProvenance(end.Envelope.IngestOrdinal));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Prepared_all_five_pages_roll_back_Frame_Startup_and_End_when_atomic_writer_fails_or_cancels(bool cancellation)
    {
        using var f = new ProjectionFixture(DefaultProcessors);
        var writer = SeedFrame(f);
        var previous = f.Store.Watermark;
        var chunks = FullChunks(f);
        var end = f.Source(20, previous.IngestOrdinal + 1, f.Boundary(chunks[0].CaptureId, CapturePhase.End));
        var provenance = new SourceProvenance(f.Scope.Process, 540,
            new(Guid.NewGuid(), 1, new(f.Info.QpcFrequency * 2, f.Info.QpcFrequency, 500)));
        var preparer = new BaselinePagePreparer(f.Scope, previous, f.Processors);
        var prepared = preparer.Prepare(end, f.Info, ChangedQuality, chunks, provenance);
        using var stop = new CancellationTokenSource();
        IEnumerable<IReadOnlyList<ProjectionChange>> Pages()
        {
            foreach (var page in prepared.Pages) yield return page;
            if (cancellation) stop.Cancel();
            else throw new InvalidDataException("Controlled atomic publication failure after all five were inserted.");
        }
        var error = Record.Exception(() =>
            writer.CommitBaseline(prepared.End, prepared.Info, prepared.Quality, Pages(), prepared.Provenance, stop.Token));
        if (cancellation) Assert.IsAssignableFrom<OperationCanceledException>(error);
        else Assert.IsType<InvalidDataException>(error);
        Assert.Throws<InvalidOperationException>(() => preparer.Prepare(end, f.Info, ChangedQuality, chunks));
        using var reopened = f.Reopen();
        Assert.Equal(previous, reopened.Watermark);
        Assert.All(Assert.Single((await reopened.Queries.GetFrames(new(f.Scope, new(0, 30), new(128)), default)).Items).Changes,
            c => Assert.Equal(MeasurementKind.Measured, c.Quality.Kind));
        Assert.All((await reopened.Queries.GetStartupTimeline(new(f.Scope, new(128)), default)).Items,
            row => Assert.NotEqual(MeasurementKind.Estimated, row.Quality.Kind));
        Assert.Empty((await reopened.Queries.GetXamlTree(new(f.Scope, previous.TimestampQpc, new(128)), default)).Items);
        Assert.Empty((await reopened.Queries.GetResourceUsage(new(f.Scope,
            new(previous.TimestampQpc, previous.TimestampQpc), new(128)), default)).Items);
        Assert.Null(reopened.ReadSourceProvenance(end.Envelope.IngestOrdinal));
        Assert.Empty(reopened.ReadSourceClocks());
    }

    [Theory]
    [InlineData("source")]
    [InlineData("duplicate")]
    [InlineData("metric")]
    [InlineData("unsupported")]
    [InlineData("oversized")]
    public void Ordinary_End_output_uses_the_same_source_key_type_and_value_checks(string invalid)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var end = End(f, chunks);
        IEnumerable<ProjectionChange> Output(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
        {
            Assert.Equal(EventCodec.Encode(end.Envelope), EventCodec.Encode(Assert.Single(events).Envelope));
            Assert.Equal(new(1, 20), context.BatchBoundary);
            yield return invalid switch
            {
                "source" => History(f, 1) with { SourceOrdinal = 2 },
                "metric" => new(1, new MetricSample(20, MetricKind.ProcessPrivateWorkingSet,
                    new(ValueKind.Integer, ValueUnit.Bytes, 1, null, null, null, DataQuality.Complete),
                    new(20, 20), "fixture", "1")),
                "unsupported" => new(1, f.Info),
                "oversized" => History(f, 1, new string('x', 65_536)),
                _ => History(f, 1)
            };
            if (invalid == "duplicate") yield return History(f, 1);
        }
        var preparer = new BaselinePagePreparer(f.Scope, default, [new OrdinaryOutputProcessor(Output)]);
        var error = Record.Exception(() => preparer.Prepare(end, f.Info, ProjectionFixture.Partial, chunks));
        if (invalid == "oversized") Assert.IsType<ProcessingLimitException>(error);
        else Assert.IsAssignableFrom<ArgumentException>(error);
        Assert.Equal(default, f.Store.Watermark);
    }

    [Fact]
    public void Sixth_processor_remains_rejected()
    {
        using var f = new ProjectionFixture(DefaultProcessors);
        Assert.Throws<ArgumentException>(() => new BaselinePagePreparer(f.Scope, default,
            [.. f.Processors, new MetricProcessor()]));
    }

    private sealed class OrdinaryOutputProcessor(
        Func<IReadOnlyList<NormalizedEvent>, ProcessingContext, IEnumerable<ProjectionChange>> output) : IEventProcessor
    {
        public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context) =>
            output(events, context);
        public void Invalidate(HealthSnapshot health) => throw new InvalidOperationException();
    }

    private static CaptureChunkPayload[] FullChunks(ProjectionFixture f)
    {
        var chunks = Chunks(f, 2);
        var visual = new ObjectKey(f.Scope.Process, IdentityDomain.CompositionVisual, f.Epoch, 1);
        var resource = new ObjectKey(f.Scope.Process, IdentityDomain.Resource, f.Epoch, 1);
        chunks[0] = chunks[0] with
        {
            Objects = [.. chunks[0].Objects, chunks[0].Objects[0] with { Key = visual, Classification = null }],
            Tree = [.. chunks[0].Tree, f.Edge(visual, relationship: RelationshipKind.VisualRoot,
                membership: TreeMembershipKind.Composition)],
            Visuals = [new(visual, VisualProperty.Opacity,
                new(ValueKind.Number, ValueUnit.Scalar, null, .5, null, null, DataQuality.Complete))],
            Resources = [new(resource, ResourceOperation.BaselinePresent, Guid.NewGuid(), Guid.NewGuid(), 4096,
                SizeProvenance.ActualAllocation, f.Key(1), 32, 32, ProjectionFixture.Partial,
                new(ResourceKind.Texture2D, ResourceKindEvidence.CreationApi, "Qualified resource baseline fixture"))],
            Correlations = [new(visual, resource, CorrelationKind.VisualResource, true)]
        };
        return chunks;
    }

    private static DataQuality ChangedQuality { get; } =
        new(MeasurementKind.Estimated, CoverageState.Partial, "Changed quality at verified End.");

    private static NormalizedEvent[] FrameSources(ProjectionFixture f)
    {
        var attempt = new AttemptReference(f.Epoch, 1, f.Root, 1);
        var cause = new ChangeCause(CauseKind.ExplicitAttempt, attempt, null);
        return
        [
            f.Source(10, 1, new FramePayload(attempt, null, FrameOutcome.Attempt, null)),
            f.Source(11, 2, new BoundaryPayload(BoundaryKind.LayoutMeasure, BoundaryPhase.Begin, 1, null, cause)),
            f.Source(12, 3, f.Property(f.Key(1)) with { Cause = cause }),
            f.Source(13, 4, new BoundaryPayload(BoundaryKind.LayoutMeasure, BoundaryPhase.End, 1, 0, cause)),
            f.Source(14, 5, new FramePayload(attempt, 1, FrameOutcome.Submitted, 0)),
            f.Source(15, 6, new FramePayload(attempt, null, FrameOutcome.AttemptCompleted, 0))
        ];
    }

    private static SessionWriter SeedFrame(ProjectionFixture f)
    {
        var sources = FrameSources(f);
        var context = new ProcessingContext(f.Scope, new(6, 15), ProjectionFixture.Partial, new(SessionWriter.MaximumBatchBytes));
        var changes = f.Processors.SelectMany(p => p.Process(sources, context)).ToArray();
        var writer = f.Store.CreateWriter();
        writer.Commit(new(sources, changes, f.Info, ProjectionFixture.Partial));
        return writer;
    }

    [Fact]
    public void Foreign_thread_cannot_invoke_owner_preparation()
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var chunks = Chunks(f, 1);
        var preparer = new BaselinePagePreparer(f.Scope, default, f.Processors);
        Exception? failure = null;
        var foreign = new Thread(() => failure = Record.Exception(() =>
            preparer.Prepare(End(f, chunks), f.Info, ProjectionFixture.Partial, chunks)));
        foreign.Start(); foreign.Join();
        Assert.IsType<InvalidOperationException>(failure);
    }

    private static ProjectionChange History(ProjectionFixture f, int id, string? typeName = null) =>
        new(1, new ObjectHistoryEntry(f.Key((ulong)id), 20, 1,
            new LifetimeObservation(LifetimeOperation.BaselinePresent, typeName, null),
            new(CauseKind.Unknown, null, "No fabricated birth."), ProjectionFixture.Partial));

    private sealed class OutputProcessor(Func<IEnumerable<ProjectionChange>> output) : IVerifiedBaselineProcessor
    {
        public IEnumerable<ProjectionChange> PublishBaseline(NormalizedEvent end, IReadOnlyList<CaptureChunkPayload> chunks,
            ProcessingContext context) => output();
        public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context) =>
            throw new InvalidOperationException("Ordinary processing is not page preparation.");
        public void Invalidate(HealthSnapshot health) => throw new InvalidOperationException();
    }

    internal static CaptureChunkPayload[] Chunks(ProjectionFixture f, int count)
    {
        var id = Guid.NewGuid();
        return Enumerable.Range(0, (count + 63) / 64).Select(index =>
        {
            var keys = Enumerable.Range(index * 64 + 1, Math.Min(64, count - index * 64))
                .Select(n => f.Key((ulong)n)).ToArray();
            return f.Chunk(id, keys[0]) with
            {
                ChunkIndex = (uint)index,
                Objects = keys.Select(key => f.Chunk(id, key).Objects[0]).ToArray(),
                Tree = keys.Select(key => f.Edge(key)).ToArray()
            };
        }).ToArray();
    }

    internal static NormalizedEvent End(ProjectionFixture f, IReadOnlyList<CaptureChunkPayload> chunks) =>
        f.Source(20, 1, f.Boundary(chunks[0].CaptureId, CapturePhase.End) with { ChunkCount = (uint)chunks.Count });
}
