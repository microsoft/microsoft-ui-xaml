using System.Text.Json.Nodes;
using System.Xml.Linq;
using RenderBench.Analysis;
using RenderBench.Analyzer;
using static RenderBench.Tests.Check;

namespace RenderBench.Tests;

// D2D-HW readiness: adapter policy (mirror of the native factory), W3 GPU paint time, M3 on the render adapter, skipped
// D2D-HW runs on GPU-less machines, and the Gpu WPR profile.
static class D2DHardwareTests
{
    const uint Nvidia = 0x10de, Amd = 0x1002, Intel = 0x8086;

    static AdapterInfo Adapter(uint vendor, uint device, ulong luid, uint flags = 0, string name = "")
        => new(name.Length > 0 ? name : $"{vendor:x4}:{device:x4}", vendor, device, flags, luid, 0);

    static readonly AdapterInfo s_warp = Adapter(0x1414, 0x8c, 0x10, AdapterPolicy.SoftwareFlag, "Microsoft Basic Render Driver");
    static readonly AdapterInfo s_basicRender = Adapter(0x1414, 0x8c, 0x11, name: "Microsoft Basic Render Driver");
    static readonly AdapterInfo s_hyperV = Adapter(0x1414, 0x5353, 0x12, name: "Microsoft Hyper-V Video");
    static readonly AdapterInfo s_remote = Adapter(0x1414, 0x8e, 0x13, name: "Microsoft Remote Display Adapter");
    static readonly AdapterInfo s_nvidia = Adapter(Nvidia, 0x2684, 0x20);
    static readonly AdapterInfo s_amd = Adapter(Amd, 0x744c, 0x21);
    static readonly AdapterInfo s_intel = Adapter(Intel, 0xa780, 0x22);

    public static IEnumerable<TestCase> All() =>
    [
        new("Adapters.PolicyRejectsSoftwareAndMicrosoftAdapters", PolicyClassifies),
        new("Adapters.ChoosesFirstQualifyingAndOverrideIsStrict", ChoosesAdapter),
        new("Adapters.ParsesLuidOverride", ParsesLuidOverride),
        new("Adapters.ReportExitCodes", ReportExitCodes),
        new("Adapters.MachineEnumerationIsConsistent", MachineEnumeration),
        new("Analysis.GpuPaintTime.AttributedByFrame", GpuPaintAttributedByFrame),
        new("Analysis.GpuMemory.RenderAdapterOnly", RenderAdapterMemory),
        new("Analysis.Skipped.IsNotAGateFailure", SkippedIsNotFailure),
        new("Analysis.Json.OlderMetricsStillLoad", OlderMetricsLoad),
        new("Report.SkippedD2DHardwareShowsNoGpu", ReportShowsNoGpu),
        new("Report.ShowsGpuPaintAndRenderAdapterMemory", ReportShowsW3AndM3),
        new("Wprp.GpuProfileCapturesGpuTimeAndMemory", GpuProfile),
    ];

    static void PolicyClassifies()
    {
        Equal(AdapterVerdict.SoftwareAdapter, AdapterPolicy.Classify(s_warp), "WARP");
        Equal(AdapterVerdict.SoftwareAdapter, AdapterPolicy.Classify(Adapter(Nvidia, 1, 1, AdapterPolicy.SoftwareFlag)),
            "software flag wins over vendor");
        Equal(AdapterVerdict.BasicRenderDriver, AdapterPolicy.Classify(s_basicRender), "BRD without software flag");
        Equal(AdapterVerdict.MicrosoftVirtualAdapter, AdapterPolicy.Classify(s_hyperV), "Hyper-V");
        Equal(AdapterVerdict.MicrosoftVirtualAdapter, AdapterPolicy.Classify(s_remote), "Remote Display");
        foreach (var gpu in new[] { s_nvidia, s_amd, s_intel })
        {
            Expect(AdapterPolicy.Qualifies(gpu), $"{gpu.Description} qualifies");
        }
    }

    static void ChoosesAdapter()
    {
        Equal(s_amd, AdapterPolicy.Choose([s_hyperV, s_warp, s_amd, s_intel]), "first qualifying in preference order");
        Equal(null, AdapterPolicy.Choose([s_hyperV, s_basicRender, s_remote, s_warp]), "VM adapters: none");
        Equal(null, AdapterPolicy.Choose([]), "no adapters");
        Equal(s_intel, AdapterPolicy.Choose([s_amd, s_intel], s_intel.Luid), "override pins");
        Equal(null, AdapterPolicy.Choose([s_amd, s_intel], 0x999), "unknown override never substitutes");
        Equal(null, AdapterPolicy.Choose([s_amd, s_hyperV], s_hyperV.Luid), "override to a rejected adapter");
    }

    static void ParsesLuidOverride()
    {
        (string Text, ulong Luid)[] good = [("1a2b", 0x1a2b), ("0x400BB5EA9", 0x400bb5ea9), ("  0X10  ", 0x10), ("ffffffffffffffff", ulong.MaxValue)];
        foreach (var (text, luid) in good)
        {
            Expect(AdapterPolicy.TryParseLuidOverride(text, out var parsed), $"parse '{text}'");
            Equal(luid, parsed, text);
        }
        foreach (var text in new[] { null, "", "  ", "0x", "0", "0x0", "12g", "-1", "1 2", "10000000000000000" })
        {
            Expect(!AdapterPolicy.TryParseLuidOverride(text, out _), $"reject '{text}'");
        }
    }

    static void ReportExitCodes()
    {
        var output = new StringWriter();
        Equal(AdapterReport.NoHardwareAdapter, AdapterReport.Evaluate([s_basicRender, s_hyperV, s_warp], null, output), "VM");
        Expect(output.ToString().Contains(AdapterPolicy.NoHardwareAdapterReason), output.ToString());
        Equal(AdapterReport.Qualifies, AdapterReport.Evaluate([s_warp, s_nvidia], "", TextWriter.Null), "GPU");
        Equal(AdapterReport.Qualifies, AdapterReport.Evaluate([s_nvidia, s_intel], "0x22", TextWriter.Null), "override");
        Equal(AdapterReport.NoHardwareAdapter, AdapterReport.Evaluate([s_nvidia], "0x22", TextWriter.Null), "override missing");
        Equal(AdapterReport.BadOverride, AdapterReport.Evaluate([s_nvidia], "zz", TextWriter.Null), "malformed override");
    }

    static void MachineEnumeration()
    {
        var adapters = DxgiAdapters.Enumerate();
        Expect(adapters.Count > 0, "DXGI always reports at least the Basic Render Driver");
        Expect(adapters.All(a => a.Luid != 0), "LUIDs are populated");
        Equal(adapters.Count, adapters.Select(a => a.Luid).Distinct().Count(), "LUIDs are unique");
        var chosen = AdapterPolicy.Choose(adapters);
        Console.WriteLine($"      {adapters.Count} adapters; d2d-hw: {(chosen is null ? AdapterPolicy.NoHardwareAdapterReason : chosen.Description)}");
        Equal(chosen is null ? AdapterReport.NoHardwareAdapter : AdapterReport.Qualifies,
            AdapterReport.Evaluate(adapters, null, TextWriter.Null), "adapters command agrees with the policy");
    }

    // Interaction frames in SyntheticRun are 3..102 (frame i begins at 61550 + 100*(i-3)).
    static List<TraceRecord> WithGpuPaintTimes()
    {
        var records = new SyntheticRun().Build();
        for (uint i = 0; i < 100; i++)
        {
            // Read back 70 ms after FrameBegin: the last few land after Interaction ends but still belong to it.
            records.Add(new GpuPaintTimeRecord(61550 + i * 100 + 70, SyntheticRun.Pid, 3 + i, 1000 + (i % 10) * 100));
        }
        // An untraced frame falls back to its event time (Steady).
        records.Add(new GpuPaintTimeRecord(2000, SyntheticRun.Pid, 9999, 5000));
        // Another process is ignored.
        records.Add(new GpuPaintTimeRecord(62000, SyntheticRun.OtherPid, 3, 999_000));
        return records.OrderBy(r => r.TimeMs).ToList();
    }

    static void GpuPaintAttributedByFrame()
    {
        var metrics = MetricsCalculator.Compute(WithGpuPaintTimes());
        var interaction = NotNull(NotNull(metrics.Phase(BenchPhase.Interaction), "interaction").GpuPaintMs, "gpu paint");
        Equal(100, interaction.Count, "all interaction paints, including ones read back after the phase ended");
        Near(1.45, interaction.P50, 1e-9, "p50 ms");
        Near(1.9, interaction.Max, 1e-9, "max ms");
        var steady = NotNull(NotNull(metrics.Phase(BenchPhase.Steady), "steady").GpuPaintMs, "steady gpu paint");
        Equal(1, steady.Count, "untraced frame by event time");
        Equal(null, metrics.Phase(BenchPhase.Recovery)!.GpuPaintMs, "nothing in recovery");
        Equal(null, MetricsCalculator.Compute(new SyntheticRun().Build()).Phase(BenchPhase.Interaction)!.GpuPaintMs,
            "absent without GpuPaintTime events");
    }

    static void RenderAdapterMemory()
    {
        const long MB = 1024 * 1024;
        var records = new SyntheticRun().Build(); // DeviceInfo.AdapterLuid = 0x1234
        for (int i = 0; i < 3; i++)
        {
            records.Add(new GpuAdapterMemoryRecord(2000 + i * 10000, SyntheticRun.Pid, BenchPhase.Steady, 0x1234, (30 + i) * MB, 4 * MB));
            records.Add(new GpuAdapterMemoryRecord(2000 + i * 10000, SyntheticRun.Pid, BenchPhase.Steady, 0x9999, 500 * MB, 50 * MB));
        }
        var steady = NotNull(MetricsCalculator.Compute(records).Phase(BenchPhase.Steady), "steady");
        Near(31, steady.RenderAdapterGpuLocalMBMedian, 1e-9, "render adapter local");
        Near(4, steady.RenderAdapterGpuNonLocalMBMedian, 1e-9, "render adapter non-local");
        Near(40, steady.GpuLocalMBMedian, 1e-9, "all-adapter sum keeps its meaning (MemorySample)");

        var withoutDevice = records.Where(r => r is not DeviceInfoRecord).ToList();
        Equal(null, MetricsCalculator.Compute(withoutDevice).Phase(BenchPhase.Steady)!.RenderAdapterGpuLocalMBMedian,
            "no DeviceInfo, no render adapter");
    }

    static void SkippedIsNotFailure()
    {
        var skipped = MetricsCalculator.Skipped("Dashboard", "d2d-hw", AdapterPolicy.NoHardwareAdapterReason);
        Expect(skipped.IsSkipped, "skipped");
        Expect(!skipped.GatePassed, "a skipped run does not count as a pass");
        Equal(0, skipped.Gate.Count(g => !g.Passed), "and has no failing gate checks");
        Equal(BackendMode.ImmediateD2DHardware, skipped.Identity.Requested, "requested");
        var back = MetricsJson.Deserialize(MetricsJson.Serialize(skipped));
        Equal(AdapterPolicy.NoHardwareAdapterReason, back.SkipReason, "reason round-trips");
        Expect(back.IsSkipped && !back.GatePassed, "flags round-trip");
        Expect(!MetricsCalculator.Compute(new SyntheticRun().Build()).IsSkipped, "executed runs are not skipped");
    }

    static void OlderMetricsLoad()
    {
        var node = JsonNode.Parse(MetricsJson.Serialize(MetricsCalculator.Compute(WithGpuPaintTimes())))!.AsObject();
        node.Remove("SkipReason");
        node.Remove("IsSkipped");
        foreach (var phase in node["Phases"]!.AsArray())
        {
            phase!.AsObject().Remove("GpuPaintMs");
            phase.AsObject().Remove("RenderAdapterGpuLocalMBMedian");
            phase.AsObject().Remove("RenderAdapterGpuNonLocalMBMedian");
        }
        var back = MetricsJson.Deserialize(node.ToJsonString());
        Expect(!back.IsSkipped, "not skipped");
        Expect(back.GatePassed, "gate");
        Equal(null, back.Phase(BenchPhase.Interaction)!.GpuPaintMs, "missing W3 is null");
    }

    static SyntheticRun Composition() => new()
    {
        HarnessRequested = "",
        Requested = BackendMode.Composition,
        Effective = BackendMode.Composition,
        DeviceMode = BackendMode.Composition,
        IsWarp = true,
    };

    static string[] Cells(string report, string rowName)
    {
        var line = report.Split('\n').FirstOrDefault(l => l.StartsWith($"| {rowName} |", StringComparison.Ordinal))
            ?? throw new TestFailure($"row '{rowName}' missing:\n{report}");
        return line.Trim().Trim('|').Split('|').Select(c => c.Trim()).ToArray();
    }

    // Columns: Metric, COMP, GDIP, D2D-HW, D2D-SW.
    const int D2DHardwareColumn = 3;

    static void ReportShowsNoGpu()
    {
        var comp = MetricsCalculator.Compute(Composition().Build());
        var skipped = MetricsCalculator.Skipped("Dashboard", "d2d-hw", AdapterPolicy.NoHardwareAdapterReason);
        string report = ComparisonReport.Build([("comp", comp), ("hw1", skipped), ("hw2", skipped)]);
        Expect(Cells(report, "Gate (passed/runs)")[D2DHardwareColumn].StartsWith("skipped: no hardware adapter", StringComparison.Ordinal),
            report);
        Equal("1/1", Cells(report, "Gate (passed/runs)")[1], "COMP gate");
        foreach (var row in new[] { "W1 UI frame p50, interaction (ms)", "W3 GPU paint p95, interaction (ms)", "M3 GPU local, steady (MB)" })
        {
            Equal("n/a (no GPU)", Cells(report, row)[D2DHardwareColumn], row);
        }
        Equal("-", Cells(report, "W1 UI frame p50, interaction (ms)")[2], "modes without runs stay '-'");
    }

    static void ReportShowsW3AndM3()
    {
        var records = WithGpuPaintTimes();
        records.Add(new GpuAdapterMemoryRecord(3000, SyntheticRun.Pid, BenchPhase.Steady, 0x1234, 64L * 1024 * 1024, 0));
        var hardware = MetricsCalculator.Compute(records);
        Expect(hardware.GatePassed, "synthetic D2D-HW run passes");
        var skipped = MetricsCalculator.Skipped("Dashboard", "d2d-hw", AdapterPolicy.NoHardwareAdapterReason);
        string report = ComparisonReport.Build([("hw", hardware), ("hw-skipped", skipped)]);
        Equal("1.450", Cells(report, "W3 GPU paint p50, interaction (ms)")[D2DHardwareColumn], "W3 p50");
        Equal("64.0", Cells(report, "M3 GPU local, render adapter, steady (MB)")[D2DHardwareColumn], "M3 render adapter");
        Expect(Cells(report, "Gate (passed/runs)")[D2DHardwareColumn].Contains("1 skipped (no hardware adapter)", StringComparison.Ordinal),
            "mixed executed/skipped gate cell");
    }

    static void GpuProfile()
    {
        var doc = XDocument.Load(Path.Combine(AppContext.BaseDirectory, "profiles", "ImmediateRendering.wprp"));
        var providers = doc.Descendants("EventProvider").ToDictionary(e => (string)e.Attribute("Id")!);
        var gpu = doc.Descendants("Profile").Single(p => (string?)p.Attribute("Name") == "Gpu");
        var used = gpu.Descendants("EventProviderId").Select(e => providers[(string)e.Attribute("Value")!]).ToList();

        static ulong Keywords(XElement provider) => (provider.Element("Keywords")?.Elements("Keyword") ?? [])
            .Select(k => Convert.ToUInt64((string)k.Attribute("Value")!, 16)).Aggregate(0ul, (a, b) => a | b);

        var dxgKrnl = used.Where(p => (string?)p.Attribute("Name") == "Microsoft-Windows-DxgKrnl").ToList();
        Equal(1, dxgKrnl.Count, "one DxgKrnl provider in Gpu");
        // Base (0x1) | Profiler (0x2, DMA packets / GPU utilization) | Memory (0x80, VidMm / GPU memory tables).
        Equal(0x83ul, Keywords(dxgKrnl[0]) & 0x83ul, "DxgKrnl keywords for GPU utilization and memory");
        Equal("5", (string?)dxgKrnl[0].Attribute("Level"), "DxgKrnl verbose");
        Expect(dxgKrnl[0].Element("CaptureStateOnStart") is not null, "DxgKrnl rundown at start (adapters, existing allocations)");

        var renderingBackend = used.Single(p => string.Equals((string?)p.Attribute("Name"),
            BackendModes.RenderingBackendProviderId.ToString(), StringComparison.OrdinalIgnoreCase));
        Equal(0x4ul, Keywords(renderingBackend) & 0x4ul, "RenderingBackend Paint keyword (GpuPaintTime)");
        Equal("5", (string?)renderingBackend.Attribute("Level"), "GpuPaintTime is verbose");

        foreach (var name in new[] { "Microsoft-Windows-DXGI", "Microsoft-Windows-Direct3D11" })
        {
            var provider = used.SingleOrDefault(p => (string?)p.Attribute("Name") == name);
            Expect(provider?.Element("CaptureStateOnStart") is not null, $"{name} with rundown (device/adapter identity in WPA)");
        }
    }
}
