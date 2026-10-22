using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine;

public static class SequenceContinuity
{
    public static SequenceGapAssessment Assess(SequenceObservationWindow window, ListenerDomainInterval interval,
        ProducerSequenceCut? producerCut = null, SequenceDeliveryClosure? delivery = null, SequenceReceiptEvidence? receipt = null)
    {
        Require.NotNull(window); Require.NotNull(window.Observations); Require.NotNull(interval);
        interval.Domain.Validate(); Require.Id(interval.ListenerId); Require.Id(interval.ContinuityId); Require.NotNull(interval.Quality);
        if (window.AfterSequence > window.ThroughSequence || window.Observations.Count > 8192)
            throw new ArgumentException("Observation window is inverted or exceeds its bounded checkpoint count.");
        var observations = window.Observations.ToArray();
        var ordinals = new HashSet<long>();
        var observed = new HashSet<ulong>();
        var continuous = interval.Quality.Kind == MeasurementKind.Measured && interval.Quality.Coverage == CoverageState.Complete &&
            interval.Interval.StartQpc < interval.Interval.EndQpc;
        foreach (var observation in observations)
        {
            Require.NotNull(observation); observation.Domain.Validate(); Require.Id(observation.ListenerId); Require.Id(observation.ContinuityId);
            Require.Positive(observation.Sequence); Require.Nonnegative(observation.TimestampQpc); Require.Positive(observation.IngestOrdinal);
            if (observation.Domain != interval.Domain || observation.ListenerId != interval.ListenerId ||
                observation.Sequence < window.AfterSequence || observation.Sequence > window.ThroughSequence || !ordinals.Add(observation.IngestOrdinal))
                throw new ArgumentException("Checkpoint has a foreign listener/domain, lies outside the sequence window or repeats an ingestion ordinal.");
            continuous &= observation.ContinuityId == interval.ContinuityId &&
                observation.TimestampQpc >= interval.Interval.StartQpc && observation.TimestampQpc < interval.Interval.EndQpc;
            if (observation.Sequence > window.AfterSequence) observed.Add(observation.Sequence);
        }
        if (producerCut is not null)
        {
            producerCut.Validate();
            if (producerCut.Domain != interval.Domain) throw new ArgumentException("Producer cut belongs to a different delivery domain.");
        }
        if (delivery is not null)
        {
            Require.Id(delivery.ListenerId); Require.Id(delivery.ContinuityId); Require.Id(delivery.FinalizedAfterCutId);
            Require.NotNull(delivery.SealedSource).Validate();
            Require.Nonnegative(delivery.FinalByteLength); Require.Nonnegative(delivery.EnumeratedByteLength); Require.Nonnegative(delivery.DecodedThroughOrdinal);
            if (delivery.FinalIngestOrdinal is { } final) Require.Nonnegative(final);
            if (delivery.ListenerId != interval.ListenerId || delivery.ContinuityId != interval.ContinuityId ||
                producerCut is not null && delivery.FinalizedAfterCutId != producerCut.CutId ||
                delivery.EnumeratedByteLength > delivery.FinalByteLength || delivery.DecodedThroughOrdinal > delivery.FinalIngestOrdinal ||
                delivery.EnabledAfterSequence > delivery.EnabledThroughSequence ||
                observations.Any(o => o.IngestOrdinal > delivery.FinalIngestOrdinal))
                throw new ArgumentException("Delivery closure has a foreign listener/cut or inconsistent enumeration/decoding bounds.");
        }
        if (receipt is not null)
        {
            Require.Id(receipt.ListenerId); Require.Id(receipt.ContinuityId); Require.Id(receipt.SettledCutId);
            Require.Nonnegative(receipt.ProcessedThroughOrdinal);
            if (receipt.ListenerId != interval.ListenerId || receipt.ContinuityId != interval.ContinuityId ||
                producerCut is not null && receipt.SettledCutId != producerCut.CutId ||
                receipt.EnabledAfterSequence > receipt.EnabledThroughSequence)
                throw new ArgumentException("Live receipt has a foreign listener/cut or inverted eligibility bounds.");
            var processed = observations.Where(o => o.Sequence > window.AfterSequence && o.IngestOrdinal <= receipt.ProcessedThroughOrdinal)
                .Select(o => o.Sequence).ToHashSet();
            var emptyEligible = window.AfterSequence == window.ThroughSequence && observations.Length == 0 &&
                receipt.EnabledAfterSequence == window.AfterSequence && receipt.EnabledThroughSequence == window.ThroughSequence &&
                interval.Quality.Kind == MeasurementKind.Measured && interval.Quality.Coverage == CoverageState.Complete;
            if ((continuous || emptyEligible) && producerCut is not null && producerCut.SettledThrough >= window.ThroughSequence &&
                window.AfterSequence >= receipt.EnabledAfterSequence && window.ThroughSequence <= receipt.EnabledThroughSequence &&
                (ulong)processed.Count == window.ThroughSequence - window.AfterSequence)
                return new(SequenceContinuityState.Contiguous, 0, null);
        }
        if (!continuous || producerCut is null || producerCut.SettledThrough < window.ThroughSequence || delivery is null ||
            delivery.EnumeratedByteLength != delivery.FinalByteLength || delivery.FinalIngestOrdinal is null ||
            delivery.DecodedThroughOrdinal != delivery.FinalIngestOrdinal ||
            window.AfterSequence < delivery.EnabledAfterSequence || window.ThroughSequence > delivery.EnabledThroughSequence)
            return new(SequenceContinuityState.Unproven, null, "Requires eligible completed receipt of every settled sequence; absent observations remain pending without adequate closed-source proof.");
        var missing = window.ThroughSequence - window.AfterSequence - (ulong)observed.Count;
        return missing == 0 ? new(SequenceContinuityState.Contiguous, 0, null)
            : new(SequenceContinuityState.GapDetected, missing, "Missing observations after producer settlement and closed-source delivery/reordering reconciliation.");
    }

    public static SequenceGapAssessment Assess(SequenceCheckpoint? previous, SequenceCheckpoint current, ListenerDomainInterval interval)
    {
        Require.NotNull(current); Require.NotNull(interval); current.Domain.Validate(); interval.Domain.Validate();
        Require.Id(current.ListenerId); Require.Id(current.ContinuityId); Require.Positive(current.Sequence); Require.Nonnegative(current.TimestampQpc); Require.Positive(current.IngestOrdinal);
        Require.Id(interval.ListenerId); Require.Id(interval.ContinuityId); Require.NotNull(interval.Quality);
        if (current.Domain != interval.Domain || current.ListenerId != interval.ListenerId || current.ContinuityId != interval.ContinuityId)
            throw new ArgumentException("Current checkpoint is not in the declared listener/domain continuity segment.");
        if (previous is null) return new(SequenceContinuityState.Unproven, null, "No previous checkpoint in this listener domain.");
        previous.Domain.Validate(); Require.Id(previous.ListenerId); Require.Id(previous.ContinuityId);
        Require.Positive(previous.Sequence); Require.Nonnegative(previous.TimestampQpc); Require.Positive(previous.IngestOrdinal);
        if (previous.Domain != current.Domain || previous.ListenerId != current.ListenerId)
            throw new ArgumentException("Checkpoints from different event delivery domains/listeners cannot establish loss.");
        if (previous.ContinuityId != current.ContinuityId || interval.Quality.Coverage != CoverageState.Complete ||
            interval.Quality.Kind != MeasurementKind.Measured ||
            previous.TimestampQpc < interval.Interval.StartQpc || current.TimestampQpc >= interval.Interval.EndQpc ||
            previous.TimestampQpc > current.TimestampQpc || current.Sequence <= previous.Sequence)
            return new(SequenceContinuityState.Unproven, null, "Filtering/enable continuity, source order or delivery coverage is not proven.");
        return new(SequenceContinuityState.Unproven, null, "Arrival pairs do not prove settlement or closed delivery; reconcile the complete observation window with explicit closure evidence.");
    }
}
