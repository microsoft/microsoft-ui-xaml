using System.Globalization;
using System.Net;
using System.Text;
using System.Text.Json;

namespace RenderBench.Coverage;

// Gate results and artifacts of one (page, mode) run.
public sealed record CoverageRunResult
{
    public string PageId { get; init; } = "";
    public string Family { get; init; } = "";
    public string Name { get; init; } = "";
    public string Mode { get; init; } = "";
    public bool Interop { get; init; }
    public bool Passed { get; init; }
    public List<CoverageGate> Gates { get; init; } = [];
    public double? Diff { get; init; }
    public double? DifferingPixels { get; init; }
    public double MaxMeanAbsDiff { get; init; }
    // Immediate modes: WINUI_IMMEDIATE_CAPTURE vs the window capture of the same run (validates the capture method).
    public double? CaptureCrossCheckDiff { get; init; }
    public string ImageSource { get; init; } = "";
    public List<string> Unsupported { get; init; } = [];
    public List<string> Fallbacks { get; init; } = [];
    public int Frames { get; init; }
    public double? PrivateMBIdle { get; init; }
    public bool? Stable { get; init; }
    public string? ExitCode { get; init; }
    public string? CapturePng { get; init; }
    public string? DiffPng { get; init; }
    public string? ThumbPng { get; init; }
    public string? DiffThumbPng { get; init; }
}

public sealed record CoverageRate(int Passed, int Total)
{
    public double Percent => Total == 0 ? 0 : 100.0 * Passed / Total;
    public override string ToString() => Total == 0 ? "n/a" : string.Create(CultureInfo.InvariantCulture, $"{Passed}/{Total} ({Percent:0.#}%)");
}

public sealed record CoverageSummary
{
    public List<string> Modes { get; init; } = [];
    public int Pages { get; init; }
    // Non-interop pages that pass in every immediate mode that ran (test plan target: >= 95%).
    public CoverageRate Overall { get; init; } = new(0, 0);
    public double TargetPercent { get; init; } = 95;
    // F11 / ExpectFallback pages that fall back and then match Comp (target 100%).
    public CoverageRate Interop { get; init; } = new(0, 0);
    public Dictionary<string, CoverageRate> PerMode { get; init; } = [];
    public Dictionary<string, Dictionary<string, CoverageRate>> PerFamily { get; init; } = [];
    public List<string> CatalogErrors { get; init; } = [];
    public List<CoverageRunResult> Runs { get; init; } = [];
}

public static class CoverageReport
{
    static readonly JsonSerializerOptions s_json = new() { WriteIndented = true };

    // Evaluates every run under `resultsDir` (layout: catalog.json + <family>\<name>\<mode>\...), writes result.json
    // and PNGs per run, and coverage.json + coverage.html at the root.
    public static CoverageSummary Build(string resultsDir)
    {
        var pages = CoverageCatalog.Deserialize(File.ReadAllText(Path.Combine(resultsDir, CoverageFiles.Catalog)));
        string errorsPath = Path.Combine(resultsDir, "catalog-errors.txt");
        var catalogErrors = File.Exists(errorsPath) ? File.ReadAllLines(errorsPath).Where(l => l.Length > 0).ToList() : [];
        var runs = new List<CoverageRunResult>();
        var modesSeen = new HashSet<BackendMode>();
        foreach (var page in pages)
        {
            string pageDir = Path.Combine(resultsDir, CoverageCatalog.RelativeDirectory(page));
            if (!Directory.Exists(pageDir))
            {
                continue;
            }
            var modes = Directory.EnumerateDirectories(pageDir)
                .Select(d => BackendModes.TryParse(Path.GetFileName(d), out var m) && BackendModes.Name(m) == Path.GetFileName(d) ? (BackendMode?)m : null)
                .OfType<BackendMode>().OrderBy(m => m).ToList();
            BgraImage? reference = null;
            foreach (var mode in modes)
            {
                modesSeen.Add(mode);
                var run = Evaluate(page, mode, Path.Combine(pageDir, BackendModes.Name(mode)), ref reference, resultsDir);
                runs.Add(run);
            }
        }

        var modeNames = modesSeen.OrderBy(m => m).Select(BackendModes.Name).ToList();
        var summary = Summarize(pages, runs, modeNames, catalogErrors);
        File.WriteAllText(Path.Combine(resultsDir, CoverageFiles.Summary), JsonSerializer.Serialize(summary, s_json));
        File.WriteAllText(Path.Combine(resultsDir, CoverageFiles.Html), Html(summary));
        return summary;
    }

    static CoverageRunResult Evaluate(CoveragePageInfo page, BackendMode mode, string dir, ref BgraImage? reference, string root)
    {
        CoverageAppResult? app = null;
        string appPath = Path.Combine(dir, CoverageFiles.App);
        if (File.Exists(appPath))
        {
            try { app = CoverageAppResult.Deserialize(File.ReadAllText(appPath)); } catch (JsonException) { }
        }
        string statsPath = Path.Combine(dir, CoverageFiles.Stats);
        var stats = File.Exists(statsPath) ? CoverageStats.Parse(ReadShared(statsPath)) : CoverageStats.Empty;
        string exitText = File.Exists(Path.Combine(dir, CoverageFiles.Exit)) ? File.ReadAllText(Path.Combine(dir, CoverageFiles.Exit)).Trim() : "";
        bool timedOut = exitText == "timeout";
        int? exitCode = int.TryParse(exitText, NumberStyles.Integer, CultureInfo.InvariantCulture, out int c) ? c : null;

        var capture = LoadBmp(Path.Combine(dir, CoverageFiles.Capture));
        var immediate = mode == BackendMode.Composition ? null : LoadBmp(Path.Combine(dir, CoverageFiles.Immediate));
        string source = "window capture";
        var image = capture;
        if ((image is null || CoverageImaging.IsUniform(image)) && immediate is not null)
        {
            image = immediate;
            source = "WINUI_IMMEDIATE_CAPTURE (window capture missing or blank)";
        }
        if (mode == BackendMode.Composition)
        {
            reference = image;
        }
        double? diff = null, differing = null, crossCheck = null;
        string? diffPng = null, diffThumb = null;
        if (mode != BackendMode.Composition && image is not null && reference is not null)
        {
            diff = CoverageImaging.MeanAbsDiff(reference, image);
            differing = CoverageImaging.DifferingPixelFraction(reference, image);
            var diffImage = CoverageImaging.DiffImage(reference, image);
            diffPng = Save(Path.Combine(dir, "diff.png"), diffImage, root);
            diffThumb = Save(Path.Combine(dir, "diff.thumb.png"), CoverageImaging.Thumbnail(diffImage, 240), root);
        }
        if (capture is not null && immediate is not null)
        {
            crossCheck = CoverageImaging.MeanAbsDiff(capture, immediate);
        }

        var input = new CoverageRunInput
        {
            Page = page, Mode = mode, ExitCode = exitCode, TimedOut = timedOut, App = app, Stats = stats,
            CaptureBlank = image is not null && CoverageImaging.IsUniform(image), Diff = diff,
        };
        var gates = CoverageGates.Evaluate(input);
        if (image is null)
        {
            gates[0] = gates[0] with { Passed = false, Detail = gates[0].Passed ? "no capture" : gates[0].Detail + "; no capture" };
        }
        var result = new CoverageRunResult
        {
            PageId = page.Id, Family = page.Family, Name = page.Name, Mode = BackendModes.Name(mode), Interop = page.IsInterop,
            Passed = gates.All(g => g.Passed), Gates = gates, Diff = diff, DifferingPixels = differing, MaxMeanAbsDiff = page.MaxMeanAbsDiff,
            CaptureCrossCheckDiff = crossCheck, ImageSource = image is null ? "none" : source,
            Unsupported = stats.Unsupported.Select(u => u.ToString()).ToList(), Fallbacks = stats.Fallbacks, Frames = stats.Frames,
            PrivateMBIdle = app?.PrivateMBIdle, Stable = app?.Stable, ExitCode = exitText.Length == 0 ? null : exitText,
            CapturePng = image is null ? null : Save(Path.Combine(dir, "capture.png"), image, root),
            ThumbPng = image is null ? null : Save(Path.Combine(dir, "capture.thumb.png"), CoverageImaging.Thumbnail(image, 240), root),
            DiffPng = diffPng, DiffThumbPng = diffThumb,
        };
        File.WriteAllText(Path.Combine(dir, CoverageFiles.Result), JsonSerializer.Serialize(result, s_json));
        return result;
    }

    public static CoverageSummary Summarize(IReadOnlyList<CoveragePageInfo> pages, IReadOnlyList<CoverageRunResult> runs,
        IReadOnlyList<string> modes, List<string> catalogErrors)
    {
        var immediateModes = modes.Where(m => m != BackendModes.Name(BackendMode.Composition)).ToList();
        var byPage = runs.GroupBy(r => r.PageId).ToDictionary(g => g.Key, g => g.ToList());
        bool PassesAll(CoveragePageInfo page) => byPage.TryGetValue(page.Id, out var list) &&
            immediateModes.All(m => list.Any(r => r.Mode == m && r.Passed));
        var ran = pages.Where(p => byPage.ContainsKey(p.Id)).ToList();
        var nonInterop = ran.Where(p => !p.IsInterop).ToList();
        var interop = ran.Where(p => p.IsInterop).ToList();

        var perMode = new Dictionary<string, CoverageRate>();
        foreach (var mode in modes)
        {
            var modeRuns = runs.Where(r => r.Mode == mode).ToList();
            perMode[mode] = new(modeRuns.Count(r => r.Passed), modeRuns.Count);
        }
        var perFamily = new Dictionary<string, Dictionary<string, CoverageRate>>();
        foreach (var family in ran.Select(p => p.Family).Distinct())
        {
            perFamily[family] = modes.ToDictionary(m => m, m =>
            {
                var familyRuns = runs.Where(r => r.Family == family && r.Mode == m).ToList();
                return new CoverageRate(familyRuns.Count(r => r.Passed), familyRuns.Count);
            });
        }
        return new CoverageSummary
        {
            Modes = modes.ToList(), Pages = ran.Count, Overall = new(nonInterop.Count(PassesAll), nonInterop.Count),
            Interop = new(interop.Count(PassesAll), interop.Count), PerMode = perMode, PerFamily = perFamily,
            CatalogErrors = catalogErrors, Runs = runs.ToList(),
        };
    }

    static string Html(CoverageSummary s)
    {
        var html = new StringBuilder();
        string E(string? text) => WebUtility.HtmlEncode(text ?? "");
        string Img(string? path, string alt) => path is null ? "<div class='none'>none</div>" :
            $"<a href='{E(Path.ChangeExtension(path, null).Replace(".thumb", "") + ".png")}'><img src='{E(path)}' alt='{E(alt)}' loading='lazy'></a>";
        html.Append("""
            <!DOCTYPE html><html><head><meta charset="utf-8"><title>XAML coverage</title><style>
            body{font-family:Segoe UI,sans-serif;margin:16px;font-size:13px}table{border-collapse:collapse}
            td,th{border:1px solid #ccc;padding:4px;vertical-align:top}th{background:#f3f3f3}
            .pass{background:#e6f4ea}.fail{background:#fce8e6}.ref{background:#eef3fb}img{width:240px;display:block}
            .gate{font-size:11px;color:#444;max-width:240px;word-wrap:break-word}.none{color:#999}.big{font-size:20px}
            </style></head><body><h1>XAML coverage (immediate backends vs Composition)</h1>
            """);
        bool met = s.Overall.Percent >= s.TargetPercent;
        html.Append($"<p class='big'>Non-interop pages passing in every immediate mode: <b>{E(s.Overall.ToString())}</b> " +
            $"(target ≥ {s.TargetPercent}%: <b style='color:{(met ? "green" : "red")}'>{(met ? "met" : "not met")}</b>). " +
            $"Interop pages: {E(s.Interop.ToString())}.</p>");
        if (s.CatalogErrors.Count > 0)
        {
            html.Append("<p style='color:red'>Catalog errors:<br>").AppendJoin("<br>", s.CatalogErrors.Select(E)).Append("</p>");
        }

        html.Append("<h2>Pass rates</h2><table><tr><th>Family</th>").AppendJoin("", s.Modes.Select(m => $"<th>{E(m)}</th>")).Append("</tr>");
        foreach (var (family, rates) in s.PerFamily)
        {
            html.Append($"<tr><td>{E(family)}</td>").AppendJoin("", s.Modes.Select(m => $"<td>{E(rates[m].ToString())}</td>")).Append("</tr>");
        }
        html.Append("<tr><th>All runs</th>").AppendJoin("", s.Modes.Select(m => $"<th>{E(s.PerMode[m].ToString())}</th>")).Append("</tr></table>");

        html.Append("<h2>Matrix</h2><table><tr><th>Page</th>").AppendJoin("", s.Modes.Select(m => $"<th>{E(m)}</th>")).Append("</tr>");
        foreach (var page in s.Runs.GroupBy(r => r.PageId))
        {
            html.Append($"<tr><td><b>{E(page.Key)}</b>{(page.First().Interop ? "<br>(interop)" : "")}</td>");
            foreach (var mode in s.Modes)
            {
                var run = page.FirstOrDefault(r => r.Mode == mode);
                if (run is null)
                {
                    html.Append("<td class='none'>not run</td>");
                    continue;
                }
                bool isReference = mode == BackendModes.Name(BackendMode.Composition);
                html.Append($"<td class='{(isReference && run.Passed ? "ref" : run.Passed ? "pass" : "fail")}'>");
                html.Append($"<b>{(run.Passed ? "PASS" : "FAIL")}</b>");
                if (run.Diff is { } d)
                {
                    html.Append(string.Create(CultureInfo.InvariantCulture, $" diff {d:0.0000} / {run.MaxMeanAbsDiff:0.###}"));
                }
                if (run.PrivateMBIdle is { } mb)
                {
                    html.Append(string.Create(CultureInfo.InvariantCulture, $" · {mb:0.0} MB"));
                }
                html.Append(Img(run.ThumbPng, $"{run.PageId} {mode}"));
                if (!isReference)
                {
                    html.Append(Img(run.DiffThumbPng, $"{run.PageId} {mode} diff"));
                }
                foreach (var gate in run.Gates.Where(g => !g.Passed))
                {
                    html.Append($"<div class='gate'>✗ {E(gate.Name)}: {E(gate.Detail)}</div>");
                }
                if (run.Unsupported.Count > 0 && run.Gates.All(g => g.Name != CoverageGates.Unsupported || g.Passed))
                {
                    html.Append($"<div class='gate'>declared: {E(string.Join("; ", run.Unsupported))}</div>");
                }
                html.Append("</td>");
            }
            html.Append("</tr>");
        }
        html.Append("</table></body></html>");
        return html.ToString();
    }

    static string Save(string path, BgraImage image, string root)
    {
        PngCodec.Save(path, image);
        return Path.GetRelativePath(root, path).Replace('\\', '/');
    }

    static BgraImage? LoadBmp(string path) => File.Exists(path) ? BmpCodec.TryDecode(ReadSharedBytes(path)) : null;

    // The stats file and capture may still be open in a process that was killed on timeout.
    static IEnumerable<string> ReadShared(string path)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        using var reader = new StreamReader(stream);
        var lines = new List<string>();
        while (reader.ReadLine() is { } line)
        {
            lines.Add(line);
        }
        return lines;
    }

    static byte[] ReadSharedBytes(string path)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        using var memory = new MemoryStream();
        stream.CopyTo(memory);
        return memory.ToArray();
    }
}
