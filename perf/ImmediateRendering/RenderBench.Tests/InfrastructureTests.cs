using System.Diagnostics.Tracing;
using System.Security.Principal;
using System.Xml.Linq;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using RenderBench.Analysis;
using RenderBench.Analyzer;
using static RenderBench.Tests.Check;

namespace RenderBench.Tests;

static class OptionsTests
{
    public static IEnumerable<TestCase> All() =>
    [
        new("Options.Defaults", () =>
        {
            Expect(BenchOptions.TryParse([], out var o, out _), "empty args parse");
            Equal(SceneId.Settings, o.Scene, "default scene");
            Equal(200, o.Tiles, "default tiles");
            Expect(o.Exit, "exits by default");
        }),
        new("Options.ParsesAll", () =>
        {
            Expect(BenchOptions.TryParse(["--scene", "dashboard", "--interaction", "12.5", "--seed", "7", "--tiles", "50",
                "--update-fraction", "0.25", "--screenshot", "a.bmp", "--no-exit"], out var o, out var error), error ?? "");
            Equal(SceneId.Dashboard, o.Scene, "scene");
            Near(12.5, o.InteractionSeconds, 1e-9, "interaction");
            Equal(7, o.Seed, "seed");
            Equal(50, o.Tiles, "tiles");
            Near(0.25, o.UpdateFraction, 1e-9, "fraction");
            Equal("a.bmp", o.ScreenshotPath, "screenshot");
            Expect(!o.Exit, "no-exit");
        }),
        new("Options.RejectsBadInput", () =>
        {
            Expect(!BenchOptions.TryParse(["--scene", "chart"], out _, out _), "unknown scene");
            Expect(!BenchOptions.TryParse(["--scene", "2"], out _, out _), "numeric scene out of range");
            Expect(!BenchOptions.TryParse(["--update-fraction", "1.5"], out _, out _), "fraction > 1");
            Expect(!BenchOptions.TryParse(["--update-hz", "0"], out _, out _), "zero update rate");
            Expect(!BenchOptions.TryParse(["--warmup"], out _, out var error), "missing value");
            Expect(error!.Contains("--warmup"), "error names the argument");
            Expect(!BenchOptions.TryParse(["--bogus", "1"], out _, out _), "unknown argument");
        }),
        new("BackendModes.ParseNamesAndLabels", () =>
        {
            (string Text, BackendMode Mode)[] cases =
            [
                ("composition", BackendMode.Composition), ("COMP", BackendMode.Composition),
                ("gdiplus", BackendMode.ImmediateGdiPlus), ("GDIP", BackendMode.ImmediateGdiPlus),
                ("d2d-hw", BackendMode.ImmediateD2DHardware), ("D2D-SW", BackendMode.ImmediateD2DSoftware),
            ];
            foreach (var (text, mode) in cases)
            {
                Expect(BackendModes.TryParse(text, out var parsed), $"parse {text}");
                Equal(mode, parsed, text);
            }
            Expect(!BackendModes.TryParse("gdi", out _), "plain GDI is not a backend");
            Expect(!BackendModes.TryParse("", out _), "empty is not parsed (caller maps empty to composition)");
        }),
        new("BackendModes.NativeValuesStable", () =>
        {
            // Must match RenderingBackend::Mode and ::FallbackReason in dxaml\xcp\components\base\inc\RenderingBackend.h.
            Equal(0u, (uint)BackendMode.Composition, "Composition");
            Equal(1u, (uint)BackendMode.ImmediateGdiPlus, "GdiPlus");
            Equal(2u, (uint)BackendMode.ImmediateD2DHardware, "D2D HW");
            Equal(3u, (uint)BackendMode.ImmediateD2DSoftware, "D2D SW");
            Equal(1u, (uint)FallbackReason.NotImplemented, "NotImplemented");
            Equal(4u, (uint)FallbackReason.UnsupportedContent, "UnsupportedContent");
        }),
        new("Providers.GuidsAreNameHashes", () =>
        {
            Equal(BenchEventSource.ProviderId, EventSource.GetGuid(typeof(BenchEventSource)), "harness provider GUID");
            Equal(BackendModes.RenderingBackendProviderId, EventSource.GetGuid(typeof(FakeRenderingBackendSource)),
                "native TraceLogging GUID must equal the name hash so WPR/analyzer can use either");
        }),
    ];
}

static class ProfileTests
{
    static readonly string s_profilePath = Path.Combine(AppContext.BaseDirectory, "profiles", "ImmediateRendering.wprp");

    public static IEnumerable<TestCase> All() =>
    [
        new("Wprp.ProfilesAndProviders", () =>
        {
            var doc = XDocument.Load(s_profilePath);
            var providers = doc.Descendants("EventProvider").ToDictionary(e => (string)e.Attribute("Id")!, e => (string)e.Attribute("Name")!);
            foreach (var name in new[] { "Timing", "Memory", "Gpu" })
            {
                var profile = doc.Descendants("Profile").SingleOrDefault(p => (string?)p.Attribute("Name") == name);
                NotNull(profile, $"profile {name}");
                Equal("File", (string?)profile!.Attribute("LoggingMode"), $"{name} logging mode");
                Equal($"{name}.Verbose.File", (string?)profile.Attribute("Id"), $"{name} id convention");
                var used = profile.Descendants("EventProviderId").Select(e => providers[(string)e.Attribute("Value")!]).ToList();
                Expect(used.Contains(BackendModes.RenderingBackendProviderId.ToString(), StringComparer.OrdinalIgnoreCase),
                    $"{name} enables the native provider");
                Expect(used.Contains(BenchEventSource.ProviderId.ToString(), StringComparer.OrdinalIgnoreCase),
                    $"{name} enables the harness provider");
                bool hasProfiler = used.Contains("a1b2c3d4-e5f6-4a5b-9c8d-7e6f5a4b3c2d", StringComparer.OrdinalIgnoreCase);
                Expect(hasProfiler == (name == "Memory"), $"{name}: XAML profiler provider only in Memory");
                Expect(profile.Descendants("SystemProviderId").Any(), $"{name} has kernel process events");
            }
        }),
        new("Wprp.StartStopSmoke", () =>
        {
            RequireAdmin();
            if (Environment.GetEnvironmentVariable("RENDERBENCH_WPR_SMOKE") != "1")
            {
                Skip("set RENDERBENCH_WPR_SMOKE=1 to start/stop WPR (cancels any running WPR session)");
            }
            string etl = Path.Combine(Path.GetTempPath(), $"renderbench-smoke-{Environment.ProcessId}.etl");
            try
            {
                foreach (var name in new[] { "Timing", "Memory", "Gpu" })
                {
                    Equal(0, Wpr($"-start \"{s_profilePath}\"!{name} -filemode"), $"wpr -start {name}");
                    BenchEventSource.Log.RunStarted("Smoke", "", 0, 1, 1, 1.0);
                    Equal(0, Wpr($"-stop \"{etl}\""), $"wpr -stop {name}");
                    var records = EtlReader.Read(etl);
                    Expect(records.OfType<RunStartedRecord>().Any(r => r.ProcessId == Environment.ProcessId),
                        $"{name}: harness event captured");
                }
            }
            finally
            {
                File.Delete(etl);
            }
        }),
    ];

    static int Wpr(string arguments)
    {
        using var process = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo("wpr.exe", arguments)
        {
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
        })!;
        process.WaitForExit();
        if (process.ExitCode != 0)
        {
            Console.WriteLine(process.StandardOutput.ReadToEnd() + process.StandardError.ReadToEnd());
        }
        return process.ExitCode;
    }

    public static void RequireAdmin()
    {
        if (!new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator))
        {
            Skip("requires an elevated process");
        }
    }
}

// Same name (so same GUID) and field names as the native TraceLogging provider; lets the ETL path be tested end to end
// without a WinUI build.
[EventSource(Name = BackendModes.RenderingBackendProvider)]
sealed class FakeRenderingBackendSource : EventSource
{
    public static readonly FakeRenderingBackendSource Log = new();
    FakeRenderingBackendSource() : base(EventSourceSettings.EtwSelfDescribingEventFormat) { }

    [Event(1)]
    public void BackendSelected(uint RequestedMode, uint EffectiveMode, uint FallbackReason, string RequestedName,
        string EffectiveName, string FallbackReasonName, uint AvailableModes)
        => WriteEvent(1, RequestedMode, EffectiveMode, FallbackReason, RequestedName, EffectiveName, FallbackReasonName, AvailableModes);

    [Event(2)]
    public void FrameBegin(uint FrameNumber, uint EffectiveMode) => WriteEvent(2, FrameNumber, EffectiveMode);

    [Event(3)]
    public void FrameEnd(uint FrameNumber, bool FrameDrawn) => WriteEvent(3, FrameNumber, FrameDrawn);

    [Event(4)]
    public void DeviceInfo(uint Mode, ulong AdapterLuid, bool IsWarp, uint FeatureLevel)
        => WriteEvent(4, Mode, AdapterLuid, IsWarp, FeatureLevel);

    [Event(5)]
    public void PaintEnd(uint FrameNumber, ulong RootId, ulong DamagedPixels, ulong PaintedPixels, uint PrimitiveCount,
        uint DamageRectCount)
        => WriteEvent(5, FrameNumber, RootId, DamagedPixels, PaintedPixels, PrimitiveCount, DamageRectCount);

    // ---- Appended by immediate-rendering-gdiplus: W6 ----
    [Event(16)]
    public void ResourceChurn(uint Mode, uint FrameNumber, uint Created, uint Destroyed)
        => WriteEvent(16, Mode, FrameNumber, Created, Destroyed);
}

static class EtlTests
{
    public static IEnumerable<TestCase> All() =>
    [
        new("Etl.RoundTripThroughRealSession", () =>
        {
            ProfileTests.RequireAdmin();
            string etl = Path.Combine(Path.GetTempPath(), $"renderbench-etl-{Environment.ProcessId}.etl");
            try
            {
                using (var session = new TraceEventSession($"RenderBenchTest-{Environment.ProcessId}", etl))
                {
                    session.EnableProvider(BenchEventSource.ProviderId, TraceEventLevel.Verbose);
                    session.EnableProvider(BackendModes.RenderingBackendProviderId, TraceEventLevel.Verbose);
                    Thread.Sleep(500);
                    BenchEventSource.Log.RunStarted("Settings", "d2d-sw", 3, 1280, 800, 1.5);
                    FakeRenderingBackendSource.Log.BackendSelected(3, 0, 1, "d2d-sw", "composition", "NotImplemented", 1);
                    FakeRenderingBackendSource.Log.DeviceInfo(0, 0xABCDEF, true, 0xb100);
                    BenchEventSource.Log.PhaseBegin((int)BenchPhase.Interaction, "Interaction");
                    FakeRenderingBackendSource.Log.FrameBegin(42, 0);
                    FakeRenderingBackendSource.Log.PaintEnd(42, 7, 1000, 1200, 33, 2);
                    FakeRenderingBackendSource.Log.FrameEnd(42, true);
                    BenchEventSource.Log.MemorySample((int)BenchPhase.Interaction, 100, 200, 300, 400, 500, 6, 7, 8.5, 9.5);
                    BenchEventSource.Log.PhaseEnd((int)BenchPhase.Interaction, "Interaction");
                    BenchEventSource.Log.RunCompleted(0, "ok");
                    Thread.Sleep(500);
                }

                var records = EtlReader.Read(etl).Where(r => r.ProcessId == Environment.ProcessId).ToList();
                var run = NotNull(records.OfType<RunStartedRecord>().FirstOrDefault(), "RunStarted");
                Equal("d2d-sw", run.RequestedBackend, "requested");
                Near(1.5, run.RasterizationScale, 1e-9, "scale");
                var selected = NotNull(records.OfType<BackendSelectedRecord>().FirstOrDefault(), "BackendSelected");
                Equal(BackendMode.ImmediateD2DSoftware, selected.Requested, "requested mode");
                Equal(FallbackReason.NotImplemented, selected.Reason, "reason");
                var device = NotNull(records.OfType<DeviceInfoRecord>().FirstOrDefault(), "DeviceInfo");
                Expect(device.IsWarp, "warp flag");
                Equal(0xABCDEFul, device.AdapterLuid, "luid");
                var end = NotNull(records.OfType<FrameEndRecord>().FirstOrDefault(), "FrameEnd");
                Equal(42u, end.Frame, "frame");
                Expect(end.Drawn, "drawn");
                var paint = NotNull(records.OfType<PaintEndRecord>().FirstOrDefault(), "PaintEnd");
                Equal(1200ul, paint.PaintedPixels, "painted");
                var sample = NotNull(records.OfType<MemorySampleRecord>().FirstOrDefault(), "MemorySample");
                Equal(BenchPhase.Interaction, sample.Phase, "phase");
                Equal(400L, sample.GpuLocalBytes, "gpu local");
                Near(9.5, sample.CpuKernelMs, 1e-9, "cpu kernel");
                Expect(records.OfType<PhaseRecord>().Count() == 2, "phase begin/end");
                Equal(0, NotNull(records.OfType<RunCompletedRecord>().FirstOrDefault(), "RunCompleted").Status, "status");

                var metrics = MetricsCalculator.Compute(records);
                Expect(!metrics.GatePassed, "fallback run fails the gate");
                Expect(metrics.Gate.Single(g => g.Name == GateNames.EffectiveMatchesRequested).Detail.Contains("NotImplemented"),
                    "gate names the fallback reason");
            }
            finally
            {
                File.Delete(etl);
            }
        }),
    ];
}
