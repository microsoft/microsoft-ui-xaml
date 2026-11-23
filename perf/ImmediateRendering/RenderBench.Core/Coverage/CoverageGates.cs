using System.Globalization;
using System.Text.Json;

namespace RenderBench.Coverage;

// Files of one (page, mode) run under <results>\<family>\<name>\<mode>\.
public static class CoverageFiles
{
    public const string Catalog = "catalog.json";
    public const string App = "app.json";             // CoverageAppResult written by RenderBench --coverage
    public const string Stats = "stats.txt";          // WINUI_IMMEDIATE_STATS
    public const string Capture = "capture.bmp";      // window capture (PrintWindow of the window and its popups), every mode
    public const string Immediate = "immediate.bmp";  // WINUI_IMMEDIATE_CAPTURE (main island's presented buffer)
    public const string Exit = "exit.txt";            // process exit code, or "timeout"
    public const string Result = "result.json";       // CoverageRunResult written by the analyzer
    public const string Summary = "coverage.json";
    public const string Html = "coverage.html";
}

// What RenderBench --coverage reports about one page in one mode.
public sealed record CoverageAppResult
{
    public string PageId { get; init; } = "";
    public string Mode { get; init; } = "";
    public string Status { get; init; } = "ok";    // ok | error
    public string? Error { get; init; }
    public bool Loaded { get; init; }
    public bool ExerciseCompleted { get; init; }
    public int Width { get; init; }
    public int Height { get; init; }
    public double RasterizationScale { get; init; }
    public double LoadMs { get; init; }
    // Two consecutive captures matched before the deadline (pages with endless animations stay unstable).
    public bool Stable { get; init; }
    public int IdleMs { get; init; }
    public int PopupWindows { get; init; }
    public string CaptureMethod { get; init; } = "";
    public double PrivateMBIdle { get; init; }

    static readonly JsonSerializerOptions s_json = new() { WriteIndented = true };
    public string Serialize() => JsonSerializer.Serialize(this, s_json);
    public static CoverageAppResult? Deserialize(string json) => JsonSerializer.Deserialize<CoverageAppResult>(json);
}

public sealed record UnsupportedEntry(string ClassName, string Detail)
{
    public override string ToString() => ClassName.Length == 0 ? Detail : $"{ClassName} {Detail}";
}

// The lines of a WINUI_IMMEDIATE_STATS file that the gates need.
public sealed record CoverageStats(int Frames, List<UnsupportedEntry> Unsupported, List<string> Fallbacks, List<string> PaintFailures,
    int PopupFrames)
{
    public static CoverageStats Empty { get; } = new(0, [], [], [], 0);

    public static CoverageStats Parse(IEnumerable<string> lines)
    {
        int frames = 0, popups = 0;
        var unsupported = new List<UnsupportedEntry>();
        var fallbacks = new List<string>();
        var failures = new List<string>();
        foreach (var line in lines)
        {
            if (line.StartsWith("# unsupported ", StringComparison.Ordinal))
            {
                string rest = line["# unsupported ".Length..];
                int space = rest.IndexOf(' ');
                var entry = space < 0 ? new UnsupportedEntry("", rest.Trim()) : new UnsupportedEntry(rest[..space], rest[(space + 1)..].Trim());
                if (!unsupported.Contains(entry))
                {
                    unsupported.Add(entry);
                }
            }
            else if (line.StartsWith("# fallback ", StringComparison.Ordinal))
            {
                fallbacks.Add(line["# fallback ".Length..].Trim());
            }
            else if (line.StartsWith("# paint-failed", StringComparison.Ordinal))
            {
                failures.Add(line["# paint-failed".Length..].Trim());
            }
            else if (line.StartsWith("# popup-first-frame", StringComparison.Ordinal))
            {
                popups++;
            }
            else if (line.Length > 0 && char.IsAsciiDigit(line[0]))
            {
                frames++;
            }
        }
        return new CoverageStats(frames, unsupported, fallbacks, failures, popups);
    }
}

public sealed record CoverageGate(string Name, bool Passed, string Detail);

// Everything the gates look at for one (page, mode) run.
public sealed record CoverageRunInput
{
    public required CoveragePageInfo Page { get; init; }
    public required BackendMode Mode { get; init; }
    public int? ExitCode { get; init; }
    public bool TimedOut { get; init; }
    public CoverageAppResult? App { get; init; }
    public CoverageStats Stats { get; init; } = CoverageStats.Empty;
    public bool CaptureBlank { get; init; }
    // Mean absolute per-channel difference (0..1) against the Composition capture; null when either is missing.
    public double? Diff { get; init; }
}

public static class CoverageGates
{
    public const string Stability = "Stability", Unsupported = "Unsupported", Fallback = "Fallback", Fidelity = "Fidelity";

    // Test plan, "XAML coverage": in an immediate mode a page passes when it is stable, reports only the unsupported
    // details it declares, falls back exactly when it declares ExpectFallback, and is within MaxMeanAbsDiff of the
    // Composition rendering. Composition runs are the reference, so only stability applies to them.
    public static List<CoverageGate> Evaluate(CoverageRunInput run)
    {
        var gates = new List<CoverageGate> { EvaluateStability(run) };
        if (run.Mode == BackendMode.Composition)
        {
            return gates;
        }

        var unexpected = run.Stats.Unsupported.Where(u => !IsExpected(run.Page, u, run.Stats.Fallbacks)).ToList();
        gates.Add(new(Unsupported, unexpected.Count == 0, unexpected.Count == 0
            ? (run.Stats.Unsupported.Count == 0 ? "none" : "declared: " + string.Join("; ", run.Stats.Unsupported))
            : "unexpected: " + string.Join("; ", unexpected)));

        bool fellBack = run.Stats.Fallbacks.Count > 0;
        gates.Add(new(Fallback, fellBack == run.Page.ExpectFallback, fellBack
            ? $"fell back ({string.Join("; ", run.Stats.Fallbacks)}){(run.Page.ExpectFallback ? "" : " but the page doesn't declare ExpectFallback")}"
            : run.Page.ExpectFallback ? "expected a fallback, none reported" : "none"));

        gates.Add(run.Diff is { } diff
            ? new(Fidelity, diff <= run.Page.MaxMeanAbsDiff, string.Create(CultureInfo.InvariantCulture, $"{diff:0.0000} (max {run.Page.MaxMeanAbsDiff:0.####})"))
            : new(Fidelity, false, "no capture to compare (missing Composition reference or immediate capture)"));
        return gates;
    }

    static CoverageGate EvaluateStability(CoverageRunInput run)
    {
        var problems = new List<string>();
        if (run.TimedOut)
        {
            problems.Add("timed out");
        }
        else if (run.ExitCode is { } code && code != 0)
        {
            problems.Add($"exit code 0x{code:X8}");
        }
        if (run.App is null)
        {
            problems.Add("no app result (crash before the capture)");
        }
        else
        {
            if (run.App.Status != "ok")
            {
                problems.Add($"error: {run.App.Error}");
            }
            if (!run.App.Loaded)
            {
                problems.Add("page never loaded");
            }
            else if (!run.App.ExerciseCompleted)
            {
                problems.Add("exercise did not complete");
            }
        }
        if (run.Stats.PaintFailures.Count > 0)
        {
            problems.Add($"{run.Stats.PaintFailures.Count} failed paint(s): {run.Stats.PaintFailures[0]}");
        }
        if (run.Mode != BackendMode.Composition && run.App is not null && run.Stats.Frames == 0 && run.Stats.Fallbacks.Count == 0)
        {
            problems.Add("no immediate frames painted");
        }
        if (run.CaptureBlank)
        {
            problems.Add("capture is one uniform color");
        }
        return new(Stability, problems.Count == 0, problems.Count == 0 ? "ok" : string.Join("; ", problems));
    }

    // Approximations the design accepts for every page, so pages need not declare them; the fidelity gate still
    // catches any visible difference. AnimatedIcon (S12) shows its FallbackIconSource instead of the Lottie visual, and
    // MUXC templates use it for every chevron (DropDownButton, SplitButton, Expander, NavigationView, ...).
    public static readonly string[] GloballyExpected = ["AnimatedIcon:approximated"];

    // A declared entry matches an unsupported detail ("Mica:approximated"), "Class detail", or a glob over either.
    // On a page that expects a fallback, the content that caused it is expected too.
    public static bool IsExpected(CoveragePageInfo page, UnsupportedEntry entry, IReadOnlyList<string> fallbacks)
    {
        foreach (var declared in page.ExpectedUnsupported.Concat(GloballyExpected))
        {
            if (CoverageCatalog.GlobMatch(declared, entry.Detail) || CoverageCatalog.GlobMatch(declared, entry.ToString()))
            {
                return true;
            }
        }
        return page.ExpectFallback && fallbacks.Any(f => f.Contains(entry.Detail, StringComparison.OrdinalIgnoreCase));
    }
}
