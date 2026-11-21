using System.Xml.Linq;
using RenderBench.Analysis;
using RenderBench.Analyzer;
using static RenderBench.Tests.Check;

namespace RenderBench.Tests;

// Added by immediate-rendering-d2d-sw: per-thread CPU split (W1/W2), WARP raster-stack versions, DeviceLost.
static class ThreadCpuTests
{
    public static IEnumerable<TestCase> All() =>
    [
        new("ThreadCpu.Classify.ModuleNames", ClassifyModuleNames),
        new("ThreadCpu.Classify.UiThreadWinsOverModule", () =>
            Equal(ThreadCategory.Ui, ThreadCategories.Classify(@"C:\Windows\System32\d3d10warp.dll", isUiThread: true), "ui")),
        new("ThreadCpu.Accumulator.KeepsExitedThreadsAndSplitsRecycledIds", AccumulatorKeepsExitedThreads),
        new("ThreadCpu.Accumulator.ModuleSummary", AccumulatorModuleSummary),
        new("ThreadCpu.Metrics.PerPhaseSplit", PerPhaseSplit),
        new("ThreadCpu.Metrics.AbsentWithoutSamples", () =>
            Expect(MetricsCalculator.Compute(new SyntheticRun().Build()).Phase(BenchPhase.Interaction)!.ThreadCpu is null, "no samples, no split")),
        new("ThreadCpu.Metrics.ModulesVersionsAndDeviceLost", ModulesVersionsAndDeviceLost),
        new("ThreadCpu.Metrics.JsonRoundTrip", JsonRoundTrip),
        new("ThreadCpu.Report.HasCategoryRows", ReportRows),
        new("Wprp.TimingProfileSupportsWarpAnalysis", TimingProfileSupportsWarpAnalysis),
    ];

    static void ClassifyModuleNames()
    {
        (string? Module, ThreadCategory Expected)[] cases =
        [
            (@"C:\Windows\System32\d3d10warp.dll", ThreadCategory.WarpRasterizer),
            ("D3D10WARP.DLL", ThreadCategory.WarpRasterizer),
            (@"C:\Windows\System32\d2d1.dll", ThreadCategory.D2DDWrite),
            (@"C:\Windows\System32\DWrite.dll", ThreadCategory.D2DDWrite),
            (@"C:\app\DWriteCore.dll", ThreadCategory.D2DDWrite),
            (@"C:\Windows\System32\dcomp.dll", ThreadCategory.Composition),
            (@"C:\Windows\System32\dcompi.dll", ThreadCategory.Composition),
            (@"C:\app\Microsoft.UI.Composition.OSSupport.dll", ThreadCategory.Composition),
            (@"C:\Windows\System32\CoreMessaging.dll", ThreadCategory.Composition),
            (@"C:\app\CoreMessagingXP.dll", ThreadCategory.Composition),
            (@"C:\app\dwmcorei.dll", ThreadCategory.Composition),
            (@"C:\Program Files\dotnet\shared\Microsoft.NETCore.App\10.0.0\coreclr.dll", ThreadCategory.Other),
            (@"C:\Windows\System32\ntdll.dll", ThreadCategory.Other),
            (@"C:\Windows\System32\d3d11.dll", ThreadCategory.Other),
            (@"C:\app\Microsoft.UI.Xaml.dll", ThreadCategory.Other),
            ("d2d1.dll.mui", ThreadCategory.Other),
            ("", ThreadCategory.Other),
            (null, ThreadCategory.Other),
        ];
        foreach (var (module, expected) in cases)
        {
            Equal(expected, ThreadCategories.Classify(module, isUiThread: false), $"classify '{module}'");
        }
        Equal("d2d1.dll", ThreadCategories.ModuleName(@"C:\Windows\System32\d2d1.dll"), "module name");
    }

    static void AccumulatorKeepsExitedThreads()
    {
        var acc = new ThreadCpuAccumulator { UiThreadId = 1 };
        var (ms, live) = acc.Add(
        [
            new(1, 100, @"C:\app\RenderBench.exe", 10),
            new(2, 100, "d3d10warp.dll", 20),
            new(3, 100, "ntdll.dll", 5),
        ]);
        Equal(3, live, "live threads");
        Near(10, ms[(int)ThreadCategory.Ui], 1e-9, "ui");
        Near(20, ms[(int)ThreadCategory.WarpRasterizer], 1e-9, "warp");

        // Thread 2 exits; its CPU is kept. Thread id 3 is recycled by a new WARP worker (different creation time).
        (ms, live) = acc.Add(
        [
            new(1, 100, @"C:\app\RenderBench.exe", 15),
            new(3, 200, "d3d10warp.dll", 7),
        ]);
        Equal(2, live, "live threads after exit");
        Near(15, ms[(int)ThreadCategory.Ui], 1e-9, "ui grows");
        Near(27, ms[(int)ThreadCategory.WarpRasterizer], 1e-9, "exited warp thread kept + recycled id counted separately");
        Near(5, ms[(int)ThreadCategory.Other], 1e-9, "exited other thread kept");
    }

    static void AccumulatorModuleSummary()
    {
        var acc = new ThreadCpuAccumulator();
        acc.Add([new(2, 1, @"C:\x\d3d10warp.dll", 20), new(3, 1, @"C:\y\D3D10Warp.dll", 30), new(4, 1, null, 1)]);
        var modules = acc.Modules();
        Equal(2, modules.Count, "module rows merge case-insensitively");
        Equal(ThreadCategory.WarpRasterizer, modules[0].Category, "busiest first");
        Equal("(unknown)", modules[1].Module, "unknown start module");
    }

    // Synthetic run plus ThreadCpuSample records: Interaction samples at 62000, 66000, 70000 (8 s window).
    static List<TraceRecord> RunWithThreadSamples(params TraceRecord[] extra)
    {
        var records = new SyntheticRun { HarnessRequested = "d2d-sw", Requested = BackendMode.ImmediateD2DSoftware,
            Effective = BackendMode.ImmediateD2DSoftware, DeviceMode = BackendMode.ImmediateD2DSoftware, IsWarp = true }.Build();
        for (int i = 0; i < 3; i++)
        {
            records.Add(new ThreadCpuSampleRecord(62000 + i * 4000, SyntheticRun.Pid, BenchPhase.Interaction,
                UiMs: 1000 + i * 400, WarpRasterizerMs: 500 + i * 800, D2DDWriteMs: 10 + i * 4, CompositionMs: 50 + i * 40,
                OtherMs: 200 + i * 80, ThreadCount: 20));
        }
        records.Add(new ThreadCpuSampleRecord(2000, SyntheticRun.Pid, BenchPhase.Steady, 900, 400, 10, 40, 150, 18));
        records.Add(new ThreadCpuSampleRecord(12000, SyntheticRun.Pid, BenchPhase.Steady, 905, 400, 10, 42, 151, 18));
        // Another process's samples are ignored.
        records.Add(new ThreadCpuSampleRecord(64000, SyntheticRun.OtherPid, BenchPhase.Interaction, 1e6, 1e6, 1e6, 1e6, 1e6, 1));
        records.AddRange(extra);
        return records.OrderBy(r => r.TimeMs).ToList();
    }

    static void PerPhaseSplit()
    {
        var metrics = MetricsCalculator.Compute(RunWithThreadSamples());
        var split = NotNull(metrics.Phase(BenchPhase.Interaction)!.ThreadCpu, "interaction split");
        Near(8000, split.WindowMs, 1e-9, "window");
        Near(800, split.UiMs, 1e-9, "ui ms");
        Near(1600, split.WarpRasterizerMs, 1e-9, "warp ms");
        Near(8, split.D2DDWriteMs, 1e-9, "d2d ms");
        Near(80, split.CompositionMs, 1e-9, "composition ms");
        Near(160, split.OtherMs, 1e-9, "other ms");
        Near(200, split.MsPerSecond(ThreadCategory.WarpRasterizer), 1e-9, "warp ms/s");
        Near(2648, split.TotalMs, 1e-9, "total");
        var steady = NotNull(metrics.Phase(BenchPhase.Steady)!.ThreadCpu, "steady split");
        Near(5, steady.UiMs, 1e-9, "steady ui");
        Near(0, steady.WarpRasterizerMs, 1e-9, "steady warp idle");
        Expect(metrics.Phase(BenchPhase.Warmup)!.ThreadCpu is null, "warmup has no samples");
    }

    static void ModulesVersionsAndDeviceLost()
    {
        var metrics = MetricsCalculator.Compute(RunWithThreadSamples(
            new ThreadModuleSummaryRecord(72550, SyntheticRun.Pid, "ntdll.dll", "Other", 12, 300),
            new ThreadModuleSummaryRecord(72550, SyntheticRun.Pid, "d3d10warp.dll", "WarpRasterizer", 4, 2100),
            new RasterModuleVersionRecord(72551, SyntheticRun.Pid, "d3d10warp.dll", "10.0.26100.1"),
            new RasterModuleVersionRecord(72551, SyntheticRun.OtherPid, "d2d1.dll", "ignored"),
            new DeviceLostRecord(65000, SyntheticRun.Pid, BackendMode.ImmediateD2DSoftware, unchecked((int)0x887A0005),
                unchecked((int)0x887A0007), 42)));
        var modules = NotNull(metrics.ThreadModules, "modules");
        Equal("d3d10warp.dll", modules[0].Module, "busiest module first");
        Equal(4, modules[0].Threads, "warp threads");
        var versions = NotNull(metrics.RasterModuleVersions, "versions");
        Equal(1, versions.Count, "only this process's versions");
        Equal("10.0.26100.1", versions["D3D10WARP.DLL"], "case-insensitive lookup");
        Equal(1, metrics.DeviceLostCount, "device lost count");
    }

    static void JsonRoundTrip()
    {
        var metrics = MetricsCalculator.Compute(RunWithThreadSamples(
            new ThreadModuleSummaryRecord(72550, SyntheticRun.Pid, "d3d10warp.dll", "WarpRasterizer", 4, 2100),
            new RasterModuleVersionRecord(72551, SyntheticRun.Pid, "d3d10warp.dll", "10.0.26100.1")));
        var back = MetricsJson.Deserialize(MetricsJson.Serialize(metrics));
        Near(1600, back.Phase(BenchPhase.Interaction)!.ThreadCpu?.WarpRasterizerMs, 1e-9, "warp ms");
        Equal("d3d10warp.dll", back.ThreadModules?[0].Module, "modules");
        Equal("10.0.26100.1", back.RasterModuleVersions?["d3d10warp.dll"], "versions");

        // metrics.json written before the split existed still loads.
        var old = MetricsJson.Deserialize(MetricsJson.Serialize(MetricsCalculator.Compute(new SyntheticRun().Build())));
        Expect(old.ThreadModules is null && old.DeviceLostCount == 0, "defaults");
    }

    static void ReportRows()
    {
        var metrics = MetricsCalculator.Compute(RunWithThreadSamples());
        string report = ComparisonReport.Build([("a", metrics)]);
        Expect(report.Contains("| W2 CPU WARP rasterizer, interaction (ms/s) |"), "warp row");
        Expect(report.Contains("| W2 CPU UI thread, interaction (ms/s) |"), "ui row");
        Expect(report.Contains("| W2 CPU composition, interaction (ms/s) |"), "composition row");
        Expect(report.Contains(" 200.0 |"), "warp ms/s value");
    }

    // WARP rasterizes on worker threads; WPA needs CPU samples, context switches and ready-thread events with stacks (and
    // image loads to symbolize them) to attribute that work. The native provider carries PaintBegin/End and DeviceLost.
    static void TimingProfileSupportsWarpAnalysis()
    {
        var doc = XDocument.Load(Path.Combine(AppContext.BaseDirectory, "profiles", "ImmediateRendering.wprp"));
        var profile = doc.Descendants("Profile").Single(p => (string?)p.Attribute("Name") == "Timing");
        var systemProviderIds = profile.Descendants("SystemProviderId").Select(e => (string)e.Attribute("Value")!).ToList();
        var systemProviders = doc.Descendants("SystemProvider").ToDictionary(e => (string)e.Attribute("Id")!);

        var keywords = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var stacks = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var id in systemProviderIds)
        {
            // Follow Base= chains.
            for (var provider = systemProviders[id]; provider is not null;
                provider = (string?)provider.Attribute("Base") is { } b ? systemProviders[b] : null)
            {
                keywords.UnionWith(provider.Element("Keywords")?.Elements("Keyword").Select(k => (string)k.Attribute("Value")!) ?? []);
                stacks.UnionWith(provider.Element("Stacks")?.Elements("Stack").Select(k => (string)k.Attribute("Value")!) ?? []);
            }
        }
        foreach (var keyword in new[] { "SampledProfile", "CSwitch", "ReadyThread", "Loader", "ProcessThread" })
        {
            Expect(keywords.Contains(keyword), $"Timing kernel keyword {keyword}");
        }
        foreach (var stack in new[] { "SampledProfile", "CSwitch", "ReadyThread" })
        {
            Expect(stacks.Contains(stack), $"Timing stack {stack}");
        }

        var native = doc.Descendants("EventProvider").Single(e =>
            string.Equals((string?)e.Attribute("Name"), BackendModes.RenderingBackendProviderId.ToString(), StringComparison.OrdinalIgnoreCase));
        Expect(profile.Descendants("EventProviderId").Any(e => (string?)e.Attribute("Value") == (string?)native.Attribute("Id")),
            "Timing enables the native provider");
        Expect(int.Parse((string)native.Attribute("Level")!) >= 3, "native level includes WARNING (DeviceLost)");
    }
}
