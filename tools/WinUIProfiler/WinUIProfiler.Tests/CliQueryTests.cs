using System.Text.Json;
using WinUIProfiler.Cli;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

internal static class CliQueryTests
{
    internal static void Run()
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-CliFields-{Guid.NewGuid():N}.sqlite");
        try
        {
            using (var writer = SessionWriter.Create(path, new(42, "CliFields", DateTime.FromFileTimeUtc(100),
                1000, 0, SessionCoverage.Trace)))
            {
                writer.Process(1, new(1, 1, 42, 7, XamlProvider.Xaml, true, "Fields", 0,
                    [new("nan", double.NaN), new("positive", double.PositiveInfinity), new("negative", float.NegativeInfinity),
                        new("unsigned", ulong.MaxValue), new("signed", long.MinValue)]));
            }
            foreach (string[] command in new[]
            {
                new[] { "dump", path, "--json" },
                ["events", path, "--thread", "7", "--first-seq", "1", "--last-seq", "1", "--json"]
            })
            {
                using var output = new StringWriter();
                using var error = new StringWriter();
                var code = CliApplication.Run(command, output, error, default);
                if (code != 0) throw new InvalidOperationException($"{command[0]} must serialize nonfinite fields: {error}");
                using var json = JsonDocument.Parse(output.ToString());
                var text = output.ToString();
                if (!text.Contains("\"NaN\"") || !text.Contains("\"Infinity\"") || !text.Contains("\"-Infinity\"") ||
                    !text.Contains("18446744073709551615") || !text.Contains("-9223372036854775808"))
                    throw new InvalidOperationException("Raw fields must preserve named floats and full-width integer tokens.");
            }
            Console.WriteLine("Passed: CLI query and dump JSON preserve named floating-point values and full-width integers.");
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" }) File.Delete(file);
        }
    }
}
