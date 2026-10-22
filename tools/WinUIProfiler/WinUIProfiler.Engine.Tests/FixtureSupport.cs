using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Tests;

internal sealed class FakeClock(long frequency, long initialQpc = 0)
{
    public long Frequency { get; } = Require.Positive(frequency);
    public long TimestampQpc { get; private set; } = Require.Nonnegative(initialQpc);
    public long Advance(long ticks)
    {
        Require.Nonnegative(ticks);
        TimestampQpc = checked(TimestampQpc + ticks);
        return TimestampQpc;
    }
}

internal sealed class SyntheticEventBuilder
{
    internal static readonly Guid CollectorEpoch = Guid.Parse("30000000-0000-0000-0000-000000000001");
    private readonly FakeClock clock = new(1_000_000);
    private readonly List<EventEnvelope> events = [];
    private readonly Dictionary<Guid, ulong> sequences = [];
    internal IReadOnlyList<EventEnvelope> Events => events.AsReadOnly();

    internal void Add(IEventPayload payload, bool collector = false)
    {
        var provider = collector ? EventCatalog.CollectorProviderId : EventCatalog.RuntimeProviderId;
        var definition = EventCatalog.All.Single(d => d.PayloadType == payload.GetType() && d.Identity.ProviderId == provider);
        var sequence = sequences.GetValueOrDefault(provider) + 1;
        sequences[provider] = sequence;
        var pid = collector ? 99u : CoreTests.Process.ProcessId;
        var envelope = new EventEnvelope(CoreTests.Session, CoreTests.Process, pid, 7, clock.Advance(1000),
            events.Count + 1, sequence, null, definition.Identity, EventCodec.EncodePayload(payload, CoreTests.Process));
        events.Add(EventCodec.Normalize(envelope, new(provider, pid, collector ? CollectorEpoch : CoreTests.Epoch)).Envelope);
    }

    public static IReadOnlyList<EventEnvelope> BuildMinimal()
    {
        var b = new SyntheticEventBuilder();
        ObjectKey Key(IdentityDomain domain, ulong id) => new(CoreTests.Process, domain, CoreTests.Epoch, id);
        var root = Key(IdentityDomain.XamlCore, 1);
        var child = Key(IdentityDomain.XamlCore, 2);
        var peer = Key(IdentityDomain.XamlPeer, 3);
        var comp = Key(IdentityDomain.CompNode, 4);
        var visual = Key(IdentityDomain.CompositionVisual, 5);
        var resource = Key(IdentityDomain.Resource, 6);
        var rootId = Guid.Parse("40000000-0000-0000-0000-000000000001");
        var device = Guid.Parse("50000000-0000-0000-0000-000000000001");
        var backing = Guid.Parse("60000000-0000-0000-0000-000000000001");
        var capture = Guid.Parse("70000000-0000-0000-0000-000000000001");
        var allocator = Guid.Parse("80000000-0000-0000-0000-000000000001");
        var unknown = new DataQuality(MeasurementKind.Unavailable, CoverageState.Unavailable, "not observed by this synthetic source");
        var estimate = new DataQuality(MeasurementKind.Estimated, CoverageState.Partial, "scoped size excludes external allocations");
        TypedValue Bytes(long bytes) => new(ValueKind.Integer, ValueUnit.Bytes, bytes, null, null, null, DataQuality.Complete);
        var type = new TypeResolvedPayload(CoreTests.Epoch, 1, "Synthetic.Panel");
        var association = new AssociationPayload(root, peer, true);
        var tree = new TreeEdgePayload[]
        {
            new(null, root, RelationshipKind.Root, 0, rootId, 1, true),
            new(root, child, RelationshipKind.Child, 0, rootId, 1, true),
            new(null, comp, RelationshipKind.CompRoot, 0, rootId, 1, true),
            new(null, visual, RelationshipKind.VisualRoot, 0, rootId, 1, true)
        };
        var correlations = new CorrelationPayload[]
        {
            new(root, comp, CorrelationKind.XamlCompNode, true),
            new(comp, visual, CorrelationKind.CompNodeVisual, true),
            new(visual, resource, CorrelationKind.VisualResource, true)
        };
        var delta = new VisualDeltaPayload(visual, VisualProperty.Opacity, new(ValueKind.Number, ValueUnit.Scalar, null, 0.8, null, null, DataQuality.Complete));
        var resourceCreated = new ResourcePayload(resource, ResourceOperation.Create, device, backing, 4096,
            SizeProvenance.ActualAllocation, root, 32, 32, DataQuality.Complete);
        var allocation = new AllocationPayload(1, AllocationOperation.Allocate, null, 64, 80, allocator, null, DataQuality.Complete);
        b.Add(new ProcessPayload(ProcessOperation.Start, 0, DataQuality.Complete), true);
        b.Add(new RuntimeCapabilitiesPayload(CoreTests.Epoch, "synthetic-runtime/1", HostArchitecture.X64, CoreTests.Process, b.clock.Frequency,
            [new(Capability.ObjectLifetimes, 1, DataQuality.Complete), new(Capability.LightweightSnapshot, 1, DataQuality.Complete)]));
        b.Add(new BoundaryPayload(BoundaryKind.Initialization, BoundaryPhase.Begin, 1, null));
        b.Add(new DevicePayload(device, DeviceOperation.Create, "D3D11", DataQuality.Complete));
        b.Add(type);
        var keys = new[] { root, child, peer, comp, visual, resource };
        foreach (var key in keys) b.Add(new ObjectLifetimePayload(key, LifetimeOperation.Birth, 1, 0x1000 + key.RuntimeObjectId * 0x100, DataQuality.Complete));
        b.Add(association);
        foreach (var edge in tree) b.Add(edge);
        foreach (var correlation in correlations) b.Add(correlation);
        b.Add(delta);
        b.Add(resourceCreated);
        b.Add(allocation);
        b.Add(new BoundaryPayload(BoundaryKind.Initialization, BoundaryPhase.End, 1, 0));
        b.Add(new BoundaryPayload(BoundaryKind.LayoutMeasure, BoundaryPhase.Begin, 2, null));
        b.Add(new BoundaryPayload(BoundaryKind.LayoutMeasure, BoundaryPhase.End, 2, 0));
        b.Add(new FramePayload(1, null, FrameOutcome.Attempt, 1, rootId, null));
        b.Add(new FramePayload(1, 1, FrameOutcome.Submitted, 1, rootId, 0));
        b.Add(new FramePayload(1, null, FrameOutcome.AttemptCompleted, 1, rootId, 0));
        var firstSequence = b.sequences[EventCatalog.RuntimeProviderId] + 1;
        b.Add(new CaptureBoundaryPayload(capture, CaptureKind.Baseline, CapturePhase.Begin, rootId, 1, firstSequence, null, null, DataQuality.Complete));
        b.Add(new CaptureChunkPayload(capture, 0, rootId, 1,
            keys.Select(key => new SnapshotObject(key, "Synthetic.Panel", null, new(ValueKind.Integer, ValueUnit.Bytes, 64, null, null, null, estimate))).ToArray(),
            [new(root, child, "XAML child", OwnershipEvidence.Proven, DataQuality.Complete)],
            tree, correlations, [resourceCreated], [delta], [association], [type], [allocation]));
        b.Add(new CaptureBoundaryPayload(capture, CaptureKind.Baseline, CapturePhase.End, rootId, 1, firstSequence, firstSequence + 2, 1, DataQuality.Complete));
        var interval = new TimeRange(0, b.clock.TimestampQpc);
        b.Add(new MetricPayload(MetricKind.CpuUtilization, new(ValueKind.Number, ValueUnit.Percent, null, 6.25, null, null, DataQuality.Complete),
            interval, "GetProcessTimes delta normalized by logical processors", "synthetic-collector/1", null, null), true);
        b.Add(new MetricPayload(MetricKind.GpuUtilization, new(ValueKind.Missing, ValueUnit.Percent, null, null, null, null, unknown),
            interval, "OS GPU engine counters", "synthetic-collector/1", null, null), true);
        b.Add(new MetricPayload(MetricKind.ProcessPrivateWorkingSet, Bytes(4096), interval, "OS working set", "synthetic-collector/1", null, null), true);
        b.Add(new MetricPayload(MetricKind.ProcessPrivateCommit, Bytes(8192), interval, "OS private usage", "synthetic-collector/1", null, null), true);
        b.Add(resourceCreated with { Operation = ResourceOperation.OwnershipRelease, Owner = null });
        b.Add(resourceCreated with { Operation = ResourceOperation.Destroy, Owner = null });
        b.Add(allocation with { Operation = AllocationOperation.Free, RequestedBytes = 0, ActualBytes = null });
        foreach (var correlation in correlations) b.Add(correlation with { Linked = false });
        b.Add(association with { Linked = false });
        foreach (var edge in tree.Reverse()) b.Add(edge with { Connected = false });
        foreach (var key in keys.Reverse()) b.Add(new ObjectLifetimePayload(key, LifetimeOperation.Destroy, 1, null, DataQuality.Complete));
        b.Add(new DevicePayload(device, DeviceOperation.Destroy, "D3D11", DataQuality.Complete));
        b.Add(new ProcessPayload(ProcessOperation.Exit, 0, DataQuality.Complete), true);
        return b.Events;
    }
}
