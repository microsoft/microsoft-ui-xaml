using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using RenderBench.Analysis;
using RenderBench.Analyzer;
using static RenderBench.Tests.Check;

namespace RenderBench.Tests;

// Extended scenes S3-S10: options/variants, the metrics they add (frame intervals and dropped frames, animation tax,
// per-window memory steps, typing and popup latency, unsupported content by type and per-scene expectations).
static class ExtendedScenesTests
{
    public static IEnumerable<TestCase> All() =>
    [
        new("Extended.Options.SceneNamesAndPlanIds", SceneNamesAndPlanIds),
        new("Extended.Options.RejectsNumericAndUnknownScenes", RejectsNumericAndUnknownScenes),
        new("Extended.Options.Variants", Variants),
        new("Extended.Options.RejectsBadVariants", RejectsBadVariants),
        new("Extended.Options.PerSceneDefaultsAndOverrides", PerSceneDefaultsAndOverrides),
        new("Extended.Options.SceneLabels", SceneLabels),
        new("Extended.Options.CompositionEngineAndOptionalChanges", CompositionEngineAndOptionalChanges),
        new("Extended.Metrics.FrameIntervalsAndDroppedFrames", FrameIntervalsAndDroppedFrames),
        new("Extended.Metrics.DiscreteInteractionIgnoresIdleGaps", DiscreteInteractionIgnoresIdleGaps),
        new("Extended.Metrics.DefaultRefreshWithoutDisplayInfo", DefaultRefreshWithoutDisplayInfo),
        new("Extended.Metrics.AnimationTaxAndSteadyExemption", AnimationTaxAndSteadyExemption),
        new("Extended.Metrics.WindowStepsMarginalAndReclaimed", WindowStepsMarginalAndReclaimed),
        new("Extended.Metrics.TypingLatencyExcludesProgrammatic", TypingLatencyExcludesProgrammatic),
        new("Extended.Metrics.PopupOpenTime", PopupOpenTime),
        new("Extended.Metrics.SceneValues", SceneValues),
        new("Extended.Metrics.UnsupportedCountsByType", UnsupportedCountsByType),
        new("Extended.Gate.ExpectedUnsupportedPerScene", ExpectedUnsupportedPerScene),
        new("Extended.Metrics.InputModeRecorded", InputModeRecorded),
        new("Extended.Metrics.JsonRoundTrip", JsonRoundTrip),
        new("Extended.Report.NewRowsAndStepTable", ReportRowsAndSteps),
        new("Etl.ExtendedHarnessEvents", EtlExtendedHarnessEvents),
    ];

    // ---- Options ----

    static BenchOptions Parse(params string[] args)
    {
        Expect(BenchOptions.TryParse(args, out var options, out var error), error ?? "parse");
        return options;
    }

    static void SceneNamesAndPlanIds()
    {
        (string Text, SceneId Scene, string PlanId)[] cases =
        [
            ("settings", SceneId.Settings, "S1"), ("dashboard", SceneId.Dashboard, "S2"),
            ("inbox", SceneId.Inbox, "S3"), ("reader", SceneId.Reader, "S4"), ("gallery", SceneId.Gallery, "S5"),
            ("animated", SceneId.Animated, "S6"), ("layered", SceneId.Layered, "S7"), ("datagrid", SceneId.DataGrid, "S8"),
            ("multiwindow", SceneId.MultiWindow, "S9"), ("form", SceneId.Form, "S10"),
        ];
        foreach (var (text, scene, planId) in cases)
        {
            Equal(scene, Parse("--scene", text).Scene, text);
            Equal(scene, Parse("--scene", text.ToUpperInvariant()).Scene, $"{text} upper case");
            Equal(scene, Parse("--scene", planId.ToLowerInvariant()).Scene, $"plan id {planId}");
            Equal(planId, SceneCatalog.Get(scene).PlanId, $"{text} plan id");
            Equal(text, SceneCatalog.Get(scene).CliName, $"{text} cli name");
        }
        Equal(10, SceneCatalog.All.Count, "catalog size");
        Equal(SceneId.Settings, SceneCatalog.All[0].Id, "S1 first");
        Equal(SceneId.Dashboard, SceneCatalog.All[1].Id, "S2 second");
    }

    static void RejectsNumericAndUnknownScenes()
    {
        Expect(!BenchOptions.TryParse(["--scene", "3"], out _, out _), "numeric scene");
        Expect(!BenchOptions.TryParse(["--scene", "s11"], out _, out _), "unknown plan id");
        Expect(!BenchOptions.TryParse(["--scene", "grid"], out _, out var error), "unknown name");
        Expect(error!.Contains("inbox"), $"error lists valid scenes: {error}");
    }

    static void Variants()
    {
        var o = Parse("--scene", "gallery", "--decode-to-size", "--input-mode", "sendinput", "--scroll-speed", "640",
            "--items", "120", "--screenshot-method", "printwindow");
        Expect(o.DecodeToSize, "decode-to-size");
        Equal(InputMode.SendInput, o.InputMode, "input mode");
        Near(640, o.ScrollSpeed, 1e-9, "scroll speed");
        Equal(120, o.Items, "items");
        Equal(ScreenshotMethod.PrintWindow, o.ScreenshotMethod, "screenshot method");

        var a = Parse("--scene", "animated", "--no-progressring");
        Expect(a.NoProgressRing, "no-progressring");

        var m = Parse("--scene", "multiwindow", "--windows", "5", "--step-seconds", "0.5");
        Equal(5, m.Windows, "windows");
        Near(0.5, m.StepSeconds, 1e-9, "step seconds");
        Expect(Parse("--scene", "multiwindow", "--tabs").Tabs, "tabs");

        var d = Parse();
        Equal(InputMode.Auto, d.InputMode, "input mode defaults to auto");
        Equal(10, d.Windows, "windows default");
        Equal(ScreenshotMethod.Auto, d.ScreenshotMethod, "screenshot default");
        Equal(InputMode.Programmatic, Parse("--input-mode", "programmatic").InputMode, "programmatic");
        Equal(ScreenshotMethod.Xaml, Parse("--screenshot-method", "xaml").ScreenshotMethod, "xaml screenshot");
    }

    static void CompositionEngineAndOptionalChanges()
    {
        var d = Parse();
        Equal(CompositionEngineChoice.Lifted, d.CompositionEngine, "engine defaults to lifted");
        Equal(OptionalChangesChoice.None, d.OptionalChanges, "optional changes default to none");

        var s = Parse("--scene", "inbox", "--composition-engine", "system", "--optional-changes", "all");
        Equal(CompositionEngineChoice.System, s.CompositionEngine, "system engine");
        Equal(OptionalChangesChoice.All, s.OptionalChanges, "all optional changes");
        Equal(CompositionEngineChoice.Lifted, Parse("--composition-engine", "LIFTED").CompositionEngine, "case-insensitive");

        Expect(!BenchOptions.TryParse(["--composition-engine", "dwm"], out _, out _), "unknown engine");
        Expect(!BenchOptions.TryParse(["--optional-changes", "some"], out _, out _), "unknown optional-changes value");

        // Program.Main reads these before Application.Start, ignoring everything else on the command line.
        var (engine, changes) = BenchOptions.ParseStartup(["--scene", "s3", "--composition-engine", "system", "--optional-changes", "all"]);
        Equal(CompositionEngineChoice.System, engine, "startup engine");
        Equal(OptionalChangesChoice.All, changes, "startup optional changes");
        Equal((CompositionEngineChoice.Lifted, OptionalChangesChoice.None), BenchOptions.ParseStartup(["--composition-engine"]), "missing value");
    }

    static void RejectsBadVariants()
    {
        Expect(!BenchOptions.TryParse(["--input-mode", "touch"], out _, out _), "unknown input mode");
        Expect(!BenchOptions.TryParse(["--windows", "0"], out _, out _), "zero windows");
        Expect(!BenchOptions.TryParse(["--windows", "65"], out _, out _), "too many windows");
        Expect(!BenchOptions.TryParse(["--scroll-speed", "0"], out _, out _), "zero scroll speed");
        Expect(!BenchOptions.TryParse(["--step-seconds", "-1"], out _, out _), "negative step");
        Expect(!BenchOptions.TryParse(["--screenshot-method", "camera"], out _, out _), "unknown screenshot method");
        Expect(!BenchOptions.TryParse(["--scene", "settings", "--tabs"], out _, out var error), "--tabs only for multiwindow");
        Expect(error!.Contains("--tabs"), "error names the variant");
        Expect(!BenchOptions.TryParse(["--scene", "inbox", "--decode-to-size"], out _, out _), "--decode-to-size only for gallery");
        Expect(!BenchOptions.TryParse(["--scene", "form", "--no-progressring"], out _, out _), "--no-progressring only for animated");
    }

    static void PerSceneDefaultsAndOverrides()
    {
        var settings = Parse("--scene", "settings");
        Near(30, settings.InteractionSeconds, 1e-9, "S1 interaction unchanged");
        Near(2, settings.InputHz, 1e-9, "S1 input hz unchanged");
        Near(10, settings.UpdateHz, 1e-9, "S1 update hz unchanged");

        foreach (var info in SceneCatalog.All)
        {
            var o = Parse("--scene", info.CliName);
            Near(info.Defaults.InteractionSeconds, o.InteractionSeconds, 1e-9, $"{info.CliName} interaction");
            Near(info.Defaults.InputHz, o.InputHz, 1e-9, $"{info.CliName} input hz");
            Near(info.Defaults.UpdateHz, o.UpdateHz, 1e-9, $"{info.CliName} update hz");
            Near(info.Defaults.ScrollSpeed, o.ScrollSpeed, 1e-9, $"{info.CliName} scroll speed");
            Equal(info.Defaults.Items, o.Items, $"{info.CliName} items");
        }
        Equal(10_000, Parse("--scene", "inbox").Items, "S3 items");
        Equal(500, Parse("--scene", "gallery").Items, "S5 images");
        Equal(1_000, Parse("--scene", "datagrid").Items, "S8 rows");

        // Explicit values win regardless of argument order.
        var o2 = Parse("--interaction", "7", "--input-hz", "3", "--scene", "inbox", "--items", "50");
        Near(7, o2.InteractionSeconds, 1e-9, "explicit interaction");
        Near(3, o2.InputHz, 1e-9, "explicit input hz");
        Equal(50, o2.Items, "explicit items");
    }

    static void SceneLabels()
    {
        Equal("Settings", Parse().SceneLabel, "S1");
        Equal("Dashboard", Parse("--scene", "dashboard").SceneLabel, "S2");
        Equal("Inbox", Parse("--scene", "inbox").SceneLabel, "S3");
        Equal("Gallery", Parse("--scene", "gallery").SceneLabel, "S5");
        Equal("Gallery-DecodeToSize", Parse("--scene", "gallery", "--decode-to-size").SceneLabel, "S5 variant");
        Equal("Animated", Parse("--scene", "animated").SceneLabel, "S6");
        Equal("Animated-NoProgressRing", Parse("--scene", "animated", "--no-progressring").SceneLabel, "S6 variant");
        Equal("MultiWindow-10", Parse("--scene", "multiwindow").SceneLabel, "S9 default");
        Equal("MultiWindow-5", Parse("--scene", "multiwindow", "--windows", "5").SceneLabel, "S9 windows");
        Equal("MultiWindow-Tabs", Parse("--scene", "multiwindow", "--tabs").SceneLabel, "S9 tabs");
        Equal(SceneId.Gallery, SceneCatalog.FromLabel("Gallery-DecodeToSize"), "label to scene");
        Equal(SceneId.MultiWindow, SceneCatalog.FromLabel("MultiWindow-Tabs"), "label to scene (tabs)");
        Equal(null, SceneCatalog.FromLabel("Nope"), "unknown label");
    }

    // ---- Metrics on synthetic traces ----

    // A minimal well-formed run: Warmup [100,200), Steady [200,60200), Interaction [60200,62200), Recovery [62200,62300).
    sealed class MiniRun
    {
        public const int Pid = 5150;
        public string Scene { get; init; } = "Inbox";
        public string Requested { get; init; } = "";
        public BackendMode Mode { get; init; } = BackendMode.Composition;
        public List<TraceRecord> Extra { get; } = [];
        public const double InteractionBegin = 60200, InteractionEnd = 62200;

        public List<TraceRecord> Build()
        {
            var r = new List<TraceRecord>
            {
                new ProcessStartRecord(0, Pid, "RenderBench.exe"),
                new RunStartedRecord(10, Pid, Scene, Requested, 1, 1280, 800, 1.0),
                new BackendSelectedRecord(20, Pid, Mode, Mode, FallbackReason.None, 0xF),
                new DeviceInfoRecord(30, Pid, Mode, 0x1, Mode == BackendMode.ImmediateD2DSoftware, 0xb100),
                new SceneReadyRecord(50, Pid, Scene, 100),
                new PhaseRecord(100, Pid, BenchPhase.Warmup, true), new PhaseRecord(200, Pid, BenchPhase.Warmup, false),
                new PhaseRecord(200, Pid, BenchPhase.Steady, true), new PhaseRecord(60200, Pid, BenchPhase.Steady, false),
                new PhaseRecord(InteractionBegin, Pid, BenchPhase.Interaction, true),
                new PhaseRecord(InteractionEnd, Pid, BenchPhase.Interaction, false),
                new PhaseRecord(62200, Pid, BenchPhase.Recovery, true), new PhaseRecord(62300, Pid, BenchPhase.Recovery, false),
                new MemorySampleRecord(1000, Pid, BenchPhase.Steady, 100 * MB, 150 * MB, 150 * MB, 0, 0, 10, 10, 1000, 0),
                new MemorySampleRecord(31000, Pid, BenchPhase.Steady, 100 * MB, 150 * MB, 150 * MB, 0, 0, 10, 10, 1600, 0),
                new RunCompletedRecord(62400, Pid, 0, "ok"),
            };
            r.AddRange(Extra);
            return r.OrderBy(x => x.TimeMs).ToList();
        }

        public RunMetrics Compute() => MetricsCalculator.Compute(Build());

        uint _frame = 1;

        public void DrawnFrame(double endMs, double lengthMs = 4)
        {
            Extra.Add(new FrameBeginRecord(endMs - lengthMs, Pid, _frame));
            Extra.Add(new FrameEndRecord(endMs, Pid, _frame++, true));
        }
    }

    const long MB = 1024 * 1024;

    static void FrameIntervalsAndDroppedFrames()
    {
        var run = new MiniRun();
        run.Extra.Add(new DisplayInfoRecord(40, MiniRun.Pid, 60));
        run.Extra.Add(new SceneTraitsRecord(45, MiniRun.Pid, AnimatesWhileIdle: false, ContinuousInteraction: true));
        // 61 frames on a 60 Hz cadence, minus 3 isolated frames (3 intervals of 2 vsyncs) and one 400 ms stall.
        double vsync = 1000.0 / 60;
        var skip = new HashSet<int> { 10, 20, 30 };
        double t = MiniRun.InteractionBegin + 10;
        for (int i = 0; i <= 60; i++)
        {
            if (!skip.Contains(i))
            {
                run.DrawnFrame(t + i * vsync, lengthMs: i == 5 ? 30 : 4);
            }
        }
        run.DrawnFrame(t + 60 * vsync + 400);
        var interaction = NotNull(run.Compute().Phase(BenchPhase.Interaction), "interaction");
        var intervals = NotNull(interaction.FrameIntervalMs, "intervals");
        Equal(58, intervals.Count, "intervals between 59 drawn frames");
        Near(vsync, intervals.P50, 1e-6, "p50 interval is one vsync");
        Equal(4, interaction.DroppedFrames, "3 skipped frames + 1 stall exceed 1.5x vsync");
        Near(400, interaction.LongestFrameGapMs, 1e-6, "stall is the longest gap in a continuous scene");
        Equal(1, interaction.LongFrames, "one frame's UI work exceeded 1.5x vsync");
        Near(60, run.Compute().RefreshHz, 1e-9, "refresh");
    }

    static void DiscreteInteractionIgnoresIdleGaps()
    {
        // Clicks at 2 Hz: each click draws a short burst; the gaps between bursts are idle, not drops.
        var run = new MiniRun { Scene = "Settings" };
        run.Extra.Add(new DisplayInfoRecord(40, MiniRun.Pid, 60));
        double vsync = 1000.0 / 60;
        for (int click = 0; click < 4; click++)
        {
            double start = MiniRun.InteractionBegin + 100 + click * 500;
            for (int k = 0; k < 3; k++)
            {
                run.DrawnFrame(start + k * vsync);
            }
        }
        var interaction = NotNull(run.Compute().Phase(BenchPhase.Interaction), "interaction");
        Equal(8, NotNull(interaction.FrameIntervalMs, "intervals").Count, "only intra-burst intervals");
        Equal(0, interaction.DroppedFrames, "idle gaps are not drops");
        Near(vsync, interaction.LongestFrameGapMs, 1e-6, "longest gap excludes idle gaps");
    }

    static void DefaultRefreshWithoutDisplayInfo()
    {
        var run = new MiniRun();
        run.Extra.Add(new SceneTraitsRecord(45, MiniRun.Pid, false, true));
        run.DrawnFrame(MiniRun.InteractionBegin + 100);
        run.DrawnFrame(MiniRun.InteractionBegin + 120); // 20 ms < 1.5 x 16.7 ms
        run.DrawnFrame(MiniRun.InteractionBegin + 150); // 30 ms > 25 ms
        var metrics = run.Compute();
        Equal(null, metrics.RefreshHz, "no DisplayInfo");
        Equal(1, metrics.Phase(BenchPhase.Interaction)!.DroppedFrames, "60 Hz assumed");
    }

    static void AnimationTaxAndSteadyExemption()
    {
        var run = new MiniRun { Scene = "Animated" };
        run.Extra.Add(new SceneTraitsRecord(45, MiniRun.Pid, AnimatesWhileIdle: true, ContinuousInteraction: true));
        for (int i = 0; i < 600; i++)
        {
            run.DrawnFrame(1000 + i * 50); // 20 frames/s during Steady
        }
        var metrics = run.Compute();
        // Steady samples: 600 ms of CPU over 30 s.
        Near(20, metrics.AnimationCpuMsPerSecond, 1e-6, "animation tax ms/s");
        Expect(metrics.AnimatesWhileIdle, "trait recorded");
        var idle = metrics.Gate.Single(g => g.Name == GateNames.SteadyStateIsIdle);
        Expect(idle.Passed, $"animated scenes are exempt: {idle.Detail}");
        Expect(idle.Detail.Contains("animation tax"), $"detail says why: {idle.Detail}");

        var plain = new MiniRun { Scene = "Inbox" };
        for (int i = 0; i < 600; i++)
        {
            plain.DrawnFrame(1000 + i * 50);
        }
        var plainMetrics = plain.Compute();
        Expect(!plainMetrics.Gate.Single(g => g.Name == GateNames.SteadyStateIsIdle).Passed, "non-animated scene must idle");
        Equal(null, plainMetrics.AnimationCpuMsPerSecond, "no animation tax for static scenes");
    }

    static void WindowStepsMarginalAndReclaimed()
    {
        var run = new MiniRun { Scene = "MultiWindow-3" };
        (string Name, int Windows, long PrivateMB)[] steps =
        [
            ("baseline", 0, 100), ("open-1", 1, 110), ("open-2", 2, 120), ("open-3", 3, 130),
            ("minimize-all", 3, 126), ("restore-all", 3, 128), ("close-all", 0, 106), ("settled", 0, 104),
        ];
        for (int i = 0; i < steps.Length; i++)
        {
            double t = MiniRun.InteractionBegin + 10 + i * 200;
            run.Extra.Add(new SceneStepRecord(t, MiniRun.Pid, i, steps[i].Name, steps[i].Windows));
            // An early sample that must be ignored in favor of the settled one at the end of the step.
            run.Extra.Add(new MemorySampleRecord(t + 20, MiniRun.Pid, BenchPhase.Interaction, 999 * MB, 0, 0, 0, 0, 0, 0, 0, 0));
            run.Extra.Add(new MemorySampleRecord(t + 190, MiniRun.Pid, BenchPhase.Interaction, steps[i].PrivateMB * MB,
                200 * MB, 200 * MB, 5 * MB, 0, 40 + steps[i].Windows, 20 + steps[i].Windows, 0, 0));
        }
        var metrics = run.Compute();
        var recorded = NotNull(metrics.Steps, "steps");
        Equal(steps.Length, recorded.Count, "step count");
        Equal("open-2", recorded[2].Name, "step name");
        Equal(2, recorded[2].Windows, "windows");
        Near(120, recorded[2].PrivateMB, 1e-6, "settled private MB of open-2");
        Equal(42, recorded[2].GdiObjects, "gdi objects");
        Near(10, metrics.MarginalPrivateMBPerWindow, 1e-6, "marginal MB per window");
        Near(26.0 / 30.0, metrics.PrivateMBReclaimedFraction, 1e-6, "reclaimed fraction after close");
    }

    static void TypingLatencyExcludesProgrammatic()
    {
        var run = new MiniRun { Scene = "Form" };
        double t = MiniRun.InteractionBegin + 100;
        for (int i = 0; i < 5; i++)
        {
            run.Extra.Add(new InputInjectedRecord(t + i * 100, MiniRun.Pid, i + 1, "key", "TextBox"));
            run.DrawnFrame(t + i * 100 + 8 + i); // 8..12 ms
        }
        run.Extra.Add(new InputInjectedRecord(t + 600, MiniRun.Pid, 10, "click", "Button"));
        run.DrawnFrame(t + 630);
        run.Extra.Add(new InputInjectedRecord(t + 700, MiniRun.Pid, 11, "programmatic:key", "TextBox"));
        run.DrawnFrame(t + 703);
        var metrics = run.Compute();
        var byKind = NotNull(metrics.InputToFrameMsByKind, "by kind");
        var keys = byKind["key"];
        Equal(5, keys.Count, "keystrokes");
        Near(10, keys.P50, 1e-6, "key p50");
        Near(30, byKind["click"].P50, 1e-6, "click");
        Expect(!byKind.ContainsKey("programmatic:key"), "programmatic input is not R1");
        Equal(6, NotNull(metrics.InputToFrameMs, "R1").Count, "R1 counts only real input");
        Near(3, NotNull(metrics.ProgrammaticToFrameMs, "programmatic").P50, 1e-6, "state change to frame");
    }

    static void PopupOpenTime()
    {
        var run = new MiniRun { Scene = "Form" };
        double t = MiniRun.InteractionBegin + 100;
        run.Extra.Add(new InputInjectedRecord(t, MiniRun.Pid, 50, "programmatic:popup", "ComboBox"));
        run.DrawnFrame(t + 10); // before the popup opened: not the popup frame
        run.Extra.Add(new PopupOpenedRecord(t + 20, MiniRun.Pid, 50, "ComboBox"));
        run.DrawnFrame(t + 35);
        run.Extra.Add(new InputInjectedRecord(t + 500, MiniRun.Pid, 51, "popup", "Flyout"));
        run.Extra.Add(new PopupOpenedRecord(t + 540, MiniRun.Pid, 51, "Flyout"));
        run.DrawnFrame(t + 555);
        run.Extra.Add(new PopupOpenedRecord(t + 900, MiniRun.Pid, 99, "ToolTip")); // no matching input: ignored
        var popups = NotNull(run.Compute().PopupOpenMs, "popup open");
        Equal(2, popups.Count, "matched popups");
        Near(45, popups.P50, 1e-6, "p50 of 35 and 55 ms (request to first drawn frame after Opened)");
        Near(55, popups.Max, 1e-6, "max");
    }

    static void SceneValues()
    {
        var run = new MiniRun();
        run.Extra.Add(new SceneValueRecord(1000, MiniRun.Pid, "RealizedItems", 18));
        foreach (var (dt, v) in new[] { (100.0, 20.0), (200, 22), (300, 24) })
        {
            run.Extra.Add(new SceneValueRecord(MiniRun.InteractionBegin + dt, MiniRun.Pid, "RealizedItems", v));
        }
        run.Extra.Add(new SceneValueRecord(MiniRun.InteractionBegin + 50, MiniRun.Pid, "ImageLoadMs", 7.5));
        var metrics = run.Compute();
        var interaction = NotNull(NotNull(metrics.Phase(BenchPhase.Interaction), "interaction").SceneValues, "values");
        Near(22, interaction["RealizedItems"].P50, 1e-9, "realized p50");
        Near(24, interaction["RealizedItems"].Max, 1e-9, "realized max");
        Near(7.5, interaction["ImageLoadMs"].P50, 1e-9, "image load");
        Near(18, metrics.Phase(BenchPhase.Steady)!.SceneValues!["RealizedItems"].P50, 1e-9, "steady value");
        Equal(null, metrics.Phase(BenchPhase.Recovery)!.SceneValues, "no values -> null");
    }

    static void UnsupportedCountsByType()
    {
        var run = new MiniRun { Scene = "Reader", Requested = "gdiplus", Mode = BackendMode.ImmediateGdiPlus };
        run.Extra.Add(new UnsupportedContentRecord(500, MiniRun.Pid, BackendMode.ImmediateGdiPlus, "RichTextBlock", "Element content"));
        run.Extra.Add(new UnsupportedContentRecord(600, MiniRun.Pid, BackendMode.ImmediateGdiPlus, "Border", "Non-rectangular clip"));
        run.Extra.Add(new UnsupportedContentRecord(700, MiniRun.Pid, BackendMode.ImmediateGdiPlus, "Grid", "Non-rectangular clip"));
        run.Extra.Add(new UnsupportedContentRecord(800, MiniRun.Pid, BackendMode.ImmediateGdiPlus, "Border", "Non-rectangular clip"));
        var metrics = run.Compute();
        var byType = NotNull(metrics.UnsupportedByType, "by type");
        Equal(2, byType["Border|Non-rectangular clip"], "Border clip events");
        Equal(1, byType["RichTextBlock|Element content"], "RTB");
        var byDetail = NotNull(metrics.UnsupportedByDetail, "by detail");
        Equal(3, byDetail["Non-rectangular clip"], "clip events of any type");
        Expect(!metrics.Gate.Single(g => g.Name == GateNames.NoUnsupportedContent).Passed, "unexpected content fails");
        Equal(0, metrics.ExpectedUnsupported!.Count, "nothing expected for S4");
    }

    static void ExpectedUnsupportedPerScene()
    {
        static GateCheck Gate(string scene, string type, string detail, out RunMetrics metrics)
        {
            var run = new MiniRun { Scene = scene, Requested = "d2d-sw", Mode = BackendMode.ImmediateD2DSoftware };
            run.Extra.Add(new UnsupportedContentRecord(500, MiniRun.Pid, BackendMode.ImmediateD2DSoftware, type, detail));
            metrics = run.Compute();
            return metrics.Gate.Single(g => g.Name == GateNames.NoUnsupportedContent);
        }

        var ring = Gate("Animated", "Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer", "Hand-in Composition visual", out var m1);
        Expect(ring.Passed, $"S6 ProgressRing's Lottie visual is expected: {ring.Detail}");
        Expect(ring.Detail.Contains("expected"), $"detail marks it expected: {ring.Detail}");
        Equal(1, m1.ExpectedUnsupported!.Count, "listed as expected");
        Expect(!Gate("Animated-NoProgressRing", "Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer", "Hand-in Composition visual", out _).Passed,
            "no ProgressRing in the variant: nothing expected");
        Expect(!Gate("Animated", "Grid", "Projection", out _).Passed, "other content in S6 is not expected");
        Expect(Gate("Layered", "Border", "Brush", out _).Passed, "S7 acrylic brush expected");
        Expect(Gate("Layered", "Border", "ThemeShadow", out _).Passed, "S7 shadow expected");
        Expect(Gate("Layered", "Window", "SystemBackdrop (Mica)", out _).Passed, "S7 mica expected");
        Expect(!Gate("Layered", "Border", "Non-rectangular clip", out _).Passed, "S7 rounded clip is a real gap");
        Expect(!Gate("Settings", "Border", "Brush", out _).Passed, "S1 expects nothing");
        Expect(!Gate("Form", "Popup", "Open popups", out _).Passed, "S10 needs popups");
    }

    static void InputModeRecorded()
    {
        var run = new MiniRun();
        run.Extra.Add(new InputModeRecord(45, MiniRun.Pid, "Programmatic", "session 2 is not the active console session"));
        var metrics = run.Compute();
        Equal("Programmatic", metrics.InputMode, "input mode");
        Expect(metrics.InputModeReason!.Contains("console"), "reason");
    }

    static void JsonRoundTrip()
    {
        var run = new MiniRun { Scene = "MultiWindow-1" };
        run.Extra.Add(new DisplayInfoRecord(40, MiniRun.Pid, 144));
        run.Extra.Add(new SceneTraitsRecord(45, MiniRun.Pid, true, true));
        run.Extra.Add(new SceneStepRecord(MiniRun.InteractionBegin + 10, MiniRun.Pid, 0, "baseline", 0));
        run.Extra.Add(new SceneValueRecord(MiniRun.InteractionBegin + 20, MiniRun.Pid, "RealizedItems", 3));
        run.Extra.Add(new InputInjectedRecord(MiniRun.InteractionBegin + 30, MiniRun.Pid, 1, "key", "TextBox"));
        run.DrawnFrame(MiniRun.InteractionBegin + 40);
        run.DrawnFrame(MiniRun.InteractionBegin + 47);
        var metrics = run.Compute();
        var back = MetricsJson.Deserialize(MetricsJson.Serialize(metrics));
        Near(144, back.RefreshHz, 1e-9, "refresh");
        Expect(back.AnimatesWhileIdle, "trait");
        Equal(1, back.Steps!.Count, "steps");
        Near(3, back.Phase(BenchPhase.Interaction)!.SceneValues!["RealizedItems"].P50, 1e-9, "scene value");
        Equal(1, back.InputToFrameMsByKind!["key"].Count, "by kind");
        Equal(metrics.Phase(BenchPhase.Interaction)!.DroppedFrames, back.Phase(BenchPhase.Interaction)!.DroppedFrames, "dropped");

        // metrics.json written before these fields existed still loads.
        string old = MetricsJson.Serialize(MetricsCalculator.Compute(new SyntheticRun().Build()));
        var legacy = MetricsJson.Deserialize(old.Replace("\"FrameIntervalMs\"", "\"Ignored1\"").Replace("\"Steps\"", "\"Ignored2\""));
        Equal(null, legacy.Steps, "legacy steps");
    }

    static void ReportRowsAndSteps()
    {
        var run = new MiniRun { Scene = "MultiWindow-2" };
        run.Extra.Add(new SceneTraitsRecord(45, MiniRun.Pid, false, false));
        for (int i = 0; i < 3; i++)
        {
            double t = MiniRun.InteractionBegin + 10 + i * 200;
            run.Extra.Add(new SceneStepRecord(t, MiniRun.Pid, i, i == 0 ? "baseline" : $"open-{i}", i));
            run.Extra.Add(new MemorySampleRecord(t + 100, MiniRun.Pid, BenchPhase.Interaction, (100 + 10 * i) * MB, 0, 0, 0, 0, 0, 0, 0, 0));
            run.DrawnFrame(t + 50);
        }
        run.Extra.Add(new InputInjectedRecord(MiniRun.InteractionBegin + 900, MiniRun.Pid, 1, "key", "TextBox"));
        run.DrawnFrame(MiniRun.InteractionBegin + 912);
        run.Extra.Add(new SceneValueRecord(MiniRun.InteractionBegin + 20, MiniRun.Pid, "RealizedItems", 3));
        string report = ComparisonReport.Build([("a", run.Compute())]);
        foreach (var row in new[]
        {
            "| R2 frame interval p95, interaction (ms) |", "| R2 dropped frames (>1.5x vsync), interaction |",
            "| R3 longest frame gap, interaction (ms) |", "| R1* key to frame end p95 (ms) |",
            "| Popup open to frame end p95 (ms) |", "| Programmatic change to frame end p95 (ms) |",
            "| E1 animation tax CPU, steady (ms/s) |", "| M1 marginal private per window (MB) |",
            "| M1 private reclaimed after close (%) |", "| Unsupported content events, expected/unexpected |",
            "| Scene RealizedItems p50, interaction |",
        })
        {
            Expect(report.Contains(row), $"report row {row}");
        }
        Expect(report.Contains("### Steps: MultiWindow-2"), "step table");
        Expect(report.Contains("| open-2 (2 windows) |"), "step row");
        Expect(report.Contains(" 120.0 |"), "step private MB");
    }

    // Requires elevation: emits the new harness events through a real ETW session and reads them back.
    static void EtlExtendedHarnessEvents()
    {
        ProfileTests.RequireAdmin();
        string etl = Path.Combine(Path.GetTempPath(), $"renderbench-ext-{Environment.ProcessId}.etl");
        try
        {
            using (var session = new TraceEventSession($"RenderBenchExtTest-{Environment.ProcessId}", etl))
            {
                session.EnableProvider(BenchEventSource.ProviderId, TraceEventLevel.Verbose);
                Thread.Sleep(500);
                BenchEventSource.Log.InputModeSelected("Programmatic", "not console");
                BenchEventSource.Log.SceneTraits(true, false);
                BenchEventSource.Log.DisplayInfo(59.94);
                BenchEventSource.Log.SceneStep(3, "open-3", 3);
                BenchEventSource.Log.SceneValue("RealizedItems", 42.5);
                BenchEventSource.Log.PopupOpened(77, "ComboBox");
                BenchEventSource.Log.ScreenshotSaved("x.bmp", 10, 20, "PrintWindow");
                Thread.Sleep(500);
            }
            var records = EtlReader.Read(etl).Where(r => r.ProcessId == Environment.ProcessId).ToList();
            var mode = NotNull(records.OfType<InputModeRecord>().FirstOrDefault(), "InputModeSelected");
            Equal("Programmatic", mode.Mode, "mode");
            Equal("not console", mode.Reason, "reason");
            var traits = NotNull(records.OfType<SceneTraitsRecord>().FirstOrDefault(), "SceneTraits");
            Expect(traits.AnimatesWhileIdle && !traits.ContinuousInteraction, "traits");
            Near(59.94, NotNull(records.OfType<DisplayInfoRecord>().FirstOrDefault(), "DisplayInfo").RefreshHz, 1e-9, "hz");
            var step = NotNull(records.OfType<SceneStepRecord>().FirstOrDefault(), "SceneStep");
            Equal(3, step.Step, "step");
            Equal("open-3", step.Name, "step name");
            Equal(3, step.Windows, "windows");
            var value = NotNull(records.OfType<SceneValueRecord>().FirstOrDefault(), "SceneValue");
            Equal("RealizedItems", value.Name, "value name");
            Near(42.5, value.Value, 1e-9, "value");
            var popup = NotNull(records.OfType<PopupOpenedRecord>().FirstOrDefault(), "PopupOpened");
            Equal(77L, popup.InputId, "popup input id");
            Equal("ComboBox", popup.Popup, "popup");
        }
        finally
        {
            File.Delete(etl);
        }
    }
}
