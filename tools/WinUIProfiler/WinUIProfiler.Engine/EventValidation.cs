using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine;

internal static class EventValidation
{
    internal static void Validate(IEventPayload payload, ProcessInstanceKey process, Guid? producerEpoch = null)
    {
        ArgumentNullException.ThrowIfNull(payload);
        process.Validate();
        void Key(ObjectKey key, IdentityDomain? domain = null)
        {
            key.Validate();
            if (key.Process != process || domain is not null && key.Domain != domain ||
                producerEpoch is { } expected && key.ProducerEpoch != expected)
                throw new ArgumentException("Object identity has the wrong process or domain.");
        }
        void OptionalKey(ObjectKey? key) { if (key is { } value) Key(value); }
        static void OptionalId(Guid? id) { if (id is { } value) Require.Id(value); }
        static void Quality(DataQuality quality) => Require.NotNull(quality);
        static void Value(TypedValue value) => Require.NotNull(value).Validate();
        switch (payload)
        {
            case RuntimeCapabilitiesPayload p:
                Require.Id(p.Epoch); Require.Text(p.Build); Require.Enum(p.Architecture); Require.Positive(p.QpcFrequency);
                if (p.Process != process) throw new ArgumentException("Capability target mismatch.");
                if (producerEpoch is { } runtimeEpoch && p.Epoch != runtimeEpoch) throw new ArgumentException("Capability epoch mismatch.");
                ValidateCapabilities(p.Capabilities);
                break;
            case ProcessPayload p:
                Require.Enum(p.Operation); Quality(p.Quality);
                if (p.CreationQpc is { } qpc) Require.Nonnegative(qpc);
                break;
            case ObjectLifetimePayload p:
                Key(p.Object); Require.Enum(p.Operation); Quality(p.Quality);
                if (p.TypeId is { } type) Require.Positive(type);
                if (p.Address is 0) throw new ArgumentException("Unknown addresses must be null, not zero.");
                break;
            case TypeResolvedPayload p:
                Require.Id(p.ProducerEpoch); Require.Positive(p.TypeId); Require.Text(p.Name);
                if (producerEpoch is { } typeEpoch && p.ProducerEpoch != typeEpoch) throw new ArgumentException("Type epoch mismatch.");
                break;
            case AssociationPayload p:
                Key(p.Core, IdentityDomain.XamlCore); Key(p.Peer, IdentityDomain.XamlPeer);
                break;
            case TreeEdgePayload p:
                OptionalKey(p.Parent); Key(p.Child); Require.Enum(p.Relationship); Require.Id(p.RootId); Require.Positive(p.DispatcherId);
                var root = p.Relationship is RelationshipKind.Root or RelationshipKind.CompRoot or RelationshipKind.VisualRoot;
                if (p.Order < 0 || p.Parent == p.Child || root && p.Parent is not null || !root && p.Parent is null)
                    throw new ArgumentException("Invalid tree relationship, order or parent.");
                var domain = p.Relationship switch
                {
                    RelationshipKind.CompChild or RelationshipKind.CompRoot => IdentityDomain.CompNode,
                    RelationshipKind.VisualChild or RelationshipKind.VisualRoot => IdentityDomain.CompositionVisual,
                    _ => IdentityDomain.XamlCore
                };
                Key(p.Child, domain);
                if (p.Parent is { } parent) Key(parent, domain);
                break;
            case VisualDeltaPayload p:
                Key(p.Visual, IdentityDomain.CompositionVisual); Require.Enum(p.Property); Value(p.Value);
                var expectedUnit = p.Property switch
                {
                    VisualProperty.IsVisible => ValueUnit.Boolean,
                    VisualProperty.OffsetX or VisualProperty.OffsetY or VisualProperty.Width or VisualProperty.Height => ValueUnit.Pixels,
                    _ => ValueUnit.Scalar
                };
                if (p.Value.Unit != expectedUnit ||
                    p.Property == VisualProperty.IsVisible && p.Value.Kind is not (ValueKind.Boolean or ValueKind.Missing) ||
                    p.Property != VisualProperty.IsVisible && p.Value.Kind is not (ValueKind.Integer or ValueKind.Number or ValueKind.Missing) ||
                    p.Property == VisualProperty.Opacity && (p.Value.Number is < 0 or > 1 || p.Value.Integer is < 0 or > 1))
                    throw new ArgumentException("Visual property units/discriminator/value do not match.");
                break;
            case CorrelationPayload p:
                Key(p.From); Key(p.To); Require.Enum(p.Kind);
                var domains = p.Kind switch
                {
                    CorrelationKind.CorePeer => (IdentityDomain.XamlCore, IdentityDomain.XamlPeer),
                    CorrelationKind.XamlCompNode => (IdentityDomain.XamlCore, IdentityDomain.CompNode),
                    CorrelationKind.CompNodeVisual => (IdentityDomain.CompNode, IdentityDomain.CompositionVisual),
                    CorrelationKind.XamlVisual => (IdentityDomain.XamlCore, IdentityDomain.CompositionVisual),
                    CorrelationKind.VisualResource => (IdentityDomain.CompositionVisual, IdentityDomain.Resource),
                    CorrelationKind.ResourceBacking => (IdentityDomain.Resource, IdentityDomain.Resource),
                    _ => throw new ArgumentException("Unsupported correlation.")
                };
                Key(p.From, domains.Item1); Key(p.To, domains.Item2);
                break;
            case BoundaryPayload p:
                Require.Enum(p.Kind); Require.Enum(p.Phase); Require.Positive(p.OperationId);
                if (p.Phase == BoundaryPhase.Begin && p.HResult is not null)
                    throw new ArgumentException("A boundary begin has no result.");
                break;
            case FramePayload p:
                Require.Enum(p.Outcome); Require.Positive(p.DispatcherId); Require.Id(p.RootId);
                if (p.Outcome != FrameOutcome.StandaloneCommit) Require.Positive(p.AttemptId);
                if (p.Outcome == FrameOutcome.Submitted)
                {
                    if (p.SubmittedFrameId is null or 0 || p.HResult is not >= 0)
                        throw new ArgumentException("Successful submissions require a nonzero frame ID and successful result.");
                }
                else if (p.SubmittedFrameId is not null)
                    throw new ArgumentException("Only successful XAML submissions have submitted frame IDs.");
                if (p.Outcome == FrameOutcome.Failed && p.HResult is not < 0)
                    throw new ArgumentException("Failed frames require a failing HRESULT.");
                if (p.Outcome == FrameOutcome.AttemptCompleted && p.HResult is not >= 0 ||
                    p.Outcome == FrameOutcome.Attempt && p.HResult is not null)
                    throw new ArgumentException("Attempt begins have no result; completed attempts require success.");
                break;
            case DevicePayload p:
                Require.Id(p.DeviceEpoch); Require.Enum(p.Operation); Require.Text(p.Api); Quality(p.Quality);
                break;
            case ResourcePayload p:
                Key(p.Resource, IdentityDomain.Resource); Require.Enum(p.Operation); Require.Id(p.DeviceEpoch);
                OptionalId(p.BackingAllocationId); OptionalKey(p.Owner); Require.Enum(p.SizeProvenance); Quality(p.Quality);
                if (p.SizeProvenance == SizeProvenance.Unknown && p.AllocationBytes is not null ||
                    p.SizeProvenance != SizeProvenance.Unknown && p.AllocationBytes is null ||
                    p.SizeProvenance == SizeProvenance.ActualAllocation && p.BackingAllocationId is null)
                    throw new ArgumentException("Allocation size must agree with provenance and backing identity.");
                break;
            case AllocationPayload p:
                Require.Positive(p.AllocationId); Require.Enum(p.Operation); Require.Id(p.AllocatorEpoch); OptionalId(p.BaselineCaptureId); Quality(p.Quality);
                if (p.Operation == AllocationOperation.Reallocate && p.PreviousAllocationId is null or 0 ||
                    p.Operation == AllocationOperation.Reallocate && p.PreviousAllocationId == p.AllocationId ||
                    p.Operation != AllocationOperation.Reallocate && p.PreviousAllocationId is not null ||
                    p.Operation != AllocationOperation.Free && p.ActualBytes < p.RequestedBytes)
                    throw new ArgumentException("Invalid allocation predecessor or actual byte count.");
                break;
            case CaptureBoundaryPayload p:
                Require.Id(p.CaptureId); Require.Enum(p.Kind); Require.Enum(p.Phase); OptionalId(p.RootId); Quality(p.Quality);
                if (p.DispatcherId is 0 || p.FirstSequence is 0 || p.LastSequence is 0 ||
                    p.FirstSequence > p.LastSequence || p.Phase == CapturePhase.End && p.ChunkCount is null)
                    throw new ArgumentException("Invalid capture boundary or sequence interval.");
                break;
            case CaptureChunkPayload p:
                Require.Id(p.CaptureId); Require.Id(p.RootId); Require.Positive(p.DispatcherId);
                Require.NotNull(p.Objects); Require.NotNull(p.Ownership);
                if (p.Objects.Count > 4096 || p.Ownership.Count > 8192) throw new ArgumentException("Capture chunk exceeds bounded item limits.");
                foreach (var item in p.Objects)
                {
                    Require.NotNull(item); Key(item.Key); Value(item.Size);
                    if (item.Address is 0) throw new ArgumentException("Unknown addresses must be null, not zero.");
                }
                foreach (var edge in p.Ownership) { Require.NotNull(edge); Key(edge.From); Key(edge.To); Require.Text(edge.Relationship); Require.Enum(edge.Evidence); Quality(edge.Quality); }
                ValidateItems(p.Tree); ValidateItems(p.Correlations); ValidateItems(p.Resources); ValidateItems(p.Visuals);
                ValidateItems(p.Associations); ValidateItems(p.Types); ValidateItems(p.Allocations);
                break;
            case LossPayload p:
                Require.Id(p.ProducerEpoch); Require.Text(p.Reason);
                if (producerEpoch is { } lossEpoch && p.ProducerEpoch != lossEpoch) throw new ArgumentException("Loss epoch mismatch.");
                if (p.LostEvents is 0) throw new ArgumentException("Unknown loss is null, not zero.");
                break;
            case MetricPayload p:
                Require.Enum(p.Metric); Value(p.Value); Require.Text(p.Source); Require.Text(p.SourceVersion);
                var unit = p.Metric switch
                {
                    MetricKind.CpuUtilization or MetricKind.GpuUtilization => ValueUnit.Percent,
                    MetricKind.ConnectedXamlElements or MetricKind.CanonicalCompositionVisuals => ValueUnit.Count,
                    _ => ValueUnit.Bytes
                };
                if (p.Value.Unit != unit || p.Value.Kind is not (ValueKind.Integer or ValueKind.Number or ValueKind.Missing) ||
                    p.Value.Integer is < 0 || p.Value.Number is < 0 ||
                    unit == ValueUnit.Percent && (p.Value.Integer is > 100 || p.Value.Number is > 100))
                    throw new ArgumentException("Metric units or values are invalid.");
                break;
            default:
                throw new NotSupportedException("Unknown payload schema.");
        }
        void ValidateItems<T>(IReadOnlyList<T> items) where T : IEventPayload
        {
            Require.NotNull(items);
            if (items.Count > 4096) throw new ArgumentException("Capture state list exceeds the bounded item limit.");
            foreach (var item in items) Validate(item, process, producerEpoch);
        }
    }

    internal static void ValidateCapabilities(IReadOnlyList<CapabilityInfo> capabilities)
    {
        Require.NotNull(capabilities);
        if (capabilities.Any(c => c is null)) throw new ArgumentException("Capabilities cannot contain null items.");
        if (capabilities.Count > 128 || capabilities.Select(c => c.Capability).Distinct().Count() != capabilities.Count)
            throw new ArgumentException("Capabilities must be bounded and unique.");
        foreach (var capability in capabilities)
        {
            Require.NotNull(capability); Require.Enum(capability.Capability); Require.Positive(capability.Version); Require.NotNull(capability.Quality);
        }
    }
}
