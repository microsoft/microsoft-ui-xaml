using RenderBench.Analysis;
using RenderBench.Analyzer;

const string Usage = """
    RenderBench.Analyzer analyze <trace.etl> [--out metrics.json]   Parse one run's ETL into metrics JSON.
    RenderBench.Analyzer gate <metrics.json>...                     Print gate checks; exit 1 if any run fails.
    RenderBench.Analyzer report <resultsDir> [--out report.md]      Aggregate **/metrics.json into a comparison table.
    """;

if (args.Length < 2)
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
    default:
        Console.Error.WriteLine(Usage);
        return 2;
}

static bool PrintGate(string label, RunMetrics metrics)
{
    Console.WriteLine($"{label}: scene={metrics.Scene} requested={metrics.Identity.Requested} effective={metrics.Identity.Effective} " +
        $"gate={(metrics.GatePassed ? "PASS" : "FAIL")}");
    foreach (var check in metrics.Gate)
    {
        Console.WriteLine($"  {(check.Passed ? "ok  " : "FAIL")} {check.Name}: {check.Detail}");
    }
    return metrics.GatePassed;
}
