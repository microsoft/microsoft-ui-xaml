using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine;

public static class SequenceContinuity
{
    public static SequenceGapAssessment Assess(SequenceCheckpoint? previous, SequenceCheckpoint current, ListenerDomainInterval interval)
    {
        Require.NotNull(current); Require.NotNull(interval); current.Domain.Validate(); interval.Domain.Validate();
        Require.Id(current.ListenerId); Require.Id(current.ContinuityId); Require.Positive(current.Sequence); Require.Nonnegative(current.TimestampQpc);
        Require.Id(interval.ListenerId); Require.Id(interval.ContinuityId); Require.NotNull(interval.Quality);
        if (current.Domain != interval.Domain || current.ListenerId != interval.ListenerId || current.ContinuityId != interval.ContinuityId)
            throw new ArgumentException("Current checkpoint is not in the declared listener/domain continuity segment.");
        if (previous is null) return new(SequenceContinuityState.Unproven, null, "No previous checkpoint in this listener domain.");
        previous.Domain.Validate(); Require.Id(previous.ListenerId); Require.Id(previous.ContinuityId);
        Require.Positive(previous.Sequence); Require.Nonnegative(previous.TimestampQpc);
        if (previous.Domain != current.Domain || previous.ListenerId != current.ListenerId)
            throw new ArgumentException("Checkpoints from different event delivery domains/listeners cannot establish loss.");
        if (previous.ContinuityId != current.ContinuityId || interval.Quality.Coverage != CoverageState.Complete ||
            interval.Quality.Kind != MeasurementKind.Measured ||
            previous.TimestampQpc < interval.Interval.StartQpc || current.TimestampQpc >= interval.Interval.EndQpc ||
            previous.TimestampQpc > current.TimestampQpc || current.Sequence <= previous.Sequence)
            return new(SequenceContinuityState.Unproven, null, "Filtering/enable continuity, source order or delivery coverage is not proven.");
        var missing = current.Sequence - previous.Sequence - 1;
        return missing == 0 ? new(SequenceContinuityState.Contiguous, 0, null)
            : new(SequenceContinuityState.GapDetected, missing, "Missing observations within a continuously enabled delivery-homogeneous event domain.");
    }
}
