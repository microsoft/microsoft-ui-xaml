using RenderBench;
using RenderBench.Analysis;
using RenderBench.Analyzer;

const string Usage = """
    RenderBench.Analyzer analyze <trace.etl> [--out metrics.json]   Parse one run's ETL into metrics JSON.
    RenderBench.Analyzer gate <metrics.json>...                     Print gate checks; exit 1 if any run fails.
    RenderBench.Analyzer report <resultsDir> [--out report.md]      Aggregate **/metrics.json into a comparison table.
    RenderBench.Analyzer adapters                                   List DXGI adapters with the D2D-HW verdict; exit 0 if
                                                                    one qualifies, 1 if none, 2 if the LUID override is bad.
    RenderBench.Analyzer skip <scene> <mode> <reason> --out <json>  Write metrics.json for a run that was not executed.
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
    // ---- Added by the D2D-HW branch ----
    case "adapters":
        return AdapterReport.Print(Environment.GetEnvironmentVariable(AdapterPolicy.LuidEnvironmentVariable));
    case "skip" when args.Length >= 4 && Option("--out") is { } skipOutput:
    {
        string scene = Enum.TryParse<SceneId>(args[1], ignoreCase: true, out var sceneId) ? sceneId.ToString() : args[1];
        File.WriteAllText(skipOutput, MetricsJson.Serialize(MetricsCalculator.Skipped(scene, args[2], args[3])));
        Console.WriteLine($"{scene} {args[2]}: SKIPPED ({args[3]}) -> {skipOutput}");
        return 0;
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
