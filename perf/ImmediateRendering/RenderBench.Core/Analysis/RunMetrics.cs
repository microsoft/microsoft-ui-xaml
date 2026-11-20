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
    // ---- Appended by immediate-rendering-gdiplus ----
    // W6: backend resources created/destroyed per second of the phase (ResourceChurn events); null when none were emitted.
    double? ResourcesCreatedPerSecond = null,
    double? ResourcesDestroyedPerSecond = null,
    // M7: last minus first GetGuiResources(GR_GDIOBJECTS) sample in the phase; a GDI handle leak shows as growth.
    int? GdiObjectsGrowth = null,
    // ---- Added by the D2D-HW branch (defaults keep older metrics.json readable) ----
    // W3: GPU time per paint from GpuPaintTime, attributed to the phase of its frame's FrameEnd.
    Distribution? GpuPaintMs = null,
    // M3 on the adapter WinUI renders with (DeviceInfo.AdapterLuid). GpuLocalMBMedian/GpuNonLocalMBMedian stay the sum
    // over all adapters.
    double? RenderAdapterGpuLocalMBMedian = null,
    double? RenderAdapterGpuNonLocalMBMedian = null,
    ThreadCpuMetrics? ThreadCpu = null, // immediate-rendering-d2d-sw: W1/W2 CPU split
    // ---- Extended scenes S3-S10 ----
    // R2/R3: intervals between consecutive drawn FrameEnds. In scenes without continuous interaction, gaps longer than
    // MetricsCalculator.MaxContinuousGapMs are idle time between discrete inputs and are excluded.
    Distribution? FrameIntervalMs = null,
    // Intervals longer than 1.5x the vsync period (DisplayInfo; 60 Hz assumed when absent).
    int? DroppedFrames = null,
    // Frames whose FrameBegin-to-FrameEnd (UI thread work) exceeded 1.5x vsync.
    int? LongFrames = null,
    double? LongestFrameGapMs = null,
    // SceneValue counters (RealizedItems, ImageLoadMs, ...) emitted during the phase; null when none.
    IReadOnlyDictionary<string, Distribution>? SceneValues = null);

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

// Memory after one S9 lifecycle step: the last sample before the next step (or the end of the phase).
public sealed record StepMetrics(int Step, string Name, int Windows, double PrivateMB, double WorkingSetMB, double GpuLocalMB,
    int GdiObjects, int UserObjects);

public sealed record StartupMetrics(double? ProcessStartToSceneReadyMs, double? ProcessStartToFirstFrameMs);

// S15: one deliberate UI-thread block. MaxFrameGapMs covers the last drawn frame before the block through the first
// after it; null when there is no drawn frame on either side.
public sealed record UiBlockMetrics(int BlockId, double DurationMs, int FramesDrawn, double? MaxFrameGapMs);

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
    // ---- Added by the D2D-HW branch ----
    // Set when the run was not executed (e.g. AdapterPolicy.NoHardwareAdapterReason for D2D-HW on a GPU-less machine).
    // A skipped run is neither a pass nor a gate failure.
    string? SkipReason = null,
    // ---- Added by immediate-rendering-d2d-sw ----
    IReadOnlyList<ThreadModuleMetrics>? ThreadModules = null,
    IReadOnlyDictionary<string, string>? RasterModuleVersions = null,
    int DeviceLostCount = 0,
    // ---- End immediate-rendering-d2d-sw ----
    // ---- Extended scenes S3-S10 ----
    double? RefreshHz = null,
    bool AnimatesWhileIdle = false,
    // E1 'animation tax': Steady CPU ms/s of a scene that animates while idle (S6); null for static scenes.
    double? AnimationCpuMsPerSecond = null,
    string? InputMode = null,
    string? InputModeReason = null,
    // R1 per input kind (key, click, wheel, ...); programmatic state changes are excluded.
    IReadOnlyDictionary<string, Distribution>? InputToFrameMsByKind = null,
    // Programmatic state change (InputInjected kind 'programmatic:*') to next drawn frame; not input latency.
    Distribution? ProgrammaticToFrameMs = null,
    // Popup request (InputInjected) to the first drawn frame after the popup's Opened event.
    Distribution? PopupOpenMs = null,
    // S9 lifecycle steps with the settled memory of each step.
    IReadOnlyList<StepMetrics>? Steps = null,
    double? MarginalPrivateMBPerWindow = null,
    double? PrivateMBReclaimedFraction = null,
    // UnsupportedContent event counts by 'TypeName|Detail' and by Detail, and the entries the scene expects.
    IReadOnlyDictionary<string, int>? UnsupportedByType = null,
    IReadOnlyDictionary<string, int>? UnsupportedByDetail = null,
    IReadOnlyList<string>? ExpectedUnsupported = null,
    // ---- Full-backend scenes S11-S23 ----
    // WindowFallback events (per-window switch to Composition) and whether this scene variant expects one.
    int WindowFallbackCount = 0,
    IReadOnlyList<string>? WindowFallbackReasons = null,
    bool ExpectsWindowFallback = false,
    // S15: drawn frames and the longest gap between drawn frames around each deliberate UI-thread block.
    IReadOnlyList<UiBlockMetrics>? UiBlocks = null,
    int? FramesDuringUiBlock = null,
    double? MaxFrameGapDuringUiBlockMs = null,
    // S20/S23: programmatic change (resize, theme, dpi, navigate) to the next drawn frame, by kind, within
    // MetricsCalculator.MaxChangeToFrameMs; changes without a frame in that window are counted as unmatched.
    IReadOnlyDictionary<string, Distribution>? ChangeToFrameMsByKind = null,
    int? UnmatchedChanges = null,
    // S20: private MB at step cycle-100 minus cycle-10 (each sampled right before its step).
    double? CycleLeakPrivateMB = null,
    // S21: DeviceLost to the next drawn frame, and private MB after the 100th recovery minus after the 10th.
    Distribution? DeviceLostRecoveryMs = null,
    double? DeviceLostLeakPrivateMB = null,
    // S14: popup open time per popup kind, and the slope of private MB over the number of open popups.
    IReadOnlyDictionary<string, Distribution>? PopupOpenMsByKind = null,
    double? PrivateMBPerOpenPopup = null,
    // S22: share of successful RenderTargetBitmap renders and the mean of the capture comparisons (0..255).
    double? RtbSuccessRate = null,
    double? CaptureMeanAbsDiff = null)
{
    public bool IsSkipped => SkipReason is not null;
    public bool GatePassed => !IsSkipped && Gate.All(g => g.Passed);
    public PhaseMetrics? Phase(BenchPhase phase) => Phases.FirstOrDefault(p => p.Phase == phase);
}
