namespace RenderBench.Analysis;

// Normalized trace events. The ETL reader in RenderBench.Analyzer produces these; tests build them directly.
public abstract record TraceRecord(double TimeMs, int ProcessId);

// ---- Microsoft-WinUI-RenderingBackend (native) ----
public sealed record BackendSelectedRecord(double TimeMs, int ProcessId, BackendMode Requested, BackendMode Effective,
    FallbackReason Reason, uint AvailableModes) : TraceRecord(TimeMs, ProcessId);
public sealed record DeviceInfoRecord(double TimeMs, int ProcessId, BackendMode Mode, ulong AdapterLuid, bool IsWarp,
    uint FeatureLevel) : TraceRecord(TimeMs, ProcessId);
public sealed record FrameBeginRecord(double TimeMs, int ProcessId, uint Frame) : TraceRecord(TimeMs, ProcessId);
public sealed record FrameEndRecord(double TimeMs, int ProcessId, uint Frame, bool Drawn) : TraceRecord(TimeMs, ProcessId);
public sealed record RenderWalkRecord(double TimeMs, int ProcessId, uint Frame, uint Visited, uint Rendered)
    : TraceRecord(TimeMs, ProcessId);
public sealed record PaintBeginRecord(double TimeMs, int ProcessId, uint Frame, ulong RootId) : TraceRecord(TimeMs, ProcessId);
public sealed record PaintEndRecord(double TimeMs, int ProcessId, uint Frame, ulong RootId, ulong DamagedPixels,
    ulong PaintedPixels, uint PrimitiveCount, uint DamageRectCount) : TraceRecord(TimeMs, ProcessId);
public sealed record ResourceInventoryRecord(double TimeMs, int ProcessId, BackendMode Mode, ulong SurfaceBytes,
    ulong CacheBytes, uint CachedObjectCount, ulong UploadBytes) : TraceRecord(TimeMs, ProcessId);
public sealed record UnsupportedContentRecord(double TimeMs, int ProcessId, BackendMode Mode, string TypeName, string Detail)
    : TraceRecord(TimeMs, ProcessId);

// ---- Microsoft-WinUI-RenderBench (harness) ----
public sealed record RunStartedRecord(double TimeMs, int ProcessId, string Scene, string RequestedBackend, int Seed,
    int Width, int Height, double RasterizationScale) : TraceRecord(TimeMs, ProcessId);
public sealed record PhaseRecord(double TimeMs, int ProcessId, BenchPhase Phase, bool IsBegin) : TraceRecord(TimeMs, ProcessId);
public sealed record SceneReadyRecord(double TimeMs, int ProcessId, string Scene, int ElementCount) : TraceRecord(TimeMs, ProcessId);
public sealed record InputInjectedRecord(double TimeMs, int ProcessId, long InputId, string Kind, string Target)
    : TraceRecord(TimeMs, ProcessId);
public sealed record MemorySampleRecord(double TimeMs, int ProcessId, BenchPhase Phase, long PrivateBytes, long WorkingSetBytes,
    long PeakWorkingSetBytes, long GpuLocalBytes, long GpuNonLocalBytes, int GdiObjects, int UserObjects, double CpuUserMs,
    double CpuKernelMs) : TraceRecord(TimeMs, ProcessId);
public sealed record UpdateAppliedRecord(double TimeMs, int ProcessId, long Tick, int ChangedCount) : TraceRecord(TimeMs, ProcessId);
public sealed record RunCompletedRecord(double TimeMs, int ProcessId, int Status, string Detail) : TraceRecord(TimeMs, ProcessId);

// ---- Kernel ----
public sealed record ProcessStartRecord(double TimeMs, int ProcessId, string ImageName) : TraceRecord(TimeMs, ProcessId);

// ---- Begin: W6 resource churn (appended by immediate-rendering-gdiplus) ----
// Microsoft-WinUI-RenderingBackend "ResourceChurn": backend resources created/destroyed while painting one frame.
public sealed record ResourceChurnRecord(double TimeMs, int ProcessId, BackendMode Mode, uint Frame, uint Created, uint Destroyed)
    : TraceRecord(TimeMs, ProcessId);
// ---- End: W6 resource churn ----
// ---- Added by the D2D-HW branch ----
// Microsoft-WinUI-RenderingBackend GpuPaintTime: GPU time of one immediate paint (timestamp queries), read back a frame
// or two after the paint, so attribute it by Frame rather than by event time.
public sealed record GpuPaintTimeRecord(double TimeMs, int ProcessId, uint Frame, ulong GpuMicroseconds) : TraceRecord(TimeMs, ProcessId);
// Microsoft-WinUI-RenderBench GpuAdapterMemorySample: one adapter's QueryVideoMemoryInfo for the harness process.
public sealed record GpuAdapterMemoryRecord(double TimeMs, int ProcessId, BenchPhase Phase, ulong AdapterLuid, long LocalBytes,
    long NonLocalBytes) : TraceRecord(TimeMs, ProcessId);
// ---- Added by immediate-rendering-d2d-sw ----
// Native: the painter's device was removed/reset (Reason/RemovedReason are HRESULTs).
public sealed record DeviceLostRecord(double TimeMs, int ProcessId, BackendMode Mode, int Reason, int RemovedReason, uint Frame)
    : TraceRecord(TimeMs, ProcessId);
// Harness: cumulative CPU ms per ThreadCategory (W1/W2 CPU split).
public sealed record ThreadCpuSampleRecord(double TimeMs, int ProcessId, BenchPhase Phase, double UiMs, double WarpRasterizerMs,
    double D2DDWriteMs, double CompositionMs, double OtherMs, int ThreadCount) : TraceRecord(TimeMs, ProcessId)
{
    public double this[ThreadCategory category] => category switch
    {
        ThreadCategory.Ui => UiMs,
        ThreadCategory.WarpRasterizer => WarpRasterizerMs,
        ThreadCategory.D2DDWrite => D2DDWriteMs,
        ThreadCategory.Composition => CompositionMs,
        _ => OtherMs,
    };
}
public sealed record ThreadModuleSummaryRecord(double TimeMs, int ProcessId, string Module, string Category, int Threads, double CpuMs)
    : TraceRecord(TimeMs, ProcessId);
public sealed record RasterModuleVersionRecord(double TimeMs, int ProcessId, string Module, string Version) : TraceRecord(TimeMs, ProcessId);
// ---- End immediate-rendering-d2d-sw ----
