using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Ingestion.NativeMetricAbi;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record ProcessMemorySample(ProcessInstanceKey Process,
ulong ClockId,
ulong Begin100ns,
ulong End100ns,
string Source,
string SourceVersion,
uint NativeError,
TypedValue PrivateWorkingSet,
TypedValue TotalWorkingSet,
TypedValue PrivateCommit)
{
    internal uint Status
    {
        get; init;
    }
}

internal static class MetricSampler
{
    internal static ProcessMemorySample DecodeMemory(in MemoryResult result, ProcessInstanceKey target)
    {
        if (result.Size != SizeOf<MemoryResult>() || result.Version != MemoryVersion)
        {
            throw new InvalidDataException("Unsupported native memory result size or version.");
        }

        var source = result.Source;
        var status = result.Status;
        var error = result.Win32Error;
        if (source is not (MetricSource.ProcessMemoryEx2 or MetricSource.ProcessMemoryEx or MetricSource.NtProcessSnapshotMemory) ||
status > MetricStatus.IdentityChanged ||
result.Reserved != 0)
        {
            throw new InvalidDataException("Invalid native memory result source, status or reserved fields.");
        }

        RequireProcess(in result.Process, target);
        if (result.ClockId != Clock.Boot100ns || result.Begin100ns > result.End100ns || result.End100ns > long.MaxValue)
        {
            throw new InvalidDataException("Invalid native memory acquisition clock or interval.");
        }

        var privateWorkingSet = ReadBytes(in result.PrivateWorkingSet, error);
        var totalWorkingSet = ReadBytes(in result.TotalWorkingSet, error);
        var privateCommit = ReadBytes(in result.PrivateCommit, error);
        var measured = (privateWorkingSet.Kind != ValueKind.Missing ? 1 : 0) + (totalWorkingSet.Kind != ValueKind.Missing ? 1 : 0) + (privateCommit.Kind != ValueKind.Missing ? 1 : 0);
        if (status == MetricStatus.Ok &&
(measured != 3 ||
error != 0) ||
status == MetricStatus.Partial &&
measured is not (1 or 2) ||
status is not (MetricStatus.Ok or MetricStatus.Partial) &&
(measured != 0 ||
result.PrivateWorkingSet.Status != status ||
result.TotalWorkingSet.Status != status ||
result.PrivateCommit.Status != status))
        {
            throw new InvalidDataException("Native memory result and per-measurement statuses disagree.");
        }

        if (source == MetricSource.ProcessMemoryEx && privateWorkingSet.Kind != ValueKind.Missing)
        {
            throw new InvalidDataException("PROCESS_MEMORY_COUNTERS_EX does not report private working set.");
        }

        if (privateWorkingSet.Integer.HasValue &&
totalWorkingSet.Integer.HasValue &&
privateWorkingSet.Integer.Value > totalWorkingSet.Integer.Value)
        {
            throw new InvalidDataException("Private working set exceeds total working set.");
        }

        return new(target,
(ulong)result.ClockId,
result.Begin100ns,
result.End100ns,
source == MetricSource.NtProcessSnapshotMemory ? "NtQuerySystemInformation/SystemProcessInformation" : source == MetricSource.ProcessMemoryEx2 ? "K32GetProcessMemoryInfo/PROCESS_MEMORY_COUNTERS_EX2" : "K32GetProcessMemoryInfo/PROCESS_MEMORY_COUNTERS_EX",
"1",
error,
privateWorkingSet,
totalWorkingSet,
privateCommit)
        {
            Status = (uint)status
        };
    }

    private static TypedValue ReadBytes(in ByteMetric metric, uint error)
    {
        var status = metric.Status;
        var value = metric.Value;
        if (status > MetricStatus.IdentityChanged ||
metric.Units != ByteUnit.Bytes ||
status == MetricStatus.Ok &&
value > long.MaxValue ||
status != MetricStatus.Ok &&
value != ulong.MaxValue)
        {
            throw new InvalidDataException("Invalid native byte measurement.");
        }

        if (status == MetricStatus.Ok)
        {
            return new(ValueKind.Integer, ValueUnit.Bytes, checked((long)value), null, null, null, DataQuality.Complete);
        }

        var reason = FormattableString.Invariant($"Native memory status {(uint)status}; Win32 error {error}.");
        return new(ValueKind.Missing,
ValueUnit.Bytes,
null,
null,
null,
null,
new(MeasurementKind.Unavailable, CoverageState.Unavailable, reason));
    }
}
