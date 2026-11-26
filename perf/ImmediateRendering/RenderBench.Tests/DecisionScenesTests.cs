using RenderBench.Analysis;
using static RenderBench.Tests.Check;
using MiniRun = RenderBench.Tests.ExtendedScenesTests.MiniRun;

namespace RenderBench.Tests;

// Backend decision scenes S24-S32: options, per-step window metrics (settled/peak memory, CPU rate, DWM delta) and the
// sweep fits and summaries in RunMetrics.Decision.
static class DecisionScenesTests
{
    public static IEnumerable<TestCase> All() =>
    [
        new("Decision.Options.SceneNamesAndPlanIds", SceneNamesAndPlanIds),
        new("Decision.Options.DefaultSizesAndStepSeconds", DefaultSizesAndStepSeconds),
        new("Decision.Fit.SlopeInterceptRSquared", FitSlopeInterceptRSquared),
        new("Decision.Metrics.StepWindowCpuPeakAndDwm", StepWindowCpuPeakAndDwm),
        new("Decision.Metrics.PerWindowCostWithDwm", PerWindowCostWithDwm),
        new("Decision.Metrics.NavGrowthExcludesFirstUse", NavGrowthExcludesFirstUse),
        new("Decision.Metrics.SustainedThreadCpuRate", SustainedThreadCpuRate),
    ];

    const long MB = 1024 * 1024;

    static BenchOptions Parse(params string[] args)
    {
        Expect(BenchOptions.TryParse(args, out var options, out var error), error ?? "parse");
        return options;
    }

    static void SceneNamesAndPlanIds()
    {
        (string Text, SceneId Scene, string PlanId)[] cases =
        [
            ("winsize", SceneId.WindowSize, "S24"), ("density", SceneId.Density, "S25"), ("windowcount", SceneId.WindowCount, "S26"),
            ("background", SceneId.Background, "S27"), ("navchurn", SceneId.NavChurn, "S28"), ("sustained", SceneId.Sustained, "S29"),
            ("resize", SceneId.Resize, "S30"), ("minimal", SceneId.Minimal, "S31"), ("largecanvas", SceneId.LargeCanvas, "S32"), ("scrollphysics", SceneId.ScrollPhysics, "S33"),
        ];
        foreach (var (text, scene, planId) in cases)
        {
            Equal(scene, Parse("--scene", text).Scene, text);
            Equal(scene, Parse("--scene", planId.ToLowerInvariant()).Scene, planId);
            Equal(planId, SceneCatalog.Get(scene).PlanId, $"{text} plan id");
            Expect(SceneCatalog.IsDecisionScene(scene), $"{text} is a decision scene");
        }
        Expect(!SceneCatalog.IsDecisionScene(SceneId.AppShell), "S23 is not a decision scene");
    }

    static void DefaultSizesAndStepSeconds()
    {
        var tool = Parse("--scene", "windowcount");
        Equal((400, 300), (tool.Width, tool.Height), "S26 tool windows");
        var tray = Parse("--scene", "minimal");
        Equal((320, 200), (tray.Width, tray.Height), "S31 tray window");
        var sized = Parse("--scene", "minimal", "--width", "800", "--height", "600");
        Equal((800, 600), (sized.Width, sized.Height), "--width/--height override the scene default");
        Equal((1280, 800), (Parse("--scene", "winsize").Width, Parse("--scene", "winsize").Height), "S24 starts at the default size");

        Near(BenchOptions.DecisionStepSeconds, Parse("--scene", "density").StepSeconds, 0, "decision scenes hold longer steps");
        Near(2, Parse("--scene", "density", "--step-seconds", "2").StepSeconds, 0, "--step-seconds applies to decision scenes");
        Near(2, Parse("--scene", "multiwindow").StepSeconds, 0, "S9 keeps its default");
        Expect(!BenchOptions.TryParse(["--scene", "settings", "--step-seconds", "3"], out _, out _), "still rejected for S1");
    }

    static void FitSlopeInterceptRSquared()
    {
        var exact = NotNull(MetricsCalculator.Fit([(0.3, 100.6), (1.0, 102.0), (2.1, 104.2), (3.7, 107.4)]), "fit");
        Near(2, exact.Slope, 1e-9, "slope");
        Near(100, exact.Intercept, 1e-9, "intercept");
        Near(1, exact.RSquared, 1e-9, "exact line");
        Equal(4, exact.Points, "points");

        var noisy = NotNull(MetricsCalculator.Fit([(1, 10), (2, 13), (3, 11), (4, 15)]), "noisy");
        Near(1.3, noisy.Slope, 1e-9, "least-squares slope");
        Near(9, noisy.Intercept, 1e-9, "least-squares intercept");
        Expect(noisy.RSquared is > 0 and < 1, $"R² {noisy.RSquared}");

        Equal(null, MetricsCalculator.Fit([(2, 1), (2, 5)]), "one distinct x");
        Equal(null, MetricsCalculator.Fit([(1, 1)]), "one point");
        Equal(2, MetricsCalculator.Fit([(1, 1), (double.NaN, 9), (2, 3)])!.Points, "NaN points are skipped");
    }

    // Adds one step: the marker (with the sample BenchRunner.Step takes right before it), an early spike in the first
    // half of the window and settled samples in the second half.
    static void AddStep(MiniRun run, int index, string name, int quantity, double at, double length, double privateMB,
        double spikeMB = 0, double cpuMs = 0, double? dwmMB = null)
    {
        int pid = MiniRun.Pid;
        run.Extra.Add(new MemorySampleRecord(at, pid, BenchPhase.Interaction, (long)(privateMB * MB), 0, 0, 0, 0, 0, 0, 0, 0)
            with { CpuUserMs = cpuMs });
        run.Extra.Add(new SceneStepRecord(at + 0.1, pid, index, name, quantity));
        if (spikeMB > 0)
        {
            run.Extra.Add(new MemorySampleRecord(at + length * 0.2, pid, BenchPhase.Interaction, (long)(spikeMB * MB), 0, 0, 0, 0, 0, 0, 0, 0));
        }
        foreach (double f in new[] { 0.6, 0.8, 0.95 })
        {
            run.Extra.Add(new MemorySampleRecord(at + length * f, pid, BenchPhase.Interaction, (long)(privateMB * MB),
                (long)(privateMB * 2 * MB), 0, 0, 0, 0, 0, cpuMs + 100 * f * length / 1000, 0));
            if (dwmMB is { } dwm)
            {
                run.Extra.Add(new DwmMemoryRecord(at + length * f, pid, (int)BenchPhase.Interaction, (long)(dwm * MB), 0, 77,
                    CpuMs: 5000 + 40 * f * length / 1000));
            }
        }
    }

    static void DwmBaseline(MiniRun run, double mb)
    {
        run.Extra.Add(new DwmMemoryRecord(1, MiniRun.Pid, 0, (long)(mb * MB), 0, 77));
        // A failed query (-1) must not become the baseline or a sample.
        run.Extra.Add(new DwmMemoryRecord(2, MiniRun.Pid, 0, -1, -1, 0));
    }

    static void StepWindowCpuPeakAndDwm()
    {
        var run = new MiniRun { Scene = "Minimal" };
        DwmBaseline(run, 50);
        AddStep(run, 0, "idle", 1, MiniRun.InteractionBegin + 10, 1000, privateMB: 40, spikeMB: 55, cpuMs: 1000, dwmMB: 53);
        AddStep(run, 1, "after-gc", 1, MiniRun.InteractionBegin + 1010, 980, privateMB: 38, cpuMs: 1100, dwmMB: 52);
        var metrics = run.Compute();
        var steps = NotNull(metrics.Steps, "steps");
        var idle = steps[0];
        Near(40, idle.PrivateMBMedian, 1e-6, "settled median ignores the early spike");
        Near(55, idle.PeakPrivateMB, 1e-6, "peak includes it");
        Near(80, idle.WorkingSetMBMedian, 1e-6, "working set");
        // CPU from the opening sample (1000 ms) to the next step's opening sample (1100 ms) one second later.
        Near(100, idle.CpuMsPerSecond, 1e-6, "process CPU ms/s");
        Near(3, idle.DwmDeltaMB, 1e-6, "DWM delta vs the pre-window baseline");
        Near(43, idle.TotalMB, 1e-6, "total = private + DWM delta");
        Near(40, idle.DwmCpuMsPerSecond, 1e-6, "DWM CPU between the first and last DWM sample");

        var decision = NotNull(metrics.Decision, "decision");
        Near(50, decision.DwmBaselineMB, 1e-6, "baseline");
        Near(38, decision.Summary["FloorMB"], 1e-6, "floor is after-gc");
        Near(40, decision.Summary["FloorTotalMB"], 1e-6, "floor total");
        Near(100, decision.Summary["IdleCpuMsPerSecond"], 1e-6, "idle CPU");
        Equal(null, new MiniRun { Scene = "Settings" }.Compute().Decision, "no decision metrics for S1");
    }

    static void PerWindowCostWithDwm()
    {
        var run = new MiniRun { Scene = "WindowCount" };
        DwmBaseline(run, 100);
        int[] totals = [1, 2, 4, 8];
        for (int i = 0; i < totals.Length; i++)
        {
            int n = totals[i];
            double length = i == totals.Length - 1 ? MiniRun.InteractionEnd - (MiniRun.InteractionBegin + 10 + i * 400) : 400;
            // 30 MB + 5 MB per window in-process; DWM grows 1.5 MB per window.
            AddStep(run, i, $"windows-{n}", n, MiniRun.InteractionBegin + 10 + i * 400, length, 30 + 5 * n, dwmMB: 100 + 1.5 * n);
        }
        var decision = NotNull(run.Compute().Decision, "decision");
        Equal("window", decision.XUnit, "unit");
        Near(5, decision.PrivateFit!.Slope, 1e-6, "private fit slope");
        Near(1, decision.PrivateFit.RSquared, 1e-9, "R²");
        Near(5, decision.Summary["MBPerWindow"], 1e-6, "MB per extra window");
        Near(6.5, decision.Summary["TotalMBPerWindow"], 1e-6, "including DWM");
        Near(35, decision.Summary["FirstWindowMB"], 1e-6, "one window");
    }

    static void NavGrowthExcludesFirstUse()
    {
        var run = new MiniRun { Scene = "NavChurn" };
        // nav-0 = 100 MB; the first 20 navigations add 20 MB (first use); then 1 MB per 20 navigations.
        (int Navs, double MB)[] points = [(0, 100), (20, 120), (40, 121), (60, 122), (80, 123)];
        for (int i = 0; i < points.Length; i++)
        {
            double at = MiniRun.InteractionBegin + 10 + i * 390;
            double length = i == points.Length - 1 ? MiniRun.InteractionEnd - at : 390;
            AddStep(run, i, $"nav-{points[i].Navs}", points[i].Navs, at, length, points[i].MB);
        }
        run.Extra.Add(new ResourceInventoryRecord(MiniRun.InteractionBegin + 500, MiniRun.Pid, BackendMode.ImmediateGdiPlus, 8 * MB, 3 * MB, 40, 0));
        run.Extra.Add(new ResourceInventoryRecord(MiniRun.InteractionBegin + 1300, MiniRun.Pid, BackendMode.ImmediateGdiPlus, 8 * MB, 2 * MB, 30, 0));
        var metrics = run.Compute();
        var decision = NotNull(metrics.Decision, "decision");
        Near(5, decision.Summary["GrowthMBPer100Nav"], 1e-6, "slope from nav-20 on");
        Equal(4, decision.PrivateFit!.Points, "nav-0 not fitted");
        Near(20, decision.Summary["FirstUseDeltaMB"], 1e-6, "first-use cost");
        Near(23, decision.Summary["OverallDeltaMB"], 1e-6, "overall");
        Near(3, decision.Summary["CacheMBMax"], 1e-6, "cache max");
        Near(2, decision.Summary["CacheMBAtEnd"], 1e-6, "cache carried to the last (idle) step");
        Equal(null, metrics.Steps![0].BackendCacheMB, "no frame painted before nav-0 ended");
    }

    static void SustainedThreadCpuRate()
    {
        var run = new MiniRun { Scene = "Sustained" };
        double begin = MiniRun.InteractionBegin + 10;
        AddStep(run, 0, "pre", 0, begin, 500, 60, cpuMs: 100);
        AddStep(run, 1, "animating", 200, begin + 500, 1000, 64, cpuMs: 200);
        AddStep(run, 2, "stopped", 0, begin + 1500, MiniRun.InteractionEnd - begin - 1500, 63, cpuMs: 300);
        // UI thread 300 ms and rasterizer 150 ms over the 1 s animating window.
        run.Extra.Add(new ThreadCpuSampleRecord(begin + 500, MiniRun.Pid, BenchPhase.Interaction, 1000, 500, 0, 0, 10, 8));
        run.Extra.Add(new ThreadCpuSampleRecord(begin + 1500 - 1, MiniRun.Pid, BenchPhase.Interaction, 1300, 650, 0, 0, 10, 8));
        for (int f = 0; f < 50; f++)
        {
            run.DrawnFrame(begin + 510 + f * 19.5);
        }
        var decision = NotNull(run.Compute().Decision, "decision");
        Near(100, decision.Summary["AnimCpuMsPerSecond"], 1e-6, "process CPU");
        Near(300 / 0.999, decision.Summary["AnimUiMsPerSecond"], 1e-6, "UI thread ms/s");
        Near(150 / 0.999, decision.Summary["AnimWarpRasterizerMsPerSecond"], 1e-6, "rasterizer ms/s");
        Near(50, decision.Summary["AnimFps"], 1e-6, "50 drawn frames in 1 s");
        Near(4, decision.Summary["AnimDeltaMB"], 1e-6, "animating vs pre");
    }
}
