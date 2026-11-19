using Microsoft.Diagnostics.Tracing;
using RenderBench.Analysis;

namespace RenderBench.Analyzer;

public static class EtlReader
{
    const string KernelProcessProvider = "Microsoft-Windows-Kernel-Process";

    public static List<TraceRecord> Read(string etlPath)
    {
        var records = new List<TraceRecord>();
        var startedPids = new HashSet<int>();
        using var source = new ETWTraceEventSource(etlPath);
        source.Dynamic.All += e =>
        {
            if (TryConvert(e) is { } record)
            {
                if (record is ProcessStartRecord start && !startedPids.Add(start.ProcessId))
                {
                    return;
                }
                records.Add(record);
            }
        };
        source.Kernel.ProcessStart += e =>
        {
            if (startedPids.Add(e.ProcessID))
            {
                records.Add(new ProcessStartRecord(e.TimeStampRelativeMSec, e.ProcessID, e.ImageFileName));
            }
        };
        source.Process();
        return records;
    }

    public static TraceRecord? TryConvert(TraceEvent e)
    {
        double t = e.TimeStampRelativeMSec;
        int pid = e.ProcessID;
        string name = EventName(e);

        if (IsProvider(e, BackendModes.RenderingBackendProvider, BackendModes.RenderingBackendProviderId))
        {
            return name switch
            {
                "BackendSelected" => new BackendSelectedRecord(t, pid, (BackendMode)U32(e, "RequestedMode"),
                    (BackendMode)U32(e, "EffectiveMode"), (FallbackReason)U32(e, "FallbackReason"), U32(e, "AvailableModes")),
                "DeviceInfo" => new DeviceInfoRecord(t, pid, (BackendMode)U32(e, "Mode"), U64(e, "AdapterLuid"),
                    Bool(e, "IsWarp"), U32(e, "FeatureLevel")),
                "FrameBegin" => new FrameBeginRecord(t, pid, U32(e, "FrameNumber")),
                "FrameEnd" => new FrameEndRecord(t, pid, U32(e, "FrameNumber"), Bool(e, "FrameDrawn")),
                "RenderWalkStats" => new RenderWalkRecord(t, pid, U32(e, "FrameNumber"), U32(e, "ElementsVisited"),
                    U32(e, "ElementsRendered")),
                "PaintBegin" => new PaintBeginRecord(t, pid, U32(e, "FrameNumber"), U64(e, "RootId")),
                "PaintEnd" => new PaintEndRecord(t, pid, U32(e, "FrameNumber"), U64(e, "RootId"), U64(e, "DamagedPixels"),
                    U64(e, "PaintedPixels"), U32(e, "PrimitiveCount"), U32(e, "DamageRectCount")),
                "ResourceInventory" => new ResourceInventoryRecord(t, pid, (BackendMode)U32(e, "Mode"), U64(e, "SurfaceBytes"),
                    U64(e, "CacheBytes"), U32(e, "CachedObjectCount"), U64(e, "UploadBytes")),
                "UnsupportedContent" => new UnsupportedContentRecord(t, pid, (BackendMode)U32(e, "Mode"), Str(e, "TypeName"),
                    Str(e, "Detail")),
                _ => null,
            };
        }

        if (IsProvider(e, BenchEventSource.ProviderName, BenchEventSource.ProviderId))
        {
            return name switch
            {
                "RunStarted" => new RunStartedRecord(t, pid, Str(e, "Scene"), Str(e, "RequestedBackend"), I32(e, "Seed"),
                    I32(e, "Width"), I32(e, "Height"), F64(e, "RasterizationScale")),
                "PhaseBegin" => new PhaseRecord(t, pid, (BenchPhase)I32(e, "Phase"), true),
                "PhaseEnd" => new PhaseRecord(t, pid, (BenchPhase)I32(e, "Phase"), false),
                "SceneReady" => new SceneReadyRecord(t, pid, Str(e, "Scene"), I32(e, "ElementCount")),
                "InputInjected" => new InputInjectedRecord(t, pid, I64(e, "InputId"), Str(e, "Kind"), Str(e, "Target")),
                "MemorySample" => new MemorySampleRecord(t, pid, (BenchPhase)I32(e, "Phase"), I64(e, "PrivateBytes"),
                    I64(e, "WorkingSetBytes"), I64(e, "PeakWorkingSetBytes"), I64(e, "GpuLocalBytes"), I64(e, "GpuNonLocalBytes"),
                    I32(e, "GdiObjects"), I32(e, "UserObjects"), F64(e, "CpuUserMs"), F64(e, "CpuKernelMs")),
                "UpdateApplied" => new UpdateAppliedRecord(t, pid, I64(e, "Tick"), I32(e, "ChangedCount")),
                "RunCompleted" => new RunCompletedRecord(t, pid, I32(e, "Status"), Str(e, "Detail")),
                _ => null,
            };
        }

        if (e.ProviderName == KernelProcessProvider && name.StartsWith("ProcessStart", StringComparison.Ordinal))
        {
            int newPid = I32(e, "ProcessID");
            return new ProcessStartRecord(t, newPid, Str(e, "ImageName"));
        }
        return null;
    }

    static bool IsProvider(TraceEvent e, string name, Guid id)
        => e.ProviderGuid == id || string.Equals(e.ProviderName, name, StringComparison.OrdinalIgnoreCase);

    // TraceLogging/EventSource names can carry an opcode suffix (e.g. "PhaseBegin/Start"); keep the task part.
    static string EventName(TraceEvent e)
    {
        string name = e.EventName ?? "";
        int slash = name.IndexOf('/');
        return slash > 0 && !name.StartsWith("ProcessStart", StringComparison.Ordinal) ? name[..slash] : name;
    }

    static object? Payload(TraceEvent e, string field)
    {
        int index = e.PayloadIndex(field);
        return index < 0 ? null : e.PayloadValue(index);
    }

    static uint U32(TraceEvent e, string field) => Payload(e, field) is { } v ? Convert.ToUInt32(v) : 0;
    static ulong U64(TraceEvent e, string field) => Payload(e, field) is { } v ? Convert.ToUInt64(v) : 0;
    static int I32(TraceEvent e, string field) => Payload(e, field) is { } v ? Convert.ToInt32(v) : 0;
    static long I64(TraceEvent e, string field) => Payload(e, field) is { } v ? Convert.ToInt64(v) : 0;
    static double F64(TraceEvent e, string field) => Payload(e, field) is { } v ? Convert.ToDouble(v) : 0;
    static string Str(TraceEvent e, string field) => Payload(e, field)?.ToString() ?? "";

    static bool Bool(TraceEvent e, string field) => Payload(e, field) switch
    {
        bool b => b,
        null => false,
        var v => Convert.ToInt64(v) != 0,
    };
}
