namespace RenderBench.Analysis;

public sealed record Distribution(int Count, double P50, double P95, double P99, double Max, double Mean)
{
    public static Distribution? From(IEnumerable<double> values)
    {
        var sorted = values.Where(v => !double.IsNaN(v)).OrderBy(v => v).ToArray();
        if (sorted.Length == 0)
        {
            return null;
        }
        return new Distribution(sorted.Length, Percentile(sorted, 50), Percentile(sorted, 95), Percentile(sorted, 99),
            sorted[^1], sorted.Average());
    }

    // Linear interpolation between closest ranks.
    public static double Percentile(IReadOnlyList<double> sorted, double percentile)
    {
        if (sorted.Count == 0)
        {
            return double.NaN;
        }
        double rank = percentile / 100.0 * (sorted.Count - 1);
        int lower = (int)Math.Floor(rank);
        int upper = (int)Math.Ceiling(rank);
        return sorted[lower] + (sorted[upper] - sorted[lower]) * (rank - lower);
    }
}

public sealed record GateCheck(string Name, bool Passed, string Detail);

public sealed record IdentityMetrics(
    string? HarnessRequested,
    BackendMode? Requested,
    BackendMode? Effective,
    FallbackReason? Reason,
    bool? IsWarp,
    ulong? AdapterLuid,
    IReadOnlyList<string> UnsupportedContent);

public sealed record PhaseMetrics(
    BenchPhase Phase,
    double DurationMs,
    int FramesDrawn,
    int FramesTotal,
    double FramesDrawnPerMinute,
    Distribution? FrameMs,
    double? AvgElementsVisited,
    double? AvgElementsRendered,
    Distribution? PaintMs,
    ulong DamagedPixels,
    ulong PaintedPixels,
    ulong UploadBytes,
    double? PrivateMBMedian,
    double? WorkingSetMBMedian,
    double? GpuLocalMBMedian,
    double? GpuNonLocalMBMedian,
    int? GdiObjectsMax,
    int? UserObjectsMax,
    double? CpuMsPerSecond,
    double? PrivateMBPerMinuteSlope,
    double? BackendSurfaceMB,
    double? BackendCacheMB,
    int UpdatesApplied,
    ThreadCpuMetrics? ThreadCpu = null); // immediate-rendering-d2d-sw: W1/W2 CPU split

// ---- Added by immediate-rendering-d2d-sw ----
// CPU ms per ThreadCategory over a phase (last minus first ThreadCpuSample in the phase). Threads that start in ntdll
// (thread pool) land in Other even when they run WARP/D2D work; use WPA CPU sampling with stacks for that attribution.
public sealed record ThreadCpuMetrics(double WindowMs, double UiMs, double WarpRasterizerMs, double D2DDWriteMs,
    double CompositionMs, double OtherMs)
{
    public double Ms(ThreadCategory category) => category switch
    {
        ThreadCategory.Ui => UiMs,
        ThreadCategory.WarpRasterizer => WarpRasterizerMs,
        ThreadCategory.D2DDWrite => D2DDWriteMs,
        ThreadCategory.Composition => CompositionMs,
        _ => OtherMs,
    };

    public double? MsPerSecond(ThreadCategory category) => WindowMs > 0 ? Ms(category) / (WindowMs / 1000.0) : null;

    public double TotalMs => UiMs + WarpRasterizerMs + D2DDWriteMs + CompositionMs + OtherMs;
}

public sealed record ThreadModuleMetrics(string Module, string Category, int Threads, double CpuMs);
// ---- End immediate-rendering-d2d-sw ----

public sealed record StartupMetrics(double? ProcessStartToSceneReadyMs, double? ProcessStartToFirstFrameMs);

public sealed record RunMetrics(
    string SchemaVersion,
    string? Scene,
    int? ProcessId,
    IdentityMetrics Identity,
    StartupMetrics Startup,
    IReadOnlyList<PhaseMetrics> Phases,
    Distribution? InputToFrameMs,
    double? PeakPrivateMB,
    double? PeakGpuLocalMB,
    IReadOnlyList<GateCheck> Gate,
    // ---- Added by immediate-rendering-d2d-sw ----
    IReadOnlyList<ThreadModuleMetrics>? ThreadModules = null,
    IReadOnlyDictionary<string, string>? RasterModuleVersions = null,
    int DeviceLostCount = 0)
    // ---- End immediate-rendering-d2d-sw ----
{
    public bool GatePassed => Gate.All(g => g.Passed);
    public PhaseMetrics? Phase(BenchPhase phase) => Phases.FirstOrDefault(p => p.Phase == phase);
}
