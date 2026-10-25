using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class CorrectionPublicationTests
{
    [Fact]
    public void R1_Frame_changes_include_causal_layout_boundaries_and_typed_type_resolution()
    {
        var attempt = CorrectionContractTests.Attempt;
        var changes = new FrameChange[]
        {
            new(100, 10, null, new BoundaryObservation(BoundaryKind.LayoutMeasure, BoundaryPhase.Begin, 2, null),
                CorrectionContractTests.Explicit, FrameChangeRelation.Causal, DataQuality.Complete)
        };
        var frame = CorrectionContractTests.RoundTrip(CorrectionContractTests.Page(new FrameInfo(attempt, new(90, 110), FrameAttemptStatus.Completed,
            null, 0, changes)));
        Assert.IsType<BoundaryObservation>(frame.Items[0].Changes[0].Observation);
        var history = CorrectionContractTests.RoundTrip(CorrectionContractTests.Page(new ObjectHistoryEntry(CoreTests.Key(), 100, 10,
            new TypeObservation(1, "Panel", CorrectionContractTests.UiElement), new(CauseKind.OutsideAttempt, null, null), DataQuality.Complete)));
        Assert.Equal(CorrectionContractTests.UiElement, Assert.IsType<TypeObservation>(history.Items[0].Observation).Classification);
    }

    [Fact]
    public void R4_Baseline_resource_cannot_claim_ui_element_type_evidence()
    {
        var resource = new ObjectKey(CoreTests.Process, IdentityDomain.Resource, CoreTests.Epoch, 1);
        var chunk = new CaptureChunkPayload(CoreTests.Session, 0, CoreTests.Session, 1,
            [new(resource, "MisleadingUIElementName", null, CorrectionContractTests.Integer(4), CorrectionContractTests.UiElement)],
            [], [], [], [], [], [], [], []);
        Assert.Throws<ArgumentException>(() => CorrectionContractTests.Normalize(chunk));
    }

    [Fact]
    public void R8_Session_results_freeze_listener_domain_intervals_and_reject_foreign_targets()
    {
        var domain = new SequenceDomainKey(new(EventCatalog.RuntimeProviderId, 42, CoreTests.Epoch), CoreTests.Process, 2, 1);
        var intervals = new[] { new ListenerDomainInterval(CoreTests.Session, CoreTests.Epoch, domain, new(10, 100), DataQuality.Complete) };
        var row = new SessionInfo(SessionMode.LiveAttach, SessionStatus.Recording, 1_000_000, null, [], null, intervals);
        var result = new QueryResult<SessionInfo>(CorrectionContractTests.Context, row);
        intervals[0] = null!;
        Assert.NotNull(result.Value.ListenerIntervals![0]);
        Assert.Equal(domain, CorrectionContractTests.RoundTrip(result).Value.ListenerIntervals![0].Domain);
        var foreign = domain with { Target = new(42, null, CoreTests.Epoch) };
        Assert.Throws<ArgumentException>(() => new QueryResult<SessionInfo>(CorrectionContractTests.Context, row with
        {
            ListenerIntervals = [new(CoreTests.Session, CoreTests.Epoch, foreign, new(10, 100), DataQuality.Complete)]
        }));
    }

    [Fact]
    public void R8_Null_rundown_domains_are_bad_data_not_null_reference_failures()
    {
        Assert.ThrowsAny<ArgumentException>(() => CorrectionContractTests.Normalize(new CaptureBoundaryPayload(CoreTests.Session,
            CaptureKind.Baseline, CapturePhase.End, CoreTests.Session, 1, [null!], 1, DataQuality.Complete)));
    }

    [Fact]
    public void R1_Frame_change_lists_are_chronological_not_arbitrary_bags()
    {
        FrameChange Change(long time, long ordinal) => new(time, ordinal, null,
            new BoundaryObservation(BoundaryKind.LayoutMeasure, BoundaryPhase.Begin, 1, null),
            CorrectionContractTests.Explicit, FrameChangeRelation.Causal, DataQuality.Complete);
        var row = new FrameInfo(CorrectionContractTests.Attempt, new(90, 110), FrameAttemptStatus.Completed, null, 0,
            [Change(102, 11), Change(100, 10)]);
        Assert.Throws<ArgumentException>(() => CorrectionContractTests.Page(row));
        Assert.Throws<ArgumentException>(() => CorrectionContractTests.Page(row with { Changes = [Change(100, 11), Change(100, 10)] }));
    }

    [Fact]
    public void R2_Pending_source_can_be_unknown_but_not_an_empty_version_or_finished_extent()
    {
        SnapshotMetadata Pending(string? source, long? end = null) => new(CoreTests.Epoch, CoreTests.Session, CoreTests.Process,
            SnapshotKind.Lightweight, new(10, end, CaptureMechanism.RuntimeBridge, CaptureConsistency.NonAtomic, []),
            0, null, SnapshotState.Pending, new(MeasurementKind.Unavailable, CoverageState.Unavailable, "queued"),
            null, null, 1, source, 1_000_000);
        var restored = CorrectionContractTests.RoundTrip(CorrectionContractTests.Page(Pending(null))).Items[0];
        Assert.Null(restored.ProducerEpoch);
        Assert.Null(restored.SourceVersion);
        Assert.Throws<ArgumentException>(() => Pending(""));
        Assert.Throws<ArgumentException>(() => Pending(null, 40));
    }

    [Fact]
    public void R3_Count_comparisons_are_integer_counts_with_signed_integer_deltas()
    {
        var key = new SnapshotComparisonKey(SnapshotAttribute.ObjectCount, ComparisonGrouping.Session, null, null, null, null);
        var row = new SnapshotDelta(CoreTests.Session, key, CorrectionContractTests.Number(2.5, ValueUnit.Count),
            CorrectionContractTests.Integer(1, ValueUnit.Count), CorrectionContractTests.Number(-1.5, ValueUnit.Count),
            ComparisonCompatibility.Compatible, null);
        Assert.Throws<ArgumentException>(() => CorrectionContractTests.Page(row));
    }
}
