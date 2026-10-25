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
// SettledThrough is a contiguous terminal-emission prefix, not the largest write that returned.
public sealed record ProducerSequenceCut(Guid CutId, SequenceDomainKey Domain, ulong ReservedThrough, ulong SettledThrough)
{
    public void Validate()
    {
        Require.Id(CutId); Domain.Validate();
        if (SettledThrough > ReservedThrough) throw new ArgumentException("Settled emission prefix cannot exceed reserved sequences.");
    }
}
// Requires the verified transport/manifest guarantee in docs\event-contract.md, not End/flush receipt.
public sealed record SequenceDeliveryClosure(Guid ListenerId, Guid ContinuityId, Guid FinalizedAfterCutId,
    SnapshotArtifact SealedSource, long FinalByteLength, long EnumeratedByteLength, long? FinalIngestOrdinal, long DecodedThroughOrdinal,
    ulong EnabledAfterSequence, ulong EnabledThroughSequence);
// The contiguous completed decode/processing prefix is independent of source finalization.
public sealed record SequenceReceiptEvidence(Guid ListenerId, Guid ContinuityId, Guid SettledCutId,
    ulong EnabledAfterSequence, ulong EnabledThroughSequence, long ProcessedThroughOrdinal);
public sealed record SequenceObservationWindow(ulong AfterSequence, ulong ThroughSequence, IReadOnlyList<SequenceCheckpoint> Observations);
public sealed record SequenceDomainRange(SequenceDomainKey Domain, ulong? BeforeSequence, ulong? ThroughSequence, DataQuality Quality, ProducerSequenceCut? SettledCut = null);
public sealed record ListenerDomainInterval(Guid ListenerId, Guid ContinuityId, SequenceDomainKey Domain, TimeRange Interval, DataQuality Quality);
public sealed record SequenceCheckpoint(Guid ListenerId, Guid ContinuityId, SequenceDomainKey Domain, ulong Sequence, long TimestampQpc, long IngestOrdinal);
public enum SequenceContinuityState { Contiguous, GapDetected, Unproven }
public sealed record SequenceGapAssessment(SequenceContinuityState State, ulong? MissingObservations, string? Reason);
