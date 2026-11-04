using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Ingestion.NativeMetricAbi;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record ProcessCpuSample(ProcessInstanceKey Process,
ulong ClockId,
ulong Begin100ns,
ulong End100ns,
string Source,
uint Status,
TypedValue SystemUtilization,
TypedValue ProcessUtilization,
TypedValue InterruptUtilization,
TypedValue OtherOrUnattributedUtilization,
TypedValue ProcessUtility)
{
    internal string SourceVersion => "capture-1/normalize-2";
}

internal static class CpuMetricCodec
{
    internal static CpuCaptureResult DecodeCapture(in CpuCaptureResult capture, ProcessInstanceKey target)
    {
        var snapshot = capture.Snapshot;
        if (capture.Size != SizeOf<CpuCaptureResult>() ||
capture.Version != CaptureVersion ||
capture.ErrorDomain > ErrorDomain.NtStatus ||
snapshot.Size != SizeOf<CpuSnapshot>() ||
snapshot.Version != CpuVersion ||
snapshot.Source is not (MetricSource.NtGroupProcessTimesCompatibility or MetricSource.NtProcessSnapshotCpu) ||
snapshot.Status > MetricStatus.IdentityChanged ||
snapshot.Reserved != 0)
        {
            throw new InvalidDataException("Invalid native CPU capture headers.");
        }

        RequireProcess(in snapshot.Process, target);
        var status = snapshot.Status;
        var error = capture.ErrorCode;
        var domain = capture.ErrorDomain;
        var begin = capture.Begin100ns;
        var end = capture.End100ns;
        var timestamp = snapshot.Timestamp100ns;
        if (snapshot.ClockId != Clock.Boot100ns ||
begin > end ||
end > long.MaxValue ||
(status == MetricStatus.Ok ||
timestamp != 0) &&
(timestamp < begin ||
timestamp > end) ||
status == MetricStatus.Ok &&
(error != 0 ||
domain != ErrorDomain.None) ||
error != 0 &&
domain == ErrorDomain.None)
        {
            throw new InvalidDataException("Invalid native CPU capture clock, acquisition interval or error domain.");
        }

        var count = snapshot.GroupCount;
        var expected = snapshot.ExpectedGroupCount;
        var present = snapshot.TargetPresent;
        if (count > MaxCpuGroups ||
expected > MaxCpuGroups ||
count > expected ||
present > 1 ||
status == MetricStatus.Ok &&
(count == 0 ||
count != expected ||
present != 1) ||
status != MetricStatus.Ok &&
(count != 0 ||
present != 0))
        {
            throw new InvalidDataException("Invalid native CPU capture group count or availability.");
        }

        var ids = new HashSet<uint>();
        for (var index = 0; index < count; index++)
        {
            var group = capture.Groups[index];
            if (group.Size != SizeOf<CpuGroup>() ||
group.Version != CpuVersion ||
group.GroupId >= MaxCpuGroups ||
!ids.Add(group.GroupId) ||
group.LogicalProcessorCount is 0 or > MaxLogicalProcessorsPerGroup)
            {
                throw new InvalidDataException("Invalid or duplicate native CPU group.");
            }
        }

        return capture;
    }

    internal static ProcessCpuSample Decode(in CpuResult result, ProcessInstanceKey target)
    {
        if (result.Size != SizeOf<CpuResult>() ||
result.Version != CpuVersion ||
result.Source is not (MetricSource.NtGroupProcessTimesCompatibility or MetricSource.NtProcessSnapshotCpu) ||
result.Status > MetricStatus.IdentityChanged)
        {
            throw new InvalidDataException("Invalid native CPU result size, version, source or status.");
        }

        RequireProcess(in result.Process, target);
        var status = result.Status;
        if (result.ClockId != Clock.Boot100ns ||
result.Begin100ns > result.End100ns ||
result.End100ns > long.MaxValue ||
status == MetricStatus.Ok &&
result.Begin100ns == result.End100ns)
        {
            throw new InvalidDataException("Invalid native CPU acquisition clock or interval.");
        }

        var system = ReadPercent(in result.SystemUtilization);
        var process = ReadPercent(in result.ProcessUtilization);
        var interrupt = ReadPercent(in result.InterruptUtilization);
        var other = ReadPercent(in result.OtherOrUnattributedUtilization);
        var utility = ReadPercent(in result.ProcessUtility);
        var measured = new[]
        {
            system,
            process,
            interrupt,
            other
        };
        var metrics = new[]
        {
            result.SystemUtilization,
            result.ProcessUtilization,
            result.InterruptUtilization,
            result.OtherOrUnattributedUtilization
        };
        if (result.ProcessUtility.Status != MetricStatus.Unavailable ||
status == MetricStatus.Ok &&
measured.Any(value => value.Kind == ValueKind.Missing) ||
status is not (MetricStatus.Ok or MetricStatus.Partial) &&
metrics.Any(value => value.Status != status))
        {
            throw new InvalidDataException("Native CPU result and per-measurement statuses disagree.");
        }

        if (status == MetricStatus.Partial)
        {
            var allMissing = metrics.All(value => value.Status == MetricStatus.Partial);
            var asynchronous = result.SystemUtilization.Status == MetricStatus.Ok && result.ProcessUtilization.Status == MetricStatus.Partial && result.InterruptUtilization.Status == MetricStatus.Ok && result.OtherOrUnattributedUtilization.Status == MetricStatus.Partial;
            if (!allMissing && !asynchronous)
            {
                throw new InvalidDataException("Invalid partial native CPU measurement shape.");
            }
        }

        return new(target,
(ulong)result.ClockId,
result.Begin100ns,
result.End100ns,
result.Source == MetricSource.NtProcessSnapshotCpu ? "NtQuerySystemInformation/SystemProcessInformation + NtQuerySystemInformationEx" : "NtQuerySystemInformationEx/GetProcessTimes compatibility",
(uint)status,
system,
process,
interrupt,
other,
utility);
    }

    private static TypedValue ReadPercent(in PercentMetric metric)
    {
        var status = metric.Status;
        var value = metric.Percent;
        if (status > MetricStatus.IdentityChanged ||
metric.Reserved != 0 ||
status == MetricStatus.Ok &&
(!double.IsFinite(value) ||
value is < 0 or > 100) ||
status != MetricStatus.Ok &&
!double.IsNaN(value))
        {
            throw new InvalidDataException("Invalid native CPU percentage.");
        }

        return status == MetricStatus.Ok ? new(ValueKind.Number, ValueUnit.Percent, null, value, null, null, DataQuality.Complete) : new(ValueKind.Missing,
ValueUnit.Percent,
null,
null,
null,
null,
new(MeasurementKind.Unavailable,
CoverageState.Unavailable,
FormattableString.Invariant($"Native CPU status {(uint)status}.")));
    }
}
