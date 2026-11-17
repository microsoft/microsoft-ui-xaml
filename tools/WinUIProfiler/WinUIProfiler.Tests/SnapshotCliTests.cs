using System.Globalization;
using System.Text.Json;
using WinUIProfiler.Cli;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Snapshots;
using WinUIProfiler.Engine.Storage;
using static WinUIProfiler.Tests.SnapshotDiffTests;

namespace WinUIProfiler.Tests;

internal static class SnapshotCliTests
{
    internal static async Task RunAsync()
    {
        var directory = Path.Combine(Environment.CurrentDirectory, $".snapshot-cli-tests-{Guid.NewGuid():N}");
        Directory.CreateDirectory(directory);
        try
        {
            var path = Path.Combine(directory, "snapshot.json");
            var document = SnapshotFixtures.Document();
            await SnapshotFixtures.SaveAsync(path, document);
            await ListingAsync(directory, path);
            await InspectionAsync(path);
            await ComparisonAsync(directory);
            await ResourcesAsync(path, document);
            await HistoryAsync(path, directory, document);
            await DeepTreeAsync(path, document);
            await ErrorsAsync(path);
        }
        finally { Directory.Delete(directory, recursive: true); }
        Console.WriteLine("Passed: snapshot CLI inspection, comparison, resources, history, deep trees and errors.");
    }

    private static async Task ListingAsync(string directory, string snapshotPath)
    {
        using (var json = await JsonAsync("snapshot", "list", directory, "--json"))
        {
            var entry = json.RootElement.GetProperty("snapshots")[0];
            Check(entry.GetProperty("path").GetString() == snapshotPath &&
                entry.GetProperty("snapshot").GetProperty("processId").GetUInt32() == 42 &&
                entry.GetProperty("lastWriteTimeUtc").ValueKind == JsonValueKind.String,
                "Persisted snapshot listing must include validated identities, file paths and saved times.");
        }
        var broken = Path.Combine(directory, "broken.json");
        await File.WriteAllTextAsync(broken, "{}");
        var result = await RunAsync(["snapshot", "list", directory, "--json"]);
        using var malformed = JsonDocument.Parse(result.Output);
        Check(result.Code == 3 && result.Error.Contains("broken.json", StringComparison.Ordinal) &&
            malformed.RootElement.GetProperty("snapshots").GetArrayLength() == 2 &&
            malformed.RootElement.GetProperty("snapshots").EnumerateArray().Any(entry =>
                entry.GetProperty("error").ValueKind == JsonValueKind.String),
            "Malformed persisted snapshots must produce explicit per-file diagnostics, not silent skipping.");
        File.Delete(broken);
    }

    private static async Task InspectionAsync(string path)
    {
        using var node = await JsonAsync("snapshot", "node", path, "--node", "3", "--json");
        var row = node.RootElement.GetProperty("nodes")[0];
        Check(row.GetProperty("node").GetProperty("properties").GetProperty("opacity").GetInt32() == 1 &&
            row.GetProperty("node").GetProperty("properties").GetProperty("counter").GetUInt64() == ulong.MaxValue,
            "Node output must include defaults and full-precision captured properties.");
        Check(row.GetProperty("node").GetProperty("rootId").GetUInt64() == 2 &&
            row.GetProperty("node").GetProperty("parentId").GetUInt64() == 2 &&
            row.GetProperty("matchingIds")[0].GetUInt64() == 10, "Node output must replace parent/root/match navigation.");
        using var tree = await JsonAsync("snapshot", "tree", path, "--node", "2", "--depth", "0", "--json");
        Check(tree.RootElement.GetProperty("nodes").GetArrayLength() == 1 &&
            tree.RootElement.GetProperty("nodes")[0].GetProperty("childIds")[0].GetUInt64() == 3,
            "Explicit depth limits must retain child navigation IDs.");
        using var matches = await JsonAsync("snapshot", "matches", path, "--node", "10", "--tree", "composition", "--json");
        Check(matches.RootElement.GetProperty("matches")[0].GetProperty("matchingIds")[0].GetUInt64() == 3,
            "Composition to XAML mapping must use the shared correlation index.");
        using var composition = await JsonAsync("snapshot", "node", path, "--node", "10", "--tree", "composition", "--json");
        Check(composition.RootElement.GetProperty("nodes")[0].GetProperty("node").GetProperty("visualAddress").GetString() ==
            "0xFFFFFFFFFFFFFFF0", "Unsigned pointer addresses must remain exact strings.");
    }

    private static async Task ComparisonAsync(string directory)
    {
        var (before, after) = SnapshotFixtures.Comparison();
        var oldPath = Path.Combine(directory, "before.json");
        var newPath = Path.Combine(directory, "after.json");
        await SnapshotFixtures.SaveAsync(oldPath, before);
        await SnapshotFixtures.SaveAsync(newPath, after);
        using var json = await JsonAsync("snapshot", "diff", oldPath, newPath, "--changes", "properties",
            "--properties", "changed", "--json");
        var nodes = json.RootElement.GetProperty("nodes").EnumerateArray().ToArray();
        Check(nodes.Length == 3 && nodes.Any(node => node.GetProperty("id").GetString() == "after:11" &&
            node.GetProperty("context").GetBoolean()), "Filtered diffs must include unchanged ancestors.");
        var changed = nodes.Single(node => node.GetProperty("id").GetString() == "after:15");
        Check(changed.GetProperty("after").GetProperty("properties").ValueKind == JsonValueKind.Null &&
            changed.GetProperty("properties").EnumerateArray().Any(property =>
                property.GetProperty("name").GetString() == "properties.nullable" &&
                !property.GetProperty("beforePresent").GetBoolean() && property.GetProperty("afterPresent").GetBoolean()),
            "Changed-property output must distinguish absent values from captured nulls.");
        Check(json.RootElement.GetProperty("correlatedNodes")[0].GetProperty("id").GetString() == "after:20" &&
            changed.GetProperty("matches")[0].GetProperty("before").GetBoolean() &&
            changed.GetProperty("matches")[0].GetProperty("after").GetBoolean(),
            "Acyclic diff output must include unchanged cross-tree correlated nodes with side IDs.");
        using var moved = await JsonAsync("snapshot", "diff", oldPath, newPath, "--node", "4", "--side", "before", "--json");
        Check(moved.RootElement.GetProperty("nodes").EnumerateArray().Any(node =>
            node.GetProperty("id").GetString() == "after:14" &&
            node.GetProperty("changes").EnumerateArray().Any(change => change.GetString() == "moved")),
            "Before-side selection must resolve moved nodes even after IDs change.");
    }

    private static async Task ResourcesAsync(string path, TreeSnapshotDocument document)
    {
        var resources = document with
        {
            SchemaVersion = 4,
            ResourceSnapshot = new("available", 0, 10, 20,
                new(-1, 2, 3, 4, 5, [new(0, ulong.MaxValue, 0, 0), new(17, ulong.MaxValue, 1, ulong.MaxValue - 1)]))
        };
        await SnapshotFixtures.SaveAsync(path, resources);
        using var json = await JsonAsync("snapshot", "resources", path, "--json");
        var rows = json.RootElement.GetProperty("resources").GetProperty("rows").EnumerateArray().ToArray();
        Check(rows.Single(row => row.GetProperty("metric").GetString() == "AllocCount")
            .GetProperty("after").ValueKind == JsonValueKind.Null, "Unavailable allocator sentinels must remain null.");
        Check(rows.Any(row => row.GetProperty("metric").GetString() == "TotalCreated" &&
            row.GetProperty("after").GetDecimal() == (decimal)ulong.MaxValue), "Resources must retain decimal precision.");
        Check(rows.Any(row => row.GetProperty("metric").GetString() == "TotalDestroyed" &&
            !row.GetProperty("isTracked").GetBoolean() && row.GetProperty("after").ValueKind == JsonValueKind.Null),
            "Untracked resource counters must not be invented as zero.");
        await SnapshotFixtures.SaveAsync(path, document);
    }

    private static async Task HistoryAsync(string path, string directory, TreeSnapshotDocument document)
    {
        var session = Path.Combine(directory, "session.sqlite");
        void WriteSession(long? created)
        {
            using var writer = SessionWriter.Create(session, new(42, "SnapshotTests",
                created.HasValue ? DateTime.FromFileTimeUtc(created.Value) : null, 1_000_000, 0, SessionCoverage.Connection));
            ProfilerEvent Event(long qpc, long element) => new(qpc, qpc / 1000.0, 42, 2,
                XamlProvider.XamlProfiler, true, "ElementEnteredTree", 0,
                [new("ElementId", element), new("ParentId", 0L), new("PeerHandle", 0x3000L)]);
            writer.Process(1, Event(20, 0xA000));
            writer.Process(2, Event(80, 0xB000));
        }
        WriteSession(SnapshotFixtures.Created);
        Check(SessionFile.ReadObjects(session, ObjectKind.Element).Count == 2,
            "The history fixture must index both genuine XAML element instances before testing correlation.");
        using (var json = await JsonAsync("snapshot", "history", path, "--node", "3", "--session", session, "--json"))
        {
            Check(json.RootElement.GetProperty("rootEndQpc").GetInt64() == 70,
                "Object history must use the selected root's EndQpc, not the document's EndQpc.");
            var id = json.RootElement.GetProperty("objectId").GetInt64();
            Check(SessionFile.ReadObjects(session, objectId: id).Single().Address == 0xA000 &&
                json.RootElement.GetProperty("events").GetArrayLength() == 1,
                "History must resolve the object instance at the root capture time and return its complete timeline.");
        }
        var visual = SnapshotObjectIdentity.Create(document, SnapshotViewerData.FromDocument(document).Composition.Nodes[10],
            SnapshotTreeKind.Composition);
        Check(visual.Addresses.Single() == unchecked((long)0xFFFFFFFFFFFFFFF0UL),
            "Composition history must correlate IVisual, not the composition object's IUnknown.");
        File.Delete(session);
        WriteSession(SnapshotFixtures.Created + 1);
        var mismatch = await RunAsync(["snapshot", "history", path, "--node", "3", "--session", session, "--json"]);
        Check(mismatch.Code == 3 && mismatch.Output.Length == 0 && mismatch.Error.Contains("different process instances", StringComparison.Ordinal),
            "PID reuse must fail before returning any object history.");
        File.Delete(session);
        WriteSession(null);
        var unavailable = await RunAsync(["snapshot", "history", path, "--node", "3", "--session", session]);
        Check(unavailable.Code == 3 && unavailable.Error.Contains("unavailable", StringComparison.Ordinal),
            "Unknown process creation identity must be explicit, not an unsafe PID-only match.");
    }

    private static async Task DeepTreeAsync(string path, TreeSnapshotDocument document)
    {
        const int count = 2048;
        var nodes = Enumerable.Range(1, count).Select(index => new XamlSnapshotNode((ulong)index, 1,
            index == 1 ? null : (ulong)(index - 1), 0, "Grid", null, $"0x{index + 0x1000:X}", null,
            JsonSerializer.Deserialize<JsonElement>("{}"))).ToArray();
        await SnapshotFixtures.SaveAsync(path, document with
        {
            Roots = [document.Roots[0]], XamlNodes = nodes,
            CompositionNodes = [document.CompositionNodes[0] with { RootId = 1 }],
            Correlations = [new((ulong)count, 10)]
        });
        var result = await RunAsync(["snapshot", "tree", path]);
        Check(result.Code == 0 && result.Output.Contains("[depth=2047 id=2048", StringComparison.Ordinal) &&
            result.Output.Length < 500_000, "Deep trees must be iterative, readable and never silently truncated.");
        using var json = await JsonAsync("snapshot", "tree", path, "--node", "2048", "--json");
        Check(json.RootElement.GetProperty("nodes").GetArrayLength() == 1,
            "Direct selection of a deep node must not require recursive ancestor serialization.");
    }

    private static async Task ErrorsAsync(string path)
    {
        foreach (var command in new[] { "list", "tree", "node", "matches", "diff", "resources", "history" })
        {
            var help = await RunAsync(["snapshot", command, "--help"]);
            Check(help.Code == 0 && help.Output.Contains("snapshot " + command, StringComparison.Ordinal) && help.Error.Length == 0,
                "Every snapshot command must expose successful help.");
        }
        string[][] invalid =
        [
            ["snapshot", "tree", path, "--depth", "-1"],
            ["snapshot", "tree", path, "--tree", "invalid"],
            ["snapshot", "tree", path, "--unknown"],
            ["snapshot", "tree", path, "--json", "--json"],
            ["snapshot", "node", path],
            ["snapshot", "tree", path, "--node", "18446744073709551616"],
            ["snapshot", "diff", path, path, "--changes", "all,added"],
            ["snapshot", "diff", path, path, "--properties", "invalid"],
            ["snapshot", "history", path, "--node", "1"]
        ];
        foreach (var args in invalid)
        {
            var result = await RunAsync(args);
            Check(result.Code == 2 && result.Output.Length == 0 && result.Error.Length > 0,
                "Invalid snapshot options must fail as usage on stderr.");
        }
        var missing = await RunAsync(["snapshot", "node", path, "--node", "99999"]);
        Check(missing.Code == 3 && missing.Output.Length == 0 && missing.Error.Contains("does not exist", StringComparison.Ordinal),
            "Missing snapshot nodes must be explicit operational failures.");
        await File.WriteAllTextAsync(path, "{}");
        var malformed = await RunAsync(["snapshot", "tree", path, "--json"]);
        Check(malformed.Code == 3 && malformed.Output.Length == 0 && malformed.Error.Length > 0,
            "Malformed snapshot files must not leak partial success output.");
        using var cancelled = new CancellationTokenSource();
        cancelled.Cancel();
        var cancellation = await RunAsync(["snapshot", "tree", path], cancelled.Token);
        Check(cancellation.Code == 4 && cancellation.Output.Length == 0 && cancellation.Error.Length > 0,
            "Cancelled snapshot loading must return exit 4 on stderr.");
    }

    private static async Task<JsonDocument> JsonAsync(params string[] args)
    {
        var result = await RunAsync(args);
        Check(result.Code == 0 && result.Error.Length == 0, $"Command failed: {string.Join(' ', args)}: {result.Error}");
        return JsonDocument.Parse(result.Output);
    }

    private static async Task<(int Code, string Output, string Error)> RunAsync(string[] args, CancellationToken token = default)
    {
        using var output = new StringWriter(CultureInfo.InvariantCulture);
        using var error = new StringWriter(CultureInfo.InvariantCulture);
        var code = await SnapshotCommands.RunAsync(args, output, error, token);
        return (code, output.ToString(), error.ToString());
    }
}
