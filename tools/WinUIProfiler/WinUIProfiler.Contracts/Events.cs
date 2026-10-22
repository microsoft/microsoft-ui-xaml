namespace WinUIProfiler.Contracts;

[Flags]
public enum EventKeyword : ulong { Capabilities = 1, Objects = 2, XamlTree = 4, Composition = 8, Frames = 16, Resources = 32, Allocator = 64, Capture = 128, Metrics = 256 }
public enum LifetimeOperation { Birth, Destroy, BaselinePresent }
public enum RelationshipKind { Root, Child, Content, Template, Popup, CompRoot, CompChild, VisualRoot, VisualChild }
public enum CorrelationKind { CorePeer, XamlCompNode, CompNodeVisual, XamlVisual, VisualResource, ResourceBacking }
public enum VisualProperty { OffsetX, OffsetY, Width, Height, Opacity, IsVisible, TransformM11, TransformM12, TransformM21, TransformM22 }
public enum BoundaryKind { Initialization, DeviceCreation, LayoutMeasure, LayoutArrange, Render }
public enum BoundaryPhase { Begin, End }
public enum FrameOutcome { Attempt, Submitted, Failed, StandaloneCommit, AttemptCompleted }
public enum ResourceOperation { Create, Destroy, OwnershipRelease, Close, Rebind, Resize, Share }
public enum SizeProvenance { ActualAllocation, ApiDescriptionEstimate, Unknown }
public enum AllocationOperation { Allocate, Free, Reallocate }
public enum CaptureKind { Baseline, LightweightSnapshot, OwnershipGraph }
public enum CapturePhase { Begin, End }
public enum MetricKind { CpuUtilization, GpuUtilization, ProcessPrivateWorkingSet, ProcessPrivateCommit, ProcessWorkingSet, XamlBytes, ConnectedXamlElements, CanonicalCompositionVisuals, GpuDedicatedCommittedBytes, GpuSharedCommittedBytes, GpuDedicatedResidentBytes, GpuSharedResidentBytes }
public enum DeviceOperation { Create, Lost, Destroy }
public enum ProcessOperation { Start, Exit, Attach }

public interface IEventPayload;
public sealed record RuntimeCapabilitiesPayload(Guid Epoch, string Build, HostArchitecture Architecture, ProcessInstanceKey Process, long QpcFrequency, IReadOnlyList<CapabilityInfo> Capabilities) : IEventPayload;
public sealed record ProcessPayload(ProcessOperation Operation, long? CreationQpc, DataQuality Quality) : IEventPayload;
public sealed record ObjectLifetimePayload(ObjectKey Object, LifetimeOperation Operation, ulong? TypeId, ulong? Address, DataQuality Quality, XamlTypeClassification? Classification = null, ChangeCause? Cause = null) : IEventPayload;
public sealed record TypeResolvedPayload(Guid ProducerEpoch, ulong TypeId, string Name, XamlTypeClassification? Classification = null) : IEventPayload;
public sealed record AssociationPayload(ObjectKey Core, ObjectKey Peer, bool Linked, ChangeCause? Cause = null) : IEventPayload;
public sealed record TreeEdgePayload(ObjectKey? Parent, ObjectKey Child, RelationshipKind Relationship, int Order, Guid RootId, ulong DispatcherId, bool Connected, ChangeCause? Cause = null, TreeMembershipKind Membership = TreeMembershipKind.Unknown) : IEventPayload;
public sealed record VisualDeltaPayload(ObjectKey Visual, VisualProperty Property, TypedValue Value, ChangeCause? Cause = null) : IEventPayload;
public sealed record CorrelationPayload(ObjectKey From, ObjectKey To, CorrelationKind Kind, bool Linked, ChangeCause? Cause = null) : IEventPayload;
public sealed record BoundaryPayload(BoundaryKind Kind, BoundaryPhase Phase, ulong OperationId, int? HResult, ChangeCause? Cause = null) : IEventPayload;
public sealed record FramePayload(AttemptReference? Attempt, ulong? SubmittedFrameId, FrameOutcome Outcome, int? HResult) : IEventPayload;
public sealed record DevicePayload(Guid DeviceEpoch, DeviceOperation Operation, string Api, DataQuality Quality) : IEventPayload;
public sealed record ResourcePayload(ObjectKey Resource, ResourceOperation Operation, Guid DeviceEpoch, Guid? BackingAllocationId, ulong? AllocationBytes, SizeProvenance SizeProvenance, ObjectKey? Owner, uint? WidthPixels, uint? HeightPixels, DataQuality Quality, ResourceClassification Classification, ChangeCause? Cause = null) : IEventPayload;
public sealed record AllocationPayload(ulong AllocationId, AllocationOperation Operation, ulong? PreviousAllocationId, ulong RequestedBytes, ulong? ActualBytes, Guid AllocatorEpoch, Guid? BaselineCaptureId, DataQuality Quality, ChangeCause? Cause = null) : IEventPayload;
public sealed record CaptureBoundaryPayload(Guid CaptureId, CaptureKind Kind, CapturePhase Phase, Guid? RootId, ulong? DispatcherId, IReadOnlyList<SequenceDomainRange> Domains, uint? ChunkCount, DataQuality Quality) : IEventPayload;
public sealed record CaptureChunkPayload(Guid CaptureId, uint ChunkIndex, Guid RootId, ulong DispatcherId,
    IReadOnlyList<SnapshotObject> Objects, IReadOnlyList<OwnershipEdge> Ownership,
    IReadOnlyList<TreeEdgePayload> Tree, IReadOnlyList<CorrelationPayload> Correlations,
    IReadOnlyList<ResourcePayload> Resources, IReadOnlyList<VisualDeltaPayload> Visuals,
    IReadOnlyList<AssociationPayload> Associations, IReadOnlyList<TypeResolvedPayload> Types,
    IReadOnlyList<AllocationPayload> Allocations) : IEventPayload;
public sealed record LossPayload(Guid ProducerEpoch, TimeRange Interval, ulong? LostEvents, string Reason, IReadOnlyList<SequenceDomainKey>? Domains = null) : IEventPayload;
public sealed record MetricPayload(MetricKind Metric, TypedValue Value, TimeRange Interval, string Source, string SourceVersion, GpuSampleIdentity? Gpu) : IEventPayload;

public sealed record EventDefinition(EventIdentity Identity, byte Level, EventKeyword Keyword, Type PayloadType);

public static class EventCatalog
{
    public static readonly Guid RuntimeProviderId = Guid.Parse("43a73524-bc37-4f6e-9ba8-84b5346f61c2");
    public static readonly Guid CollectorProviderId = Guid.Parse("b58fa3ac-f7e8-4a24-88cd-478ab589de21");
    public const string RuntimeProviderName = "WinUIProfiler-Runtime";
    public const string CollectorProviderName = "WinUIProfiler-Collector";
    public const byte Version = 1;
    public static IReadOnlyList<EventDefinition> All { get; } = Array.AsReadOnly(new[]
    {
        Define<RuntimeCapabilitiesPayload>(RuntimeProviderId, 1, "RuntimeCapabilities", EventKeyword.Capabilities),
        Define<ObjectLifetimePayload>(RuntimeProviderId, 2, "ObjectLifetime", EventKeyword.Objects),
        Define<TypeResolvedPayload>(RuntimeProviderId, 3, "TypeResolved", EventKeyword.Objects),
        Define<AssociationPayload>(RuntimeProviderId, 4, "CorePeerAssociation", EventKeyword.Objects),
        Define<TreeEdgePayload>(RuntimeProviderId, 5, "TreeEdge", EventKeyword.XamlTree),
        Define<VisualDeltaPayload>(RuntimeProviderId, 6, "VisualDelta", EventKeyword.Composition),
        Define<CorrelationPayload>(RuntimeProviderId, 7, "Correlation", EventKeyword.Composition),
        Define<BoundaryPayload>(RuntimeProviderId, 8, "Boundary", EventKeyword.Frames),
        Define<FramePayload>(RuntimeProviderId, 9, "Frame", EventKeyword.Frames),
        Define<DevicePayload>(RuntimeProviderId, 10, "Device", EventKeyword.Resources),
        Define<ResourcePayload>(RuntimeProviderId, 11, "Resource", EventKeyword.Resources),
        Define<AllocationPayload>(RuntimeProviderId, 12, "Allocation", EventKeyword.Allocator),
        Define<CaptureBoundaryPayload>(RuntimeProviderId, 13, "CaptureBoundary", EventKeyword.Capture),
        Define<CaptureChunkPayload>(RuntimeProviderId, 14, "CaptureChunk", EventKeyword.Capture),
        Define<LossPayload>(RuntimeProviderId, 15, "Loss", EventKeyword.Capabilities, 3),
        Define<XamlPropertyChangedPayload>(RuntimeProviderId, 16, "XamlPropertyChanged", EventKeyword.Objects),
        Define<ProcessPayload>(CollectorProviderId, 1, "Process", EventKeyword.Capabilities),
        Define<MetricPayload>(CollectorProviderId, 2, "Metric", EventKeyword.Metrics),
        Define<LossPayload>(CollectorProviderId, 3, "Loss", EventKeyword.Capabilities, 3)
    });
    private static EventDefinition Define<T>(Guid provider, ushort id, string name, EventKeyword keyword, byte level = 4) where T : IEventPayload =>
        new(new(provider, name, id, Version, (ulong)keyword), level, keyword, typeof(T));
    public static EventDefinition Get(Guid providerId, ushort eventId) =>
        All.SingleOrDefault(d => d.Identity.ProviderId == providerId && d.Identity.EventId == eventId)
        ?? throw new NotSupportedException("Unknown producer/event identity.");
}
