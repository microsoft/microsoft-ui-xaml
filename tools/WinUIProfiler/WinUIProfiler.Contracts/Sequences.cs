using System.Text.Json.Serialization;

namespace WinUIProfiler.Contracts;

[method: JsonConstructor]
public readonly record struct SequenceDomainKey(ProducerStreamKey Stream, ProcessInstanceKey Target, ushort EventId, byte EventVersion)
{
    public void Validate()
    {
        Stream.Validate(); Target.Validate();
        if (Stream.ProviderId == EventCatalog.RuntimeProviderId && Stream.ProducerProcessId != Target.ProcessId)
            throw new ArgumentException("Runtime delivery domains originate in their semantic target process.");
        var definition = EventCatalog.Get(Stream.ProviderId, EventId);
        if (EventVersion != definition.Identity.Version) throw new NotSupportedException("Unsupported delivery-domain event version.");
    }
}
public sealed record SequenceDomainRange(SequenceDomainKey Domain, ulong? BeforeSequence, ulong? ThroughSequence, DataQuality Quality);
public sealed record ListenerDomainInterval(Guid ListenerId, Guid ContinuityId, SequenceDomainKey Domain, TimeRange Interval, DataQuality Quality);
public sealed record SequenceCheckpoint(Guid ListenerId, Guid ContinuityId, SequenceDomainKey Domain, ulong Sequence, long TimestampQpc);
public enum SequenceContinuityState { Contiguous, GapDetected, Unproven }
public sealed record SequenceGapAssessment(SequenceContinuityState State, ulong? MissingObservations, string? Reason);
