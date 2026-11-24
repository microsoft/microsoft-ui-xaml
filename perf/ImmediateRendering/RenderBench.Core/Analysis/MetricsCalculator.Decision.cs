namespace RenderBench.Analysis;

// Backend decision scenes S24-S32 (test plan, "Backend decision scenarios"): per-step settled memory, peak, CPU, frames
// and session DWM memory, and the cost curve of each sweep (fixed cost + cost per megapixel, element, window or
// navigation).
public static partial class MetricsCalculator
{
    // S28: the first 20 navigations create every page type once (first-use cost); growth is fitted from here on.
    public const int NavChurnFitFrom = 20;

    // Least-squares fit of y = slope * x + intercept; null with fewer than two distinct x values.
    public static LinearFit? Fit(IReadOnlyList<(double X, double Y)> points)
    {
        var valid = points.Where(p => !double.IsNaN(p.X) && !double.IsNaN(p.Y)).ToList();
        if (valid.Count < 2)
        {
            return null;
        }
        double meanX = valid.Average(p => p.X);
        double meanY = valid.Average(p => p.Y);
        double sxx = valid.Sum(p => (p.X - meanX) * (p.X - meanX));
        if (sxx == 0)
        {
            return null;
        }
        double slope = valid.Sum(p => (p.X - meanX) * (p.Y - meanY)) / sxx;
        double intercept = meanY - slope * meanX;
        double total = valid.Sum(p => (p.Y - meanY) * (p.Y - meanY));
        double residual = valid.Sum(p => Math.Pow(p.Y - (slope * p.X + intercept), 2));
        return new LinearFit(slope, intercept, total == 0 ? 1 : 1 - residual / total, valid.Count);
    }

    static StepMetrics DescribeStepWindow(StepMetrics step, double begin, double end, List<TraceRecord> records, List<FrameSpan> frames)
    {
        bool InWindow(TraceRecord r) => r.TimeMs >= begin && r.TimeMs < end;
        double settledFrom = begin + (end - begin) / 2;

        var samples = records.OfType<MemorySampleRecord>().Where(InWindow).ToList();
        var settled = samples.Where(s => s.TimeMs >= settledFrom).ToList();
        if (settled.Count == 0 && samples.Count > 0)
        {
            settled.Add(samples[^1]);
        }
        // BenchRunner.Step samples right before the SceneStep marker; that sample opens the CPU window.
        var opening = records.OfType<MemorySampleRecord>().LastOrDefault(s => s.TimeMs <= begin) ?? samples.FirstOrDefault();
        var closing = samples.LastOrDefault();
        double? cpuRate = null;
        if (opening is not null && closing is not null && closing.TimeMs > opening.TimeMs)
        {
            double cpu = (closing.CpuUserMs + closing.CpuKernelMs) - (opening.CpuUserMs + opening.CpuKernelMs);
            cpuRate = cpu / ((closing.TimeMs - opening.TimeMs) / 1000.0);
        }

        var threadSamples = records.OfType<ThreadCpuSampleRecord>().Where(InWindow).ToList();
        var threadOpening = records.OfType<ThreadCpuSampleRecord>().LastOrDefault(s => s.TimeMs <= begin) ?? threadSamples.FirstOrDefault();
        ThreadCpuMetrics? threadCpu = null;
        if (threadOpening is not null && threadSamples.LastOrDefault() is { } threadClosing && threadClosing.TimeMs > threadOpening.TimeMs)
        {
            double Delta(ThreadCategory c) => Math.Max(0, threadClosing[c] - threadOpening[c]);
            threadCpu = new ThreadCpuMetrics(threadClosing.TimeMs - threadOpening.TimeMs, Delta(ThreadCategory.Ui),
                Delta(ThreadCategory.WarpRasterizer), Delta(ThreadCategory.D2DDWrite), Delta(ThreadCategory.Composition),
                Delta(ThreadCategory.Other));
        }

        double duration = end - begin;
        int drawn = frames.Count(f => f.Drawn && f.EndMs >= begin && f.EndMs < end);

        double? dwmBaseline = DwmBaselineMB(records);
        double? dwmSettled = Median(records.OfType<DwmMemoryRecord>()
            .Where(d => d.Phase != 0 && d.PrivateBytes >= 0 && d.TimeMs >= settledFrom && d.TimeMs < end)
            .Select(d => d.PrivateBytes / MB));
        double? dwmDelta = dwmBaseline is { } b && dwmSettled is { } s ? s - b : null;
        double? privateMedian = Median(settled.Select(x => x.PrivateBytes / MB));
        var dwmCpu = records.OfType<DwmMemoryRecord>().Where(d => d.Phase != 0 && d.PrivateBytes >= 0).ToList();
        var dwmOpen = dwmCpu.LastOrDefault(d => d.TimeMs <= begin) ?? dwmCpu.FirstOrDefault(InWindow);
        var dwmClose = dwmCpu.LastOrDefault(InWindow);
        double? dwmCpuRate = dwmOpen is not null && dwmClose is not null && dwmClose.TimeMs > dwmOpen.TimeMs
            && dwmClose.DwmProcessId == dwmOpen.DwmProcessId && dwmClose.CpuMs > 0
            ? (dwmClose.CpuMs - dwmOpen.CpuMs) / ((dwmClose.TimeMs - dwmOpen.TimeMs) / 1000.0) : null;

        // Emitted per painted frame (and per window): the last frame painted before the window ends.
        var inventory = records.OfType<ResourceInventoryRecord>().LastOrDefault(r => r.TimeMs < end);
        var values = records.OfType<SceneValueRecord>().Where(InWindow)
            .GroupBy(v => v.Name, StringComparer.Ordinal)
            .ToDictionary(g => g.Key, g => g.Last().Value, StringComparer.Ordinal);

        return step with
        {
            DurationMs = duration,
            PrivateMBMedian = privateMedian,
            PeakPrivateMB = samples.Count == 0 ? null : samples.Max(x => x.PrivateBytes) / MB,
            WorkingSetMBMedian = Median(settled.Select(x => x.WorkingSetBytes / MB)),
            CpuMsPerSecond = cpuRate,
            ThreadCpu = threadCpu,
            FramesDrawn = drawn,
            Fps = duration > 0 ? drawn / (duration / 1000.0) : null,
            DwmDeltaMB = dwmDelta,
            TotalMB = privateMedian is { } p && dwmDelta is { } d ? p + d : null,
            DwmCpuMsPerSecond = dwmCpuRate,
            BackendSurfaceMB = inventory is null ? null : inventory.SurfaceBytes / MB,
            BackendCacheMB = inventory is null ? null : inventory.CacheBytes / MB,
            CachedObjects = inventory is null ? null : (int)inventory.CachedObjectCount,
            Values = values.Count == 0 ? null : values,
        };
    }

    // The pre-window (Phase 0) sample, else the first valid one.
    static double? DwmBaselineMB(List<TraceRecord> records)
    {
        var valid = records.OfType<DwmMemoryRecord>().Where(d => d.PrivateBytes >= 0).ToList();
        var baseline = valid.FirstOrDefault(d => d.Phase == 0) ?? valid.FirstOrDefault();
        return baseline is null ? null : baseline.PrivateBytes / MB;
    }

    static DecisionMetrics? ComputeDecision(string? sceneLabel, List<TraceRecord> records, IReadOnlyList<StepMetrics>? steps)
    {
        if (SceneCatalog.FromLabel(sceneLabel) is not { } scene || !SceneCatalog.IsDecisionScene(scene) || steps is null)
        {
            return null;
        }
        var summary = new SortedDictionary<string, double>(StringComparer.Ordinal);
        StepMetrics? Named(string name) => steps.FirstOrDefault(s => s.Name == name);
        void Put(string key, double? value)
        {
            if (value is { } v && !double.IsNaN(v))
            {
                summary[key] = v;
            }
        }
        // Settled private MB (TotalMB when total is set) of step a minus step b.
        void PutDelta(string key, StepMetrics? a, StepMetrics? b)
        {
            if (a is null || b is null)
            {
                return;
            }
            Put(key + "DeltaMB", a.PrivateMBMedian - b.PrivateMBMedian);
            Put(key + "TotalDeltaMB", a.TotalMB - b.TotalMB);
        }

        (string Unit, string Prefix, Func<StepMetrics, double> X, Func<StepMetrics, bool> Include)? sweep = scene switch
        {
            SceneId.WindowSize => ("MP", "size-", s => s.Windows / 1e6, _ => true),
            SceneId.Density => ("1k elements", "n-", s => s.Windows / 1000.0, _ => true),
            SceneId.WindowCount => ("window", "windows-", s => s.Windows, _ => true),
            SceneId.NavChurn => ("100 navigations", "nav-", s => s.Windows / 100.0, s => s.Windows >= NavChurnFitFrom),
            _ => null,
        };
        LinearFit? privateFit = null, totalFit = null;
        if (sweep is { } sw)
        {
            var points = steps.Where(s => s.Name.StartsWith(sw.Prefix, StringComparison.Ordinal) && sw.Include(s)).ToList();
            privateFit = Fit(points.Where(s => s.PrivateMBMedian is not null).Select(s => (sw.X(s), s.PrivateMBMedian!.Value)).ToList());
            totalFit = Fit(points.Where(s => s.TotalMB is not null).Select(s => (sw.X(s), s.TotalMB!.Value)).ToList());
        }

        switch (scene)
        {
            case SceneId.WindowSize:
                Put("MBPerMegapixel", privateFit?.Slope);
                Put("InterceptMB", privateFit?.Intercept);
                Put("TotalMBPerMegapixel", totalFit?.Slope);
                Put("TotalInterceptMB", totalFit?.Intercept);
                break;
            case SceneId.Density:
                Put("MBPer1kElements", privateFit?.Slope);
                Put("TotalMBPer1kElements", totalFit?.Slope);
                Put("FullFrameCpuMsAtMax", steps.LastOrDefault(s => s.Values?.ContainsKey("FullFrameCpuMs") == true)?.Values!["FullFrameCpuMs"]);
                break;
            case SceneId.WindowCount:
                Put("MBPerWindow", privateFit?.Slope);
                Put("TotalMBPerWindow", totalFit?.Slope);
                Put("FirstWindowMB", privateFit is null ? null : privateFit.Intercept + privateFit.Slope);
                break;
            case SceneId.Background:
            {
                var visible = Named("visible");
                foreach (var name in new[] { "minimized", "restored", "hidden", "shown", "occluded", "unoccluded" })
                {
                    PutDelta(char.ToUpperInvariant(name[0]) + name[1..], Named(name), visible);
                }
                Put("MinimizedCpuMsPerSecond", Named("minimized")?.CpuMsPerSecond);
                break;
            }
            case SceneId.NavChurn:
                Put("GrowthMBPer100Nav", privateFit?.Slope);
                Put("TotalGrowthMBPer100Nav", totalFit?.Slope);
                PutDelta("FirstUse", Named($"nav-{NavChurnFitFrom}"), Named("nav-0"));
                PutDelta("Overall", steps.LastOrDefault(s => s.Name.StartsWith("nav-", StringComparison.Ordinal)), Named("nav-0"));
                Put("CacheMBAtEnd", steps.LastOrDefault(s => s.BackendCacheMB is not null)?.BackendCacheMB);
                Put("CacheMBMax", steps.Where(s => s.BackendCacheMB is not null).Select(s => s.BackendCacheMB!.Value).DefaultIfEmpty(double.NaN).Max());
                break;
            case SceneId.Sustained:
            {
                var anim = Named("animating");
                Put("AnimCpuMsPerSecond", anim?.CpuMsPerSecond);
                Put("AnimFps", anim?.Fps);
                Put("AnimDwmCpuMsPerSecond", anim?.DwmCpuMsPerSecond);
                Put("AnimPrivateMB", anim?.PrivateMBMedian);
                Put("AnimTotalMB", anim?.TotalMB);
                foreach (var category in Enum.GetValues<ThreadCategory>())
                {
                    Put($"Anim{category}MsPerSecond", anim?.ThreadCpu?.MsPerSecond(category));
                }
                PutDelta("Anim", anim, Named("pre"));
                Put("StoppedCpuMsPerSecond", Named("stopped")?.CpuMsPerSecond);
                break;
            }
            case SceneId.Resize:
            {
                var pre = Named("pre");
                var storm = Named("storm");
                Put("PreMB", pre?.PrivateMBMedian);
                Put("StormPeakMB", storm?.PeakPrivateMB);
                Put("StormPeakDeltaMB", storm?.PeakPrivateMB - pre?.PrivateMBMedian);
                Put("StormCpuMsPerSecond", storm?.CpuMsPerSecond);
                PutDelta("Settle", Named("settle"), pre);
                PutDelta("Recovered", Named("recovered"), pre);
                break;
            }
            case SceneId.Minimal:
            {
                var floor = Named("after-gc") ?? Named("idle");
                Put("FloorMB", floor?.PrivateMBMedian);
                Put("FloorTotalMB", floor?.TotalMB);
                Put("FloorWorkingSetMB", floor?.WorkingSetMBMedian);
                Put("IdleCpuMsPerSecond", Named("idle")?.CpuMsPerSecond);
                break;
            }
            case SceneId.LargeCanvas:
            {
                var origin = Named("z1-origin");
                PutDelta("Zoom025", Named("z0.25"), origin);
                PutDelta("Zoom4", Named("z4"), origin);
                Put("PeakMB", steps.Where(s => s.PeakPrivateMB is not null).Select(s => s.PeakPrivateMB!.Value).DefaultIfEmpty(double.NaN).Max());
                Put("PanCpuMsPerSecond", Median(steps.Where(s => s.Name.Contains("pan", StringComparison.Ordinal) && s.CpuMsPerSecond is not null)
                    .Select(s => s.CpuMsPerSecond!.Value)));
                break;
            }
        }

        return new DecisionMetrics(sweep?.Unit ?? "", DwmBaselineMB(records), privateFit, totalFit, summary);
    }
}
