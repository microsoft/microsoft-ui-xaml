using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class LiveSequenceReceiptTests
{
    private static readonly ProducerSequenceCut Cut = SequenceClosureTests.Cut with { ReservedThrough = 8 };
    private static readonly SequenceReceiptEvidence Receipt = new(SequenceClosureTests.Interval.ListenerId,
        SequenceClosureTests.Interval.ContinuityId, Cut.CutId, 0, 7, 100);

    private static SequenceGapAssessment Assess(SequenceObservationWindow window, SequenceReceiptEvidence? receipt = null) =>
        SequenceContinuity.Assess(window, SequenceClosureTests.Interval, Cut, receipt: receipt ?? Receipt);

    [Fact]
    public void G1_Reordered_decoded_live_prefix_completes_without_sealing_or_waiting_for_later_reservations()
    {
        var receipt = CorrectionContractTests.RoundTrip(Receipt);
        var result = Assess(SequenceClosureTests.Window(4, 7, 5, 6), receipt);
        Assert.Equal(SequenceContinuityState.Contiguous, result.State);
        Assert.Equal(0UL, result.MissingObservations);
        Assert.True(Cut.ReservedThrough > Cut.SettledThrough);
    }

    [Fact]
    public void G1_Missing_live_ID_is_pending_until_it_arrives_not_a_loss_count()
    {
        var pending = Assess(SequenceClosureTests.Window(4, 7, 5));
        Assert.Equal(SequenceContinuityState.Unproven, pending.State);
        Assert.Null(pending.MissingObservations);
        var received = Assess(SequenceClosureTests.Window(4, 7, 5, 6));
        Assert.Equal(SequenceContinuityState.Contiguous, received.State);
    }

    [Fact]
    public void G1_Duplicate_does_not_replace_a_missing_live_ID()
    {
        var pending = Assess(SequenceClosureTests.Window(4, 7, 5, 5));
        Assert.Equal(SequenceContinuityState.Unproven, pending.State);
        Assert.Null(pending.MissingObservations);
        Assert.Equal(SequenceContinuityState.Contiguous, Assess(SequenceClosureTests.Window(4, 7, 5, 5, 6)).State);
    }

    [Fact]
    public void G1_Callback_or_queued_processing_is_not_completed_receipt()
    {
        var window = SequenceClosureTests.Window(4, 7, 5, 6);
        var pending = Assess(window, Receipt with { ProcessedThroughOrdinal = 3 });
        Assert.Equal(SequenceContinuityState.Unproven, pending.State);
        Assert.Null(pending.MissingObservations);
        Assert.Equal(SequenceContinuityState.Contiguous, Assess(window, Receipt with { ProcessedThroughOrdinal = 4 }).State);
    }

    [Fact]
    public void G1_An_unprocessed_duplicate_does_not_block_an_already_processed_distinct_prefix()
    {
        Assert.Equal(SequenceContinuityState.Contiguous,
            Assess(SequenceClosureTests.Window(4, 7, 5, 6, 7), Receipt with { ProcessedThroughOrdinal = 4 }).State);
    }

    [Theory]
    [InlineData(0UL, false)]
    [InlineData(4UL, false)]
    [InlineData(0UL, true)]
    [InlineData(4UL, true)]
    public void G1_Honest_empty_relevant_spans_need_no_archive_or_invented_processed_record(ulong point, bool zeroDuration)
    {
        var window = new SequenceObservationWindow(point, point, []);
        var receipt = Receipt with { EnabledAfterSequence = point, EnabledThroughSequence = point, ProcessedThroughOrdinal = 0 };
        var interval = zeroDuration ? SequenceClosureTests.Interval with { Interval = new(0, 0) } : SequenceClosureTests.Interval;
        var cut = Cut with { SettledThrough = point };
        var result = SequenceContinuity.Assess(window, interval, cut, receipt: receipt);
        Assert.Equal(SequenceContinuityState.Contiguous, result.State);
        Assert.Equal(0UL, result.MissingObservations);
    }

    [Theory]
    [InlineData("listener")]
    [InlineData("continuity")]
    [InlineData("cut")]
    [InlineData("inverted-eligibility")]
    [InlineData("negative-progress")]
    public void G1_Live_evidence_is_validated_before_positive_completeness(string invalid)
    {
        var receipt = invalid switch
        {
            "listener" => Receipt with { ListenerId = CoreTests.Epoch },
            "continuity" => Receipt with { ContinuityId = CoreTests.Session },
            "cut" => Receipt with { SettledCutId = CoreTests.Epoch },
            "inverted-eligibility" => Receipt with { EnabledAfterSequence = 8 },
            _ => Receipt with { ProcessedThroughOrdinal = -1 }
        };
        Assert.ThrowsAny<ArgumentException>(() => Assess(SequenceClosureTests.Window(4, 7, 5, 6), receipt));
    }

    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public void G1_Complete_received_set_does_not_extend_the_enabled_sequence_span(bool lower)
    {
        var receipt = lower ? Receipt with { EnabledAfterSequence = 5 } : Receipt with { EnabledThroughSequence = 6 };
        Assert.Equal(SequenceContinuityState.Unproven, Assess(SequenceClosureTests.Window(4, 7, 5, 6), receipt).State);
    }

    [Fact]
    public void G1_Complete_live_set_still_needs_a_covering_settled_cut_and_receipt_evidence()
    {
        var window = SequenceClosureTests.Window(4, 7, 5, 6);
        Assert.Equal(SequenceContinuityState.Unproven, SequenceContinuity.Assess(window, SequenceClosureTests.Interval, Cut).State);
        Assert.Equal(SequenceContinuityState.Unproven, SequenceContinuity.Assess(window, SequenceClosureTests.Interval, receipt: Receipt).State);
        Assert.Equal(SequenceContinuityState.Unproven,
            SequenceContinuity.Assess(window, SequenceClosureTests.Interval, Cut with { SettledThrough = 6 }, receipt: Receipt).State);
    }

    [Fact]
    public void G1_Live_positive_path_preserves_domain_continuity_quality_and_timestamp_checks()
    {
        var window = SequenceClosureTests.Window(4, 7, 5, 6);
        var foreign = window with { Observations = [SequenceClosureTests.Check(7, 1) with { Domain = SequenceClosureTests.Domain with { EventId = 9 } }] };
        Assert.Throws<ArgumentException>(() => Assess(foreign));
        var oldSegment = window with { Observations = window.Observations.Select(o => o with { ContinuityId = CoreTests.Session }).ToArray() };
        Assert.Equal(SequenceContinuityState.Unproven, Assess(oldSegment).State);
        var outside = window with { Observations = window.Observations.Select(o => o with { TimestampQpc = 1000 }).ToArray() };
        Assert.Equal(SequenceContinuityState.Unproven, Assess(outside).State);
        var partial = SequenceClosureTests.Interval with { Quality = new(MeasurementKind.Measured, CoverageState.Partial, "eligibility history unknown") };
        Assert.Equal(SequenceContinuityState.Unproven, SequenceContinuity.Assess(window, partial, Cut, receipt: Receipt).State);
    }

    [Fact]
    public void G1_Closed_source_still_proves_genuine_absence_and_missing_tails()
    {
        Assert.Equal(2UL, SequenceContinuity.Assess(SequenceClosureTests.Window(4, 7),
            SequenceClosureTests.Interval, Cut, SequenceClosureTests.Closed).MissingObservations);
        Assert.Equal(3UL, SequenceContinuity.Assess(SequenceClosureTests.Window(4),
            SequenceClosureTests.Interval, Cut, SequenceClosureTests.Closed).MissingObservations);
    }

    [Fact]
    public void G1_Validated_rundown_chunks_and_all_received_settled_domains_can_reconcile_while_recording_continues()
    {
        var events = SyntheticEventBuilder.BuildMinimal();
        var endEnvelope = events.Single(e => e.Identity.Name == "CaptureBoundary" &&
            EventCodec.ReadPayload<CaptureBoundaryPayload>(e).Phase == CapturePhase.End);
        var end = EventCodec.ReadPayload<CaptureBoundaryPayload>(endEnvelope);
        var prefix = events.Where(e => e.IngestOrdinal <= endEnvelope.IngestOrdinal).ToArray();
        var chunks = prefix.Where(e => e.Identity.Name == "CaptureChunk").Select(EventCodec.ReadPayload<CaptureChunkPayload>).ToArray();
        Assert.Equal(end.ChunkCount, (uint)chunks.Length);
        Assert.All(chunks, chunk => Assert.True(chunk.CaptureId == end.CaptureId && chunk.RootId == end.RootId && chunk.DispatcherId == end.DispatcherId));
        Assert.Contains(events, e => e.IngestOrdinal > endEnvelope.IngestOrdinal);
        foreach (var range in end.Domains)
        {
            var cut = range.SettledCut!;
            Assert.Equal(CoreTests.Epoch, cut.Domain.Stream.ProducerEpoch);
            var after = range.BeforeSequence!.Value;
            var through = range.ThroughSequence!.Value;
            var interval = SequenceClosureTests.Interval with { Domain = range.Domain, Interval = new(0, endEnvelope.TimestampQpc + 1) };
            var observed = prefix.Where(e => e.Identity.ProviderId == range.Domain.Stream.ProviderId &&
                e.Identity.EventId == range.Domain.EventId && e.SourceSequence > after && e.SourceSequence <= through)
                .Reverse().Select(e => new SequenceCheckpoint(interval.ListenerId, interval.ContinuityId, range.Domain,
                    e.SourceSequence!.Value, e.TimestampQpc, e.IngestOrdinal)).ToArray();
            var receipt = new SequenceReceiptEvidence(interval.ListenerId, interval.ContinuityId, cut.CutId,
                after, through, endEnvelope.IngestOrdinal);
            Assert.Equal(SequenceContinuityState.Contiguous,
                SequenceContinuity.Assess(new(after, through, observed), interval, cut, receipt: receipt).State);
        }
    }
}
