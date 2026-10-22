using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class SequenceClosureTests
{
    internal static readonly SequenceDomainKey Domain = new(new(EventCatalog.RuntimeProviderId, 42, CoreTests.Epoch), CoreTests.Process, 2, 1);
    internal static readonly ListenerDomainInterval Interval = new(CoreTests.Session, CoreTests.Epoch, Domain, new(0, 1000), DataQuality.Complete);
    internal static readonly ProducerSequenceCut Cut = new(CoreTests.Session, Domain, 7, 7);
    internal static readonly SequenceDeliveryClosure Closed = new(Interval.ListenerId, Interval.ContinuityId, Cut.CutId,
        new("synthetic-source.json", new string('a', 64)), 100, 100, 100, 100, 0, 7);
    internal static SequenceCheckpoint Check(ulong sequence, long ordinal) =>
        new(Interval.ListenerId, Interval.ContinuityId, Domain, sequence, (long)sequence * 10, ordinal);
    internal static SequenceObservationWindow Window(params ulong[] arrivals) =>
        new(4, 7, arrivals.Select((sequence, index) => Check(sequence, index + 1)).ToArray());

    [Fact]
    public void F1_Arrivals_4_7_5_6_stay_provisional_then_close_without_loss()
    {
        var arrivals = new ulong[] { 4, 7, 5, 6 };
        for (var count = 2; count <= arrivals.Length; count++)
        {
            var provisional = SequenceContinuity.Assess(Window(arrivals[..count]), Interval);
            Assert.Equal(SequenceContinuityState.Unproven, provisional.State);
            Assert.Null(provisional.MissingObservations);
        }
        var final = SequenceContinuity.Assess(Window(arrivals), Interval, Cut, Closed);
        Assert.Equal(SequenceContinuityState.Contiguous, final.State);
        Assert.Equal(0UL, final.MissingObservations);
    }

    [Fact]
    public void F1_An_outstanding_lower_reservation_prevents_closure_even_after_source_enumeration()
    {
        var result = SequenceContinuity.Assess(Window(4, 7), Interval, Cut with { SettledThrough = 4 }, Closed);
        Assert.Equal(SequenceContinuityState.Unproven, result.State);
        Assert.Null(result.MissingObservations);
    }

    [Theory]
    [InlineData("no-cut")]
    [InlineData("no-delivery")]
    [InlineData("enumerating")]
    [InlineData("unknown-final-ordinal")]
    [InlineData("decoding")]
    public void F1_Absent_or_incomplete_closure_cannot_finalize_missing_observations(string incomplete)
    {
        var cut = incomplete == "no-cut" ? null : Cut;
        var delivery = incomplete switch
        {
            "no-delivery" => null,
            "enumerating" => Closed with { EnumeratedByteLength = 90 },
            "unknown-final-ordinal" => Closed with { FinalIngestOrdinal = null },
            "decoding" => Closed with { DecodedThroughOrdinal = 50 },
            _ => Closed
        };
        var result = SequenceContinuity.Assess(Window(4, 7), Interval, cut, delivery);
        Assert.Equal(SequenceContinuityState.Unproven, result.State);
        Assert.Null(result.MissingObservations);
    }

    [Fact]
    public void F1_Genuinely_missing_observations_are_final_after_settlement_and_closed_delivery()
    {
        var result = SequenceContinuity.Assess(Window(4, 7), Interval, Cut, Closed);
        Assert.Equal(SequenceContinuityState.GapDetected, result.State);
        Assert.Equal(2UL, result.MissingObservations);
        var emptySuffix = SequenceContinuity.Assess(Window(4), Interval, Cut, Closed);
        Assert.Equal(3UL, emptySuffix.MissingObservations);
    }

    [Theory]
    [InlineData("listener")]
    [InlineData("continuity")]
    [InlineData("cut-id")]
    [InlineData("cut-domain")]
    [InlineData("cut-counters")]
    [InlineData("byte-count")]
    [InlineData("ordinal")]
    [InlineData("artifact")]
    public void F1_Closure_evidence_is_validated_and_bound_to_its_source_and_listener(string invalid)
    {
        var cut = invalid switch
        {
            "cut-domain" => Cut with { Domain = Domain with { EventId = 9 } },
            "cut-counters" => Cut with { SettledThrough = 8 },
            _ => Cut
        };
        var delivery = invalid switch
        {
            "listener" => Closed with { ListenerId = CoreTests.Epoch },
            "continuity" => Closed with { ContinuityId = CoreTests.Session },
            "cut-id" => Closed with { FinalizedAfterCutId = CoreTests.Epoch },
            "byte-count" => Closed with { EnumeratedByteLength = 101 },
            "ordinal" => Closed with { DecodedThroughOrdinal = 101 },
            "artifact" => Closed with { SealedSource = new("..\\escape", "not-a-digest") },
            _ => Closed
        };
        Assert.ThrowsAny<ArgumentException>(() => SequenceContinuity.Assess(Window(4, 7), Interval, cut, delivery));
    }

    [Theory]
    [InlineData("outstanding")]
    [InlineData("absent")]
    [InlineData("foreign")]
    public void F1_Complete_capture_vectors_require_an_appropriate_settled_cut(string invalid)
    {
        var cut = invalid switch
        {
            "outstanding" => Cut with { SettledThrough = 4 },
            "absent" => null,
            _ => Cut with { Domain = Domain with { EventId = 9 } }
        };
        var range = new SequenceDomainRange(Domain, 4, 7, DataQuality.Complete, cut);
        var end = new CaptureBoundaryPayload(CoreTests.Session, CaptureKind.Baseline, CapturePhase.End, CoreTests.Session, 1,
            [range], 1, DataQuality.Complete);
        Assert.ThrowsAny<ArgumentException>(() => CorrectionContractTests.Normalize(end));
    }

    [Fact]
    public void F1_Partial_End_with_outstanding_reservations_roundtrips_but_is_not_delivery_closure()
    {
        var partial = new DataQuality(MeasurementKind.Measured, CoverageState.Partial, "reservations remain in flight");
        var range = new SequenceDomainRange(Domain, 4, 7, partial, Cut with { SettledThrough = 4 });
        var end = new CaptureBoundaryPayload(CoreTests.Session, CaptureKind.Baseline, CapturePhase.End, CoreTests.Session, 1,
            [range], 1, partial);
        var read = EventCodec.ReadPayload<CaptureBoundaryPayload>(CorrectionContractTests.Normalize(end));
        Assert.Equal(4UL, read.Domains[0].SettledCut!.SettledThrough);
        var result = SequenceContinuity.Assess(Window(4, 7), Interval, read.Domains[0].SettledCut);
        Assert.Equal(SequenceContinuityState.Unproven, result.State);
    }

    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public void F1_Closed_source_does_not_extend_the_listener_enabled_sequence_span(bool beforeEnable)
    {
        var delivery = beforeEnable ? Closed with { EnabledAfterSequence = 5 } : Closed with { EnabledThroughSequence = 6 };
        var result = SequenceContinuity.Assess(Window(4, 7), Interval, Cut, delivery);
        Assert.Equal(SequenceContinuityState.Unproven, result.State);
        Assert.Null(result.MissingObservations);
    }

    [Fact]
    public void F1_Enabled_sequence_span_cannot_be_inverted()
    {
        Assert.Throws<ArgumentException>(() => SequenceContinuity.Assess(Window(4, 7), Interval, Cut,
            Closed with { EnabledAfterSequence = 8, EnabledThroughSequence = 7 }));
    }

    [Fact]
    public void F1_Complete_producer_End_preserves_the_cut_but_not_listener_delivery_proof()
    {
        var range = new SequenceDomainRange(Domain, 4, 7, DataQuality.Complete, Cut);
        var end = new CaptureBoundaryPayload(CoreTests.Session, CaptureKind.Baseline, CapturePhase.End, CoreTests.Session, 1,
            [range], 1, DataQuality.Complete);
        var read = EventCodec.ReadPayload<CaptureBoundaryPayload>(CorrectionContractTests.Normalize(end));
        Assert.Equal(Cut, read.Domains[0].SettledCut);
        Assert.Equal(SequenceContinuityState.Unproven, SequenceContinuity.Assess(Window(4, 7), Interval, read.Domains[0].SettledCut).State);
        var delivery = CorrectionContractTests.RoundTrip(Closed);
        Assert.Equal(2UL, SequenceContinuity.Assess(Window(4, 7), Interval, read.Domains[0].SettledCut, delivery).MissingObservations);
    }

    [Fact]
    public void F1_Later_outstanding_reservations_do_not_invalidate_a_closed_earlier_prefix()
    {
        var result = SequenceContinuity.Assess(Window(4, 7, 5, 6), Interval, Cut with { ReservedThrough = 8 }, Closed);
        Assert.Equal(SequenceContinuityState.Contiguous, result.State);
    }

    [Fact]
    public void F1_Repeated_delivery_of_one_sequence_cannot_hide_a_gap_or_inflate_the_count()
    {
        var result = SequenceContinuity.Assess(Window(4, 7, 7, 5), Interval, Cut, Closed);
        Assert.Equal(1UL, result.MissingObservations);
    }

    [Fact]
    public void F1_Empty_enabled_time_interval_cannot_prove_absence_in_a_nonempty_sequence_window()
    {
        var result = SequenceContinuity.Assess(Window(), Interval with { Interval = new(0, 0) }, Cut, Closed);
        Assert.Equal(SequenceContinuityState.Unproven, result.State);
    }
}
