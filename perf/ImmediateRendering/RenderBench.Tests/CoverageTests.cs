using System.IO.Compression;
using RenderBench.Coverage;
using static RenderBench.Tests.Check;

namespace RenderBench.Tests
{
    static class CoverageTests
    {
        static BgraImage Solid(int w, int h, byte b, byte g, byte r)
        {
            var p = new byte[w * h * 4];
            for (int i = 0; i < p.Length; i += 4)
            {
                p[i] = b;
                p[i + 1] = g;
                p[i + 2] = r;
                p[i + 3] = 255;
            }
            return new BgraImage(w, h, p);
        }

        static CoveragePageInfo Page(string id = "F8-controls/Button", string[]? unsupported = null, bool fallback = false, double max = 0.01)
        {
            var parts = id.Split('/');
            return new CoveragePageInfo(id, parts[0], parts[1], unsupported ?? [], fallback, max);
        }

        static CoverageAppResult OkApp => new() { Loaded = true, ExerciseCompleted = true };

        static CoverageStats Stats(params string[] lines) => CoverageStats.Parse(["frame island ...", "1 0000 1 1 1 0 1 0 0 0 0 0 1 1 1 0", .. lines]);

        static CoverageGate Gate(CoverageRunInput run, string name) => CoverageGates.Evaluate(run).Single(g => g.Name == name);

        public static IEnumerable<TestCase> All() =>
        [
            new("Coverage.DiscoversValidPagesSortedByFamilyNumber", () =>
            {
                var (pages, errors) = CoverageCatalog.Discover([typeof(Fake.ValidPages)]);
                Equal(0, errors.Count, "errors: " + string.Join("; ", errors));
                Equal("F8-controls/Button,F8-controls/CheckBox,F12-popups/Flyout", string.Join(",", pages.Select(p => p.Id)), "ids in family-number order");
                var flyout = pages.Single(p => p.Name == "Flyout");
                Equal("Acrylic:approximated", flyout.ExpectedUnsupported.Single(), "expected unsupported");
                Near(0.02, flyout.MaxMeanAbsDiff, 1e-12, "threshold");
                Expect(pages.Single(p => p.Name == "Button").MaxMeanAbsDiff == 0.01, "default threshold");
                Equal(12, flyout.FamilyNumber, "family number");
                Expect(flyout.Method is not null, "method kept for invocation");
            }),
            new("Coverage.ReportsMalformedPages", () =>
            {
                var (pages, errors) = CoverageCatalog.Discover([typeof(Fake.BadPages)]);
                Equal("F8-controls/Good", string.Join(",", pages.Select(p => p.Id)), "only the valid page survives");
                Expect(errors.Any(e => e.Contains("Instance") && e.Contains("static")), "instance method");
                Expect(errors.Any(e => e.Contains("WrongParameter") && e.Contains("expected")), "wrong signature");
                Expect(errors.Any(e => e.Contains("BadFamily") && e.Contains("family")), "bad family id");
                Expect(errors.Any(e => e.Contains("Duplicate") && e.Contains("duplicate")), "duplicate id");
                Expect(errors.Any(e => e.Contains("ZeroThreshold") && e.Contains("MaxMeanAbsDiff")), "zero threshold");
                Expect(errors.Any(e => e.Contains("SpaceName") && e.Contains("name")), "name with a space");
                Equal(6, errors.Count, "error count");
            }),
            new("Coverage.SelectsByIdFamilyOrNumberWithExclusions", () =>
            {
                var pages = new[] { Page("F8-controls/Button"), Page("F8-controls/Slider"), Page("F12-popups/Flyout"), Page("F14-theming/Rtl") };
                Equal(4, CoverageCatalog.Select(pages, null).Count, "null selects all");
                Equal("F8-controls/Button,F8-controls/Slider", string.Join(",", CoverageCatalog.Select(pages, "F8").Select(p => p.Id)), "family number");
                Equal(0, CoverageCatalog.Select(pages, "F1").Count, "F1 doesn't match F12 or F14");
                Equal("F12-popups/Flyout", string.Join(",", CoverageCatalog.Select(pages, "f12-POPUPS").Select(p => p.Id)), "family, case-insensitive");
                Equal("F8-controls/Slider,F14-theming/Rtl", string.Join(",", CoverageCatalog.Select(pages, "*/S*, *Rtl").Select(p => p.Id)), "id globs");
                Equal("F8-controls/Slider,F12-popups/Flyout,F14-theming/Rtl", string.Join(",", CoverageCatalog.Select(pages, "!*/Button").Select(p => p.Id)), "exclusion");
            }),
            new("Coverage.ParsesStatsLines", () =>
            {
                var stats = CoverageStats.Parse([
                    "frame island elements visited painted culled commands blits blitdx blitdy damagerects damagepx full walkus paintus reused",
                    "1 000001 10 10 9 1 12 0 0 0 0 1000 1 50 60 0",
                    "# unsupported Microsoft.UI.Xaml.Controls.Grid BackdropMaterial:approximated",
                    "# unsupported  RevealBrush",
                    "# unsupported Microsoft.UI.Xaml.Controls.Grid BackdropMaterial:approximated",
                    "2 000001 10 2 2 0 3 0 0 0 1 64 0 5 6 8",
                    "# popup-first-frame 0000 100x50 1200 us",
                    "# fallback Microsoft.UI.Xaml.Controls.SwapChainPanel SwapChainPanel",
                    "# paint-failed frame 3 hr 0x887A0005",
                ]);
                Equal(2, stats.Frames, "frames");
                Equal(2, stats.Unsupported.Count, "duplicates folded");
                Equal("BackdropMaterial:approximated", stats.Unsupported[0].Detail, "detail");
                Equal("Microsoft.UI.Xaml.Controls.Grid", stats.Unsupported[0].ClassName, "class");
                Equal("", stats.Unsupported[1].ClassName, "empty class name");
                Equal("RevealBrush", stats.Unsupported[1].Detail, "detail after empty class");
                Equal("Microsoft.UI.Xaml.Controls.SwapChainPanel SwapChainPanel", stats.Fallbacks.Single(), "fallback");
                Equal("frame 3 hr 0x887A0005", stats.PaintFailures.Single(), "paint failure");
                Equal(1, stats.PopupFrames, "popup first frames");
            }),
            new("Coverage.GatesPassCleanImmediateRun", () =>
            {
                var gates = CoverageGates.Evaluate(new() { Page = Page(), Mode = BackendMode.ImmediateGdiPlus, ExitCode = 0, App = OkApp, Stats = Stats(), Diff = 0.004 });
                Equal("Stability,Unsupported,Fallback,Fidelity", string.Join(",", gates.Select(g => g.Name)), "gate order");
                Expect(gates.All(g => g.Passed), string.Join("; ", gates.Where(g => !g.Passed).Select(g => g.Detail)));
            }),
            new("Coverage.CompositionIsTheReferenceAndOnlyGatesStability", () =>
            {
                var gates = CoverageGates.Evaluate(new() { Page = Page(), Mode = BackendMode.Composition, ExitCode = 0, App = OkApp });
                Equal(CoverageGates.Stability, gates.Single().Name, "only stability");
                Expect(gates[0].Passed, gates[0].Detail);
            }),
            new("Coverage.UnsupportedGateAllowsOnlyDeclaredDetails", () =>
            {
                var stats = Stats("# unsupported Microsoft.UI.Xaml.Controls.Grid Mica:approximated");
                var undeclared = Gate(new() { Page = Page(), Mode = BackendMode.ImmediateD2DSoftware, ExitCode = 0, App = OkApp, Stats = stats, Diff = 0 }, CoverageGates.Unsupported);
                Expect(!undeclared.Passed && undeclared.Detail.Contains("Mica:approximated"), undeclared.Detail);
                var declared = Gate(new() { Page = Page(unsupported: ["Mica:approximated"]), Mode = BackendMode.ImmediateD2DSoftware, ExitCode = 0, App = OkApp, Stats = stats, Diff = 0 }, CoverageGates.Unsupported);
                Expect(declared.Passed, declared.Detail);
                var glob = Gate(new() { Page = Page(unsupported: ["*:approximated"]), Mode = BackendMode.ImmediateD2DSoftware, ExitCode = 0, App = OkApp, Stats = stats, Diff = 0 }, CoverageGates.Unsupported);
                Expect(glob.Passed, "glob over the detail");
                var other = Gate(new() { Page = Page(unsupported: ["Acrylic:approximated"]), Mode = BackendMode.ImmediateD2DSoftware, ExitCode = 0, App = OkApp, Stats = stats, Diff = 0 }, CoverageGates.Unsupported);
                Expect(!other.Passed, "a different declared detail doesn't cover it");
            }),
            new("Coverage.FallbackGateMatchesDeclaration", () =>
            {
                var fell = Stats("# fallback SwapChainPanel interop");
                Expect(!Gate(new() { Page = Page(), Mode = BackendMode.ImmediateGdiPlus, ExitCode = 0, App = OkApp, Stats = fell, Diff = 0 }, CoverageGates.Fallback).Passed,
                    "undeclared fallback fails");
                Expect(Gate(new() { Page = Page(fallback: true), Mode = BackendMode.ImmediateGdiPlus, ExitCode = 0, App = OkApp, Stats = fell, Diff = 0 }, CoverageGates.Fallback).Passed,
                    "declared fallback passes");
                Expect(!Gate(new() { Page = Page(fallback: true), Mode = BackendMode.ImmediateGdiPlus, ExitCode = 0, App = OkApp, Stats = Stats(), Diff = 0 }, CoverageGates.Fallback).Passed,
                    "declared fallback that didn't happen fails");
                // The content that caused a declared fallback isn't an unexpected unsupported detail.
                var withDetail = Stats("# unsupported SwapChainPanel interop", "# fallback SwapChainPanel interop");
                Expect(Gate(new() { Page = Page(fallback: true), Mode = BackendMode.ImmediateGdiPlus, ExitCode = 0, App = OkApp, Stats = withDetail, Diff = 0 }, CoverageGates.Unsupported).Passed,
                    "fallback cause is expected");
            }),
            new("Coverage.FidelityGateUsesThePageThreshold", () =>
            {
                CoverageGate Fidelity(double? diff, double max) => Gate(new() { Page = Page(max: max), Mode = BackendMode.ImmediateGdiPlus, ExitCode = 0, App = OkApp, Stats = Stats(), Diff = diff }, CoverageGates.Fidelity);
                Expect(Fidelity(0.01, 0.01).Passed, "equal to the threshold passes");
                Expect(!Fidelity(0.0101, 0.01).Passed, "above the threshold fails");
                Expect(Fidelity(0.04, 0.05).Passed, "page-specific threshold");
                Expect(!Fidelity(null, 0.05).Passed, "missing comparison fails");
            }),
            new("Coverage.StabilityGateCatchesCrashesTimeoutsAndFailedPaints", () =>
            {
                CoverageGate Stability(CoverageRunInput run) => Gate(run, CoverageGates.Stability);
                var ok = new CoverageRunInput { Page = Page(), Mode = BackendMode.ImmediateGdiPlus, ExitCode = 0, App = OkApp, Stats = Stats(), Diff = 0 };
                Expect(Stability(ok).Passed, Stability(ok).Detail);
                Expect(!Stability(ok with { App = null, ExitCode = unchecked((int)0xC0000409) }).Passed, "crash");
                Expect(Stability(ok with { App = null, ExitCode = unchecked((int)0xC0000409) }).Detail.Contains("C0000409"), "exit code in detail");
                Expect(!Stability(ok with { TimedOut = true, ExitCode = null }).Passed, "timeout");
                Expect(!Stability(ok with { App = OkApp with { ExerciseCompleted = false } }).Passed, "exercise incomplete");
                Expect(!Stability(ok with { App = OkApp with { Status = "error", Error = "boom" } }).Passed, "page error");
                Expect(!Stability(ok with { Stats = Stats("# paint-failed frame 2 hr 0x80004005") }).Passed, "failed paint");
                Expect(!Stability(ok with { Stats = CoverageStats.Empty }).Passed, "immediate mode painted nothing");
                Expect(!Stability(ok with { CaptureBlank = true }).Passed, "blank capture");
            }),
            new("Coverage.MeanAbsDiffIsNormalizedPerChannel", () =>
            {
                var white = Solid(10, 10, 255, 255, 255);
                Near(0, CoverageImaging.MeanAbsDiff(white, white), 1e-12, "identical");
                Near(1, CoverageImaging.MeanAbsDiff(white, Solid(10, 10, 0, 0, 0)), 1e-12, "black vs white");
                Near(51 / 255.0 / 3, CoverageImaging.MeanAbsDiff(white, Solid(10, 10, 255, 255, 204)), 1e-12, "one channel off by 51");
                var half = Solid(10, 10, 255, 255, 255);
                for (int i = 0; i < 50 * 4; i += 4)
                {
                    half.Pixels[i] = half.Pixels[i + 1] = half.Pixels[i + 2] = 0;
                }
                Near(0.5, CoverageImaging.MeanAbsDiff(white, half), 1e-12, "half the pixels inverted");
                Near(0.5, CoverageImaging.DifferingPixelFraction(white, half), 1e-12, "differing fraction");
                var alphaOnly = Solid(10, 10, 255, 255, 255);
                alphaOnly.Pixels[3] = 0;
                Near(0, CoverageImaging.MeanAbsDiff(white, alphaOnly), 1e-12, "alpha ignored");
            }),
            new("Coverage.SizeMismatchCountsAsDifferent", () =>
            {
                var a = Solid(10, 10, 0, 0, 0);
                var b = Solid(10, 5, 0, 0, 0);
                Near(0.5, CoverageImaging.MeanAbsDiff(a, b), 1e-12, "missing half counts as fully different");
                var diff = CoverageImaging.DiffImage(a, b);
                Equal((10, 10), (diff.Width, diff.Height), "diff image covers the union");
                int outside = (7 * 10 + 3) * 4;
                Equal((255, 0, 255), (diff.Pixels[outside], diff.Pixels[outside + 1], diff.Pixels[outside + 2]), "magenta outside the overlap");
                Equal((0, 0, 0), (diff.Pixels[0], diff.Pixels[1], diff.Pixels[2]), "equal black stays black");
            }),
            new("Coverage.DiffImageHighlightsChangedPixels", () =>
            {
                var a = Solid(4, 4, 200, 200, 200);
                var b = Solid(4, 4, 200, 200, 200);
                b.Pixels[5 * 4 + 2] = 100;
                var diff = CoverageImaging.DiffImage(a, b);
                Expect(diff.Pixels[5 * 4 + 2] == 255 && diff.Pixels[5 * 4] < 100, "changed pixel is red");
                Equal(50, (int)diff.Pixels[0], "unchanged pixel is the dimmed reference");
            }),
            new("Coverage.PngEncodesDecodableRgb", () =>
            {
                var image = Solid(3, 2, 10, 20, 30);
                byte[] png = PngCodec.Encode(image);
                Expect(png.AsSpan(0, 8).SequenceEqual(new byte[] { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A }), "signature");
                Equal(3, System.Buffers.Binary.BinaryPrimitives.ReadInt32BigEndian(png.AsSpan(16)), "IHDR width");
                Equal(2, System.Buffers.Binary.BinaryPrimitives.ReadInt32BigEndian(png.AsSpan(20)), "IHDR height");
                int idatLength = System.Buffers.Binary.BinaryPrimitives.ReadInt32BigEndian(png.AsSpan(33));
                Equal("IDAT", System.Text.Encoding.ASCII.GetString(png, 37, 4), "IDAT follows IHDR");
                using var z = new ZLibStream(new MemoryStream(png, 41, idatLength), CompressionMode.Decompress);
                using var raw = new MemoryStream();
                z.CopyTo(raw);
                Equal("0,30,20,10,30,20,10,30,20,10", string.Join(",", raw.ToArray().Take(10)), "filter byte then RGB");
                Equal(2 * (1 + 3 * 3), (int)raw.Length, "row count and stride");
            }),
            new("Coverage.SummaryCountsPagesPassingInEveryImmediateMode", () =>
            {
                var pages = new List<CoveragePageInfo> { Page("F8-controls/A"), Page("F8-controls/B"), Page("F11-interop/C", fallback: true), Page("F8-controls/NotRun") };
                CoverageRunResult R(string id, string mode, bool passed) => new() { PageId = id, Family = id.Split('/')[0], Mode = mode, Passed = passed };
                var runs = new List<CoverageRunResult>
                {
                    R("F8-controls/A", "composition", true), R("F8-controls/A", "gdiplus", true), R("F8-controls/A", "d2d-sw", true),
                    R("F8-controls/B", "composition", true), R("F8-controls/B", "gdiplus", true), R("F8-controls/B", "d2d-sw", false),
                    R("F11-interop/C", "composition", true), R("F11-interop/C", "gdiplus", true), R("F11-interop/C", "d2d-sw", true),
                };
                var s = CoverageReport.Summarize(pages, runs, ["composition", "gdiplus", "d2d-sw"], []);
                Equal(new CoverageRate(1, 2), s.Overall, "non-interop: A passes in both, B fails D2D-SW; NotRun is ignored");
                Equal(new CoverageRate(1, 1), s.Interop, "interop");
                Equal(new CoverageRate(2, 3), s.PerMode["d2d-sw"], "per mode");
                Equal(new CoverageRate(1, 2), s.PerFamily["F8-controls"]["d2d-sw"], "per family");
                Near(50, s.Overall.Percent, 1e-9, "percent");
            }),
            new("Coverage.BuildGatesAResultsTree", () =>
            {
                string dir = Path.Combine(Path.GetTempPath(), "rb-coverage-" + Guid.NewGuid().ToString("N"));
                try
                {
                    var pages = new[] { Page("F8-controls/Good"), Page("F8-controls/Bad") };
                    Directory.CreateDirectory(dir);
                    File.WriteAllText(Path.Combine(dir, CoverageFiles.Catalog), CoverageCatalog.Serialize(pages));
                    void Run(CoveragePageInfo page, string mode, BgraImage capture, string[] stats)
                    {
                        string runDir = Path.Combine(dir, CoverageCatalog.RelativeDirectory(page), mode);
                        Directory.CreateDirectory(runDir);
                        File.WriteAllBytes(Path.Combine(runDir, CoverageFiles.Capture), BmpCodec.Encode(capture.Width, capture.Height, capture.Pixels));
                        File.WriteAllText(Path.Combine(runDir, CoverageFiles.App), (OkApp with { PageId = page.Id, Mode = mode, PrivateMBIdle = 42 }).Serialize());
                        File.WriteAllText(Path.Combine(runDir, CoverageFiles.Exit), "0");
                        if (mode != "composition")
                        {
                            File.WriteAllLines(Path.Combine(runDir, CoverageFiles.Stats), ["header", "1 0 1 1 1 0 1 0 0 0 0 0 1 1 1 0", .. stats]);
                        }
                    }
                    var reference = Solid(20, 10, 255, 255, 255);
                    reference.Pixels[0] = 0; // not uniform
                    Run(pages[0], "composition", reference, []);
                    Run(pages[0], "gdiplus", reference, []);
                    Run(pages[1], "composition", reference, []);
                    Run(pages[1], "gdiplus", Solid(20, 10, 0, 0, 0), ["# unsupported X Reveal"]);
                    var summary = CoverageReport.Build(dir);
                    Equal(4, summary.Runs.Count, "runs");
                    var good = summary.Runs.Single(r => r.PageId == "F8-controls/Good" && r.Mode == "gdiplus");
                    Expect(good.Passed, string.Join("; ", good.Gates.Where(g => !g.Passed).Select(g => g.Detail)));
                    Near(0, good.Diff, 1e-12, "identical capture");
                    var bad = summary.Runs.Single(r => r.PageId == "F8-controls/Bad" && r.Mode == "gdiplus");
                    Expect(!bad.Passed, "bad page fails");
                    Expect(!bad.Gates.Single(g => g.Name == CoverageGates.Unsupported).Passed && !bad.Gates.Single(g => g.Name == CoverageGates.Fidelity).Passed,
                        "unsupported and fidelity both fail");
                    Expect(!bad.Gates.Single(g => g.Name == CoverageGates.Stability).Passed, "uniform black capture is a stability failure");
                    Equal(new CoverageRate(1, 2), summary.Overall, "overall");
                    Expect(File.Exists(Path.Combine(dir, CoverageFiles.Html)) && File.Exists(Path.Combine(dir, CoverageFiles.Summary)), "aggregate files");
                    Expect(File.Exists(Path.Combine(dir, "F8-controls", "Bad", "gdiplus", "diff.png")), "diff image written");
                    Expect(File.Exists(Path.Combine(dir, "F8-controls", "Bad", "gdiplus", CoverageFiles.Result)), "per-run result");
                }
                finally
                {
                    Directory.Delete(dir, recursive: true);
                }
            }),
            new("Coverage.OptionsSelectAPageAndDefaultTheWindowSize", () =>
            {
                Expect(BenchOptions.TryParse(["--coverage", "F8-controls/Button", "--coverage-out", "x"], out var o, out var error), error ?? "");
                Equal("F8-controls/Button", o.CoveragePage, "page");
                Equal((BenchOptions.CoverageWidth, BenchOptions.CoverageHeight), (o.Width, o.Height), "coverage window size");
                Expect(BenchOptions.TryParse(["--coverage", "F8-controls/Button", "--width", "800"], out var sized, out _) && sized.Width == 800, "explicit width wins");
                Expect(!BenchOptions.TryParse(["--coverage", "F8-controls/Button", "--scene", "form"], out _, out _), "exclusive with --scene");
                Expect(!BenchOptions.TryParse(["--coverage-out", "x"], out _, out _), "--coverage-out needs --coverage");
                Expect(BenchOptions.TryParse([], out var plain, out _) && plain.Width == 1280, "scene default size unchanged");
            }),
        ];
    }
}

// Stand-ins for the WinUI app's contract types (RenderBench\Coverage\CoverageContract.cs); discovery matches by name.
namespace RenderBench.Tests.Fake
{
    [AttributeUsage(AttributeTargets.Method)]
    sealed class CoveragePageAttribute(string family, string name) : Attribute
    {
        public string Family { get; } = family;
        public string Name { get; } = name;
        public string[] ExpectedUnsupported { get; init; } = [];
        public bool ExpectFallback { get; init; }
        public double MaxMeanAbsDiff { get; init; } = 0.01;
    }

    sealed class CoverageContext;

    sealed class CoveragePage;

    static class ValidPages
    {
        [CoveragePage("F12-popups", "Flyout", ExpectedUnsupported = ["Acrylic:approximated"], MaxMeanAbsDiff = 0.02)]
        public static CoveragePage Flyout(CoverageContext context) => new();

        [CoveragePage("F8-controls", "CheckBox")]
        static CoveragePage CheckBox(CoverageContext context) => new();

        [CoveragePage("F8-controls", "Button")]
        public static CoveragePage Button(CoverageContext context) => new();

        public static CoveragePage NotAPage(CoverageContext context) => new();
    }

    sealed class BadPages
    {
        [CoveragePage("F8-controls", "Good")]
        public static CoveragePage Good(CoverageContext context) => new();

        [CoveragePage("F8-controls", "Instance")]
        public CoveragePage Instance(CoverageContext context) => new();

        [CoveragePage("F8-controls", "WrongParameter")]
        public static CoveragePage WrongParameter(string context) => new();

        [CoveragePage("controls", "BadFamily")]
        public static CoveragePage BadFamily(CoverageContext context) => new();

        [CoveragePage("F8-controls", "Good")]
        public static CoveragePage Duplicate(CoverageContext context) => new();

        [CoveragePage("F8-controls", "ZeroThreshold", MaxMeanAbsDiff = 0)]
        public static CoveragePage ZeroThreshold(CoverageContext context) => new();

        [CoveragePage("F8-controls", "Space Name")]
        public static CoveragePage SpaceName(CoverageContext context) => new();
    }
}
