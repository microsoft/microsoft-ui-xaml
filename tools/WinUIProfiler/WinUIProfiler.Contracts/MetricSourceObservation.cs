namespace WinUIProfiler.Contracts;

public enum MetricClock { Boot100ns = 2, UtcFileTime100ns = 3 }
public enum MetricSampleStatus { Measured, Warmup, Unavailable, Partial, CounterReset, TargetMissing, IdentityChanged }
public enum MetricErrorDomain { None, Win32, NtStatus, HResult }

public sealed record MetricSourceObservation(
    MetricClock Clock, long Frequency, ulong Begin, ulong End, MetricSampleStatus Status,
    ulong AcquisitionBegin, ulong AcquisitionEnd, uint NativeError, MetricErrorDomain ErrorDomain, uint PdhError)
{
    public void Validate()
    {
        if (!Enum.IsDefined(Clock) || !Enum.IsDefined(Status) || !Enum.IsDefined(ErrorDomain) ||
            Frequency != 10_000_000 || Begin > long.MaxValue || End > long.MaxValue ||
            Begin > End && (Clock != MetricClock.UtcFileTime100ns || Status != MetricSampleStatus.CounterReset) ||
            AcquisitionBegin > AcquisitionEnd || AcquisitionEnd > long.MaxValue ||
            (NativeError == 0) != (ErrorDomain == MetricErrorDomain.None))
            throw new ArgumentException("Invalid native metric source clock, interval, status or error provenance.");
    }
}
