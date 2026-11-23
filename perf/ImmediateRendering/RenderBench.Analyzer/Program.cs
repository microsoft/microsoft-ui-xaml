using RenderBench;
using RenderBench.Analysis;
using RenderBench.Analyzer;

const string Usage = """
    RenderBench.Analyzer analyze <trace.etl> [--out metrics.json]   Parse one run's ETL into metrics JSON.
    RenderBench.Analyzer gate <metrics.json>...                     Print gate checks; exit 1 if any run fails.
    RenderBench.Analyzer coverage <resultsDir>                     Gate a Run-Coverage.ps1 results tree; write coverage.json/.html.
    RenderBench.Analyzer records <trace.etl>                        Print the parsed records in time order (debugging).
    RenderBench.Analyzer report <resultsDir> [--out report.md]      Aggregate **/metrics.json into a comparison table.
    RenderBench.Analyzer adapters                                   List DXGI adapters with the D2D-HW verdict; exit 0 if
                                                                    one qualifies, 1 if none, 2 if the LUID override is bad.
    RenderBench.Analyzer skip <scene> <mode> <reason> --out <json> [--scene-args "<RenderBench args>"]
                                                                    Write metrics.json for a run that was not executed;
                                                                    --scene-args (e.g. "--decode-to-size") picks the
                                                                    variant label so the report groups it correctly.
    """;

if (args.Length < (args.FirstOrDefault() == "adapters" ? 1 : 2))
{
    Console.Error.WriteLine(Usage);
    return 2;
}

string? Option(string name)
{
    int index = Array.IndexOf(args, name);
    return index >= 0 && index + 1 < args.Length ? args[index + 1] : null;
}

switch (args[0])
{
    case "analyze":
    {
        var records = EtlReader.Read(args[1]);
        var metrics = MetricsCalculator.Compute(records);
        string json = MetricsJson.Serialize(metrics);
        string output = Option("--out") ?? Path.Combine(Path.GetDirectoryName(Path.GetFullPath(args[1]))!, "metrics.json");
        File.WriteAllText(output, json);
        Console.WriteLine($"{records.Count} records -> {output}");
        PrintGate(args[1], metrics);
        return 0;
    }
    case "gate":
    {
        bool allPassed = true;
        foreach (var path in args.Skip(1))
        {
            var metrics = MetricsJson.Deserialize(File.ReadAllText(path));
            allPassed &= PrintGate(path, metrics);
        }
        return allPassed ? 0 : 1;
    }
    case "report":
    {
        var runs = Directory.EnumerateFiles(args[1], "metrics.json", SearchOption.AllDirectories)
            .Select(path => (path, MetricsJson.Deserialize(File.ReadAllText(path))))
            .ToList();
        string markdown = ComparisonReport.Build(runs);
        string output = Option("--out") ?? Path.Combine(args[1], "report.md");
        File.WriteAllText(output, markdown);
        Console.WriteLine($"{runs.Count} runs -> {output}");
        return 0;
    }
    case "records":
    {
        // Debugging aid: every converted record in time order, e.g. to see what drew a frame during Steady.
        foreach (var record in EtlReader.Read(args[1]).OrderBy(r => r.TimeMs))
        {
            Console.WriteLine(record);
        }
        return 0;
    }
    // ---- Added by the D2D-HW branch ----
    case "adapters":
        return AdapterReport.Print(Environment.GetEnvironmentVariable(AdapterPolicy.LuidEnvironmentVariable));
    case "skip" when args.Length >= 4 && Option("--out") is { } skipOutput:
    {
        string[] sceneArgs = (Option("--scene-args") ?? "").Split(' ', StringSplitOptions.RemoveEmptyEntries);
        string scene = BenchOptions.TryParse(["--scene", args[1], .. sceneArgs], out var sceneOptions, out _)
            ? sceneOptions.SceneLabel
            : args[1];
        File.WriteAllText(skipOutput, MetricsJson.Serialize(MetricsCalculator.Skipped(scene, args[2], args[3])));
        Console.WriteLine($"{scene} {args[2]}: SKIPPED ({args[3]}) -> {skipOutput}");
        return 0;
    }
    // ---- coverage ----
    case "coverage":
    {
        var summary = RenderBench.Coverage.CoverageReport.Build(args[1]);
        foreach (var run in summary.Runs.Where(r => !r.Passed))
        {
            Console.WriteLine($"FAIL {run.PageId} {run.Mode}: {string.Join("; ", run.Gates.Where(g => !g.Passed).Select(g => $"{g.Name}: {g.Detail}"))}");
        }
        foreach (var (mode, rate) in summary.PerMode)
        {
            Console.WriteLine($"{mode}: {rate}");
        }
        Console.WriteLine($"Overall (non-interop pages passing in every immediate mode): {summary.Overall}; interop: {summary.Interop}");
        Console.WriteLine($"-> {Path.Combine(args[1], RenderBench.Coverage.CoverageFiles.Html)}");
        return summary.Runs.All(r => r.Passed) && summary.CatalogErrors.Count == 0 ? 0 : 1;
    }
    default:
        Console.Error.WriteLine(Usage);
        return 2;
}

static bool PrintGate(string label, RunMetrics metrics)
{
    if (metrics.IsSkipped)
    {
        // D2D-HW branch: a run that was not executed (e.g. no hardware adapter) is not a gate failure.
        Console.WriteLine($"{label}: scene={metrics.Scene} requested={metrics.Identity.HarnessRequested} gate=SKIP ({metrics.SkipReason})");
        return true;
    }
    Console.WriteLine($"{label}: scene={metrics.Scene} requested={metrics.Identity.Requested} effective={metrics.Identity.Effective} " +
        $"gate={(metrics.GatePassed ? "PASS" : "FAIL")}");
    foreach (var check in metrics.Gate)
    {
        Console.WriteLine($"  {(check.Passed ? "ok  " : "FAIL")} {check.Name}: {check.Detail}");
    }
    return metrics.GatePassed;
}
