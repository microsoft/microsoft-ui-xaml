using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine;

internal static class EventValidation
{
    internal static void Validate(IEventPayload payload, ProcessInstanceKey process, ProducerStreamKey? stream = null)
    {
        ArgumentNullException.ThrowIfNull(payload);
        process.Validate();
        var producerEpoch = stream?.ProducerEpoch;
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
        void Cause(ChangeCause? cause, Guid? epoch = null) => cause?.Validate(epoch ?? producerEpoch);
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
                Cause(p.Cause, p.Object.ProducerEpoch);
                ObservationRules.Classification(p.Object.Domain, p.Classification);
                if (p.TypeId is { } type) Require.Positive(type);
                if (p.Address is 0) throw new ArgumentException("Unknown addresses must be null, not zero.");
                break;
            case TypeResolvedPayload p:
                Require.Id(p.ProducerEpoch); Require.Positive(p.TypeId); Require.Text(p.Name);
                p.Classification?.Validate();
                if (producerEpoch is { } typeEpoch && p.ProducerEpoch != typeEpoch) throw new ArgumentException("Type epoch mismatch.");
                break;
            case AssociationPayload p:
                Key(p.Core, IdentityDomain.XamlCore); Key(p.Peer, IdentityDomain.XamlPeer);
                Cause(p.Cause, p.Core.ProducerEpoch);
                break;
            case TreeEdgePayload p:
                OptionalKey(p.Parent); Key(p.Child);
                ObservationRules.Tree(p.Child, p.Parent, p.Relationship, p.Order, p.RootId, p.DispatcherId, p.Membership);
                Cause(p.Cause, p.Child.ProducerEpoch);
                break;
            case VisualDeltaPayload p:
                Key(p.Visual, IdentityDomain.CompositionVisual);
                ObservationRules.Visual(p.Property, p.Value); Cause(p.Cause, p.Visual.ProducerEpoch);
                break;
            case CorrelationPayload p:
                Key(p.From); Key(p.To);
                ObservationRules.Correlation(p.From, p.To, p.Kind); Cause(p.Cause, p.From.ProducerEpoch);
                break;
            case BoundaryPayload p:
                Require.Enum(p.Kind); Require.Enum(p.Phase); Require.Positive(p.OperationId);
                Cause(p.Cause);
                if (p.Phase == BoundaryPhase.Begin && p.HResult is not null)
                    throw new ArgumentException("A boundary begin has no result.");
                break;
            case FramePayload p:
                Require.Enum(p.Outcome);
                if (p.Attempt is { } attempt)
                {
                    attempt.Validate();
                    if (producerEpoch is { } epoch && attempt.ProducerEpoch != epoch) throw new ArgumentException("Attempt belongs to another producer epoch.");
                }
                if (p.Outcome != FrameOutcome.StandaloneCommit && p.Attempt is null) throw new ArgumentException("Non-standalone frames need an epoch-qualified attempt.");
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
                Require.NotNull(p.Classification).Validate(); Cause(p.Cause, p.Resource.ProducerEpoch);
                OptionalId(p.BackingAllocationId); OptionalKey(p.Owner); Require.Enum(p.SizeProvenance); Quality(p.Quality);
                if (p.SizeProvenance == SizeProvenance.Unknown && p.AllocationBytes is not null ||
                    p.SizeProvenance != SizeProvenance.Unknown && p.AllocationBytes is null ||
                    p.SizeProvenance == SizeProvenance.ActualAllocation && p.BackingAllocationId is null)
                    throw new ArgumentException("Allocation size must agree with provenance and backing identity.");
                if (p.SizeProvenance == SizeProvenance.ApiDescriptionEstimate && p.Quality.Kind != MeasurementKind.Estimated ||
                    p.SizeProvenance == SizeProvenance.ActualAllocation && p.Quality.Kind != MeasurementKind.Measured)
                    throw new ArgumentException("Measurement kind must agree with size provenance.");
                break;
            case AllocationPayload p:
                OptionalId(p.BaselineCaptureId); Quality(p.Quality);
                Cause(p.Cause);
                ObservationRules.Allocation(p.AllocationId, p.Operation, p.PreviousAllocationId, p.RequestedBytes, p.ActualBytes, p.AllocatorEpoch);
                break;
            case CaptureBoundaryPayload p:
                Require.Id(p.CaptureId); Require.Enum(p.Kind); Require.Enum(p.Phase); OptionalId(p.RootId); Quality(p.Quality);
                Require.NotNull(p.Domains);
                if (p.Domains.Any(d => d is null) || p.Domains.Count > 256 || p.Domains.Select(d => d.Domain).Distinct().Count() != p.Domains.Count)
                    throw new ArgumentException("Capture sequence domains must be bounded and unique.");
                foreach (var range in p.Domains)
                {
                    Require.NotNull(range); Domain(range.Domain); Quality(range.Quality);
                    if (range.BeforeSequence > range.ThroughSequence ||
                        range.Quality.Coverage == CoverageState.Complete && (range.BeforeSequence is null || p.Phase == CapturePhase.End && range.ThroughSequence is null))
                        throw new ArgumentException("Capture sequence vector cannot claim complete missing/inverted checkpoints.");
                    if (range.SettledCut is { } cut)
                    {
                        cut.Validate();
                        if (cut.Domain != range.Domain) throw new ArgumentException("Capture settled cut belongs to a different delivery domain.");
                    }
                    if (p.Phase == CapturePhase.End && range.Quality.Coverage == CoverageState.Complete &&
                        (range.SettledCut is null || range.SettledCut.SettledThrough < range.ThroughSequence))
                        throw new ArgumentException("Complete producer capture coverage requires a settled cut covering every reservation through End's checkpoint.");
                }
                if (p.Quality.Coverage == CoverageState.Complete && p.Domains.Count == 0) throw new ArgumentException("Complete capture requires explicit relevant sequence domains.");
                if (p.DispatcherId is 0 || p.Phase == CapturePhase.End && p.ChunkCount is null)
                    throw new ArgumentException("Invalid capture boundary or sequence interval.");
                break;
            case CaptureChunkPayload p:
                Require.Id(p.CaptureId); Require.Id(p.RootId); Require.Positive(p.DispatcherId);
                Require.NotNull(p.Objects); Require.NotNull(p.Ownership);
                if (p.Objects.Count > 4096 || p.Ownership.Count > 8192) throw new ArgumentException("Capture chunk exceeds bounded item limits.");
                foreach (var item in p.Objects)
                {
                    Require.NotNull(item); Key(item.Key); ValueRules.NonnegativeBytes(item.Size);
                    ObservationRules.Classification(item.Key.Domain, item.Classification);
                    if (item.Address is 0) throw new ArgumentException("Unknown addresses must be null, not zero.");
                }
                foreach (var edge in p.Ownership) { Require.NotNull(edge); Key(edge.From); Key(edge.To); Require.Text(edge.Relationship); Require.Enum(edge.Evidence); Quality(edge.Quality); }
                ValidateItems(p.Tree); ValidateItems(p.Correlations); ValidateItems(p.Resources); ValidateItems(p.Visuals);
                ValidateItems(p.Associations); ValidateItems(p.Types); ValidateItems(p.Allocations);
                if (p.Tree.Any(edge => edge.RootId != p.RootId || edge.DispatcherId != p.DispatcherId))
                    throw new ArgumentException("Nested topology must match its capture chunk root and dispatcher.");
                break;
            case LossPayload p:
                Require.Id(p.ProducerEpoch); Require.Text(p.Reason);
                if (producerEpoch is { } lossEpoch && p.ProducerEpoch != lossEpoch) throw new ArgumentException("Loss epoch mismatch.");
                if (p.LostEvents is 0) throw new ArgumentException("Unknown loss is null, not zero.");
                if (p.Domains is not null) foreach (var affected in p.Domains) Domain(affected);
                break;
            case MetricPayload p:
                ObservationRules.Metric(p.Metric, p.Value, p.Interval, p.Source, p.SourceVersion, p.Gpu);
                break;
            case XamlPropertyChangedPayload p:
                Key(p.Object, IdentityDomain.XamlCore); ObservationRules.Property(p.Property, p.Previous, p.Current);
                Cause(p.Cause, p.Object.ProducerEpoch);
                break;
            default:
                throw new NotSupportedException("Unknown payload schema.");
        }
        void ValidateItems<T>(IReadOnlyList<T> items) where T : IEventPayload
        {
            Require.NotNull(items);
            if (items.Count > 4096) throw new ArgumentException("Capture state list exceeds the bounded item limit.");
            foreach (var item in items) Validate(item, process, stream);
        }
        void Domain(SequenceDomainKey domain)
        {
            domain.Validate();
            if (domain.Target != process || stream is { } actual && domain.Stream != actual ||
                producerEpoch is { } epoch && domain.Stream.ProducerEpoch != epoch)
                throw new ArgumentException("Capture/loss vector domain does not belong to its authoritative source and target.");
        }
    }

    internal static void ValidateCapabilities(IReadOnlyList<CapabilityInfo> capabilities)
    {
        ObservationRules.FreezeCapabilities(capabilities);
    }
}
