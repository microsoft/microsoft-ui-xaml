using RenderBench.Analysis;
using RenderBench.Tests;

// RenderBench.Tests [--filter text]          Unit + infrastructure tests (ETL/WPR tests skip when not elevated).
// RenderBench.Tests --acceptance <results>   Acceptance: every run's metrics.json must pass its gate. Immediate modes
//                                            stay red (EffectiveMatchesRequested: NotImplemented) until a backend lands.
string? Arg(string name)
{
    int index = Array.IndexOf(args, name);
    return index >= 0 && index + 1 < args.Length ? args[index + 1] : null;
}

if (Arg("--acceptance") is { } resultsDir)
{
    var files = Directory.Exists(resultsDir)
        ? Directory.EnumerateFiles(resultsDir, "metrics.json", SearchOption.AllDirectories).ToList()
        : [];
    var tests = files.Select(path => new TestCase($"Acceptance.{Path.GetRelativePath(resultsDir, Path.GetDirectoryName(path)!)}", () =>
    {
        var metrics = MetricsJson.Deserialize(File.ReadAllText(path));
        if (metrics.IsSkipped)
        {
            Check.Skip(metrics.SkipReason!); // D2D-HW branch: e.g. no hardware adapter; not a failure
        }
        var failed = metrics.Gate.Where(g => !g.Passed).Select(g => $"{g.Name} ({g.Detail})").ToList();
        Check.Expect(failed.Count == 0, string.Join("; ", failed));
    })).ToList();
    if (tests.Count == 0)
    {
        Console.WriteLine($"FAIL  Acceptance: no metrics.json under '{resultsDir}'");
        return 1;
    }
    return TestRunner.Run(tests, null);
}

return TestRunner.Run(
    [.. AnalysisTests.All(), .. OptionsTests.All(), .. ProfileTests.All(), .. EtlTests.All(),
     .. GdiPlusReadinessTests.All(), .. D2DHardwareTests.All(), .. ThreadCpuTests.All(), .. ExtendedScenesTests.All()],
    Arg("--filter"));
