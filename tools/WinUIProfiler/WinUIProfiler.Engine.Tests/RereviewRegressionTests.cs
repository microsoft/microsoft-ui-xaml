using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class RereviewRegressionTests
{
    [Fact]
    public void F1_Enabled_delivery_does_not_finalize_a_gap_before_late_reservations_arrive()
    {
        var domain = new SequenceDomainKey(new(EventCatalog.RuntimeProviderId, 42, CoreTests.Epoch), CoreTests.Process, 2, 1);
        var interval = new ListenerDomainInterval(CoreTests.Session, CoreTests.Epoch, domain, new(0, 1000), DataQuality.Complete);
        var first = new SequenceCheckpoint(interval.ListenerId, interval.ContinuityId, domain, 4, 40, 1);
        var next = first with { Sequence = 7, TimestampQpc = 70 };
        var result = SequenceContinuity.Assess(first, next, interval);
        Assert.Equal(SequenceContinuityState.Unproven, result.State);
        Assert.Null(result.MissingObservations);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void F2_Unfinished_snapshot_scope_cannot_escape_the_committed_view(bool singleResult)
    {
        var scopes = new CaptureScopeBoundary[]
        {
            new(new(CaptureScopeKind.XamlRoot, CoreTests.Session, 1, null), new(10, 20), DataQuality.Complete),
            new(new(CaptureScopeKind.XamlRoot, CoreTests.Epoch, 2, null), new(20, 2000), DataQuality.Complete)
        };
        var row = CorrectionContractTests.Snapshot(SnapshotState.Capturing, extent:
            new(10, null, CaptureMechanism.RuntimeBridge, CaptureConsistency.NonAtomic, scopes));
        if (singleResult)
            Assert.Throws<ArgumentException>(() => new QueryResult<SnapshotMetadata>(CorrectionContractTests.Context, row));
        else
            Assert.Throws<ArgumentException>(() => CorrectionContractTests.Page(row));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void F2_Unfinished_snapshot_keeps_valid_scopes_without_inventing_an_artifact_or_end(bool singleResult)
    {
        var row = CorrectionContractTests.Snapshot(SnapshotState.Capturing, extent:
            new(10, null, CaptureMechanism.RuntimeBridge, CaptureConsistency.NonAtomic,
                [new(new(CaptureScopeKind.XamlRoot, CoreTests.Session, 1, null), new(20, 1000), DataQuality.Complete)]));
        var published = singleResult
            ? CorrectionContractTests.RoundTrip(new QueryResult<SnapshotMetadata>(CorrectionContractTests.Context, row)).Value
            : CorrectionContractTests.RoundTrip(CorrectionContractTests.Page(row)).Items[0];
        Assert.Null(published.Artifact);
        Assert.Null(published.Extent.EndedQpc);
        Assert.Equal(1000, published.Extent.Boundaries[0].Interval.EndQpc);
    }
}
