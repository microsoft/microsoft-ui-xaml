namespace RenderBench.Analysis;

// SceneValue names the full-backend scenes emit and the analyzer reads.
public static class SceneValueNames
{
    public const string RtbSuccess = "RtbSuccess";                 // S22: 1 when RenderTargetBitmap.RenderAsync succeeded
    public const string CaptureMeanAbsDiff = "CaptureMeanAbsDiff"; // S22: RTB vs on-screen capture, 0..255
}

// Scene variants built around content that makes the window fall back to Composition (S11 interop, S12 custom
// AnimatedVisualPlayer, S16 SwapChainPanel/media). Every other scene must stay immediate in immediate modes.
public static class WindowFallbackPolicy
{
    public static bool IsExpected(string? sceneLabel) => SceneCatalog.FromLabel(sceneLabel) switch
    {
        SceneId.CompositionInterop => sceneLabel != "CompositionInterop-NoInterop",
        SceneId.Lottie => sceneLabel != "Lottie-NoPlayer",
        SceneId.SwapChainMedia => true,
        _ => false,
    };
}

// Metrics and gates for the full-backend scenes S11-S23 (test plan, "Full-backend scenes").
public static partial class MetricsCalculator
{
    // Programmatic state changes measured as change-to-frame (S20 resize/theme/dpi, S23 navigate).
    public static readonly IReadOnlyList<string> ChangeKinds = ["resize", "theme", "dpi", "navigate"];
    // A theme switch or DPI change can take far longer than an input; changes are matched within this window.
    public const double MaxChangeToFrameMs = 5000;
    public const int LeakFirstCycle = 10, LeakLastCycle = 100;

    sealed record FullBackendMetrics(
        int FallbackCount, IReadOnlyList<string>? FallbackReasons, bool ExpectsFallback,
        IReadOnlyList<UiBlockMetrics>? UiBlocks,
        IReadOnlyDictionary<string, Distribution>? ChangeToFrame, int? UnmatchedChanges,
        double? CycleLeakMB,
        Distribution? DeviceLostRecovery, int DeviceLosses, int Recovered, double? DeviceLostLeakMB,
        IReadOnlyDictionary<string, Distribution>? PopupOpenByKind, double? PrivateMBPerOpenPopup,
        double? RtbSuccessRate, double? CaptureMeanAbsDiff);

    static FullBackendMetrics ComputeFullBackend(string? scene, List<TraceRecord> records, List<FrameSpan> frames,
        IReadOnlyList<StepMetrics>? steps, GateOptions options)
    {
        var drawnEnds = frames.Where(f => f.Drawn).Select(f => f.EndMs).OrderBy(t => t).ToList();
        var fallbacks = records.OfType<WindowFallbackRecord>().ToList();
        var samples = records.OfType<MemorySampleRecord>().OrderBy(s => s.TimeMs).ToList();

        var losses = records.OfType<DeviceLostRecord>().OrderBy(l => l.TimeMs).ToList();
        var recoveries = losses.Select(l => NextDrawn(drawnEnds, l.TimeMs, MaxChangeToFrameMs)).ToList();
        double? deviceLeak = null;
        if (losses.Count >= LeakLastCycle &&
            recoveries[LeakFirstCycle - 1] is { } first && recoveries[LeakLastCycle - 1] is { } last &&
            samples.FirstOrDefault(s => s.TimeMs > first) is { } before &&
            samples.FirstOrDefault(s => s.TimeMs > last) is { } after)
        {
            deviceLeak = (after.PrivateBytes - before.PrivateBytes) / MB;
        }

        var (changes, unmatched) = ChangeToFrame(records, drawnEnds);
        var popupTimes = PopupOpenTimes(records, frames, options);
        var popupByKind = popupTimes.GroupBy(p => p.Popup, StringComparer.Ordinal)
            .ToDictionary(g => g.Key, g => Distribution.From(g.Select(p => p.Ms))!, StringComparer.Ordinal);
        var values = records.OfType<SceneValueRecord>().ToList();

        return new FullBackendMetrics(
            fallbacks.Count,
            fallbacks.Count == 0 ? null : fallbacks.Select(f => f.Reason).Distinct().ToList(),
            WindowFallbackPolicy.IsExpected(scene),
            UiBlocks(records, drawnEnds),
            changes, unmatched,
            StepPrivateMB(records, samples, $"cycle-{LeakLastCycle}") - StepPrivateMB(records, samples, $"cycle-{LeakFirstCycle}"),
            Distribution.From(losses.Zip(recoveries).Where(p => p.Second is not null).Select(p => p.Second!.Value - p.First.TimeMs)),
            losses.Count, recoveries.Count(r => r is not null), deviceLeak,
            popupByKind.Count == 0 ? null : new SortedDictionary<string, Distribution>(popupByKind, StringComparer.Ordinal),
            Marginal(steps, "popups-"),
            Mean(values.Where(v => v.Name == SceneValueNames.RtbSuccess)),
            Mean(values.Where(v => v.Name == SceneValueNames.CaptureMeanAbsDiff)));
    }

    static double? Mean(IEnumerable<SceneValueRecord> values)
    {
        var list = values.Select(v => v.Value).Where(v => !double.IsNaN(v)).ToList();
        return list.Count == 0 ? null : list.Average();
    }

    // First drawn frame end strictly after timeMs, within window ms.
    static double? NextDrawn(List<double> drawnEnds, double timeMs, double window)
    {
        int index = drawnEnds.BinarySearch(timeMs);
        index = index < 0 ? ~index : index + 1;
        while (index < drawnEnds.Count && drawnEnds[index] <= timeMs)
        {
            index++;
        }
        return index < drawnEnds.Count && drawnEnds[index] - timeMs <= window ? drawnEnds[index] : null;
    }

    static List<UiBlockMetrics>? UiBlocks(List<TraceRecord> records, List<double> drawnEnds)
    {
        var blocks = new List<UiBlockMetrics>();
        var open = new Dictionary<int, UiBlockRecord>();
        foreach (var block in records.OfType<UiBlockRecord>())
        {
            if (block.IsBegin)
            {
                open[block.BlockId] = block;
                continue;
            }
            if (!open.Remove(block.BlockId, out var begin))
            {
                continue;
            }
            var inside = drawnEnds.Where(t => t >= begin.TimeMs && t <= block.TimeMs).ToList();
            double? before = drawnEnds.LastOrDefault(t => t < begin.TimeMs, double.NaN) is var b && !double.IsNaN(b) ? b : null;
            double? after = drawnEnds.FirstOrDefault(t => t > block.TimeMs, double.NaN) is var a && !double.IsNaN(a) ? a : null;
            double? gap = null;
            if (before is not null && after is not null)
            {
                var points = new List<double> { before.Value };
                points.AddRange(inside);
                points.Add(after.Value);
                gap = points.Zip(points.Skip(1), (x, y) => y - x).Max();
            }
            blocks.Add(new UiBlockMetrics(block.BlockId, block.DurationMs, inside.Count, gap));
        }
        return blocks.Count == 0 ? null : blocks;
    }

    static (IReadOnlyDictionary<string, Distribution>? ByKind, int? Unmatched) ChangeToFrame(List<TraceRecord> records,
        List<double> drawnEnds)
    {
        var byKind = new Dictionary<string, List<double>>(StringComparer.Ordinal);
        int count = 0, unmatched = 0;
        foreach (var input in records.OfType<InputInjectedRecord>())
        {
            string kind = input.Kind.StartsWith(ProgrammaticKindPrefix, StringComparison.OrdinalIgnoreCase)
                ? input.Kind[ProgrammaticKindPrefix.Length..] : input.Kind;
            if (!ChangeKinds.Contains(kind))
            {
                continue;
            }
            count++;
            if (NextDrawn(drawnEnds, input.TimeMs, MaxChangeToFrameMs) is { } end)
            {
                (byKind.TryGetValue(kind, out var list) ? list : byKind[kind] = []).Add(end - input.TimeMs);
            }
            else
            {
                unmatched++;
            }
        }
        if (count == 0)
        {
            return (null, null);
        }
        var result = new SortedDictionary<string, Distribution>(StringComparer.Ordinal);
        foreach (var (kind, list) in byKind)
        {
            result[kind] = Distribution.From(list)!;
        }
        return (result, unmatched);
    }

    // The harness samples memory right before emitting a step, so the step's value is the last sample at or before it.
    static double? StepPrivateMB(List<TraceRecord> records, List<MemorySampleRecord> samples, string name)
    {
        var step = records.OfType<SceneStepRecord>().LastOrDefault(s => s.Name == name);
        var sample = step is null ? null : samples.LastOrDefault(s => s.TimeMs <= step.TimeMs);
        return sample is null ? null : sample.PrivateBytes / MB;
    }

    static List<GateCheck> EvaluateFullBackendGates(string? scene, bool immediate, FullBackendMetrics m, GateOptions options)
    {
        var checks = new List<GateCheck> { EvaluateFallback(immediate, m) };
        switch (SceneCatalog.FromLabel(scene))
        {
            case SceneId.IndependentAnimation:
                checks.Add(new(GateNames.UiBlockObserved, m.UiBlocks is { Count: > 0 },
                    m.UiBlocks is null ? "no UiBlockBegin/UiBlockEnd pair" : $"{m.UiBlocks.Count} block(s)"));
                break;
            case SceneId.DpiTheme:
                checks.Add(EvaluateChangeRecovery(m, options));
                checks.Add(new(GateNames.NoCycleLeak, m.CycleLeakMB is { } leak && leak < options.MaxCycleLeakMB,
                    m.CycleLeakMB is null ? "no cycle-10/cycle-100 steps"
                        : $"{m.CycleLeakMB:F2} MB from cycle 10 to 100 (max {options.MaxCycleLeakMB})"));
                break;
            case SceneId.DeviceLost when !immediate:
                const string na = "n/a: Composition has no device-lost hook";
                checks.Add(new(GateNames.DeviceLostObserved, true, na));
                checks.Add(new(GateNames.RecoveryWithinLimit, true, na));
                checks.Add(new(GateNames.NoCycleLeak, true, na));
                break;
            case SceneId.DeviceLost:
                checks.Add(new(GateNames.DeviceLostObserved, m.DeviceLosses > 0,
                    m.DeviceLosses == 0 ? $"no DeviceLost events; is {ImmediateHooks.SimulateDeviceLostVariable} honored?"
                        : $"{m.DeviceLosses} device losses"));
                checks.Add(m.DeviceLosses == 0
                    ? new(GateNames.RecoveryWithinLimit, true, "n/a: no device loss")
                    : new(GateNames.RecoveryWithinLimit,
                        m.Recovered == m.DeviceLosses && m.DeviceLostRecovery!.P95 <= options.MaxRecoveryMs,
                        $"{m.Recovered}/{m.DeviceLosses} recovered within {MaxChangeToFrameMs:F0} ms; p95 " +
                        $"{m.DeviceLostRecovery?.P95.ToString("F1") ?? "n/a"} ms (max {options.MaxRecoveryMs})"));
                checks.Add(m.DeviceLostLeakMB is { } lost
                    ? new(GateNames.NoCycleLeak, lost < options.MaxCycleLeakMB,
                        $"{lost:F2} MB from loss {LeakFirstCycle} to {LeakLastCycle} (max {options.MaxCycleLeakMB})")
                    : new(GateNames.NoCycleLeak, true, $"n/a: fewer than {LeakLastCycle} device losses ({m.DeviceLosses})"));
                break;
            case SceneId.Accessibility:
                checks.Add(new(GateNames.RenderTargetBitmapOk, m.RtbSuccessRate == 1,
                    m.RtbSuccessRate is null ? $"no {SceneValueNames.RtbSuccess} values" : $"{m.RtbSuccessRate:P0} succeeded"));
                checks.Add(m.CaptureMeanAbsDiff is { } diff
                    ? new(GateNames.CaptureWithinTolerance, diff <= options.MaxCaptureMeanAbsDiff,
                        $"mean abs diff {diff:F2} (max {options.MaxCaptureMeanAbsDiff})")
                    : new(GateNames.CaptureWithinTolerance, true, "skipped: no capture available"));
                break;
        }
        return checks;
    }

    static GateCheck EvaluateFallback(bool immediate, FullBackendMetrics m)
    {
        string reasons = m.FallbackReasons is null ? "" : $" ({string.Join(", ", m.FallbackReasons)})";
        if (!immediate)
        {
            return new(GateNames.WindowFallbackAsExpected, true, $"n/a for Composition; {m.FallbackCount} fallback(s){reasons}");
        }
        if (m.ExpectsFallback)
        {
            return new(GateNames.WindowFallbackAsExpected, m.FallbackCount > 0,
                m.FallbackCount > 0 ? $"expected fallback observed {m.FallbackCount}x{reasons}" : "expected a WindowFallback event; none observed");
        }
        return new(GateNames.WindowFallbackAsExpected, m.FallbackCount == 0,
            m.FallbackCount == 0 ? "stayed immediate" : $"unexpected fallback {m.FallbackCount}x{reasons}");
    }

    static GateCheck EvaluateChangeRecovery(FullBackendMetrics m, GateOptions options)
    {
        if (m.ChangeToFrame is null)
        {
            return new(GateNames.RecoveryWithinLimit, false, "no resize/theme/dpi changes");
        }
        var slow = m.ChangeToFrame.Where(k => k.Value.P95 > options.MaxRecoveryMs).Select(k => $"{k.Key} p95 {k.Value.P95:F0} ms").ToList();
        if (m.UnmatchedChanges > 0)
        {
            slow.Add($"{m.UnmatchedChanges} change(s) without a frame within {MaxChangeToFrameMs:F0} ms");
        }
        string all = string.Join(", ", m.ChangeToFrame.Select(k => $"{k.Key} p95 {k.Value.P95:F0} ms"));
        return new(GateNames.RecoveryWithinLimit, slow.Count == 0,
            slow.Count == 0 ? $"{all} (max {options.MaxRecoveryMs})" : string.Join("; ", slow));
    }
}
