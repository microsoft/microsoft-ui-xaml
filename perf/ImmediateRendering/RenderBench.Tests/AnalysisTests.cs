using RenderBench.Analysis;
using static RenderBench.Tests.Check;

namespace RenderBench.Tests;

// Builds a synthetic, well-formed run; individual tests perturb it.
sealed class SyntheticRun
{
    public const int Pid = 4242;
    public const int OtherPid = 777;

    public List<TraceRecord> Records { get; } = [];
    public string HarnessRequested { get; set; } = "d2d-hw";
    public BackendMode Requested { get; set; } = BackendMode.ImmediateD2DHardware;
    public BackendMode Effective { get; set; } = BackendMode.ImmediateD2DHardware;
    public FallbackReason Reason { get; set; } = FallbackReason.None;
    public bool IsWarp { get; set; }
    public BackendMode DeviceMode { get; set; } = BackendMode.ImmediateD2DHardware;
    public int SteadyDrawnFrames { get; set; }
    public bool Completed { get; set; } = true;
    public bool EmitBackendSelected { get; set; } = true;

    // Timeline (ms): process start 0, RunStarted 100, scene ready 400, first frame 450,
    // Warmup [500,1500), Steady [1500,61500), Interaction [61500,71500), Recovery [71500,72500).
    public List<TraceRecord> Build()
    {
        Records.Clear();
        Add(new ProcessStartRecord(0, Pid, "RenderBench.exe"));
        Add(new ProcessStartRecord(5, OtherPid, "Other.exe"));
        Add(new RunStartedRecord(100, Pid, "Dashboard", HarnessRequested, 1, 1280, 800, 1.0));
        if (EmitBackendSelected)
        {
            Add(new BackendSelectedRecord(120, Pid, Requested, Effective, Reason, 0xF));
        }
        Add(new DeviceInfoRecord(130, Pid, DeviceMode, 0x1234, IsWarp, 0xb100));
        Add(new SceneReadyRecord(400, Pid, "Dashboard", 1600));
        uint frame = 1;
        Frame(frame++, 440, 450, drawn: true);

        Phase(BenchPhase.Warmup, 500, 1500);
        Frame(frame++, 600, 608, drawn: true);

        Phase(BenchPhase.Steady, 1500, 61500);
        for (int i = 0; i < 5; i++)
        {
            Add(new MemorySampleRecord(2000 + i * 10000, Pid, BenchPhase.Steady, (100 + i) * MB, 150 * MB, 160 * MB,
                40 * MB, 10 * MB, 30, 20, 1000 + i * 10, 500));
        }
        for (int i = 0; i < SteadyDrawnFrames; i++)
        {
            Frame(frame++, 3000 + i * 1000, 3004 + i * 1000, drawn: true);
        }
        Add(new ResourceInventoryRecord(50000, Pid, Effective, 8 * MB, 2 * MB, 12, 0));

        Phase(BenchPhase.Interaction, 61500, 71500);
        // 100 frames at 100 ms, each 4..13 ms long, with paint and walk stats.
        for (int i = 0; i < 100; i++)
        {
            double begin = 61550 + i * 100;
            double length = 4 + (i % 10);
            Add(new FrameBeginRecord(begin, Pid, frame));
            Add(new RenderWalkRecord(begin + 1, Pid, frame, 200, 20));
            Add(new PaintBeginRecord(begin + 1.5, Pid, frame, 1));
            Add(new PaintEndRecord(begin + 3.5, Pid, frame, 1, 1000, 1200, 40, 3));
            Add(new FrameEndRecord(begin + length, Pid, frame, true));
            Add(new UpdateAppliedRecord(begin - 10, Pid, i, 20));
            frame++;
        }
        // Inputs 5 ms before frames 10,20,...; latency = 5 + frame length (4 + (i % 10)) where i % 10 == 0 => 9 ms.
        for (int k = 1; k <= 9; k++)
        {
            Add(new InputInjectedRecord(61550 + k * 1000 - 5, Pid, k, "click", "tile"));
        }
        for (int i = 0; i < 3; i++)
        {
            Add(new MemorySampleRecord(62000 + i * 4000, Pid, BenchPhase.Interaction, (110 + i * 2) * MB, 160 * MB, 170 * MB,
                45 * MB, 10 * MB, 31, 20, 2000 + i * 400, 600 + i * 100));
        }
        Add(new ResourceInventoryRecord(71000, Pid, Effective, 8 * MB, 3 * MB, 14, 5 * MB));

        Phase(BenchPhase.Recovery, 71500, 72500);
        if (Completed)
        {
            Add(new RunCompletedRecord(72600, Pid, 0, "ok"));
        }

        // Noise from another process must be ignored.
        Add(new FrameBeginRecord(62000, OtherPid, 1));
        Add(new FrameEndRecord(62500, OtherPid, 1, true));
        Add(new BackendSelectedRecord(62000, OtherPid, BackendMode.Composition, BackendMode.Composition, FallbackReason.None, 1));
        return Records.OrderBy(r => r.TimeMs).ToList();
    }

    const long MB = 1024 * 1024;

    void Add(TraceRecord record) => Records.Add(record);

    void Phase(BenchPhase phase, double begin, double end)
    {
        Add(new PhaseRecord(begin, Pid, phase, true));
        Add(new PhaseRecord(end, Pid, phase, false));
    }

    void Frame(uint frame, double begin, double end, bool drawn)
    {
        Add(new FrameBeginRecord(begin, Pid, frame));
        Add(new FrameEndRecord(end, Pid, frame, drawn));
    }
}

static class AnalysisTests
{
    public static IEnumerable<TestCase> All() =>
    [
        new("Analysis.Percentile.InterpolatesBetweenRanks", PercentileInterpolates),
        new("Analysis.Percentile.EmptyIsNull", () => Expect(Distribution.From([]) is null, "empty distribution")),
        new("Analysis.Gate.WellFormedRunPasses", WellFormedRunPasses),
        new("Analysis.Gate.FallbackFailsIdentity", FallbackFailsIdentity),
        new("Analysis.Gate.MissingBackendSelectedFails", MissingBackendSelectedFails),
        new("Analysis.Gate.HarnessRequestMismatchFails", HarnessRequestMismatchFails),
        new("Analysis.Gate.SoftwareModeRequiresWarpDevice", SoftwareModeRequiresWarp),
        new("Analysis.Gate.HardwareModeRejectsWarpDevice", HardwareModeRejectsWarp),
        new("Analysis.Gate.IdleFramesInSteadyFail", IdleFramesFail),
        new("Analysis.Gate.IncompleteRunFails", IncompleteRunFails),
        new("Analysis.Gate.UnsupportedContentFails", UnsupportedContentFails),
        new("Analysis.Metrics.IgnoresOtherProcesses", IgnoresOtherProcesses),
        new("Analysis.Metrics.FrameTimesPerPhase", FrameTimesPerPhase),
        new("Analysis.Metrics.DamageAndPaint", DamageAndPaint),
        new("Analysis.Metrics.MemoryMediansPeaksAndSlope", MemoryMetrics),
        new("Analysis.Metrics.CpuRate", CpuRate),
        new("Analysis.Metrics.InputToFrameLatency", InputLatency),
        new("Analysis.Metrics.Startup", Startup),
        new("Analysis.Metrics.BackendResources", BackendResources),
        new("Analysis.Metrics.JsonRoundTrip", JsonRoundTrip),
    ];

    static RunMetrics Compute(SyntheticRun run) => MetricsCalculator.Compute(run.Build());

    static GateCheck GateFor(RunMetrics metrics, string name)
        => metrics.Gate.FirstOrDefault(g => g.Name == name) ?? throw new TestFailure($"gate check {name} missing");

    static void PercentileInterpolates()
    {
        var d = NotNull(Distribution.From([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]), "distribution");
        Near(5.5, d.P50, 1e-9, "p50");
        Near(9.55, d.P95, 1e-9, "p95");
        Near(10, d.Max, 1e-9, "max");
        Equal(10, d.Count, "count");
    }

    static void WellFormedRunPasses()
    {
        var metrics = Compute(new SyntheticRun());
        foreach (var check in metrics.Gate)
        {
            Expect(check.Passed, $"{check.Name}: {check.Detail}");
        }
        Expect(metrics.GatePassed, "gate");
        Equal(BackendMode.ImmediateD2DHardware, metrics.Identity.Effective, "effective");
    }

    static void FallbackFailsIdentity()
    {
        var metrics = Compute(new SyntheticRun
        {
            Effective = BackendMode.Composition,
            Reason = FallbackReason.NotImplemented,
            DeviceMode = BackendMode.Composition,
        });
        var check = GateFor(metrics, GateNames.EffectiveMatchesRequested);
        Expect(!check.Passed, "fallback must fail identity gate");
        Expect(check.Detail.Contains("NotImplemented"), $"detail names reason: {check.Detail}");
        Expect(!metrics.GatePassed, "gate");
    }

    static void MissingBackendSelectedFails()
    {
        var metrics = Compute(new SyntheticRun { EmitBackendSelected = false });
        Expect(!GateFor(metrics, GateNames.BackendSelectedPresent).Passed, "BackendSelected missing");
    }

    static void HarnessRequestMismatchFails()
    {
        var metrics = Compute(new SyntheticRun { HarnessRequested = "gdiplus" });
        Expect(!GateFor(metrics, GateNames.RequestReachedRuntime).Passed, "harness/runtime mismatch");
    }

    static void SoftwareModeRequiresWarp()
    {
        var run = new SyntheticRun
        {
            HarnessRequested = "d2d-sw",
            Requested = BackendMode.ImmediateD2DSoftware,
            Effective = BackendMode.ImmediateD2DSoftware,
            DeviceMode = BackendMode.ImmediateD2DSoftware,
            IsWarp = false,
        };
        Expect(!GateFor(Compute(run), GateNames.DeviceMatchesMode).Passed, "d2d-sw on hardware device must fail");
        run.IsWarp = true;
        Expect(GateFor(Compute(run), GateNames.DeviceMatchesMode).Passed, "d2d-sw on WARP passes");
    }

    static void HardwareModeRejectsWarp()
    {
        var metrics = Compute(new SyntheticRun { IsWarp = true });
        Expect(!GateFor(metrics, GateNames.DeviceMatchesMode).Passed, "d2d-hw on WARP must fail");
    }

    static void IdleFramesFail()
    {
        var metrics = Compute(new SyntheticRun { SteadyDrawnFrames = 30 });
        var steady = NotNull(metrics.Phase(BenchPhase.Steady), "steady");
        Near(30, steady.FramesDrawnPerMinute, 0.01, "frames/min");
        Expect(!GateFor(metrics, GateNames.SteadyStateIsIdle).Passed, "idle gate");
        Expect(GateFor(Compute(new SyntheticRun { SteadyDrawnFrames = 2 }), GateNames.SteadyStateIsIdle).Passed,
            "a couple of frames per minute is tolerated");
    }

    static void IncompleteRunFails()
        => Expect(!GateFor(Compute(new SyntheticRun { Completed = false }), GateNames.HarnessCompleted).Passed, "completed");

    static void UnsupportedContentFails()
    {
        var run = new SyntheticRun();
        var records = run.Build();
        records.Add(new UnsupportedContentRecord(70000, SyntheticRun.Pid, BackendMode.ImmediateD2DHardware, "AcrylicBrush", "effect"));
        var metrics = MetricsCalculator.Compute(records);
        Expect(!GateFor(metrics, GateNames.NoUnsupportedContent).Passed, "unsupported content");
        Expect(metrics.Identity.UnsupportedContent.Single().Contains("AcrylicBrush"), "lists type");
    }

    static void IgnoresOtherProcesses()
    {
        var metrics = Compute(new SyntheticRun());
        Equal(SyntheticRun.Pid, metrics.ProcessId, "pid");
        Equal(100, NotNull(metrics.Phase(BenchPhase.Interaction), "interaction").FramesDrawn, "frames in interaction");
    }

    static void FrameTimesPerPhase()
    {
        var interaction = NotNull(Compute(new SyntheticRun()).Phase(BenchPhase.Interaction), "interaction");
        var frames = NotNull(interaction.FrameMs, "frame distribution");
        Equal(100, frames.Count, "frames");
        Near(8.5, frames.P50, 1e-6, "p50 frame");
        Near(13, frames.Max, 1e-6, "max frame");
        Near(200, interaction.AvgElementsVisited, 1e-6, "visited");
        Near(20, interaction.AvgElementsRendered, 1e-6, "rendered");
        Equal(100, interaction.UpdatesApplied, "updates");
    }

    static void DamageAndPaint()
    {
        var interaction = NotNull(Compute(new SyntheticRun()).Phase(BenchPhase.Interaction), "interaction");
        Equal(100_000UL, interaction.DamagedPixels, "damaged");
        Equal(120_000UL, interaction.PaintedPixels, "painted");
        Near(2, NotNull(interaction.PaintMs, "paint").P50, 1e-6, "paint ms");
        Equal(5UL * 1024 * 1024, interaction.UploadBytes, "upload bytes");
    }

    static void MemoryMetrics()
    {
        var metrics = Compute(new SyntheticRun());
        var steady = NotNull(metrics.Phase(BenchPhase.Steady), "steady");
        Near(102, steady.PrivateMBMedian, 1e-6, "private median");
        Near(150, steady.WorkingSetMBMedian, 1e-6, "ws median");
        Near(40, steady.GpuLocalMBMedian, 1e-6, "gpu local");
        Near(10, steady.GpuNonLocalMBMedian, 1e-6, "gpu nonlocal");
        Equal(30, steady.GdiObjectsMax, "gdi");
        Equal(20, steady.UserObjectsMax, "user");
        // +1 MB per 10 s => 6 MB/min.
        Near(6, steady.PrivateMBPerMinuteSlope, 1e-6, "slope");
        Near(114, metrics.PeakPrivateMB, 1e-6, "peak private");
        Near(45, metrics.PeakGpuLocalMB, 1e-6, "peak gpu");
    }

    static void CpuRate()
    {
        // Interaction samples span 8 s; user +800 ms, kernel +200 ms => 125 ms of CPU per second.
        var interaction = NotNull(Compute(new SyntheticRun()).Phase(BenchPhase.Interaction), "interaction");
        Near(125, interaction.CpuMsPerSecond, 1e-6, "cpu ms/s");
    }

    static void InputLatency()
    {
        var latency = NotNull(Compute(new SyntheticRun()).InputToFrameMs, "latency");
        Equal(9, latency.Count, "inputs matched");
        Near(9, latency.P50, 1e-6, "latency p50");
    }

    static void Startup()
    {
        var startup = Compute(new SyntheticRun()).Startup;
        Near(400, startup.ProcessStartToSceneReadyMs, 1e-6, "to scene ready");
        Near(450, startup.ProcessStartToFirstFrameMs, 1e-6, "to first frame");
    }

    static void BackendResources()
    {
        var metrics = Compute(new SyntheticRun());
        Near(8, NotNull(metrics.Phase(BenchPhase.Steady), "steady").BackendSurfaceMB, 1e-6, "surface MB");
        Near(3, NotNull(metrics.Phase(BenchPhase.Interaction), "interaction").BackendCacheMB, 1e-6, "cache MB");
    }

    static void JsonRoundTrip()
    {
        var metrics = Compute(new SyntheticRun());
        var json = MetricsJson.Serialize(metrics);
        var back = MetricsJson.Deserialize(json);
        Equal(metrics.Identity.Effective, back.Identity.Effective, "effective");
        Equal(metrics.Gate.Count, back.Gate.Count, "gate count");
        Near(metrics.Phase(BenchPhase.Steady)!.PrivateMBMedian!.Value, back.Phase(BenchPhase.Steady)!.PrivateMBMedian, 1e-9, "private");
        Expect(json.Contains("\"ImmediateD2DHardware\""), "enums serialized as names");
    }
}
