using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Ingestion.NativeMetricAbi;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record GpuEngineSample(uint LuidHigh, uint LuidLow, uint Physical, uint Engine, TypedValue Utilization);
internal sealed record ProcessGpuSample(ProcessInstanceKey Process,
ulong ClockId,
ulong Begin100ns,
ulong End100ns,
uint Status,
TypedValue Utilization,
IReadOnlyList<GpuEngineSample> Engines,
TypedValue DedicatedCommitted,
TypedValue SharedCommitted,
TypedValue DedicatedResident,
TypedValue SharedResident)
{
    internal string Source => "English PDH GPU Engine";
    internal string SourceVersion => "1";
}

internal static class GpuMetricCodec
{
    internal static ProcessGpuSample Decode(in GpuResult result, ProcessInstanceKey target)
    {
        Header(result.Size,
SizeOf<GpuResult>(),
result.Version,
result.Source,
result.Status,
in result.Process,
result.ClockId,
target);
        var status = result.Status;
        var begin = result.Begin100ns;
        var end = result.End100ns;
        var count = result.EngineCount;
        if (begin > long.MaxValue ||
end > long.MaxValue ||
status != MetricStatus.CounterReset &&
begin > end ||
result.Utilization.Status != status ||
result.Utilization.Reserved != 0 ||
result.Reserved != 0 ||
count > MaxGpuRows ||
status == MetricStatus.Ok &&
(count == 0 ||
begin == end) ||
status != MetricStatus.Ok &&
count != 0)
        {
            throw new InvalidDataException("Invalid native GPU result interval, status or engine count.");
        }

        var utilization = Percent(status, result.Utilization.Percent);
        var engines = new List<GpuEngineSample>(checked((int)count));
        var ids = new HashSet<(uint, uint, uint, uint)>();
        for (var index = 0; index < count; index++)
        {
            var engine = result.Engines[index];
            var id = (engine.LuidHigh, engine.LuidLow, engine.Physical, engine.Engine);
            if (!ids.Add(id) || engine.Status != MetricStatus.Ok || engine.Reserved != 0)
            {
                throw new InvalidDataException("Invalid or duplicate native GPU engine.");
            }

            engines.Add(new(engine.LuidHigh, engine.LuidLow, engine.Physical, engine.Engine, Percent(MetricStatus.Ok, engine.Percent)));
        }

        if (status == MetricStatus.Ok && utilization.Number != engines.Max(engine => engine.Utilization.Number))
        {
            throw new InvalidDataException("Native GPU utilization is not the maximum engine measurement.");
        }

        return new(target,
(ulong)result.ClockId,
begin,
end,
(uint)status,
utilization,
engines.AsReadOnly(),
Memory(in result.DedicatedCommitted),
Memory(in result.SharedCommitted),
Memory(in result.DedicatedResident),
Memory(in result.SharedResident));
    }

    internal static GpuSnapshot DecodeCapture(in GpuSnapshot snapshot, ProcessInstanceKey target)
    {
        Header(snapshot.Size,
SizeOf<GpuSnapshot>(),
snapshot.Version,
snapshot.Source,
snapshot.Status,
in snapshot.Process,
snapshot.ClockId,
target);
        if (snapshot.Sample100ns > long.MaxValue ||
snapshot.Begin100ns > snapshot.End100ns ||
snapshot.End100ns > long.MaxValue ||
snapshot.AcquisitionClockId != Clock.Boot100ns ||
snapshot.RowCount > MaxGpuRows ||
snapshot.ObservedInstanceCount > MaxGpuInstances)
        {
            throw new InvalidDataException("Invalid native GPU acquisition clock, interval or row count.");
        }

        for (var index = 0; index < snapshot.RowCount; index++)
        {
            var row = snapshot.Rows[index];
            if (row.Status > MetricStatus.IdentityChanged ||
row.CounterType != Perf100nsTimer ||
row.Status == MetricStatus.Ok &&
(row.RunningTime > long.MaxValue ||
row.Timer > long.MaxValue ||
row.Timestamp100ns is 0 or > long.MaxValue))
            {
                throw new InvalidDataException("Invalid native GPU raw counter row.");
            }
        }

        return snapshot;
    }

    private static void Header(uint size,
uint expectedSize,
uint version,
MetricSource source,
MetricStatus status,
in ProcessIdentity process,
Clock clock,
ProcessInstanceKey target)
    {
        if (size != expectedSize ||
version != GpuVersion ||
source != MetricSource.EnglishPdhGpuEngine ||
status > MetricStatus.IdentityChanged ||
clock != Clock.FileTime100ns)
        {
            throw new InvalidDataException("Invalid native GPU ABI size, version, source, status or clock.");
        }

        RequireProcess(in process, target);
    }

    private static TypedValue Percent(MetricStatus status, double value)
    {
        if (status == MetricStatus.Ok &&
(!double.IsFinite(value) ||
value is < 0 or > 100) ||
status != MetricStatus.Ok &&
!double.IsNaN(value))
        {
            throw new InvalidDataException("Invalid native GPU percentage.");
        }

        return status == MetricStatus.Ok ? new(ValueKind.Number, ValueUnit.Percent, null, value, null, null, DataQuality.Complete) : new(ValueKind.Missing,
ValueUnit.Percent,
null,
null,
null,
null,
new(MeasurementKind.Unavailable,
CoverageState.Unavailable,
FormattableString.Invariant($"Native GPU status {(uint)status}.")));
    }

    private static TypedValue Memory(in ByteMetric metric)
    {
        if (metric.Status != MetricStatus.Unavailable || metric.Units != ByteUnit.Bytes || metric.Value != ulong.MaxValue)
        {
            throw new InvalidDataException("GPU ABI version 1 does not provide memory measurements.");
        }

        return new(ValueKind.Missing,
ValueUnit.Bytes,
null,
null,
null,
null,
new(MeasurementKind.Unavailable,
CoverageState.Unavailable,
"The GPU engine source does not measure committed or resident memory."));
    }
}
