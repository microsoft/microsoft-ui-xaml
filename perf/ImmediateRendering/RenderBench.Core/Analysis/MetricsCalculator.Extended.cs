namespace RenderBench.Analysis;

// Metrics added for the extended scenes S3-S10: frame cadence (R2/R3), per-kind input latency, popup open time, S9
// lifecycle steps, scene counters and unsupported content by type with per-scene expectations.
public static partial class MetricsCalculator
{
    // In scenes without continuous interaction, a gap between drawn frames longer than this is idle time (between
    // clicks, keystrokes, ...) rather than a dropped frame.
    public const double MaxContinuousGapMs = 250;
    public const double DefaultRefreshHz = 60;
    // A frame interval (or a frame's UI work) longer than this many vsync periods counts as dropped (or long).
    public const double DroppedFrameVsyncFactor = 1.5;
    public const string ProgrammaticKindPrefix = "programmatic:";

    static bool IsRealInput(InputInjectedRecord input) =>
        !input.Kind.StartsWith(ProgrammaticKindPrefix, StringComparison.OrdinalIgnoreCase);

    static PhaseMetrics ExtendPhase(PhaseMetrics phase, PhaseWindow window, List<TraceRecord> records, List<FrameSpan> frames,
        double? refreshHz, bool continuousInteraction)
    {
        double threshold = DroppedFrameVsyncFactor * 1000.0 / (refreshHz ?? DefaultRefreshHz);
        var drawn = frames.Where(f => f.Drawn && window.Contains(f.EndMs)).OrderBy(f => f.EndMs).ToList();
        var intervals = new List<double>();
        for (int i = 1; i < drawn.Count; i++)
        {
            double gap = drawn[i].EndMs - drawn[i - 1].EndMs;
            if (continuousInteraction || gap <= MaxContinuousGapMs)
            {
                intervals.Add(gap);
            }
        }

        var values = records.OfType<SceneValueRecord>()
            .Where(v => window.Contains(v.TimeMs))
            .GroupBy(v => v.Name, StringComparer.Ordinal)
            .ToDictionary(g => g.Key, g => Distribution.From(g.Select(v => v.Value))!, StringComparer.Ordinal);

        return phase with
        {
            FrameIntervalMs = Distribution.From(intervals),
            DroppedFrames = drawn.Count < 2 ? null : intervals.Count(i => i > threshold),
            LongFrames = drawn.Count == 0 ? null : drawn.Count(f => f.EndMs - f.BeginMs > threshold),
            LongestFrameGapMs = intervals.Count == 0 ? null : intervals.Max(),
            SceneValues = values.Count == 0 ? null : values,
        };
    }

    static IReadOnlyDictionary<string, Distribution>? InputLatencyByKind(List<TraceRecord> records, List<FrameSpan> frames,
        GateOptions options)
    {
        var byKind = new SortedDictionary<string, Distribution>(StringComparer.Ordinal);
        foreach (string kind in records.OfType<InputInjectedRecord>().Where(IsRealInput).Select(i => i.Kind).Distinct())
        {
            if (ComputeInputLatency(records, frames, options, i => i.Kind == kind) is { } distribution)
            {
                byKind[kind] = distribution;
            }
        }
        return byKind.Count == 0 ? null : byKind;
    }

    // Request (InputInjected with the popup's input id) to the first drawn frame that ends after the popup's Opened event.
    static Distribution? ComputePopupOpen(List<TraceRecord> records, List<FrameSpan> frames, GateOptions options)
        => Distribution.From(PopupOpenTimes(records, frames, options).Select(p => p.Ms));

    static List<(string Popup, double Ms)> PopupOpenTimes(List<TraceRecord> records, List<FrameSpan> frames, GateOptions options)
    {
        var drawnEnds = frames.Where(f => f.Drawn).Select(f => f.EndMs).OrderBy(t => t).ToList();
        var inputs = records.OfType<InputInjectedRecord>().ToList();
        var times = new List<(string Popup, double Ms)>();
        foreach (var opened in records.OfType<PopupOpenedRecord>())
        {
            var request = inputs.LastOrDefault(i => i.InputId == opened.InputId && i.TimeMs <= opened.TimeMs);
            if (request is null)
            {
                continue;
            }
            double frameEnd = drawnEnds.FirstOrDefault(t => t > opened.TimeMs, double.NaN);
            if (!double.IsNaN(frameEnd) && frameEnd - opened.TimeMs <= options.MaxInputToFrameMs)
            {
                times.Add((opened.Popup, frameEnd - request.TimeMs));
            }
        }
        return times;
    }

    // Each step's memory is the last sample before the next step starts (the settled value), or before the end of the
    // phase containing the step for the last one.
    static IReadOnlyList<StepMetrics>? ComputeSteps(List<TraceRecord> records, List<PhaseWindow> phases, List<FrameSpan> frames)
    {
        var steps = records.OfType<SceneStepRecord>().OrderBy(s => s.TimeMs).ToList();
        if (steps.Count == 0)
        {
            return null;
        }
        var samples = records.OfType<MemorySampleRecord>().OrderBy(s => s.TimeMs).ToList();
        double lastTime = records.Count == 0 ? 0 : records[^1].TimeMs;
        var result = new List<StepMetrics>();
        for (int i = 0; i < steps.Count; i++)
        {
            double begin = steps[i].TimeMs;
            double end = i + 1 < steps.Count
                ? steps[i + 1].TimeMs
                : phases.FirstOrDefault(p => p.Contains(begin))?.EndMs ?? lastTime + 1;
            var sample = samples.LastOrDefault(s => s.TimeMs >= begin && s.TimeMs < end);
            var step = sample is null
                ? new StepMetrics(steps[i].Step, steps[i].Name, steps[i].Windows, double.NaN, double.NaN, double.NaN, 0, 0)
                : new StepMetrics(steps[i].Step, steps[i].Name, steps[i].Windows, sample.PrivateBytes / MB,
                    sample.WorkingSetBytes / MB, sample.GpuLocalBytes / MB, sample.GdiObjects, sample.UserObjects);
            result.Add(DescribeStepWindow(step, begin, end, records, frames));
        }
        return result;
    }

    // M1 per window: least-squares slope of private MB over the open window count, using the baseline and open-* steps.
    static double? MarginalPerWindow(IReadOnlyList<StepMetrics>? steps) => Marginal(steps, "open-");

    // Least-squares slope of private MB over StepMetrics.Windows for the baseline step and the steps named prefix*.
    static double? Marginal(IReadOnlyList<StepMetrics>? steps, string prefix)
    {
        if (steps is null)
        {
            return null;
        }
        var points = steps
            .Where(s => !double.IsNaN(s.PrivateMB) &&
                (s.Name == "baseline" || s.Name.StartsWith(prefix, StringComparison.Ordinal)))
            .Select(s => ((double)s.Windows, s.PrivateMB))
            .ToList();
        if (points.Count < 2)
        {
            return null;
        }
        double meanX = points.Average(p => p.Item1);
        double meanY = points.Average(p => p.PrivateMB);
        double denominator = points.Sum(p => (p.Item1 - meanX) * (p.Item1 - meanX));
        return denominator == 0 ? null : points.Sum(p => (p.Item1 - meanX) * (p.PrivateMB - meanY)) / denominator;
    }

    // Share of the private bytes added between the first step and the peak that is gone by the last step.
    static double? ReclaimedFraction(IReadOnlyList<StepMetrics>? steps)
    {
        var valid = steps?.Where(s => !double.IsNaN(s.PrivateMB)).ToList();
        if (valid is null || valid.Count < 3)
        {
            return null;
        }
        double first = valid[0].PrivateMB, last = valid[^1].PrivateMB, peak = valid.Max(s => s.PrivateMB);
        return peak - first <= 0 ? null : (peak - last) / (peak - first);
    }

    sealed record UnsupportedCounts(IReadOnlyDictionary<string, int>? ByType, IReadOnlyDictionary<string, int>? ByDetail,
        IReadOnlyList<string> Expected, IReadOnlyList<string> Unexpected);

    static UnsupportedCounts CountUnsupported(string? scene, List<TraceRecord> records, bool fallbackObserved = false)
    {
        var events = records.OfType<UnsupportedContentRecord>().ToList();
        var byType = new SortedDictionary<string, int>(StringComparer.Ordinal);
        var byDetail = new SortedDictionary<string, int>(StringComparer.Ordinal);
        var expected = new List<string>();
        var unexpected = new List<string>();
        foreach (var e in events)
        {
            string key = $"{e.TypeName}|{e.Detail}";
            byType[key] = byType.GetValueOrDefault(key) + 1;
            byDetail[e.Detail] = byDetail.GetValueOrDefault(e.Detail) + 1;
            var list = UnsupportedContentPolicy.IsExpected(scene, e.TypeName, e.Detail, fallbackObserved) ? expected : unexpected;
            if (!list.Contains(key))
            {
                list.Add(key);
            }
        }
        return new UnsupportedCounts(byType.Count == 0 ? null : byType, byDetail.Count == 0 ? null : byDetail, expected, unexpected);
    }

    static GateCheck EvaluateUnsupported(List<string> unsupported, UnsupportedCounts counts)
    {
        if (unsupported.Count == 0)
        {
            return new(GateNames.NoUnsupportedContent, true, "none");
        }
        if (counts.Unexpected.Count == 0)
        {
            return new(GateNames.NoUnsupportedContent, true, $"only expected for this scene: {string.Join("; ", counts.Expected)}");
        }
        string detail = string.Join("; ", unsupported);
        if (counts.Expected.Count > 0)
        {
            detail += $" (expected for this scene: {string.Join("; ", counts.Expected)})";
        }
        return new(GateNames.NoUnsupportedContent, false, detail);
    }
}

// Unsupported content a scene is built to contain. Immediate modes report it (UnsupportedContent) and it must not fail
// that scene's gate; any other unsupported content still does. See the test plan's "Extended scenes" gate rules.
public static class UnsupportedContentPolicy
{
    // Approximations (detail ends in ":approximated", e.g. Mica drawn as its fallback color) are allowed everywhere. In a
    // scene built to make its window fall back, the content that caused the observed fallback is expected as well.
    public static bool IsExpected(string? sceneLabel, string typeName, string detail, bool fallbackObserved = false)
    {
        if (Contains(detail, ":approximated") || (fallbackObserved && WindowFallbackPolicy.IsExpected(sceneLabel)))
        {
            return true;
        }
        switch (SceneCatalog.FromLabel(sceneLabel))
        {
            case SceneId.Animated when sceneLabel != "Animated-NoProgressRing":
                // ProgressRing renders through AnimatedVisualPlayer (a Lottie Composition visual hand-in).
                return Contains(typeName, "AnimatedVisualPlayer") || Contains(typeName, "ProgressRing");
            case SceneId.Layered:
                // The acrylic card is the scene's only non-solid brush; ThemeShadow and the Mica backdrop are
                // Composition effects. Rounded clips and everything else are real painter gaps.
                return detail == "Brush" ||
                    new[] { "Shadow", "Acrylic", "Mica", "Backdrop" }.Any(k => Contains(detail, k) || Contains(typeName, k));
            default:
                return false;
        }
    }

    static bool Contains(string text, string value) => text.Contains(value, StringComparison.OrdinalIgnoreCase);
}
