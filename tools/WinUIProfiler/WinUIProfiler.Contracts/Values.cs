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

public enum HostArchitecture { X64, Arm64 }
public enum SessionMode { LiveLaunch, LiveAttach, Offline }
public enum SessionStatus { Recording, Completed, StoppedAtLimit, Failed }
public enum Capability { ObjectLifetimes, XamlTopology, CompositionTopology, ResourceBacking, TracedAllocator, LightweightSnapshot, OwnershipGraph, NativeHeap, ManagedHeap, Cpu, Gpu, ProcessMemory }
public sealed record CapabilityInfo(Capability Capability, int Version, DataQuality Quality);
public sealed record ProducerInfo(Guid ProviderId, Guid Epoch, int SchemaVersion, string Build, HostArchitecture Architecture, IReadOnlyList<CapabilityInfo> Capabilities);

public enum SnapshotKind { Lightweight, NativeHeap, ManagedHeap, OwnershipGraph }
public enum SnapshotState { Capturing, Complete, Partial, Failed }
public enum OwnershipEvidence { Proven, UncertainNativeRetention }
public sealed record SnapshotMetadata(
    Guid SnapshotId, Guid SessionId, ProcessInstanceKey Process, SnapshotKind Kind,
    long CapturedQpc, long CommittedOrdinal, Guid ProducerEpoch, SnapshotState State,
    DataQuality Quality, string ArtifactRelativePath, string Sha256,
    int SchemaVersion, string SourceVersion, long QpcFrequency)
{
    public Guid SnapshotId { get; } = Require.Id(SnapshotId);
    public Guid SessionId { get; } = Require.Id(SessionId);
    public ProcessInstanceKey Process { get; } = Require.Process(Process);
    public SnapshotKind Kind { get; } = Require.Enum(Kind);
    public long CapturedQpc { get; } = Require.Nonnegative(CapturedQpc);
    public long CommittedOrdinal { get; } = Require.Nonnegative(CommittedOrdinal);
    public Guid ProducerEpoch { get; } = Require.Id(ProducerEpoch);
    public SnapshotState State { get; } = Require.Enum(State);
    public DataQuality Quality { get; } = Require.NotNull(Quality);
    public string ArtifactRelativePath { get; } = ValidatePath(ArtifactRelativePath);
    public string Sha256 { get; } = Sha256 is { Length: 64 } && Sha256.All(Uri.IsHexDigit)
        ? Sha256 : throw new ArgumentException("A SHA-256 digest must contain 64 hexadecimal characters.");
    public int SchemaVersion { get; } = SchemaVersion > 0 ? SchemaVersion : throw new ArgumentOutOfRangeException(nameof(SchemaVersion));
    public string SourceVersion { get; } = Require.Text(SourceVersion);
    public long QpcFrequency { get; } = Require.Positive(QpcFrequency);
    private static string ValidatePath(string path)
    {
        Require.Text(path);
        if (path.StartsWith('\\') || path.StartsWith('/') || path.Contains(':') ||
            path.Split(['\\', '/']).Any(part => part is "" or "." or ".."))
            throw new ArgumentException("Snapshot artifacts must be local, relative paths without traversal.");
        return path;
    }
}
public sealed record SnapshotObject(ObjectKey Key, string? TypeName, ulong? Address, TypedValue Size);
public sealed record SnapshotDelta(Guid SnapshotId, ObjectKey? Object, TypedValue Baseline, TypedValue Current, TypedValue Delta);
public sealed record OwnershipEdge(ObjectKey From, ObjectKey To, string Relationship, OwnershipEvidence Evidence, DataQuality Quality);
public sealed record RetentionPath(ObjectKey Object, IReadOnlyList<OwnershipEdge> Edges, string RootKind, OwnershipEvidence Evidence, DataQuality Quality);
