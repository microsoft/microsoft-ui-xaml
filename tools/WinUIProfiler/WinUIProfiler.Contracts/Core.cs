using System.Text.Json.Serialization;

namespace WinUIProfiler.Contracts;

[method: JsonConstructor]
public readonly record struct ProcessInstanceKey(uint ProcessId, long? CreationTimeFileTime, Guid InstanceId)
{
    public uint ProcessId { get; } = Require.Positive(ProcessId);
    public long? CreationTimeFileTime { get; } = CreationTimeFileTime is null or > 0
        ? CreationTimeFileTime : throw new ArgumentOutOfRangeException(nameof(CreationTimeFileTime));
    public Guid InstanceId { get; } = Require.Id(InstanceId);
    public void Validate() { Require.Positive(ProcessId); Require.Id(InstanceId); }
}
public enum IdentityDomain { XamlCore, XamlPeer, CompNode, CompositionVisual, Resource }
[method: JsonConstructor]
public readonly record struct ObjectKey(ProcessInstanceKey Process, IdentityDomain Domain, Guid ProducerEpoch, ulong RuntimeObjectId)
{
    public ProcessInstanceKey Process { get; } = Require.Process(Process);
    public IdentityDomain Domain { get; } = Require.Enum(Domain);
    public Guid ProducerEpoch { get; } = Require.Id(ProducerEpoch);
    public ulong RuntimeObjectId { get; } = Require.Positive(RuntimeObjectId);
    public void Validate()
    {
        Process.Validate(); Require.Enum(Domain); Require.Id(ProducerEpoch); Require.Positive(RuntimeObjectId);
    }
}
public enum MeasurementKind { Measured, Estimated, Unavailable }
public enum CoverageState { Complete, Partial, Unavailable }
public sealed record DataQuality(MeasurementKind Kind, CoverageState Coverage, string? Reason)
{
    public MeasurementKind Kind { get; } = Require.Enum(Kind);
    public CoverageState Coverage { get; } = Require.Enum(Coverage);
    public string? Reason { get; } = Kind != MeasurementKind.Measured || Coverage != CoverageState.Complete
        ? Require.Text(Reason) : Reason;
    public static DataQuality Complete { get; } = new(MeasurementKind.Measured, CoverageState.Complete, null);
}
[method: JsonConstructor]
public readonly record struct TimeRange(long StartQpc, long EndQpc)
{
    public long StartQpc { get; } = Require.Nonnegative(StartQpc);
    public long EndQpc { get; } = EndQpc >= StartQpc ? EndQpc : throw new ArgumentOutOfRangeException(nameof(EndQpc));
}
public sealed record EventIdentity(Guid ProviderId, string Name, ushort EventId, byte Version, ulong Keywords)
{
    public Guid ProviderId { get; } = Require.Id(ProviderId);
    public string Name { get; } = Require.Text(Name);
    public ushort EventId { get; } = EventId > 0 ? EventId : throw new ArgumentOutOfRangeException(nameof(EventId));
    public byte Version { get; } = Version > 0 ? Version : throw new ArgumentOutOfRangeException(nameof(Version));
}
public sealed record EventEnvelope(
    Guid SessionId, ProcessInstanceKey Process, uint ProducerProcessId, uint ThreadId,
    long TimestampQpc, long IngestOrdinal, ulong? SourceSequence,
    Guid? ActivityId, EventIdentity Identity, ReadOnlyMemory<byte> OwnedPayload)
{
    public Guid SessionId { get; } = Require.Id(SessionId);
    public ProcessInstanceKey Process { get; } = Require.Process(Process);
    public uint ProducerProcessId { get; } = Require.Positive(ProducerProcessId);
    public uint ThreadId { get; } = Require.Positive(ThreadId);
    public long TimestampQpc { get; } = Require.Nonnegative(TimestampQpc);
    public long IngestOrdinal { get; } = Require.Positive(IngestOrdinal);
    public ulong? SourceSequence { get; } = SourceSequence is null or > 0
        ? SourceSequence : throw new ArgumentOutOfRangeException(nameof(SourceSequence));
    public Guid? ActivityId { get; } = ActivityId is null ? null : Require.Id(ActivityId.Value);
    public EventIdentity Identity { get; } = Require.NotNull(Identity);
    private readonly byte[] payload = Require.Payload(OwnedPayload);
    // Never expose the owned backing array, even through MemoryMarshal.TryGetArray.
    public ReadOnlyMemory<byte> OwnedPayload => payload.ToArray();
}

public static class QpcTime
{
    public static TimeSpan Elapsed(TimeRange range, long frequency)
    {
        Require.Positive(frequency);
        return TimeSpan.FromTicks(checked((long)((Int128)(range.EndQpc - range.StartQpc) * TimeSpan.TicksPerSecond / frequency)));
    }
}

public static class Require
{
    public static Guid Id(Guid value) => value != Guid.Empty ? value : throw new ArgumentException("An identity cannot be empty.");
    public static string Text(string? value) => !string.IsNullOrWhiteSpace(value) ? value : throw new ArgumentException("Text is required.");
    public static T NotNull<T>(T? value) where T : class => value ?? throw new ArgumentNullException(nameof(value));
    public static T Enum<T>(T value) where T : struct, Enum => System.Enum.IsDefined(value) ? value : throw new ArgumentOutOfRangeException(nameof(value));
    public static uint Positive(uint value) => value > 0 ? value : throw new ArgumentOutOfRangeException(nameof(value));
    public static ulong Positive(ulong value) => value > 0 ? value : throw new ArgumentOutOfRangeException(nameof(value));
    public static long Positive(long value) => value > 0 ? value : throw new ArgumentOutOfRangeException(nameof(value));
    public static long Nonnegative(long value) => value >= 0 ? value : throw new ArgumentOutOfRangeException(nameof(value));
    public static ProcessInstanceKey Process(ProcessInstanceKey value) { value.Validate(); return value; }
    public static byte[] Payload(ReadOnlyMemory<byte> value) => value.Length is > 0 and <= ContractVersions.MaximumPayloadBytes
        ? value.ToArray() : throw new ArgumentOutOfRangeException(nameof(value));
}

public static class ContractVersions
{
    public const int Schema = 1;
    public const int MaximumPayloadBytes = 1_048_576;
}

[method: JsonConstructor]
public readonly record struct ProducerStreamKey(Guid ProviderId, uint ProducerProcessId, Guid ProducerEpoch)
{
    public Guid ProviderId { get; } = Require.Id(ProviderId);
    public uint ProducerProcessId { get; } = Require.Positive(ProducerProcessId);
    public Guid ProducerEpoch { get; } = Require.Id(ProducerEpoch);
    public void Validate() { Require.Id(ProviderId); Require.Positive(ProducerProcessId); Require.Id(ProducerEpoch); }
}
public sealed record NormalizedEvent(ProducerStreamKey Stream, EventEnvelope Envelope)
{
    public SequenceDomainKey SequenceDomain => new(Stream, Envelope.Process, Envelope.Identity.EventId, Envelope.Identity.Version);
}
