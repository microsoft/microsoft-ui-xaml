using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class SequenceContractTests
{
    private static readonly Guid ListenerA = Guid.Parse("90000000-0000-0000-0000-000000000001");
    private static readonly Guid ListenerB = Guid.Parse("90000000-0000-0000-0000-000000000002");
    private static readonly Guid Continuity = Guid.Parse("90000000-0000-0000-0000-000000000003");
    private static SequenceDomainKey Domain(ushort eventId = 2) => new(new(EventCatalog.RuntimeProviderId, 42, CoreTests.Epoch), CoreTests.Process, eventId, 1);
    private static ListenerDomainInterval Interval(SequenceDomainKey? domain = null) => new(ListenerA, Continuity, domain ?? Domain(), new(0, 1000), DataQuality.Complete);
    private static SequenceCheckpoint Check(ulong sequence, Guid? continuity = null, ushort eventId = 2, Guid? listener = null) =>
        new(listener ?? ListenerA, continuity ?? Continuity, Domain(eventId), sequence, (long)sequence * 10);

    [Fact]
    public void R8_Selective_keywords_do_not_create_gaps_in_another_delivery_domain()
    {
        var assessment = SequenceContinuity.Assess(Check(1), Check(2), Interval());
        Assert.Equal(SequenceContinuityState.Contiguous, assessment.State);
        Assert.Equal(0UL, assessment.MissingObservations);
        Assert.Throws<ArgumentException>(() => SequenceContinuity.Assess(Check(1), Check(50, eventId: 9), Interval()));
    }

    [Fact]
    public void R8_Another_listener_selection_does_not_reset_existing_continuity()
    {
        var listenerB = new ListenerDomainInterval(ListenerB, CoreTests.Session, Domain(9), new(0, 1000), DataQuality.Complete);
        Assert.NotEqual(Interval().ListenerId, listenerB.ListenerId);
        Assert.Equal(SequenceContinuityState.Contiguous, SequenceContinuity.Assess(Check(4), Check(5), Interval()).State);
        Assert.Throws<ArgumentException>(() => SequenceContinuity.Assess(Check(4), Check(5, listener: ListenerB), Interval()));
    }

    [Fact]
    public void R8_Own_enable_mask_change_marks_only_that_domain_segment_unproven()
    {
        var next = CoreTests.Session;
        var current = Check(20, next);
        var interval = Interval() with { ContinuityId = next };
        var result = SequenceContinuity.Assess(Check(4), current, interval);
        Assert.Equal(SequenceContinuityState.Unproven, result.State);
        Assert.Null(result.MissingObservations);
        Assert.NotNull(result.Reason);
        Assert.Equal(SequenceContinuityState.Contiguous, SequenceContinuity.Assess(Check(4, eventId: 9), Check(5, eventId: 9), Interval(Domain(9))).State);
    }

    [Fact]
    public void R8_Real_missing_observations_are_detected_within_continuously_enabled_domain()
    {
        var result = SequenceContinuity.Assess(Check(4), Check(7), Interval());
        Assert.Equal(SequenceContinuityState.GapDetected, result.State);
        Assert.Equal(2UL, result.MissingObservations);
        var unknown = SequenceContinuity.Assess(Check(4), Check(7), Interval() with
        {
            Quality = new(MeasurementKind.Measured, CoverageState.Partial, "enable interval not observed")
        });
        Assert.Equal(SequenceContinuityState.Unproven, unknown.State);
    }

    [Fact]
    public void R8_Rundown_vectors_cover_independent_domains_and_roundtrip()
    {
        var ranges = new[]
        {
            new SequenceDomainRange(Domain(2), 0, 3, DataQuality.Complete),
            new SequenceDomainRange(Domain(5), 1, 4, DataQuality.Complete)
        };
        var payload = new CaptureBoundaryPayload(CoreTests.Session, CaptureKind.Baseline, CapturePhase.End, CoreTests.Session, 1,
            ranges, 1, DataQuality.Complete);
        Assert.Equal(ranges, EventCodec.ReadPayload<CaptureBoundaryPayload>(CorrectionContractTests.Normalize(payload)).Domains);
        var wrong = ranges[0] with { Domain = new(new(EventCatalog.RuntimeProviderId, 99, CoreTests.Epoch), CoreTests.Process, 2, 1) };
        Assert.Throws<ArgumentException>(() => CorrectionContractTests.Normalize(payload with { Domains = [wrong] }));
    }
}
