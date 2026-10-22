namespace WinUIProfiler.Contracts;

public enum ValueUnit { Count, Bytes, Percent, QpcTicks, Pixels, Boolean, Scalar, String }
public enum ValueKind { Integer, Number, Boolean, Text, Missing }
public sealed record TypedValue(ValueKind Kind, ValueUnit Unit, long? Integer, double? Number, bool? Boolean, string? Text, DataQuality Quality)
{
    public void Validate()
    {
        Require.Enum(Kind); Require.Enum(Unit); Require.NotNull(Quality);
        var count = (Integer.HasValue ? 1 : 0) + (Number.HasValue ? 1 : 0) + (Boolean.HasValue ? 1 : 0) + (Text is not null ? 1 : 0);
        if (Kind == ValueKind.Missing)
        {
            if (count != 0 || Quality.Kind != MeasurementKind.Unavailable)
                throw new ArgumentException("Missing values require unavailable quality and no value.");
            return;
        }

        if (count != 1 || Quality.Kind == MeasurementKind.Unavailable ||
            Kind is ValueKind.Integer or ValueKind.Number && Unit is ValueUnit.Boolean or ValueUnit.String ||
            Kind == ValueKind.Integer && !Integer.HasValue ||
            Kind == ValueKind.Number && (!Number.HasValue || !double.IsFinite(Number.Value)) ||
            Kind == ValueKind.Boolean && (!Boolean.HasValue || Unit != ValueUnit.Boolean) ||
            Kind == ValueKind.Text && (Text is null || Unit != ValueUnit.String))
            throw new ArgumentException("Value discriminator, unit and value must agree.");
    }
}

public static class ValueRules
{
    public static void NonnegativeBytes(TypedValue value)
    {
        Require.NotNull(value).Validate();
        if (value.Unit != ValueUnit.Bytes || value.Kind is not (ValueKind.Integer or ValueKind.Number or ValueKind.Missing) ||
            value.Integer is < 0 || value.Number is < 0)
            throw new ArgumentException("Object size must be nonnegative numeric Bytes or explicitly missing.");
    }
}

public enum HostArchitecture { X64, Arm64 }
public enum SessionMode { LiveLaunch, LiveAttach, Offline }
public enum SessionStatus { Recording, Completed, StoppedAtLimit, Failed, Stopped }
public enum Capability { ObjectLifetimes, XamlTopology, CompositionTopology, ResourceBacking, TracedAllocator, LightweightSnapshot, OwnershipGraph, NativeHeap, ManagedHeap, Cpu, Gpu, ProcessMemory }
public sealed record CapabilityInfo(Capability Capability, int Version, DataQuality Quality);
public sealed record ProducerInfo(Guid ProviderId, Guid Epoch, int SchemaVersion, string Build, HostArchitecture Architecture, IReadOnlyList<CapabilityInfo> Capabilities);

public enum OwnershipEvidence { Proven, UncertainNativeRetention }
public sealed record SnapshotObject(ObjectKey Key, string? TypeName, ulong? Address, TypedValue Size, XamlTypeClassification? Classification = null);
public sealed record OwnershipEdge(ObjectKey From, ObjectKey To, string Relationship, OwnershipEvidence Evidence, DataQuality Quality);
public sealed record RetentionPath(ObjectKey Object, IReadOnlyList<OwnershipEdge> Edges, string RootKind, OwnershipEvidence Evidence, DataQuality Quality) : IQueryValue;
