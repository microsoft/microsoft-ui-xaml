using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class IntegrityTests
{
    [Theory]
    [InlineData(DeviceOperation.BaselinePresent)]
    [InlineData(DeviceOperation.OwnershipRelease)]
    public void Device_presence_and_ownership_release_cannot_claim_complete_physical_lifetime(DeviceOperation operation)
    {
        var payload = new DevicePayload(CoreTests.Epoch, operation, "D3D11", DataQuality.Complete);
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(payload)));
        var partial = new DataQuality(MeasurementKind.Measured, CoverageState.Partial, "external lifetime is unknown");
        Assert.Equal(partial, EventCodec.ReadPayload<DevicePayload>(
            EventCodec.Capture(BoundaryTests.Envelope(payload with { Quality = partial }))).Quality);
    }

    [Fact]
    public void Present_time_logical_surface_does_not_claim_complete_baseline_or_physical_bytes()
    {
        var payload = new ResourcePayload(new(CoreTests.Process, IdentityDomain.Resource, CoreTests.Epoch, 1),
            ResourceOperation.BaselinePresent, CoreTests.Session, null, null, SizeProvenance.Unknown, null, 64, 32,
            DataQuality.Complete, new(ResourceKind.CompositionSurface, ResourceKindEvidence.RuntimeBridge, "logical wrapper"));
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(payload)));
        var partial = new DataQuality(MeasurementKind.Measured, CoverageState.Partial, "physical backing is unknown");
        var decoded = EventCodec.ReadPayload<ResourcePayload>(
            EventCodec.Capture(BoundaryTests.Envelope(payload with { Quality = partial })));
        Assert.Equal(ResourceOperation.BaselinePresent, decoded.Operation);
        Assert.Null(decoded.BackingAllocationId);
        Assert.Null(decoded.AllocationBytes);
        Assert.Equal(SizeProvenance.Unknown, decoded.SizeProvenance);
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(
            decoded with { AllocationBytes = 64 * 32 * 4 })));
    }

    [Fact]
    public void Visual_property_units_and_correlation_domains_are_enforced()
    {
        var visual = new ObjectKey(CoreTests.Process, IdentityDomain.CompositionVisual, CoreTests.Epoch, 1);
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(
            new VisualDeltaPayload(visual, VisualProperty.Opacity, new(ValueKind.Text, ValueUnit.String, null, null, null, "opaque", DataQuality.Complete)))));
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(
            new CorrelationPayload(CoreTests.Key(), visual, CorrelationKind.CorePeer, true))));
    }

    [Fact]
    public void Snapshot_metadata_is_validated_and_immutable()
    {
        Assert.ThrowsAny<ArgumentException>(() => Snapshot("..\\escape.bin"));
        Assert.ThrowsAny<ArgumentException>(() => Snapshot("C:\\escape.bin"));
        Assert.ThrowsAny<ArgumentException>(() => Snapshot("captures\\data.bin", hash: "invalid"));
        Assert.ThrowsAny<ArgumentException>(() => Snapshot("captures\\data.bin", kind: (SnapshotKind)99));
        Assert.All(typeof(SnapshotMetadata).GetProperties(), property => Assert.Null(property.SetMethod));
    }

    private static SnapshotMetadata Snapshot(string path, string? hash = null, SnapshotKind kind = SnapshotKind.Lightweight) =>
        new(CoreTests.Epoch, CoreTests.Session, CoreTests.Process, kind,
            new(90, 100, CaptureMechanism.Synthetic, CaptureConsistency.NonAtomic, []), 20, CoreTests.Epoch,
            SnapshotState.Complete, DataQuality.Complete, new(path, hash ?? new string('a', 64)), null, 1, "synthetic/1", 1_000_000);

    [Fact]
    public void Query_result_pages_reject_inconsistent_bounds_and_copy_mutable_items()
    {
        var scope = new SessionScope(CoreTests.Session, CoreTests.Process);
        var context = new ResultContext(scope, new(1, 100), DataQuality.Complete);
        Assert.ThrowsAny<ArgumentException>(() => new PageInfo(0, 0, null, false));
        Assert.ThrowsAny<ArgumentException>(() => new PageInfo(1, 0, null, true));
        var row = new MetricSample(10, MetricKind.ProcessPrivateCommit,
            new(ValueKind.Integer, ValueUnit.Bytes, 1, null, null, null, DataQuality.Complete), new(0, 10), "OS", "1");
        Assert.ThrowsAny<ArgumentException>(() => new QueryPage<MetricSample>(context, [row, row], new(1, 0, null, false)));
        var source = new List<MetricSample> { row };
        var result = new QueryPage<MetricSample>(context, source, new(1, 0, 1, true));
        source[0] = row with { Source = "changed" };
        Assert.Equal("OS", result.Items[0].Source);
    }

    [Fact]
    public void Protocol_rejects_unknown_types_numeric_enums_unknown_fields_and_bad_utf8()
    {
        var bytes = TapCodec.Encode(TapMessageType.Disconnect, CoreTests.Session, new DisconnectBody("done"));
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(6), 65535);
        Assert.Throws<NotSupportedException>(() => TapCodec.Decode(bytes));
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(6), (ushort)TapMessageType.Disconnect);
        var bad = new byte[TapProtocol.HeaderBytes + 1];
        bytes.AsSpan(0, TapProtocol.HeaderBytes).CopyTo(bad);
        BinaryPrimitives.WriteUInt32LittleEndian(bad, 1);
        bad[^1] = 0xff;
        Assert.Throws<JsonException>(() => TapCodec.Decode(bad));
        var e = BoundaryTests.Envelope(new ObjectLifetimePayload(CoreTests.Key(), LifetimeOperation.Birth, 1, null, DataQuality.Complete));
        var payload = Encoding.UTF8.GetString(e.OwnedPayload.Span);
        Assert.Throws<JsonException>(() => EventCodec.Capture(BoundaryTests.ReplacePayload(e, payload.Replace("\"Birth\"", "99"))));
        Assert.Throws<JsonException>(() => EventCodec.Capture(BoundaryTests.ReplacePayload(e, payload.Insert(1, "\"surprise\":0,"))));
    }

    [Fact]
    public void Schema_versions_have_fixed_distinct_providers_and_unique_event_keys()
    {
        Assert.NotEqual(EventCatalog.RuntimeProviderId, EventCatalog.CollectorProviderId);
        Assert.Equal(EventCatalog.All.Count, EventCatalog.All.Select(d => (d.Identity.ProviderId, d.Identity.EventId)).Distinct().Count());
        Assert.All(EventCatalog.All, d =>
        {
            Assert.Equal((byte)1, d.Identity.Version);
            Assert.Equal((ulong)d.Keyword, d.Identity.Keywords);
            Assert.Contains(d.Level, new byte[] { 3, 4 });
        });
    }

    [Fact]
    public void Submission_and_later_failed_attempt_are_independent_observations()
    {
        var attempt = new AttemptReference(CoreTests.Epoch, 1, CoreTests.Session, 1);
        var submitted = new FramePayload(attempt, 1, FrameOutcome.Submitted, 0);
        var failedAfterCommit = new FramePayload(attempt, null, FrameOutcome.Failed, unchecked((int)0x80004005));
        Assert.Equal(1UL, EventCodec.ReadPayload<FramePayload>(EventCodec.Capture(BoundaryTests.Envelope(submitted))).SubmittedFrameId);
        Assert.Equal(FrameOutcome.Failed, EventCodec.ReadPayload<FramePayload>(EventCodec.Capture(BoundaryTests.Envelope(failedAfterCommit))).Outcome);
        EventCodec.Capture(BoundaryTests.Envelope(new FramePayload(null, null, FrameOutcome.StandaloneCommit, 0)));
    }

    [Fact]
    public void Multiple_root_attempts_can_share_one_physical_submission_without_counting_twice()
    {
        var roots = new[] { CoreTests.Session, Guid.Parse("3544c192-28c8-43f0-b4b6-cd4d47ec774c") };
        var attempts = roots.Select((root, index) => new AttemptReference(CoreTests.Epoch, (ulong)index + 1, root, 1)).ToArray();
        var events = attempts.Select(attempt => EventCodec.Capture(BoundaryTests.Envelope(
            new FramePayload(attempt, 7, FrameOutcome.Submitted, 0)))).ToArray();
        var payloads = events.Select(EventCodec.ReadPayload<FramePayload>).ToArray();
        Assert.Equal(2, payloads.Select(payload => payload.Attempt!.Value.RootId).Distinct().Count());
        Assert.Single(events.Select((e, index) => (e.Process, payloads[index].Attempt!.Value.ProducerEpoch,
            payloads[index].SubmittedFrameId)).Distinct());
        Assert.Single(events.Select(e => e.TimestampQpc).Distinct());

        var context = new ResultContext(new(CoreTests.Session, CoreTests.Process), new(2, 200), DataQuality.Complete);
        var frames = attempts.Select(attempt => new FrameInfo(attempt, new(50, 150), FrameAttemptStatus.Completed,
            new(7, 100, CoreTests.Epoch), 0, [])).ToArray();
        var page = new QueryPage<FrameInfo>(context, frames, new(2, 0, null, false));
        Assert.Equal(2, page.Items.Count);
        Assert.Single(page.Items.Select(frame => (frame.Attempt.ProducerEpoch, frame.Submission!.SubmittedFrameId)).Distinct());
    }
}
