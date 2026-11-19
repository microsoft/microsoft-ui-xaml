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
