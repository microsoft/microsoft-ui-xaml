using System.Text.Json.Serialization;

namespace WinUIProfiler.Contracts;

[method: JsonConstructor]
public readonly record struct AttemptReference(Guid ProducerEpoch, ulong AttemptId, Guid RootId, ulong DispatcherId)
{
    public void Validate()
    {
        Require.Id(ProducerEpoch); Require.Positive(AttemptId); Require.Id(RootId); Require.Positive(DispatcherId);
    }
}
public enum CauseKind { Unknown, OutsideAttempt, ExplicitAttempt }
public sealed record ChangeCause(CauseKind Kind, AttemptReference? Attempt, string? Reason)
{
    public void Validate(Guid? producerEpoch = null)
    {
        Require.Enum(Kind);
        if (Kind == CauseKind.ExplicitAttempt)
        {
            if (Attempt is not { } attempt) throw new ArgumentException("Explicit causality requires an epoch-qualified attempt.");
            attempt.Validate();
            if (producerEpoch is { } epoch && attempt.ProducerEpoch != epoch) throw new ArgumentException("Causal attempt has a different producer epoch.");
        }
        else if (Attempt is not null) throw new ArgumentException("Unknown/outside-attempt observations cannot claim an attempt.");
        if (Kind == CauseKind.Unknown) Require.Text(Reason);
    }
}
public sealed record XamlPropertyKey(ulong PropertyId, string DeclaringType, string Name);
public sealed record XamlPropertyChangedPayload(ObjectKey Object, XamlPropertyKey Property, TypedValue Previous, TypedValue Current, ChangeCause? Cause) : IEventPayload;

[JsonPolymorphic(TypeDiscriminatorPropertyName = "kind")]
[JsonDerivedType(typeof(LifetimeObservation), "lifetime")]
[JsonDerivedType(typeof(PropertyObservation), "property")]
[JsonDerivedType(typeof(MembershipObservation), "membership")]
[JsonDerivedType(typeof(VisualPropertyObservation), "visualProperty")]
[JsonDerivedType(typeof(ResourceObservation), "resource")]
[JsonDerivedType(typeof(CorrelationObservation), "correlation")]
[JsonDerivedType(typeof(AllocationObservation), "allocation")]
[JsonDerivedType(typeof(BoundaryObservation), "boundary")]
[JsonDerivedType(typeof(TypeObservation), "type")]
public abstract record ObjectObservation;
public sealed record LifetimeObservation(LifetimeOperation Operation, string? TypeName, ulong? Address, XamlTypeClassification? Classification = null) : ObjectObservation;
public sealed record TypeObservation(ulong TypeId, string Name, XamlTypeClassification? Classification) : ObjectObservation;
public sealed record BoundaryObservation(BoundaryKind Boundary, BoundaryPhase Phase, ulong OperationId, int? HResult) : ObjectObservation;
public sealed record PropertyObservation(XamlPropertyKey Property, TypedValue Previous, TypedValue Current) : ObjectObservation;
public sealed record MembershipObservation(ObjectKey? Parent, RelationshipKind Relationship, int Order, Guid RootId, ulong DispatcherId, bool Connected, TreeMembershipKind Membership) : ObjectObservation;
public sealed record VisualPropertyObservation(VisualProperty Property, TypedValue Value) : ObjectObservation;
public sealed record ResourceObservation(ResourceOperation Operation, ResourceClassification Classification, Guid DeviceEpoch, Guid? BackingAllocationId, TypedValue Size, SizeProvenance Provenance, ObjectKey? Owner) : ObjectObservation;
public sealed record CorrelationObservation(ObjectKey Other, CorrelationKind Relationship, bool Linked, bool IsFrom) : ObjectObservation;
public sealed record AllocationObservation(ulong AllocationId, AllocationOperation Operation, ulong? PreviousAllocationId, ulong RequestedBytes, ulong? ActualBytes, Guid AllocatorEpoch) : ObjectObservation;
public enum FrameChangeRelation { Causal, TemporalOverlap }
public sealed record FrameChange(long TimestampQpc, long IngestOrdinal, ObjectKey? Object, ObjectObservation Observation, ChangeCause Cause, FrameChangeRelation Relation, DataQuality Quality);
public sealed record ObservedVisualProperty(VisualProperty Property, TypedValue Value, long TimestampQpc, ChangeCause Cause);

public enum ResourceKind { Unknown, Texture2D, CompositionSurface, CompositionBrush, Bitmap, RenderTarget, Buffer, Geometry, Other }
public enum ResourceKindEvidence { Unavailable, CreationApi, RuntimeType, RuntimeBridge }
public sealed record ResourceClassification(ResourceKind Kind, ResourceKindEvidence Evidence, string Source)
{
    public void Validate()
    {
        Require.Enum(Kind); Require.Enum(Evidence); Require.Text(Source);
        if ((Kind == ResourceKind.Unknown) != (Evidence == ResourceKindEvidence.Unavailable))
            throw new ArgumentException("Known resource kind needs authoritative evidence; unknown kind must be explicit.");
    }
}

public enum TreeMembershipKind { Unknown, Logical, Visual, Composition }
public enum XamlObjectKind { Unknown, DependencyObject, UIElement, NotApplicable }
public enum ClassificationEvidence { Unavailable, RuntimeType, RuntimeBridge }
public sealed record XamlTypeClassification(XamlObjectKind Kind, ClassificationEvidence Evidence, string Source)
{
    public void Validate()
    {
        Require.Enum(Kind); Require.Enum(Evidence); Require.Text(Source);
        if ((Kind == XamlObjectKind.Unknown) != (Evidence == ClassificationEvidence.Unavailable))
            throw new ArgumentException("XAML type classification requires authoritative evidence, not a display name.");
    }
}

[method: JsonConstructor]
public readonly record struct PhysicalAdapterKey(uint LuidLowPart, int LuidHighPart, uint PhysicalAdapterIndex);
public enum GpuEngineClass { Unknown, ThreeD, Copy, Compute, VideoDecode, VideoEncode, VideoProcessing, Other }
[method: JsonConstructor]
public readonly record struct GpuEngineKey(uint EngineId, GpuEngineClass Class);
public enum GpuMetricScope { PerEngine, OverallBusiestEngine, PerAdapterMemory }
public sealed record GpuSampleIdentity(PhysicalAdapterKey? Adapter, GpuMetricScope Scope, GpuEngineKey? Engine)
{
    public void Validate(MetricKind metric)
    {
        Require.Enum(Scope);
        if (Adapter is { LuidLowPart: 0, LuidHighPart: 0 }) throw new ArgumentException("Unknown adapter identity must be null, not a fabricated zero LUID.");
        if (Engine is { } engine) { Require.Enum(engine.Class); if (Adapter is null) throw new ArgumentException("Engine identity needs its physical adapter."); }
        if (Scope == GpuMetricScope.PerAdapterMemory)
        {
            if (!ObservationRules.IsGpuMemory(metric) || Adapter is null || Engine is not null)
                throw new ArgumentException("GPU memory is scoped to a physical adapter, not a utilization engine.");
        }
        else if (metric != MetricKind.GpuUtilization || Scope == GpuMetricScope.PerEngine && (Adapter is null || Engine is null))
            throw new ArgumentException("GPU utilization scope/engine identity does not match the metric.");
    }
}
