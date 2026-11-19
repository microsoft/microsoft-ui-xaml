using System.Globalization;
using System.Text;
using RenderBench.Analysis;

namespace RenderBench.Analyzer;

public static class ComparisonReport
{
    sealed record Row(string Name, Func<RunMetrics, double?> Select, string Format = "F1");

    static readonly Row[] s_rows =
    [
        new("S1 start to scene ready (ms)", m => m.Startup.ProcessStartToSceneReadyMs, "F0"),
        new("S1 start to first frame (ms)", m => m.Startup.ProcessStartToFirstFrameMs, "F0"),
        new("M1 private commit, steady (MB)", m => m.Phase(BenchPhase.Steady)?.PrivateMBMedian),
        new("M1 private commit, peak (MB)", m => m.PeakPrivateMB),
        new("M2 working set, steady (MB)", m => m.Phase(BenchPhase.Steady)?.WorkingSetMBMedian),
        new("M3 GPU local, steady (MB)", m => m.Phase(BenchPhase.Steady)?.GpuLocalMBMedian),
        new("M3 GPU non-local, steady (MB)", m => m.Phase(BenchPhase.Steady)?.GpuNonLocalMBMedian),
        new("M3 GPU local, peak (MB)", m => m.PeakGpuLocalMB),
        new("M6 backend surface, steady (MB)", m => m.Phase(BenchPhase.Steady)?.BackendSurfaceMB),
        new("M6 backend cache, steady (MB)", m => m.Phase(BenchPhase.Steady)?.BackendCacheMB),
        new("M7 GDI objects, steady", m => m.Phase(BenchPhase.Steady)?.GdiObjectsMax, "F0"),
        new("M8 private slope, interaction (MB/min)", m => m.Phase(BenchPhase.Interaction)?.PrivateMBPerMinuteSlope, "F2"),
        new("W1 UI frame p50, interaction (ms)", m => m.Phase(BenchPhase.Interaction)?.FrameMs?.P50, "F2"),
        new("W1 UI frame p95, interaction (ms)", m => m.Phase(BenchPhase.Interaction)?.FrameMs?.P95, "F2"),
        new("W1 paint p95, interaction (ms)", m => m.Phase(BenchPhase.Interaction)?.PaintMs?.P95, "F2"),
        new("W2 CPU, interaction (ms/s)", m => m.Phase(BenchPhase.Interaction)?.CpuMsPerSecond),
        new("W2 CPU, steady (ms/s)", m => m.Phase(BenchPhase.Steady)?.CpuMsPerSecond, "F2"),
        // ---- Added by immediate-rendering-d2d-sw: per-thread CPU split ----
        new("W2 CPU UI thread, interaction (ms/s)", m => m.Phase(BenchPhase.Interaction)?.ThreadCpu?.MsPerSecond(ThreadCategory.Ui)),
        new("W2 CPU WARP rasterizer, interaction (ms/s)", m => m.Phase(BenchPhase.Interaction)?.ThreadCpu?.MsPerSecond(ThreadCategory.WarpRasterizer)),
        new("W2 CPU D2D/DWrite, interaction (ms/s)", m => m.Phase(BenchPhase.Interaction)?.ThreadCpu?.MsPerSecond(ThreadCategory.D2DDWrite)),
        new("W2 CPU composition, interaction (ms/s)", m => m.Phase(BenchPhase.Interaction)?.ThreadCpu?.MsPerSecond(ThreadCategory.Composition)),
        new("W2 CPU other threads, interaction (ms/s)", m => m.Phase(BenchPhase.Interaction)?.ThreadCpu?.MsPerSecond(ThreadCategory.Other)),
        new("Device lost events", m => m.DeviceLostCount, "F0"),
        // ---- End immediate-rendering-d2d-sw ----
        new("W4 painted/damaged px, interaction", m => Ratio(m.Phase(BenchPhase.Interaction)), "F2"),
        new("W5 upload, interaction (MB)", m => m.Phase(BenchPhase.Interaction)?.UploadBytes / (1024.0 * 1024.0), "F2"),
        new("R1* input to frame end p95 (ms)", m => m.InputToFrameMs?.P95, "F1"),
        new("E1 drawn frames/min, steady", m => m.Phase(BenchPhase.Steady)?.FramesDrawnPerMinute),
        // ---- Appended by immediate-rendering-gdiplus: W6 churn, M7 GDI/USER objects beyond steady ----
        new("W6 resources created, interaction (/s)", m => m.Phase(BenchPhase.Interaction)?.ResourcesCreatedPerSecond),
        new("W6 resources destroyed, interaction (/s)", m => m.Phase(BenchPhase.Interaction)?.ResourcesDestroyedPerSecond),
        new("W6 resources created, steady (/s)", m => m.Phase(BenchPhase.Steady)?.ResourcesCreatedPerSecond, "F2"),
        new("M7 GDI objects, interaction (max)", m => m.Phase(BenchPhase.Interaction)?.GdiObjectsMax, "F0"),
        new("M7 GDI objects growth, interaction", m => m.Phase(BenchPhase.Interaction)?.GdiObjectsGrowth, "F0"),
        new("M7 USER objects, steady", m => m.Phase(BenchPhase.Steady)?.UserObjectsMax, "F0"),
        // ---- Added by the D2D-HW branch ----
        new("W3 GPU paint p50, interaction (ms)", m => m.Phase(BenchPhase.Interaction)?.GpuPaintMs?.P50, "F3"),
        new("W3 GPU paint p95, interaction (ms)", m => m.Phase(BenchPhase.Interaction)?.GpuPaintMs?.P95, "F3"),
        new("M3 GPU local, render adapter, steady (MB)", m => m.Phase(BenchPhase.Steady)?.RenderAdapterGpuLocalMBMedian),
        new("M3 GPU non-local, render adapter, steady (MB)", m => m.Phase(BenchPhase.Steady)?.RenderAdapterGpuNonLocalMBMedian),
    ];

    static double? Ratio(PhaseMetrics? phase)
        => phase is null || phase.DamagedPixels == 0 ? null : (double)phase.PaintedPixels / phase.DamagedPixels;

    public static string Build(IEnumerable<(string Path, RunMetrics Metrics)> runs)
    {
        var builder = new StringBuilder();
        builder.AppendLine("# RenderBench comparative report");
        builder.AppendLine();
        builder.AppendLine("Median across runs, with min-max in brackets and ratio to COMP. Runs that fail the gate are excluded");
        builder.AppendLine("from medians and counted in the gate row. R1* is input-injected to the next drawn UI frame end (a proxy;");
        builder.AppendLine("visible latency needs DWM present correlation).");
        builder.AppendLine("Skipped runs (e.g. D2D-HW without a hardware GPU) are not gate failures; their cells read n/a.");

        foreach (var scene in runs.GroupBy(r => r.Metrics.Scene ?? "unknown").OrderBy(g => g.Key))
        {
            var byMode = BackendModes.All
                .Select(mode => (Mode: mode, Runs: scene.Where(r => ModeOf(r.Metrics) == mode).Select(r => r.Metrics).ToList()))
                .ToList();
            builder.AppendLine();
            builder.AppendLine($"## Scene: {scene.Key}");
            builder.AppendLine();
            builder.Append("| Metric |");
            foreach (var (mode, _) in byMode)
            {
                builder.Append(CultureInfo.InvariantCulture, $" {BackendModes.Label(mode)} |");
            }
            builder.AppendLine();
            builder.Append("|---|");
            builder.AppendLine(string.Concat(byMode.Select(_ => "---|")));

            builder.Append("| Gate (passed/runs) |");
            foreach (var (_, modeRuns) in byMode)
            {
                if (SkipCell(modeRuns) is not null)
                {
                    builder.Append(CultureInfo.InvariantCulture, $" skipped: {SkipReasons(modeRuns)} |");
                    continue;
                }
                var executed = modeRuns.Where(r => !r.IsSkipped).ToList();
                var failures = executed.Where(r => !r.GatePassed).SelectMany(r => r.Gate.Where(g => !g.Passed).Select(g => g.Name)).Distinct();
                string detail = string.Join(", ", failures);
                int skippedCount = modeRuns.Count - executed.Count;
                builder.Append(CultureInfo.InvariantCulture,
                    $" {executed.Count(r => r.GatePassed)}/{executed.Count}{(detail.Length > 0 ? $" ({detail})" : "")}" +
                    $"{(skippedCount > 0 ? $", {skippedCount} skipped ({SkipReasons(modeRuns)})" : "")} |");
            }
            builder.AppendLine();

            var baseline = byMode.First(m => m.Mode == BackendMode.Composition).Runs.Where(r => r.GatePassed).ToList();
            foreach (var row in s_rows)
            {
                double? baselineMedian = Median(baseline.Select(row.Select));
                builder.Append(CultureInfo.InvariantCulture, $"| {row.Name} |");
                foreach (var (mode, modeRuns) in byMode)
                {
                    var values = modeRuns.Where(r => r.GatePassed).Select(row.Select).Where(v => v is not null).Select(v => v!.Value).ToList();
                    if (values.Count == 0)
                    {
                        builder.Append(CultureInfo.InvariantCulture, $" {SkipCell(modeRuns) ?? "-"} |");
                        continue;
                    }
                    double median = Median(values.Select(v => (double?)v))!.Value;
                    string cell = median.ToString(row.Format, CultureInfo.InvariantCulture);
                    if (values.Count > 1)
                    {
                        cell += $" [{values.Min().ToString(row.Format, CultureInfo.InvariantCulture)}-{values.Max().ToString(row.Format, CultureInfo.InvariantCulture)}]";
                    }
                    if (mode != BackendMode.Composition && baselineMedian is > 0)
                    {
                        cell += (median / baselineMedian.Value).ToString(" (x0.00)", CultureInfo.InvariantCulture);
                    }
                    builder.Append(CultureInfo.InvariantCulture, $" {cell} |");
                }
                builder.AppendLine();
            }
        }
        return builder.ToString();
    }

    // The harness request is the experiment label; the gate decides whether the run counts.
    static BackendMode ModeOf(RunMetrics metrics)
        => BackendModes.TryParse(metrics.Identity.HarnessRequested, out var mode) ? mode : BackendMode.Composition;

    // ---- Added by the D2D-HW branch ----
    // A mode whose runs were all skipped is shown as n/a with the reason instead of an empty column.
    static string? SkipCell(IReadOnlyList<RunMetrics> modeRuns)
    {
        if (modeRuns.Count == 0 || !modeRuns.All(r => r.IsSkipped))
        {
            return null;
        }
        return modeRuns.All(r => r.SkipReason == AdapterPolicy.NoHardwareAdapterReason) ? "n/a (no GPU)" : "n/a (skipped)";
    }

    static string SkipReasons(IEnumerable<RunMetrics> modeRuns)
        => string.Join(", ", modeRuns.Where(r => r.IsSkipped).Select(r => r.SkipReason).Distinct());

    static double? Median(IEnumerable<double?> values)
    {
        var sorted = values.Where(v => v is not null).Select(v => v!.Value).OrderBy(v => v).ToArray();
        return sorted.Length == 0 ? null : Distribution.Percentile(sorted, 50);
    }
}
