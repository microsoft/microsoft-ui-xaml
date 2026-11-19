namespace RenderBench.Analysis;

public static class GateNames
{
    public const string HarnessRunStarted = "HarnessRunStarted";
    public const string HarnessCompleted = "HarnessCompleted";
    public const string BackendSelectedPresent = "BackendSelectedPresent";
    public const string RequestReachedRuntime = "RequestReachedRuntime";
    public const string EffectiveMatchesRequested = "EffectiveMatchesRequested";
    public const string DeviceMatchesMode = "DeviceMatchesMode";
    public const string FramesObserved = "FramesObserved";
    public const string SteadyStateIsIdle = "SteadyStateIsIdle";
    public const string MemorySampled = "MemorySampled";
    public const string NoUnsupportedContent = "NoUnsupportedContent";
}

public sealed record GateOptions
{
    // Steady phase has no scripted activity; a few frames per minute (caret, tooltips, theme) are tolerated.
    public double MaxSteadyFramesPerMinute { get; init; } = 6;
    // Input that is not followed by a drawn frame within this window is reported as unmatched.
    public double MaxInputToFrameMs { get; init; } = 1000;
}

public static class MetricsCalculator
{
    public const string SchemaVersion = "1";
    const double MB = 1024.0 * 1024.0;

    public static RunMetrics Compute(IEnumerable<TraceRecord> allRecords, GateOptions? options = null)
    {
        options ??= new GateOptions();
        var ordered = allRecords.OrderBy(r => r.TimeMs).ToList();

        var runStarted = ordered.OfType<RunStartedRecord>().FirstOrDefault();
        int? pid = runStarted?.ProcessId ?? ordered.OfType<BackendSelectedRecord>().FirstOrDefault()?.ProcessId;
        var records = pid is null ? [] : ordered.Where(r => r.ProcessId == pid).ToList();

        var selected = records.OfType<BackendSelectedRecord>().FirstOrDefault();
        var unsupported = records.OfType<UnsupportedContentRecord>()
            .Select(u => $"{BackendModes.Label(u.Mode)}: {u.TypeName} {u.Detail}".Trim())
            .Distinct()
            .ToList();
        var device = selected is null
            ? records.OfType<DeviceInfoRecord>().FirstOrDefault()
            : records.OfType<DeviceInfoRecord>().FirstOrDefault(d => d.Mode == selected.Effective)
                ?? records.OfType<DeviceInfoRecord>().FirstOrDefault();

        var identity = new IdentityMetrics(runStarted?.RequestedBackend, selected?.Requested, selected?.Effective,
            selected?.Reason, device?.IsWarp, device?.AdapterLuid, unsupported);

        var frames = PairFrames(records);
        var phases = BuildPhaseWindows(records);
        var phaseMetrics = phases.Select(w => ComputePhase(w, records, frames, device?.AdapterLuid)).ToList();

        var samples = records.OfType<MemorySampleRecord>().ToList();
        double? peakPrivate = samples.Count == 0 ? null : samples.Max(s => s.PrivateBytes) / MB;
        double? peakGpu = samples.Count == 0 ? null : samples.Max(s => s.GpuLocalBytes) / MB;

        var processStart = pid is null ? null : records.OfType<ProcessStartRecord>().FirstOrDefault();
        var sceneReady = records.OfType<SceneReadyRecord>().FirstOrDefault();
        var firstFrame = frames.FirstOrDefault(f => f.Drawn);
        var startup = new StartupMetrics(
            processStart is not null && sceneReady is not null ? sceneReady.TimeMs - processStart.TimeMs : null,
            processStart is not null && firstFrame is not null ? firstFrame.EndMs - processStart.TimeMs : null);

        var latency = ComputeInputLatency(records, frames, options);

        var gate = EvaluateGate(runStarted, records, selected, device, phaseMetrics, unsupported, options);

        return new RunMetrics(SchemaVersion, runStarted?.Scene ?? sceneReady?.Scene, pid, identity, startup, phaseMetrics,
            latency, peakPrivate, peakGpu, gate);
    }

    sealed record FrameSpan(uint Frame, double BeginMs, double EndMs, bool Drawn);

    sealed record PhaseWindow(BenchPhase Phase, double BeginMs, double EndMs)
    {
        public bool Contains(double timeMs) => timeMs >= BeginMs && timeMs < EndMs;
    }

    static List<FrameSpan> PairFrames(List<TraceRecord> records)
    {
        var open = new Dictionary<uint, double>();
        var spans = new List<FrameSpan>();
        foreach (var record in records)
        {
            switch (record)
            {
                case FrameBeginRecord begin:
                    open[begin.Frame] = begin.TimeMs;
                    break;
                case FrameEndRecord end when open.Remove(end.Frame, out var beginMs):
                    spans.Add(new FrameSpan(end.Frame, beginMs, end.TimeMs, end.Drawn));
                    break;
            }
        }
        return spans;
    }

    static List<PhaseWindow> BuildPhaseWindows(List<TraceRecord> records)
    {
        double lastTime = records.Count == 0 ? 0 : records[^1].TimeMs;
        var windows = new List<PhaseWindow>();
        foreach (var begin in records.OfType<PhaseRecord>().Where(p => p.IsBegin))
        {
            var end = records.OfType<PhaseRecord>().FirstOrDefault(p => !p.IsBegin && p.Phase == begin.Phase && p.TimeMs >= begin.TimeMs);
            windows.Add(new PhaseWindow(begin.Phase, begin.TimeMs, end?.TimeMs ?? lastTime));
        }
        return windows;
    }

    static PhaseMetrics ComputePhase(PhaseWindow window, List<TraceRecord> records, List<FrameSpan> frames, ulong? renderAdapterLuid)
    {
        var inPhase = records.Where(r => window.Contains(r.TimeMs)).ToList();
        var phaseFrames = frames.Where(f => window.Contains(f.EndMs)).ToList();
        int drawn = phaseFrames.Count(f => f.Drawn);
        double minutes = (window.EndMs - window.BeginMs) / 60000.0;

        var walks = inPhase.OfType<RenderWalkRecord>().ToList();
        var paintEnds = inPhase.OfType<PaintEndRecord>().ToList();
        var paintBegins = records.OfType<PaintBeginRecord>().ToList();
        var paintMs = paintEnds
            .Select(e => (End: e, Begin: paintBegins.LastOrDefault(b => b.Frame == e.Frame && b.RootId == e.RootId && b.TimeMs <= e.TimeMs)))
            .Where(p => p.Begin is not null)
            .Select(p => p.End.TimeMs - p.Begin!.TimeMs);

        var inventories = inPhase.OfType<ResourceInventoryRecord>().ToList();
        var lastInventory = inventories.LastOrDefault();

        var samples = records.OfType<MemorySampleRecord>().Where(s => s.Phase == window.Phase).OrderBy(s => s.TimeMs).ToList();
        double? cpuRate = null;
        if (samples.Count >= 2 && samples[^1].TimeMs > samples[0].TimeMs)
        {
            double cpu = (samples[^1].CpuUserMs + samples[^1].CpuKernelMs) - (samples[0].CpuUserMs + samples[0].CpuKernelMs);
            cpuRate = cpu / ((samples[^1].TimeMs - samples[0].TimeMs) / 1000.0);
        }

        var adapterSamples = renderAdapterLuid is { } luid
            ? records.OfType<GpuAdapterMemoryRecord>().Where(s => s.Phase == window.Phase && s.AdapterLuid == luid).ToList()
            : [];

        return new PhaseMetrics(
            window.Phase,
            window.EndMs - window.BeginMs,
            drawn,
            phaseFrames.Count,
            minutes > 0 ? drawn / minutes : 0,
            Distribution.From(phaseFrames.Select(f => f.EndMs - f.BeginMs)),
            walks.Count == 0 ? null : walks.Average(w => (double)w.Visited),
            walks.Count == 0 ? null : walks.Average(w => (double)w.Rendered),
            Distribution.From(paintMs),
            (ulong)paintEnds.Sum(p => (decimal)p.DamagedPixels),
            (ulong)paintEnds.Sum(p => (decimal)p.PaintedPixels),
            (ulong)inventories.Sum(i => (decimal)i.UploadBytes),
            Median(samples.Select(s => s.PrivateBytes / MB)),
            Median(samples.Select(s => s.WorkingSetBytes / MB)),
            Median(samples.Select(s => s.GpuLocalBytes / MB)),
            Median(samples.Select(s => s.GpuNonLocalBytes / MB)),
            samples.Count == 0 ? null : samples.Max(s => s.GdiObjects),
            samples.Count == 0 ? null : samples.Max(s => s.UserObjects),
            cpuRate,
            Slope(samples.Select(s => (s.TimeMs / 60000.0, s.PrivateBytes / MB)).ToList()),
            lastInventory is null ? null : lastInventory.SurfaceBytes / MB,
            lastInventory is null ? null : lastInventory.CacheBytes / MB,
            inPhase.OfType<UpdateAppliedRecord>().Count(),
            GpuPaintMs: Distribution.From(GpuPaintMsInPhase(window, records, frames)),
            RenderAdapterGpuLocalMBMedian: Median(adapterSamples.Select(s => s.LocalBytes / MB)),
            RenderAdapterGpuNonLocalMBMedian: Median(adapterSamples.Select(s => s.NonLocalBytes / MB)));
    }

    static Distribution? ComputeInputLatency(List<TraceRecord> records, List<FrameSpan> frames, GateOptions options)
    {
        var drawnEnds = frames.Where(f => f.Drawn).Select(f => f.EndMs).OrderBy(t => t).ToList();
        var latencies = new List<double>();
        foreach (var input in records.OfType<InputInjectedRecord>())
        {
            int index = drawnEnds.BinarySearch(input.TimeMs);
            if (index < 0)
            {
                index = ~index;
            }
            else
            {
                index++;
            }
            if (index < drawnEnds.Count && drawnEnds[index] - input.TimeMs <= options.MaxInputToFrameMs)
            {
                latencies.Add(drawnEnds[index] - input.TimeMs);
            }
        }
        return Distribution.From(latencies);
    }

    static List<GateCheck> EvaluateGate(
        RunStartedRecord? runStarted,
        List<TraceRecord> records,
        BackendSelectedRecord? selected,
        DeviceInfoRecord? device,
        List<PhaseMetrics> phases,
        List<string> unsupported,
        GateOptions options)
    {
        var checks = new List<GateCheck>();
        checks.Add(new(GateNames.HarnessRunStarted, runStarted is not null,
            runStarted is null ? "no RunStarted event from Microsoft-WinUI-RenderBench" : $"scene {runStarted.Scene}"));

        var completed = records.OfType<RunCompletedRecord>().LastOrDefault();
        checks.Add(new(GateNames.HarnessCompleted, completed is { Status: 0 },
            completed is null ? "no RunCompleted event" : $"status {completed.Status} {completed.Detail}"));

        checks.Add(new(GateNames.BackendSelectedPresent, selected is not null,
            selected is null ? "no BackendSelected event from Microsoft-WinUI-RenderingBackend" : "present"));

        BackendMode harnessMode = BackendMode.Composition;
        bool harnessParsed = string.IsNullOrEmpty(runStarted?.RequestedBackend) ||
            BackendModes.TryParse(runStarted!.RequestedBackend, out harnessMode);
        bool requestReached = selected is not null && harnessParsed && selected.Requested == harnessMode;
        checks.Add(new(GateNames.RequestReachedRuntime, requestReached,
            $"harness '{runStarted?.RequestedBackend}' runtime {selected?.Requested.ToString() ?? "n/a"}"));

        bool effectiveOk = selected is not null && selected.Effective == selected.Requested;
        checks.Add(new(GateNames.EffectiveMatchesRequested, effectiveOk,
            selected is null ? "n/a" : $"requested {selected.Requested} effective {selected.Effective} reason {selected.Reason}"));

        checks.Add(EvaluateDevice(selected, device));

        var interaction = phases.FirstOrDefault(p => p.Phase == BenchPhase.Interaction);
        checks.Add(new(GateNames.FramesObserved, interaction is { FramesDrawn: > 0 },
            interaction is null ? "no Interaction phase" : $"{interaction.FramesDrawn} drawn frames in Interaction"));

        var steady = phases.FirstOrDefault(p => p.Phase == BenchPhase.Steady);
        checks.Add(new(GateNames.SteadyStateIsIdle,
            steady is not null && steady.FramesDrawnPerMinute <= options.MaxSteadyFramesPerMinute,
            steady is null ? "no Steady phase" : $"{steady.FramesDrawnPerMinute:F1} drawn frames/min (max {options.MaxSteadyFramesPerMinute})"));

        checks.Add(new(GateNames.MemorySampled, steady?.PrivateMBMedian is not null,
            steady?.PrivateMBMedian is null ? "no memory samples in Steady" : "present"));

        checks.Add(new(GateNames.NoUnsupportedContent, unsupported.Count == 0,
            unsupported.Count == 0 ? "none" : string.Join("; ", unsupported)));
        return checks;
    }

    static GateCheck EvaluateDevice(BackendSelectedRecord? selected, DeviceInfoRecord? device)
    {
        if (selected is null)
        {
            return new(GateNames.DeviceMatchesMode, false, "n/a");
        }
        switch (selected.Effective)
        {
            case BackendMode.ImmediateGdiPlus:
                return new(GateNames.DeviceMatchesMode, true, "GDI+ painter is CPU-only; presentation device not constrained");
            case BackendMode.ImmediateD2DSoftware:
                return new(GateNames.DeviceMatchesMode,
                    device is { Mode: BackendMode.ImmediateD2DSoftware, IsWarp: true },
                    device is null ? "no DeviceInfo for D2D-SW" : $"device mode {device.Mode} warp {device.IsWarp}");
            case BackendMode.ImmediateD2DHardware:
                return new(GateNames.DeviceMatchesMode,
                    device is { Mode: BackendMode.ImmediateD2DHardware, IsWarp: false },
                    device is null ? "no DeviceInfo for D2D-HW" : $"device mode {device.Mode} warp {device.IsWarp}");
            default:
                // Composition legitimately runs on WARP when no GPU is present; record, don't fail.
                return new(GateNames.DeviceMatchesMode, device is not null,
                    device is null ? "no DeviceInfo for Composition" : $"warp {device.IsWarp}");
        }
    }

    static double? Median(IEnumerable<double> values)
    {
        var sorted = values.OrderBy(v => v).ToArray();
        return sorted.Length == 0 ? null : Distribution.Percentile(sorted, 50);
    }

    // Least-squares slope of y over x.
    static double? Slope(List<(double X, double Y)> points)
    {
        if (points.Count < 3)
        {
            return null;
        }
        double meanX = points.Average(p => p.X);
        double meanY = points.Average(p => p.Y);
        double denominator = points.Sum(p => (p.X - meanX) * (p.X - meanX));
        return denominator == 0 ? null : points.Sum(p => (p.X - meanX) * (p.Y - meanY)) / denominator;
    }

    // ---- Added by the D2D-HW branch ----

    // GpuPaintTime arrives a frame or two after its paint; attribute it to the phase containing its frame's FrameEnd,
    // falling back to the event time when that frame was not traced.
    static IEnumerable<double> GpuPaintMsInPhase(PhaseWindow window, List<TraceRecord> records, List<FrameSpan> frames)
    {
        var frameEnds = new Dictionary<uint, double>();
        foreach (var frame in frames)
        {
            frameEnds[frame.Frame] = frame.EndMs;
        }
        return records.OfType<GpuPaintTimeRecord>()
            .Where(g => window.Contains(frameEnds.TryGetValue(g.Frame, out var endMs) ? endMs : g.TimeMs))
            .Select(g => g.GpuMicroseconds / 1000.0);
    }

    // Metrics for a run that was not executed, e.g. D2D-HW on a machine without a qualifying hardware adapter. Reports
    // show it as n/a; the gate treats it as skipped rather than failed.
    public static RunMetrics Skipped(string scene, string harnessRequested, string reason)
    {
        BackendMode? requested = BackendModes.TryParse(harnessRequested, out var mode) ? mode : null;
        return new RunMetrics(SchemaVersion, scene, null,
            new IdentityMetrics(harnessRequested, requested, null, null, null, null, []),
            new StartupMetrics(null, null), [], null, null, null, [], reason);
    }
}
