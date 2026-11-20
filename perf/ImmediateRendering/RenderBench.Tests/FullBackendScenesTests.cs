using System.Text;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using RenderBench.Analysis;
using RenderBench.Analyzer;
using static RenderBench.Tests.Check;
using MiniRun = RenderBench.Tests.ExtendedScenesTests.MiniRun;

namespace RenderBench.Tests;

// Full-backend scenes S11-S23 (S17/S18 out of scope): catalog, options, startup environment, fallback/approximation
// policy, the new metrics and gates, report rows, image/font helpers and the new ETW events.
static class FullBackendScenesTests
{
    public static IEnumerable<TestCase> All() =>
    [
        new("Full.Options.CatalogPlanIdsAndOrder", CatalogPlanIdsAndOrder),
        new("Full.Options.OutOfScopeScenesRejected", OutOfScopeScenesRejected),
        new("Full.Options.VariantsAndLabels", VariantsAndLabels),
        new("Full.Options.RejectsVariantsOnOtherScenes", RejectsVariantsOnOtherScenes),
        new("Full.Options.DeviceLostSpec", DeviceLostSpec),
        new("Full.Options.StartupEnvironment", StartupEnvironment),
        new("Full.Gate.WindowFallbackExpectations", WindowFallbackExpectations),
        new("Full.Gate.FallbackMakesUnsupportedExpected", FallbackMakesUnsupportedExpected),
        new("Full.Gate.ApproximatedContentAllowed", ApproximatedContentAllowed),
        new("Full.Metrics.UiBlockFrames", UiBlockFrames),
        new("Full.Metrics.ChangeToFrameAndCycleLeak", ChangeToFrameAndCycleLeak),
        new("Full.Metrics.DeviceLostRecoveryAndLeak", DeviceLostRecoveryAndLeak),
        new("Full.Gate.DeviceLostCompositionNotApplicable", DeviceLostCompositionNotApplicable),
        new("Full.Metrics.PopupOpenByKindAndMemoryPerPopup", PopupOpenByKindAndMemoryPerPopup),
        new("Full.Gate.CaptureAndRenderTargetBitmap", CaptureAndRenderTargetBitmap),
        new("Full.Metrics.JsonRoundTrip", JsonRoundTrip),
        new("Full.Report.NewRows", ReportRows),
        new("Full.Imaging.BmpReadAndMeanAbsDiff", BmpReadAndMeanAbsDiff),
        new("Full.Fonts.CffProbe", CffProbe),
        new("Etl.FullBackendEvents", EtlFullBackendEvents),
    ];

    const long MB = 1024 * 1024;

    static BenchOptions Parse(params string[] args)
    {
        Expect(BenchOptions.TryParse(args, out var options, out var error), error ?? "parse");
        return options;
    }

    // ---- Options ----

    static void CatalogPlanIdsAndOrder()
    {
        (string Cli, SceneId Scene, string PlanId, string Display)[] cases =
        [
            ("interop", SceneId.CompositionInterop, "S11", "CompositionInterop"),
            ("materials", SceneId.Materials, "S13", "Materials"),
            ("popups", SceneId.WindowedPopups, "S14", "WindowedPopups"),
            ("independent", SceneId.IndependentAnimation, "S15", "IndependentAnimation"),
            ("lottie", SceneId.Lottie, "S12", "Lottie"),
            ("text", SceneId.TextStress, "S19", "TextStress"),
            ("swapchain", SceneId.SwapChainMedia, "S16", "SwapChainMedia"),
            ("dpitheme", SceneId.DpiTheme, "S20", "DpiTheme"),
            ("devicelost", SceneId.DeviceLost, "S21", "DeviceLost"),
            ("accessibility", SceneId.Accessibility, "S22", "Accessibility"),
            ("appshell", SceneId.AppShell, "S23", "AppShell"),
        ];
        foreach (var (cli, scene, planId, display) in cases)
        {
            Equal(scene, Parse("--scene", cli).Scene, cli);
            Equal(scene, Parse("--scene", planId.ToLowerInvariant()).Scene, $"plan id {planId}");
            var info = SceneCatalog.Get(scene);
            Equal(planId, info.PlanId, $"{cli} plan id");
            Equal(cli, info.CliName, $"{cli} cli name");
            Equal(display, info.DisplayName, $"{cli} display name");
            Expect(!info.DisplayName.Contains('-'), "labels split on '-'");
            Equal(scene, SceneCatalog.FromLabel(display), $"{display} label");
        }
        // Matrix order: S1-S10, then the full-backend scenes in their execution order.
        Equal(SceneId.Form, SceneCatalog.All[9].Id, "S10 before the new scenes");
        Equal(string.Join(",", cases.Select(c => c.PlanId)), string.Join(",", SceneCatalog.All.Skip(10).Select(s => s.PlanId)),
            "execution order");
        Expect(BenchOptions.Usage.Contains("s1..s23"), "usage names the plan id range");
    }

    static void OutOfScopeScenesRejected()
    {
        foreach (string id in new[] { "s17", "s18", "webview", "ink" })
        {
            Expect(!BenchOptions.TryParse(["--scene", id], out _, out var error), id);
            Expect(error!.Contains("appshell"), $"error lists the new scenes: {error}");
        }
    }

    static void VariantsAndLabels()
    {
        Equal("CompositionInterop", Parse("--scene", "interop").SceneLabel, "S11");
        Equal("CompositionInterop-NoInterop", Parse("--scene", "s11", "--no-interop").SceneLabel, "S11 variant");
        Equal("Lottie", Parse("--scene", "lottie").SceneLabel, "S12");
        Equal("Lottie-NoPlayer", Parse("--scene", "lottie", "--no-player").SceneLabel, "S12 variant");
        Equal("SwapChainMedia", Parse("--scene", "swapchain").SceneLabel, "S16");
        Equal("SwapChainMedia-NoMedia", Parse("--scene", "swapchain", "--no-media").SceneLabel, "S16 variant");
        Equal("DeviceLost", Parse("--scene", "devicelost", "--device-lost", "every:5").SceneLabel, "S21 label ignores the spec");
        Equal(SceneId.CompositionInterop, SceneCatalog.FromLabel("CompositionInterop-NoInterop"), "variant label");
        Equal(SceneId.Lottie, SceneCatalog.FromLabel("Lottie-NoPlayer"), "variant label");

        Equal(100, Parse("--scene", "dpitheme").Items, "S20 cycles");
        Equal(50, Parse("--scene", "accessibility").Items, "S22 focus targets");
        Equal(2000, Parse("--scene", "appshell").Items, "S23 rows");
        Equal(1000, Parse("--scene", "text").Items, "S19 small text blocks");
        Equal("every:10", Parse("--scene", "devicelost").DeviceLost, "S21 default spec");
        Equal(null, Parse("--scene", "settings").DeviceLost, "no spec elsewhere");
    }

    static void RejectsVariantsOnOtherScenes()
    {
        Expect(!BenchOptions.TryParse(["--scene", "lottie", "--no-interop"], out _, out var error), "--no-interop on lottie");
        Expect(error!.Contains("interop"), error);
        Expect(!BenchOptions.TryParse(["--scene", "interop", "--no-player"], out _, out _), "--no-player on interop");
        Expect(!BenchOptions.TryParse(["--scene", "settings", "--no-media"], out _, out _), "--no-media on settings");
        Expect(!BenchOptions.TryParse(["--scene", "dpitheme", "--device-lost", "every:3"], out _, out _), "--device-lost on dpitheme");
    }

    static void DeviceLostSpec()
    {
        Equal("every:7", Parse("--scene", "devicelost", "--device-lost", "EVERY:7").DeviceLost, "every normalized");
        Equal("5,40,200", Parse("--scene", "devicelost", "--device-lost", "200, 5,40,5").DeviceLost, "list sorted, unique");
        foreach (string bad in new[] { "every:0", "every:", "every:x", "0", "-3", "3,,4", "", "sometimes" })
        {
            Expect(!BenchOptions.TryParse(["--scene", "devicelost", "--device-lost", bad], out _, out _), $"rejects '{bad}'");
        }
    }

    static void StartupEnvironment()
    {
        string dir = Path.Combine("C:\\data", "RenderBench");
        Dictionary<string, string?> env = [];
        IReadOnlyDictionary<string, string> Env(string? backend, params string[] args)
            => BenchOptions.StartupEnvironment(args, backend, 99, dir, name => env.GetValueOrDefault(name));

        var lost = Env("d2d-sw", "--scene", "devicelost", "--device-lost", "every:4");
        Equal("every:4", lost.GetValueOrDefault(ImmediateHooks.SimulateDeviceLostVariable), "device-lost hook for immediate modes");
        Equal(0, Env("", "--scene", "devicelost").Count, "composition has no device-lost hook");
        Equal(0, Env(null, "--scene", "devicelost").Count, "unset backend is composition");
        Equal(0, Env("composition", "--scene", "devicelost").Count, "explicit composition");
        Equal(0, Env("gdiplus", "--scene", "settings").Count, "other scenes untouched");
        Equal(0, Env("gdiplus", "--scene", "devicelost", "--bogus").Count, "unparsable command line sets nothing");

        var capture = Env("gdiplus", "--scene", "s22");
        Equal(Path.Combine(dir, "capture-99.bmp"), capture.GetValueOrDefault(ImmediateHooks.CaptureVariable), "capture path per process");
        Equal(0, Env("", "--scene", "s22").Count, "composition captures the screen instead");

        env[ImmediateHooks.SimulateDeviceLostVariable] = "1,2";
        env[ImmediateHooks.CaptureVariable] = "C:\\x.bmp";
        Equal(0, Env("d2d-hw", "--scene", "devicelost").Count, "an explicit environment wins");
        Equal(0, Env("d2d-hw", "--scene", "accessibility").Count, "an explicit capture path wins");
    }

    // ---- Fallback and unsupported content ----

    static RunMetrics Run(string scene, string requested, BackendMode mode, Action<MiniRun>? setup = null)
    {
        var run = new MiniRun { Scene = scene, Requested = requested, Mode = mode };
        run.DrawnFrame(MiniRun.InteractionBegin + 100);
        setup?.Invoke(run);
        return run.Compute();
    }

    static GateCheck GateFor(RunMetrics metrics, string name)
        => metrics.Gate.FirstOrDefault(g => g.Name == name) ?? throw new TestFailure($"gate check {name} missing");

    static void Fallback(MiniRun run, string reason = "SwapChainPanel")
        => run.Extra.Add(new WindowFallbackRecord(MiniRun.InteractionBegin + 50, MiniRun.Pid, BackendMode.ImmediateGdiPlus, 0x77, reason));

    static void WindowFallbackExpectations()
    {
        foreach (string label in new[] { "CompositionInterop", "Lottie", "SwapChainMedia", "SwapChainMedia-NoMedia" })
        {
            Expect(WindowFallbackPolicy.IsExpected(label), $"{label} expects a fallback");
            var observed = Run(label, "gdiplus", BackendMode.ImmediateGdiPlus, r => Fallback(r));
            Expect(GateFor(observed, GateNames.WindowFallbackAsExpected).Passed, $"{label}: fallback observed passes");
            Expect(observed.ExpectsWindowFallback, $"{label}: flag");
            Equal(1, observed.WindowFallbackCount, $"{label}: count");
            Equal("SwapChainPanel", observed.WindowFallbackReasons!.Single(), $"{label}: reason");
            var missing = Run(label, "gdiplus", BackendMode.ImmediateGdiPlus);
            Expect(!GateFor(missing, GateNames.WindowFallbackAsExpected).Passed, $"{label}: no fallback fails");
            Expect(GateFor(Run(label, "", BackendMode.Composition), GateNames.WindowFallbackAsExpected).Passed,
                $"{label}: composition needs no fallback");
        }
        foreach (string label in new[] { "CompositionInterop-NoInterop", "Lottie-NoPlayer", "Settings", "Materials" })
        {
            Expect(!WindowFallbackPolicy.IsExpected(label), $"{label} must stay immediate");
            Expect(GateFor(Run(label, "d2d-hw", BackendMode.ImmediateD2DHardware), GateNames.WindowFallbackAsExpected).Passed,
                $"{label}: no fallback passes");
            var fell = Run(label, "d2d-hw", BackendMode.ImmediateD2DHardware, r => Fallback(r, "CompositionInterop"));
            var gate = GateFor(fell, GateNames.WindowFallbackAsExpected);
            Expect(!gate.Passed, $"{label}: unexpected fallback fails");
            Expect(gate.Detail.Contains("CompositionInterop"), $"detail names the reason: {gate.Detail}");
        }
    }

    static void FallbackMakesUnsupportedExpected()
    {
        void Unsupported(MiniRun r) => r.Extra.Add(new UnsupportedContentRecord(MiniRun.InteractionBegin + 40, MiniRun.Pid,
            BackendMode.ImmediateGdiPlus, "SwapChainPanel", "Element content"));
        var withFallback = Run("SwapChainMedia", "gdiplus", BackendMode.ImmediateGdiPlus, r => { Unsupported(r); Fallback(r); });
        Expect(GateFor(withFallback, GateNames.NoUnsupportedContent).Passed, "content that triggered the fallback is expected");
        var without = Run("SwapChainMedia", "gdiplus", BackendMode.ImmediateGdiPlus, Unsupported);
        Expect(!GateFor(without, GateNames.NoUnsupportedContent).Passed, "without a fallback it was simply not rendered");
        var noInterop = Run("CompositionInterop-NoInterop", "gdiplus", BackendMode.ImmediateGdiPlus, r => { Unsupported(r); Fallback(r); });
        Expect(!GateFor(noInterop, GateNames.NoUnsupportedContent).Passed, "scenes that must stay immediate get no allowance");
    }

    static void ApproximatedContentAllowed()
    {
        var mica = Run("Materials", "d2d-sw", BackendMode.ImmediateD2DSoftware, r => r.Extra.Add(new UnsupportedContentRecord(
            MiniRun.InteractionBegin + 40, MiniRun.Pid, BackendMode.ImmediateD2DSoftware, "MicaBackdrop", "SystemBackdrop:approximated")));
        Expect(GateFor(mica, GateNames.NoUnsupportedContent).Passed, "approximations are allowed in every scene");
        Expect(mica.ExpectedUnsupported!.Contains("MicaBackdrop|SystemBackdrop:approximated"), "and listed as expected");
        var gradient = Run("Materials", "d2d-sw", BackendMode.ImmediateD2DSoftware, r => r.Extra.Add(new UnsupportedContentRecord(
            MiniRun.InteractionBegin + 40, MiniRun.Pid, BackendMode.ImmediateD2DSoftware, "Rectangle", "LinearGradientBrush")));
        Expect(!GateFor(gradient, GateNames.NoUnsupportedContent).Passed, "S13 gradients must render");
    }

    // ---- New metrics ----

    static void UiBlockFrames()
    {
        var metrics = Run("IndependentAnimation", "", BackendMode.Composition, r =>
        {
            double t = MiniRun.InteractionBegin + 200;
            r.DrawnFrame(t - 10);
            r.Extra.Add(new UiBlockRecord(t, MiniRun.Pid, 1, true, 1000));
            r.DrawnFrame(t + 300);
            r.DrawnFrame(t + 400);
            r.Extra.Add(new UiBlockRecord(t + 1000, MiniRun.Pid, 1, false, 1000));
            r.DrawnFrame(t + 1050);
        });
        var block = metrics.UiBlocks!.Single();
        Equal(1, block.BlockId, "block id");
        Equal(2, block.FramesDrawn, "frames inside the block");
        Near(1000, block.DurationMs, 1e-9, "duration");
        Near(650, block.MaxFrameGapMs, 1e-9, "gap from the last frame inside to the first after");
        Equal(2, metrics.FramesDuringUiBlock, "summary frames");
        Near(650, metrics.MaxFrameGapDuringUiBlockMs, 1e-9, "summary gap");
        Expect(GateFor(metrics, GateNames.UiBlockObserved).Passed, "block observed");

        var stalled = Run("IndependentAnimation", "gdiplus", BackendMode.ImmediateGdiPlus, r =>
        {
            double t = MiniRun.InteractionBegin + 200;
            r.DrawnFrame(t - 10);
            r.Extra.Add(new UiBlockRecord(t, MiniRun.Pid, 1, true, 1000));
            r.Extra.Add(new UiBlockRecord(t + 1000, MiniRun.Pid, 1, false, 1000));
            r.DrawnFrame(t + 1030);
        });
        Equal(0, stalled.FramesDuringUiBlock, "no frames while the UI thread is blocked");
        Near(1040, stalled.MaxFrameGapDuringUiBlockMs, 1e-9, "gap spans the block");

        var noBlock = Run("IndependentAnimation", "", BackendMode.Composition);
        Expect(!GateFor(noBlock, GateNames.UiBlockObserved).Passed, "S15 requires the block");
        Equal(null, noBlock.UiBlocks, "no blocks");
        Expect(Run("Settings", "", BackendMode.Composition).Gate.All(g => g.Name != GateNames.UiBlockObserved), "S15 only");
    }

    static void ChangeToFrameAndCycleLeak()
    {
        RunMetrics Build(double cycle100MB, double lastChangeLatency)
            => Run("DpiTheme", "", BackendMode.Composition, r =>
            {
                double t = MiniRun.InteractionBegin + 200;
                for (int i = 0; i < 4; i++, t += 100)
                {
                    r.Extra.Add(new InputInjectedRecord(t, MiniRun.Pid, i + 1, "programmatic:resize", "Window"));
                    r.DrawnFrame(t + 20 + i);
                }
                r.Extra.Add(new InputInjectedRecord(t, MiniRun.Pid, 10, "programmatic:theme", "Root"));
                r.DrawnFrame(t + lastChangeLatency);
                // Memory is sampled right before each step; the last sample at or before the step is the step's value.
                r.Extra.Add(new MemorySampleRecord(MiniRun.InteractionBegin + 1500, MiniRun.Pid, BenchPhase.Interaction, 120 * MB, 0, 0, 0, 0, 0, 0, 0, 0));
                r.Extra.Add(new SceneStepRecord(MiniRun.InteractionBegin + 1500.1, MiniRun.Pid, 0, "cycle-10", 0));
                r.Extra.Add(new MemorySampleRecord(MiniRun.InteractionBegin + 1600, MiniRun.Pid, BenchPhase.Interaction, 500 * MB, 0, 0, 0, 0, 0, 0, 0, 0));
                r.Extra.Add(new MemorySampleRecord(MiniRun.InteractionBegin + 1900, MiniRun.Pid, BenchPhase.Interaction, (long)(cycle100MB * MB), 0, 0, 0, 0, 0, 0, 0, 0));
                r.Extra.Add(new SceneStepRecord(MiniRun.InteractionBegin + 1900.1, MiniRun.Pid, 1, "cycle-100", 0));
                r.Extra.Add(new MemorySampleRecord(MiniRun.InteractionBegin + 1950, MiniRun.Pid, BenchPhase.Interaction, 900 * MB, 0, 0, 0, 0, 0, 0, 0, 0));
            });

        var ok = Build(121.5, 30);
        Equal(4, ok.ChangeToFrameMsByKind!["resize"].Count, "resize changes");
        Near(23, ok.ChangeToFrameMsByKind["resize"].Max, 1e-9, "resize max");
        Near(30, ok.ChangeToFrameMsByKind["theme"].P50, 1e-9, "theme");
        Equal(0, ok.UnmatchedChanges, "all changes reached a frame");
        Near(1.5, ok.CycleLeakPrivateMB, 1e-9, "cycle-100 minus cycle-10");
        Expect(GateFor(ok, GateNames.RecoveryWithinLimit).Passed, "within 1 s");
        Expect(GateFor(ok, GateNames.NoCycleLeak).Passed, "< 2 MB");

        var slow = Build(125, 1500);
        Near(1500, slow.ChangeToFrameMsByKind!["theme"].P50, 1e-9, "slow changes are still matched within 5 s");
        Expect(!GateFor(slow, GateNames.RecoveryWithinLimit).Passed, "1.5 s change-to-frame fails");
        Expect(!GateFor(slow, GateNames.NoCycleLeak).Passed, "5 MB leak fails");

        var lost = Build(121, 6000);
        Equal(1, lost.UnmatchedChanges, "a change without a frame in 5 s is unmatched");
        Expect(!GateFor(lost, GateNames.RecoveryWithinLimit).Passed, "unmatched changes fail");

        var noSteps = Run("DpiTheme", "", BackendMode.Composition);
        Equal(null, noSteps.CycleLeakPrivateMB, "no steps, no leak value");
        Expect(!GateFor(noSteps, GateNames.NoCycleLeak).Passed, "S20 must produce the cycle steps");

        // A short (-Quick, --items < 100) run reaches cycle-10 but not cycle-100: the leak gate is not applicable.
        var shortRun = Run("DpiTheme", "", BackendMode.Composition, r =>
            r.Extra.Add(new SceneStepRecord(MiniRun.InteractionBegin + 100, MiniRun.Pid, 0, "cycle-10", 0)));
        var shortGate = GateFor(shortRun, GateNames.NoCycleLeak);
        Expect(shortGate.Passed && shortGate.Detail.StartsWith("n/a"), $"short run is n/a: {shortGate.Detail}");
        Expect(Run("Inbox", "", BackendMode.Composition).Gate.All(g => g.Name != GateNames.NoCycleLeak), "S20/S21 only");
    }

    static void DeviceLostRecoveryAndLeak()
    {
        RunMetrics Build(int losses, double recoveryMs, double growthMB)
            => Run("DeviceLost", "d2d-hw", BackendMode.ImmediateD2DHardware, r =>
            {
                double t = MiniRun.InteractionBegin + 10;
                for (int i = 1; i <= losses; i++, t += 15)
                {
                    r.Extra.Add(new DeviceLostRecord(t, MiniRun.Pid, BackendMode.ImmediateD2DHardware, unchecked((int)0x887A0005), 0, (uint)i));
                    r.DrawnFrame(t + recoveryMs);
                    if (i == 10 || i == 100)
                    {
                        long bytes = (long)((200 + (i == 100 ? growthMB : 0)) * MB);
                        r.Extra.Add(new MemorySampleRecord(t + recoveryMs + 1, MiniRun.Pid, BenchPhase.Interaction, bytes, 0, 0, 0, 0, 0, 0, 0, 0));
                    }
                }
            });

        var ok = Build(100, 12, 0.5);
        Equal(100, ok.DeviceLostCount, "losses");
        Near(12, ok.DeviceLostRecoveryMs!.P95, 1e-9, "loss to next drawn frame");
        Near(0.5, ok.DeviceLostLeakPrivateMB, 1e-9, "after 100th minus after 10th");
        Expect(GateFor(ok, GateNames.DeviceLostObserved).Passed, "observed");
        Expect(GateFor(ok, GateNames.RecoveryWithinLimit).Passed, "recovered");
        Expect(GateFor(ok, GateNames.NoCycleLeak).Passed, "no leak");

        var leaky = Build(100, 12, 3);
        Expect(!GateFor(leaky, GateNames.NoCycleLeak).Passed, "3 MB over 90 losses fails");

        var few = Build(20, 12, 0);
        Equal(null, few.DeviceLostLeakPrivateMB, "fewer than 100 losses: no leak value");
        Expect(GateFor(few, GateNames.NoCycleLeak).Passed, "and the leak check is not applicable");
        Expect(GateFor(few, GateNames.NoCycleLeak).Detail.Contains("n/a"), "reported as n/a");

        var none = Build(0, 12, 0);
        Expect(!GateFor(none, GateNames.DeviceLostObserved).Passed, "immediate modes must honor the hook");
        Equal(null, none.DeviceLostRecoveryMs, "no recovery without losses");
    }

    static void DeviceLostCompositionNotApplicable()
    {
        var comp = Run("DeviceLost", "", BackendMode.Composition);
        foreach (string gate in new[] { GateNames.DeviceLostObserved, GateNames.RecoveryWithinLimit, GateNames.NoCycleLeak })
        {
            Expect(GateFor(comp, gate).Passed, $"{gate} is n/a for composition");
        }
        Expect(comp.GatePassed, string.Join("; ", comp.Gate.Where(g => !g.Passed).Select(g => g.Name)));
    }

    static void PopupOpenByKindAndMemoryPerPopup()
    {
        var metrics = Run("WindowedPopups", "", BackendMode.Composition, r =>
        {
            double t = MiniRun.InteractionBegin + 100;
            long id = 1;
            foreach (var (kind, open) in new[] { ("Flyout", 20.0), ("Flyout", 30.0), ("ComboBox", 50.0) })
            {
                r.Extra.Add(new InputInjectedRecord(t, MiniRun.Pid, id, "programmatic:popup", kind));
                r.Extra.Add(new PopupOpenedRecord(t + open - 5, MiniRun.Pid, id, kind));
                r.DrawnFrame(t + open);
                t += 100;
                id++;
            }
            for (int i = 0; i <= 3; i++)
            {
                double s = MiniRun.InteractionBegin + 600 + i * 200;
                r.Extra.Add(new SceneStepRecord(s, MiniRun.Pid, i, i == 0 ? "baseline" : $"popups-{i}", i));
                r.Extra.Add(new MemorySampleRecord(s + 150, MiniRun.Pid, BenchPhase.Interaction, (long)((100 + 0.25 * i) * MB), 0, 0, 0, 0, 0, 0, 0, 0));
            }
        });
        Near(25, metrics.PopupOpenMsByKind!["Flyout"].P50, 1e-9, "flyout");
        Near(50, metrics.PopupOpenMsByKind["ComboBox"].P50, 1e-9, "combobox");
        Equal(3, metrics.PopupOpenMs!.Count, "overall popup open");
        Near(0.25, metrics.PrivateMBPerOpenPopup, 1e-9, "slope over popups-N");
        Equal(null, metrics.MarginalPrivateMBPerWindow, "popup steps are not windows");
    }

    static void CaptureAndRenderTargetBitmap()
    {
        RunMetrics Build(string requested, BackendMode mode, double? diff, params double[] rtb)
            => Run("Accessibility", requested, mode, r =>
            {
                double t = MiniRun.InteractionBegin + 100;
                foreach (double v in rtb)
                {
                    r.Extra.Add(new SceneValueRecord(t += 10, MiniRun.Pid, SceneValueNames.RtbSuccess, v));
                }
                if (diff is { } d)
                {
                    r.Extra.Add(new SceneValueRecord(t + 10, MiniRun.Pid, SceneValueNames.CaptureMeanAbsDiff, d));
                    r.Extra.Add(new SceneValueRecord(t + 20, MiniRun.Pid, SceneValueNames.CaptureMeanAbsDiff, d + 2));
                }
            });

        var ok = Build("gdiplus", BackendMode.ImmediateGdiPlus, 3, 1, 1);
        Near(1, ok.RtbSuccessRate, 1e-9, "rtb rate");
        Near(4, ok.CaptureMeanAbsDiff, 1e-9, "mean of the comparisons");
        Expect(GateFor(ok, GateNames.RenderTargetBitmapOk).Passed, "rtb ok");
        Expect(GateFor(ok, GateNames.CaptureWithinTolerance).Passed, "capture ok");

        var bad = Build("gdiplus", BackendMode.ImmediateGdiPlus, 20, 1, 0);
        Near(0.5, bad.RtbSuccessRate, 1e-9, "half failed");
        Expect(!GateFor(bad, GateNames.RenderTargetBitmapOk).Passed, "rtb failure fails");
        Expect(!GateFor(bad, GateNames.CaptureWithinTolerance).Passed, "diff 21 > 8 fails");

        var noCapture = Build("", BackendMode.Composition, null, 1);
        Expect(GateFor(noCapture, GateNames.CaptureWithinTolerance).Passed, "no capture available is skipped");
        Expect(GateFor(noCapture, GateNames.CaptureWithinTolerance).Detail.Contains("skipped"), "and says so");
        Expect(!GateFor(Build("", BackendMode.Composition, null), GateNames.RenderTargetBitmapOk).Passed, "rtb must be attempted");
    }

    static void JsonRoundTrip()
    {
        var metrics = Run("SwapChainMedia", "gdiplus", BackendMode.ImmediateGdiPlus, r =>
        {
            Fallback(r);
            r.Extra.Add(new UiBlockRecord(MiniRun.InteractionBegin + 200, MiniRun.Pid, 3, true, 500));
            r.Extra.Add(new UiBlockRecord(MiniRun.InteractionBegin + 700, MiniRun.Pid, 3, false, 500));
            r.Extra.Add(new InputInjectedRecord(MiniRun.InteractionBegin + 800, MiniRun.Pid, 1, "programmatic:resize", "Window"));
            r.DrawnFrame(MiniRun.InteractionBegin + 830);
        });
        var back = MetricsJson.Deserialize(MetricsJson.Serialize(metrics));
        Equal(1, back.WindowFallbackCount, "fallbacks");
        Expect(back.ExpectsWindowFallback, "expected flag");
        Equal(3, back.UiBlocks!.Single().BlockId, "ui block");
        Equal(1, back.ChangeToFrameMsByKind!["resize"].Count, "change to frame");

        string old = MetricsJson.Serialize(MetricsCalculator.Compute(new SyntheticRun().Build()));
        var legacy = MetricsJson.Deserialize(old.Replace("\"WindowFallbackCount\"", "\"Ignored1\"").Replace("\"UiBlocks\"", "\"Ignored2\""));
        Equal(0, legacy.WindowFallbackCount, "legacy default");
        Equal(null, legacy.UiBlocks, "legacy blocks");
    }

    static void ReportRows()
    {
        // Settings: a scene without per-scene gates, so the run passes and its values reach the medians.
        var metrics = Run("Settings", "", BackendMode.Composition, r =>
        {
            r.Extra.Add(new InputInjectedRecord(MiniRun.InteractionBegin + 300, MiniRun.Pid, 1, "programmatic:theme", "Root"));
            r.DrawnFrame(MiniRun.InteractionBegin + 320);
            r.Extra.Add(new InputInjectedRecord(MiniRun.InteractionBegin + 500, MiniRun.Pid, 2, "programmatic:popup", "ComboBox"));
            r.Extra.Add(new PopupOpenedRecord(MiniRun.InteractionBegin + 510, MiniRun.Pid, 2, "ComboBox"));
            r.DrawnFrame(MiniRun.InteractionBegin + 540);
        });
        Expect(metrics.GatePassed, string.Join("; ", metrics.Gate.Where(g => !g.Passed).Select(g => $"{g.Name} {g.Detail}")));
        string report = ComparisonReport.Build([("a", metrics)]);
        foreach (var row in new[]
        {
            "| Window fallbacks |", "| Frames during UI-thread block |", "| Max frame gap during UI-thread block (ms) |",
            "| Changes without a frame |", "| Private growth cycle 10 to 100 (MB) |", "| Device lost recovery p95 (ms) |",
            "| Private growth device lost 10 to 100 (MB) |", "| Private per open popup (MB) |",
            "| RenderTargetBitmap success (%) |", "| Capture mean abs diff (0-255) |",
        })
        {
            Expect(report.Contains(row), $"report row {row}");
        }
        // Per-kind rows exist only for kinds some run measured.
        Expect(report.Contains("| Change to frame p95, theme (ms) | 20.00 |"), "theme value");
        Expect(report.Contains("| Popup open p95, ComboBox (ms) | 40.0 |"), "popup kind value");
        Expect(!report.Contains("Change to frame p95, dpi"), "no dpi row without dpi changes");
    }

    // ---- Image and font helpers ----

    static void BmpReadAndMeanAbsDiff()
    {
        // 3x2 top-down image in the WINUI_IMMEDIATE_CAPTURE / Win32.WriteBmp layout.
        byte[] pixels = new byte[3 * 2 * 4];
        for (int i = 0; i < 6; i++)
        {
            pixels[i * 4] = (byte)(10 * i);
            pixels[i * 4 + 1] = 100;
            pixels[i * 4 + 2] = 200;
            pixels[i * 4 + 3] = 255;
        }
        byte[] topDown = BmpCodec.Encode(3, 2, pixels);
        var image = NotNull(BmpCodec.TryDecode(topDown), "decode top-down");
        Equal(3, image.Width, "width");
        Equal(2, image.Height, "height");
        Equal(40, (int)image.Pixels[4 * 4], "pixel (1,1) blue");

        // Bottom-up variant (positive height) decodes to the same top-down pixels.
        byte[] bottomUp = (byte[])topDown.Clone();
        BitConverter.GetBytes(2).CopyTo(bottomUp, 22);
        Array.Copy(pixels, 12, bottomUp, 54, 12);
        Array.Copy(pixels, 0, bottomUp, 66, 12);
        Equal(Convert.ToHexString(image.Pixels), Convert.ToHexString(NotNull(BmpCodec.TryDecode(bottomUp), "bottom-up").Pixels), "bottom-up");
        Equal(null, BmpCodec.TryDecode(Encoding.ASCII.GetBytes("not a bitmap at all, sorry")), "garbage");
        Equal(null, BmpCodec.TryDecode(topDown.AsSpan(0, 40).ToArray()), "truncated");

        var crop = image.Crop(1, 1, 2, 1);
        Equal(2, crop.Width, "crop width");
        Equal(40, (int)crop.Pixels[0], "crop origin");
        Equal(50, (int)crop.Pixels[4], "crop second pixel");

        var shifted = new BgraImage(3, 2, pixels.Select((b, i) => i % 4 == 0 ? (byte)(b + 6) : b).ToArray());
        Near(2, PixelDiff.MeanAbsDiff(image, shifted), 1e-9, "blue +6 averaged over B,G,R");
        Near(0, PixelDiff.MeanAbsDiff(image, image), 1e-9, "identical");
        Near(2, PixelDiff.MeanAbsDiff(image, shifted.Crop(0, 0, 2, 2)), 1e-9, "compares the overlap when sizes differ");
        var alpha = new BgraImage(3, 2, pixels.Select((b, i) => i % 4 == 3 ? (byte)0 : b).ToArray());
        Near(0, PixelDiff.MeanAbsDiff(image, alpha), 1e-9, "alpha is ignored");
    }

    // A minimal OpenType file: header + table directory with a 'name' table holding the family (nameID 1, Windows
    // Unicode BMP, UTF-16BE) and an optional typographic family (nameID 16).
    static byte[] FakeFont(string magic, string family, string? typographic = null)
    {
        var records = new List<(ushort NameId, byte[] Text)> { (1, Encoding.BigEndianUnicode.GetBytes(family)) };
        if (typographic is not null)
        {
            records.Add((16, Encoding.BigEndianUnicode.GetBytes(typographic)));
        }
        var name = new List<byte>();
        void U16(List<byte> b, int v) { b.Add((byte)(v >> 8)); b.Add((byte)v); }
        U16(name, 0);
        U16(name, records.Count);
        U16(name, 6 + 12 * records.Count);
        int offset = 0;
        foreach (var (id, text) in records)
        {
            U16(name, 3); U16(name, 1); U16(name, 0x409); U16(name, id); U16(name, text.Length); U16(name, offset);
            offset += text.Length;
        }
        foreach (var (_, text) in records)
        {
            name.AddRange(text);
        }
        var file = new List<byte>(Encoding.ASCII.GetBytes(magic));
        U16(file, 1); U16(file, 16); U16(file, 0); U16(file, 0);
        file.AddRange(Encoding.ASCII.GetBytes("name"));
        file.AddRange(new byte[4]); // checksum
        int tableOffset = 12 + 16;
        file.AddRange(BitConverter.GetBytes(tableOffset).Reverse());
        file.AddRange(BitConverter.GetBytes(name.Count).Reverse());
        file.AddRange(name);
        return [.. file];
    }

    static void CffProbe()
    {
        var cff = FakeFont("OTTO", "Probe Sans");
        Expect(FontProbe.IsCff(cff), "OTTO is CFF");
        Expect(!FontProbe.IsCff(FakeFont("\0\u0001\0\0", "Probe Serif")), "TrueType outlines are not");
        Equal("Probe Sans", FontProbe.TryReadFamilyName(cff), "family");
        Equal("Probe Sans Display", FontProbe.TryReadFamilyName(FakeFont("OTTO", "Probe Sans Display Bold", "Probe Sans Display")),
            "typographic family wins");
        Equal(null, FontProbe.TryReadFamilyName([1, 2, 3]), "garbage");

        string dir = Path.Combine(AppContext.BaseDirectory, $"fontprobe-{Environment.ProcessId}");
        Directory.CreateDirectory(dir);
        try
        {
            File.WriteAllBytes(Path.Combine(dir, "a.ttf"), FakeFont("\0\u0001\0\0", "Not Cff"));
            File.WriteAllBytes(Path.Combine(dir, "b.otf"), FakeFont("true", "Also Not"));
            File.WriteAllBytes(Path.Combine(dir, "c.otf"), cff);
            var found = FontProbe.FindCffFont(Directory.GetFiles(dir).Order());
            Equal("Probe Sans", found?.Family, "first CFF font");
            Equal(null, FontProbe.FindCffFont([Path.Combine(dir, "a.ttf"), Path.Combine(dir, "missing.otf")]), "none");
        }
        finally
        {
            Directory.Delete(dir, recursive: true);
        }
    }

    // ---- ETW ----

    // Requires elevation: UiBlock harness events and the native WindowFallback event through a real ETW session.
    static void EtlFullBackendEvents()
    {
        ProfileTests.RequireAdmin();
        string etl = Path.Combine(Path.GetTempPath(), $"renderbench-full-{Environment.ProcessId}.etl");
        try
        {
            using (var session = new TraceEventSession($"RenderBenchFullTest-{Environment.ProcessId}", etl))
            {
                session.EnableProvider(BenchEventSource.ProviderId, TraceEventLevel.Verbose);
                session.EnableProvider(BackendModes.RenderingBackendProviderId, TraceEventLevel.Verbose);
                Thread.Sleep(500);
                BenchEventSource.Log.UiBlockBegin(4, 1000);
                BenchEventSource.Log.UiBlockEnd(4, 1001.5);
                FakeRenderingBackendSource.Log.WindowFallback(2, 0xBEEF, "SwapChainPanel");
                Thread.Sleep(500);
            }
            var records = EtlReader.Read(etl).Where(r => r.ProcessId == Environment.ProcessId).ToList();
            var blocks = records.OfType<UiBlockRecord>().ToList();
            Equal(2, blocks.Count, "begin and end");
            Expect(blocks[0].IsBegin && !blocks[1].IsBegin, "order");
            Equal(4, blocks[1].BlockId, "id");
            Near(1001.5, blocks[1].DurationMs, 1e-9, "measured duration");
            var fallback = NotNull(records.OfType<WindowFallbackRecord>().FirstOrDefault(), "WindowFallback");
            Equal(BackendMode.ImmediateD2DHardware, fallback.Mode, "mode");
            Equal(0xBEEFul, fallback.RootId, "root");
            Equal("SwapChainPanel", fallback.Reason, "reason");
        }
        finally
        {
            File.Delete(etl);
        }
    }
}
