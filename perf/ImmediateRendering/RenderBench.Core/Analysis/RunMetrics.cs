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
    int UpdatesApplied);

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
    IReadOnlyList<GateCheck> Gate)
{
    public bool GatePassed => Gate.All(g => g.Passed);
    public PhaseMetrics? Phase(BenchPhase phase) => Phases.FirstOrDefault(p => p.Phase == phase);
}
