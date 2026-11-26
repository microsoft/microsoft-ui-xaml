using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Cli;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

internal static class CliQueryTests
{
    internal static void Run()
    {
        RunComposition();
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

    internal static void RunComposition()
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-CliComposition-{Guid.NewGuid():N}.sqlite");
        try
        {
            using (var writer = SessionWriter.Create(path, new(42, "CliComposition", null, 1_000_000, 0, SessionCoverage.Trace)))
            {
                writer.Process(1, new(1000, 1, 42, 7, XamlProvider.DirectComposition, false, "RESOURCE_CREATION", 0,
                    [new("resourcePointer", ulong.MaxValue), new("resourceHandle", uint.MaxValue), new("channelHandle", 2U)],
                    EventId: 1, Version: 0));
                writer.Process(2, new(1100, 1.1, 42, 7, XamlProvider.XamlProfiler, true, "GraphicsResourceCreated", 0,
                    [new("ResourceKind", 0), new("ResourceName", "Composition.ContainerVisual"), new("HasLifetime", false)]));
                writer.Process(3, new(2000, 2, 42, 70, XamlProvider.DirectComposition, false, "RESOURCE_RELEASE", 0,
                    [new("resourcePointer", ulong.MaxValue), new("resourceHandle", uint.MaxValue), new("channelHandle", 0x20342520U)],
                    EventId: 30, Version: 0));
            }
            foreach (var view in new[] { "winui", "native" })
            {
                using var output = new StringWriter();
                using var error = new StringWriter();
                var code = CliApplication.Run(["composition", path, "--view", view, "--json"], output, error, default);
                if (code != 0) throw new InvalidOperationException($"composition {view} must succeed: {error}");
                using var json = JsonDocument.Parse(output.ToString());
                var root = json.RootElement;
                var row = root.GetProperty("Rows")[0];
                if (root.GetProperty("Summary").GetProperty("MatchedReleaseObservations").GetInt64() != 1 ||
                    row.GetProperty("ResourcePointer").GetString() != "0xffffffffffffffff" ||
                    row.GetProperty("ResourceHandle").GetString() != "0x00000000ffffffff" ||
                    row.GetProperty("ReleaseSeq").GetInt64() != 3 ||
                    (view == "winui" && row.GetProperty("Correlation").GetString() != "InferredSequence"))
                    throw new InvalidOperationException("Composition JSON must preserve native identities, observed endpoints and inferred attribution.");
            }
            foreach (string[] command in new[]
            {
                new[] { "composition", path, "--resource", "1", "--count", "1" },
                ["composition", path, "--view", "native", "--offset", "1", "--count", "0", "--json"]
            })
            {
                using var output = new StringWriter();
                using var error = new StringWriter();
                if (CliApplication.Run(command, output, error, default) != 0)
                    throw new InvalidOperationException($"Composition filtering and paging must succeed: {error}");
                if (command.Contains("--json"))
                {
                    using var json = JsonDocument.Parse(output.ToString());
                    if (json.RootElement.GetProperty("Returned").GetInt64() != 0 ||
                        json.RootElement.GetProperty("Total").GetInt64() != 1)
                        throw new InvalidOperationException("Composition paging must preserve total counts.");
                }
                else if (!output.ToString().Contains("InferredSequence / ReleaseObserved", StringComparison.Ordinal) ||
                    !output.ToString().Contains("not final COM destruction", StringComparison.Ordinal))
                    throw new InvalidOperationException("Composition text must expose confidence and avoid claiming final object destruction.");
            }
            using (var output = new StringWriter())
            using (var error = new StringWriter())
            {
                if (CliApplication.Run(["composition", path, "--view", "wrong"], output, error, default) != 2 ||
                    CliApplication.Run(["composition", path, "--resource", "0"], output, error, default) != 2 ||
                    CliApplication.Run(["composition", path, "--pid", "42"], output, error, default) != 2 ||
                    CliApplication.Run(["composition", "capture.etl"], output, error, default) != 2 ||
                    CliApplication.Run(["composition", path], output, error, new(canceled: true)) != 4)
                    throw new InvalidOperationException("Composition options and cancellation must follow CLI exit-code conventions.");
            }
            using (var connection = new SqliteConnection($"Data Source={path};Pooling=False"))
            {
                connection.Open();
                Sql.Execute(connection, "DROP VIEW WinUICompositionLifetimes; DROP VIEW CompositionLifetimes;");
            }
            using (var output = new StringWriter())
            using (var error = new StringWriter())
            {
                if (CliApplication.Run(["composition", path], output, error, default) != 3 ||
                    !error.ToString().Contains("Re-import the original ETL", StringComparison.Ordinal) ||
                    CliApplication.Run(["info", path], output, error, default) != 0)
                    throw new InvalidOperationException("Older session files must give actionable composition guidance without breaking other readers.");
            }
            Console.WriteLine("Passed: composition CLI JSON/text, native-ID filtering, paging, validation and older-session guidance.");
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" }) File.Delete(file);
        }
    }
}
