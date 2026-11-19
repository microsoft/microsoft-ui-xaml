using System.Xml.Linq;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using RenderBench.Analysis;
using RenderBench.Analyzer;
using static RenderBench.Tests.Check;

namespace RenderBench.Tests;

// Measurement readiness for the GDI+ backend: W6 resource churn, M7 GDI object growth, and heap attribution in the
// Memory profile.
static class GdiPlusReadinessTests
{
    const long MB = 1024 * 1024;

    public static IEnumerable<TestCase> All() =>
    [
        new("GdiPlus.Metrics.ResourceChurnPerSecond", ResourceChurnPerSecond),
        new("GdiPlus.Metrics.NoChurnEventsIsNull", NoChurnEventsIsNull),
        new("GdiPlus.Metrics.GdiObjectGrowth", GdiObjectGrowth),
        new("GdiPlus.Metrics.JsonRoundTripKeepsChurnAndGrowth", JsonRoundTrip),
        new("GdiPlus.Report.ShowsChurnAndGdiRows", ReportRows),
        new("GdiPlus.Wprp.MemoryProfileTracesHeap", MemoryProfileTracesHeap),
        new("GdiPlus.Etl.ResourceChurnRoundTrip", EtlRoundTrip),
    ];

    // SyntheticRun's interaction phase is [61500, 71500): 10 s with 100 frames at 61550 + i * 100.
    static List<TraceRecord> WithChurn(SyntheticRun run, uint created, uint destroyed)
    {
        var records = run.Build();
        for (uint i = 0; i < 100; i++)
        {
            records.Add(new ResourceChurnRecord(61552 + i * 100, SyntheticRun.Pid, run.Effective, 1000 + i, created, destroyed));
        }
        // Another process's churn must not count.
        records.Add(new ResourceChurnRecord(62000, SyntheticRun.OtherPid, run.Effective, 1, 500, 500));
        return [.. records.OrderBy(r => r.TimeMs)];
    }

    static void ResourceChurnPerSecond()
    {
        var metrics = MetricsCalculator.Compute(WithChurn(new SyntheticRun(), created: 3, destroyed: 2));
        var interaction = NotNull(metrics.Phase(BenchPhase.Interaction), "interaction");
        Near(30, interaction.ResourcesCreatedPerSecond, 1e-9, "created/s = 300 over 10 s");
        Near(20, interaction.ResourcesDestroyedPerSecond, 1e-9, "destroyed/s = 200 over 10 s");
    }

    static void NoChurnEventsIsNull()
    {
        var metrics = MetricsCalculator.Compute(WithChurn(new SyntheticRun(), created: 0, destroyed: 0));
        var steady = NotNull(metrics.Phase(BenchPhase.Steady), "steady");
        Expect(steady.ResourcesCreatedPerSecond is null, "no ResourceChurn events in steady => null, not 0");
        Expect(steady.ResourcesDestroyedPerSecond is null, "destroyed null");
        Near(0, metrics.Phase(BenchPhase.Interaction)!.ResourcesCreatedPerSecond, 1e-9, "zero churn events => 0/s");
    }

    static void GdiObjectGrowth()
    {
        var records = new SyntheticRun().Build();
        records.Add(new MemorySampleRecord(70000, SyntheticRun.Pid, BenchPhase.Interaction, 120 * MB, 160 * MB, 170 * MB,
            45 * MB, 10 * MB, 40, 21, 3000, 900));
        var metrics = MetricsCalculator.Compute([.. records.OrderBy(r => r.TimeMs)]);
        var interaction = NotNull(metrics.Phase(BenchPhase.Interaction), "interaction");
        Equal((int?)9, interaction.GdiObjectsGrowth, "last minus first GDI sample (40 - 31)");
        Equal((int?)40, interaction.GdiObjectsMax, "max");
        Equal((int?)0, metrics.Phase(BenchPhase.Steady)!.GdiObjectsGrowth, "flat steady phase");
    }

    static void JsonRoundTrip()
    {
        var metrics = MetricsCalculator.Compute(WithChurn(new SyntheticRun(), created: 3, destroyed: 2));
        var back = MetricsJson.Deserialize(MetricsJson.Serialize(metrics));
        var interaction = NotNull(back.Phase(BenchPhase.Interaction), "interaction");
        Near(30, interaction.ResourcesCreatedPerSecond, 1e-9, "created/s");
        Near(20, interaction.ResourcesDestroyedPerSecond, 1e-9, "destroyed/s");
        Equal((int?)0, interaction.GdiObjectsGrowth, "growth");
        Expect(back.Phase(BenchPhase.Steady)!.ResourcesCreatedPerSecond is null, "null survives");
    }

    static void ReportRows()
    {
        var run = new SyntheticRun
        {
            HarnessRequested = "gdiplus",
            Requested = BackendMode.ImmediateGdiPlus,
            Effective = BackendMode.ImmediateGdiPlus,
            DeviceMode = BackendMode.ImmediateGdiPlus,
        };
        var metrics = MetricsCalculator.Compute(WithChurn(run, created: 3, destroyed: 2));
        string report = ComparisonReport.Build([("gdiplus-1", metrics)]);
        foreach (var row in new[]
        {
            "W6 resources created, interaction (/s)",
            "W6 resources destroyed, interaction (/s)",
            "W6 resources created, steady (/s)",
            "M7 GDI objects, steady",
            "M7 GDI objects, interaction (max)",
            "M7 GDI objects growth, interaction",
            "M7 USER objects, steady",
        })
        {
            Expect(report.Contains($"| {row} |"), $"report row '{row}'");
        }
        string createdLine = report.Split('\n').Single(l => l.StartsWith("| W6 resources created, interaction (/s) |"));
        Expect(createdLine.Contains("30.0"), $"created rate shown: {createdLine}");
    }

    static void MemoryProfileTracesHeap()
    {
        var doc = XDocument.Load(Path.Combine(AppContext.BaseDirectory, "profiles", "ImmediateRendering.wprp"));
        var memory = NotNull(doc.Descendants("Profile").SingleOrDefault(p => (string?)p.Attribute("Name") == "Memory"), "Memory");
        var collectorIds = memory.Descendants("HeapEventCollectorId").ToList();
        Expect(collectorIds.Count == 1, "Memory profile has a heap collector");
        var providerIds = collectorIds[0].Descendants("HeapEventProviderId").Select(e => (string)e.Attribute("Value")!).ToList();
        Expect(providerIds.Count == 1, "heap collector enables one heap provider");
        var provider = NotNull(doc.Descendants("HeapEventProvider").SingleOrDefault(p => (string?)p.Attribute("Id") == providerIds[0]),
            "heap provider definition");
        // 0 = processes opted in with "wpr -heaptracingconfig RenderBench.exe enable" (Run-RenderBench.ps1 does this).
        Expect(provider.Descendants("HeapProcessId").Any(p => (string?)p.Attribute("Value") == "0"), "heap provider traces opted-in processes");

        var systemIds = memory.Descendants("SystemProviderId").Select(e => (string)e.Attribute("Value")!).ToHashSet();
        var system = doc.Descendants("SystemProvider").Single(p => systemIds.Contains((string)p.Attribute("Id")!));
        var stacks = system.Descendants("Stack").Select(s => (string)s.Attribute("Value")!).ToHashSet();
        foreach (var stack in new[] { "HeapAllocation", "HeapReallocation", "VirtualAllocation" })
        {
            Expect(stacks.Contains(stack), $"Memory stacks include {stack}");
        }

        foreach (var other in new[] { "Timing", "Gpu" })
        {
            var profile = doc.Descendants("Profile").Single(p => (string?)p.Attribute("Name") == other);
            Expect(!profile.Descendants("HeapEventCollectorId").Any(), $"{other} stays free of heap tracing overhead");
        }
    }

    static void EtlRoundTrip()
    {
        ProfileTests.RequireAdmin();
        string etl = Path.Combine(Path.GetTempPath(), $"renderbench-churn-{Environment.ProcessId}.etl");
        try
        {
            using (var session = new TraceEventSession($"RenderBenchChurnTest-{Environment.ProcessId}", etl))
            {
                session.EnableProvider(BackendModes.RenderingBackendProviderId, TraceEventLevel.Verbose);
                Thread.Sleep(500);
                FakeRenderingBackendSource.Log.ResourceChurn((uint)BackendMode.ImmediateGdiPlus, 77, 5, 3);
                Thread.Sleep(500);
            }

            var records = EtlReader.Read(etl).Where(r => r.ProcessId == Environment.ProcessId).ToList();
            var churn = NotNull(records.OfType<ResourceChurnRecord>().FirstOrDefault(), "ResourceChurn");
            Equal(BackendMode.ImmediateGdiPlus, churn.Mode, "mode");
            Equal(77u, churn.Frame, "frame");
            Equal(5u, churn.Created, "created");
            Equal(3u, churn.Destroyed, "destroyed");
        }
        finally
        {
            File.Delete(etl);
        }
    }
}
