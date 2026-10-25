using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class BaselineCoordinatorTests
{
    private static readonly Guid CaptureId = Guid.Parse("30000000-0000-0000-0000-000000000001");
    private static readonly Guid Root = Guid.Parse("40000000-0000-0000-0000-000000000001");
    private static readonly Guid Listener = Guid.Parse("50000000-0000-0000-0000-000000000001");
    private static readonly Guid Continuity = Guid.Parse("60000000-0000-0000-0000-000000000001");
    private static readonly ProducerStreamKey Stream = new(EventCatalog.RuntimeProviderId, CoreTests.Process.ProcessId, CoreTests.Epoch);
    private static readonly SequenceDomainKey Domain = new(Stream, CoreTests.Process, 2, EventCatalog.Version);
    private static readonly ProducerSequenceCut Cut = new(Guid.Parse("70000000-0000-0000-0000-000000000001"), Domain, 2, 2);

    [Fact]
    public void Complete_chunks_and_End_do_not_substitute_for_actual_delta_receipts()
    {
        var coordinator = Started();
        coordinator.StageChunk(Source(Chunk(), 2, 110));
        coordinator.StageEnd(Source(Boundary(CapturePhase.End), 3, 120, 2));
        var pending = coordinator.Prepare([Evidence()]);
        Assert.Equal(SequenceContinuityState.Unproven, pending.State);
        Assert.NotNull(pending.Reason);
        Assert.Null(pending.Capture);

        Observe(coordinator, Delta(2), 4, 115, 2);
        Assert.Null(coordinator.Prepare([Evidence()]).Capture);
        Observe(coordinator, Delta(1), 5, 105, 1);
        var ready = coordinator.Prepare([Evidence()]);
        Assert.Equal(SequenceContinuityState.Contiguous, ready.State);
        Assert.Null(ready.Reason);
        var capture = Assert.IsType<PreparedBaseline>(ready.Capture);
        Assert.Single(capture.Chunks);
        Assert.Equal(new long[] { 4, 5 }, capture.BufferedDeltas.Select(e => e.Envelope.IngestOrdinal));
        Assert.Equal(3, capture.End.Envelope.IngestOrdinal);
    }

    [Fact]
    public void End_may_arrive_before_its_chunks_without_publishing_an_empty_success()
    {
        var coordinator = Started(0);
        coordinator.StageEnd(Source(Boundary(CapturePhase.End, through: 0), 2, 120, 2));
        Assert.Null(coordinator.Prepare([Evidence(through: 0)]).Capture);
        coordinator.StageChunk(Source(Chunk(), 3, 110));
        Assert.NotNull(coordinator.Prepare([Evidence(through: 0)]).Capture);
    }

    [Theory]
    [InlineData("capture")]
    [InlineData("root")]
    [InlineData("dispatcher")]
    [InlineData("duplicate")]
    [InlineData("index")]
    public void Malformed_chunk_identity_or_order_poison_the_capture(string problem)
    {
        var coordinator = Started(0);
        var chunk = problem switch
        {
            "capture" => Chunk() with { CaptureId = Guid.NewGuid() },
            "root" => Chunk() with { RootId = Guid.NewGuid() },
            "dispatcher" => Chunk() with { DispatcherId = 2 },
            "index" => Chunk() with { ChunkIndex = 4096 },
            _ => Chunk()
        };
        if (problem == "duplicate") coordinator.StageChunk(Source(chunk, 2, 110));
        Assert.Throws<InvalidOperationException>(() => coordinator.StageChunk(Source(chunk, 3, 111, 2)));
        Assert.Throws<InvalidOperationException>(() => coordinator.StageEnd(Source(Boundary(CapturePhase.End, through: 0), 4, 120, 2)));
    }

    [Theory]
    [InlineData("before")]
    [InlineData("domain")]
    [InlineData("root")]
    [InlineData("time")]
    public void End_must_match_the_actual_Begin_vector_scope_and_time(string problem)
    {
        var coordinator = Started();
        var end = Boundary(CapturePhase.End);
        end = problem switch
        {
            "before" => end with { Domains = [end.Domains[0] with { BeforeSequence = 1 }] },
            "domain" => end with { Domains = [end.Domains[0] with
            {
                Domain = Domain with { EventId = 3 },
                SettledCut = Cut with { Domain = Domain with { EventId = 3 } }
            }] },
            "root" => end with { RootId = Guid.NewGuid() },
            _ => end
        };
        Assert.Throws<InvalidOperationException>(() => coordinator.StageEnd(Source(end, 2, problem == "time" ? 99 : 120, 2)));
    }

    [Fact]
    public void Receipt_progress_is_processing_not_queued_or_callback_delivery()
    {
        var coordinator = Completed();
        Observe(coordinator, Delta(1), 4, 105, 1);
        Observe(coordinator, Delta(2), 5, 115, 2);
        Assert.Null(coordinator.Prepare([Evidence(progress: 4)]).Capture);
        Assert.NotNull(coordinator.Prepare([Evidence(progress: 5)]).Capture);
    }

    [Fact]
    public void Exact_duplicate_delta_is_idempotent_but_conflicting_sequence_is_fatal()
    {
        var coordinator = Completed();
        Observe(coordinator, Delta(1), 4, 105, 1);
        Observe(coordinator, Delta(1), 5, 105, 1);
        Observe(coordinator, Delta(2), 6, 115, 2);
        Assert.Equal(2, coordinator.Prepare([Evidence()]).Capture!.BufferedDeltas.Count);

        var conflict = Completed();
        Observe(conflict, Delta(1), 4, 105, 1);
        Assert.Throws<InvalidOperationException>(() => Observe(conflict, Delta(2), 5, 105, 1));
    }

    [Fact]
    public void Deltas_delivered_before_Begin_and_after_End_cut_keep_their_real_sources()
    {
        var coordinator = new BaselineCoordinator(Source(Boundary(CapturePhase.Begin), 2, 100));
        coordinator.StageChunk(Source(Chunk(), 3, 110));
        coordinator.StageEnd(Source(Boundary(CapturePhase.End), 4, 120, 2));
        Observe(coordinator, Delta(1), 1, 99, 1);
        Observe(coordinator, Delta(2), 5, 115, 2);
        Observe(coordinator, Delta(3), 6, 125, 3);
        var prepared = coordinator.Prepare([Evidence()]).Capture!;
        Assert.Equal(new long[] { 1, 5, 6 }, prepared.BufferedDeltas.Select(e => e.Envelope.IngestOrdinal));
        Assert.Equal(new long[] { 99, 115, 125 }, prepared.BufferedDeltas.Select(e => e.Envelope.TimestampQpc));
        Assert.Equal(new ulong?[] { 1, 2, 3 }, prepared.BufferedDeltas.Select(e => e.Envelope.SourceSequence));
    }

    [Theory]
    [InlineData("listener")]
    [InlineData("continuity")]
    [InlineData("timestamp")]
    [InlineData("ordinal")]
    [InlineData("sequence")]
    public void Receipt_cannot_be_detached_from_its_actual_source_or_listener(string problem)
    {
        var coordinator = Completed();
        var source = Source(Delta(1), 4, 105, 1);
        var checkpoint = Checkpoint(source) with
        {
            TimestampQpc = problem == "timestamp" ? 106 : 105,
            IngestOrdinal = problem == "ordinal" ? 5 : 4,
            Sequence = problem == "sequence" ? 2UL : 1UL,
            ListenerId = problem == "listener" ? Guid.NewGuid() : Listener,
            ContinuityId = problem == "continuity" ? Guid.NewGuid() : Continuity
        };
        if (problem is "listener" or "continuity")
        {
            coordinator.Observe(source, checkpoint);
            Observe(coordinator, Delta(2), 5, 115, 2);
            Assert.Null(coordinator.Prepare([Evidence()]).Capture);
        }
        else Assert.Throws<InvalidOperationException>(() => coordinator.Observe(source, checkpoint));
    }

    [Fact]
    public void Declared_loss_is_terminal_even_if_every_sequence_later_arrives()
    {
        var coordinator = Completed();
        var loss = new LossPayload(CoreTests.Epoch, new(100, 120), null, "Producer selection gap.", [Domain]);
        var source = Source(loss, 4, 121);
        Assert.Throws<InvalidOperationException>(() => coordinator.Observe(source, Checkpoint(source)));
        Assert.Throws<InvalidOperationException>(() => coordinator.Prepare([Evidence()]));
    }

    [Fact]
    public void Closed_source_proof_reports_a_real_gap_not_a_successful_empty_baseline()
    {
        var coordinator = Completed();
        Observe(coordinator, Delta(1), 4, 105, 1);
        var sealedSource = new SnapshotArtifact("recording.etl", new string('a', 64));
        var closed = new SequenceDeliveryClosure(Listener, Continuity, Cut.CutId, sealedSource, 100, 100, 4, 4, 0, 2);
        var result = coordinator.Prepare([Evidence() with { Receipt = null, Delivery = closed }]);
        Assert.Equal(SequenceContinuityState.GapDetected, result.State);
        Assert.Null(result.Capture);
        Assert.NotNull(result.Reason);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void Chunk_source_timestamps_must_lie_within_the_real_capture_extent(bool endFirst)
    {
        var coordinator = Started(0);
        if (endFirst)
        {
            coordinator.StageEnd(Source(Boundary(CapturePhase.End, through: 0), 2, 120, 2));
            Assert.Throws<InvalidOperationException>(() => coordinator.StageChunk(Source(Chunk(), 3, 121)));
        }
        else
        {
            coordinator.StageChunk(Source(Chunk(), 2, 121));
            Assert.Throws<InvalidOperationException>(() => coordinator.StageEnd(Source(Boundary(CapturePhase.End, through: 0), 3, 120, 2)));
        }
    }

    [Fact]
    public void Chunk_reordering_preserves_indices_and_missing_indices_remain_pending()
    {
        var coordinator = Started(0);
        coordinator.StageChunk(Source(Chunk() with { ChunkIndex = 2 }, 2, 112, 3));
        coordinator.StageEnd(Source(Boundary(CapturePhase.End, through: 0) with { ChunkCount = 3 }, 3, 120, 2));
        coordinator.StageChunk(Source(Chunk(), 4, 110, 1));
        Assert.Null(coordinator.Prepare([Evidence(through: 0)]).Capture);
        coordinator.StageChunk(Source(Chunk() with { ChunkIndex = 1 }, 5, 111, 2));
        Assert.Equal(new uint[] { 0, 1, 2 }, coordinator.Prepare([Evidence(through: 0)]).Capture!.Chunks.Select(c => c.ChunkIndex));
    }

    [Fact]
    public void Duplicate_snapshot_objects_are_rejected_before_any_prepared_baseline_escapes()
    {
        var coordinator = Started(0);
        var item = new SnapshotObject(CoreTests.Key(), null, null,
            new(ValueKind.Integer, ValueUnit.Bytes, 0, null, null, null, DataQuality.Complete));
        coordinator.StageChunk(Source(Chunk() with { Objects = [item, item] }, 2, 110));
        coordinator.StageEnd(Source(Boundary(CapturePhase.End, through: 0), 3, 120, 2));
        Assert.Throws<InvalidOperationException>(() => coordinator.Prepare([Evidence(through: 0)]));
    }

    [Fact]
    public void Source_retention_limit_fails_explicitly_and_cannot_later_prepare_success()
    {
        var coordinator = Completed();
        Exception? failure = null;
        for (ulong i = 1; i <= 8192 && failure is null; i++)
            failure = Record.Exception(() => Observe(coordinator, Delta(i), (long)i + 3, 115, i));
        Assert.NotNull(failure);
        Assert.Equal("ProcessingLimitException", failure.GetType().Name);
        Assert.Throws<InvalidOperationException>(() => coordinator.Prepare([Evidence()]));
    }

    [Fact]
    public void Different_chunk_indices_cannot_reuse_one_producer_sequence()
    {
        var coordinator = Started(0);
        coordinator.StageChunk(Source(Chunk(), 2, 110));
        Assert.Throws<InvalidOperationException>(() =>
            coordinator.StageChunk(Source(Chunk() with { ChunkIndex = 1 }, 3, 111)));
    }

    [Fact]
    public void Staged_metadata_needs_real_receipt_when_its_domain_is_explicitly_selected()
    {
        var domain = Domain with { EventId = 13 };
        var cut = Cut with { Domain = domain, ReservedThrough = 1, SettledThrough = 1 };
        var start = Source(Boundary(CapturePhase.Begin) with
        {
            Domains = [new(domain, 0, null, DataQuality.Complete)]
        }, 1, 100);
        var coordinator = new BaselineCoordinator(start);
        coordinator.StageChunk(Source(Chunk(), 2, 110));
        coordinator.StageEnd(Source(Boundary(CapturePhase.End) with
        {
            Domains = [new(domain, 0, 1, DataQuality.Complete, cut)]
        }, 3, 120, 2));
        var proof = Evidence(through: 1);
        proof = proof with { Interval = proof.Interval with { Domain = domain } };
        Assert.Null(coordinator.Prepare([proof]).Capture);
        coordinator.Observe(start, Checkpoint(start));
        Assert.Empty(coordinator.Prepare([proof]).Capture!.BufferedDeltas);
    }

    [Fact]
    public void Deferred_metadata_does_not_reserve_a_canonical_processing_ordinal()
    {
        var coordinator = Completed();
        Observe(coordinator, Delta(1), 3, 105, 1);
        Observe(coordinator, Delta(2), 4, 115, 2);
        var prepared = coordinator.Prepare([Evidence()]).Capture!;
        Assert.Equal(3, prepared.End.Envelope.IngestOrdinal);
        Assert.Equal(new long[] { 3, 4 }, prepared.BufferedDeltas.Select(e => e.Envelope.IngestOrdinal));
    }

    private static BaselineCoordinator Started(ulong through = 2) =>
        new(Source(Boundary(CapturePhase.Begin, through: through), 1, 100));
    private static BaselineCoordinator Completed()
    {
        var coordinator = Started();
        coordinator.StageChunk(Source(Chunk(), 2, 110));
        coordinator.StageEnd(Source(Boundary(CapturePhase.End), 3, 120, 2));
        return coordinator;
    }
    private static CaptureBoundaryPayload Boundary(CapturePhase phase, ulong through = 2) =>
        new(CaptureId, CaptureKind.Baseline, phase, Root, 1,
            [new(Domain, 0, phase == CapturePhase.End ? through : null, DataQuality.Complete,
                phase == CapturePhase.End ? Cut with { ReservedThrough = through, SettledThrough = through } : null)],
            phase == CapturePhase.End ? 1U : null, DataQuality.Complete);
    private static CaptureChunkPayload Chunk() =>
        new(CaptureId, 0, Root, 1, [], [], [], [], [], [], [], [], []);
    private static ObjectLifetimePayload Delta(ulong id) => new(CoreTests.Key(id), LifetimeOperation.Birth, null, null, DataQuality.Complete);
    private static BaselineDomainEvidence Evidence(ulong through = 2, long progress = 100) =>
        new(new(Listener, Continuity, Domain, new(90, 130), DataQuality.Complete),
            new(Listener, Continuity, Cut.CutId, 0, through, progress));
    private static NormalizedEvent Source(IEventPayload payload, long ordinal, long timestamp, ulong sequence = 1)
    {
        var definition = EventCatalog.All.Single(d => d.Identity.ProviderId == EventCatalog.RuntimeProviderId && d.PayloadType == payload.GetType());
        return EventCodec.Normalize(new(CoreTests.Session, CoreTests.Process, CoreTests.Process.ProcessId, 1, timestamp, ordinal,
            sequence, null, definition.Identity, EventCodec.EncodePayload(payload, CoreTests.Process)), Stream);
    }
    private static SequenceCheckpoint Checkpoint(NormalizedEvent source) =>
        new(Listener, Continuity, source.SequenceDomain, source.Envelope.SourceSequence!.Value,
            source.Envelope.TimestampQpc, source.Envelope.IngestOrdinal);
    private static void Observe(BaselineCoordinator coordinator, IEventPayload payload, long ordinal, long timestamp, ulong sequence)
    {
        var source = Source(payload, ordinal, timestamp, sequence);
        coordinator.Observe(source, Checkpoint(source));
    }
}
