using System.Text.Json;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Contracts;
using WinUIProfiler.Cli;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Snapshots;
using WinUIProfiler.Engine.Storage;

internal static class SnapshotDiffTests
{
    internal static async Task RunAsync()
    {
        var prefix = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-SnapshotDiff-{Guid.NewGuid():N}");
        string[] paths = [prefix + "-before.json", prefix + "-after.json", prefix + "-properties.json", prefix + "-null.json"];
        try
        {
            var baseline = Capture(false);
            var before = await SaveAsync(baseline, paths[0]);
            var after = await SaveAsync(Capture(true), paths[1]);
            var diff = await SnapshotFiles.LoadAsync(before, after, default);
            foreach (var kind in Enum.GetValues<SnapshotTreeKind>())
            {
                var tree = kind == SnapshotTreeKind.Xaml ? diff.Xaml : diff.Composition;
                var target = kind == SnapshotTreeKind.Xaml ? diff.Composition : diff.Xaml;
                Require(tree.AddedOrRemovedNodes.Count == 2, "Tables must contain only the added and removed nodes.");
                var added = tree.AddedOrRemovedNodes.Single(node => node.Change == SnapshotChange.Added);
                var removed = tree.AddedOrRemovedNodes.Single(node => node.Change == SnapshotChange.Removed);
                Require(added.After!.Id == (kind == SnapshotTreeKind.Xaml ? 24UL : 124UL) &&
                    removed.Before!.Id == (kind == SnapshotTreeKind.Xaml ? 3UL : 103UL),
                    "Tables must retain added nodes from After and removed nodes from Before, despite changed snapshot IDs.");
                Require(tree.Nodes.Any(node => node.Change == SnapshotChange.Properties) &&
                    tree.Nodes.Any(node => node.Change == SnapshotChange.Moved) &&
                    tree.Nodes.Any(node => node.Change == SnapshotChange.Reordered) &&
                    tree.Nodes.Any(node => node.Change == SnapshotChange.None && node.ContainsChanges),
                    "Property edits, moves, reorders and context ancestors must remain available outside the table.");
                foreach (var row in tree.AddedOrRemovedNodes)
                {
                    var properties = row.GetProperties(false);
                    Require(properties.Count > 0 && properties.All(property =>
                        property.Before.HasValue == (row.Before is not null) &&
                        property.After.HasValue == (row.After is not null)),
                        "Properties for added or removed nodes must preserve the absent snapshot side.");
                    Require(ReferenceEquals(row.After is { } current ? tree.AfterNodes[current.Id] :
                        tree.BeforeNodes[row.Before!.Id], row) &&
                        row.Parent?.Parent is { } ancestor && tree.Roots.Single().Children.Contains(ancestor),
                        "A table row must reference the original tree node with its full ancestor path.");
                    var matches = diff.SelectMatches(kind, row);
                    Require(matches.Count > 0 && matches.All(match => target.Nodes.Contains(match.Node) &&
                        match.Before == (row.Before is not null) && match.After == (row.After is not null)),
                        "Table rows must resolve their original cross-tree matches on the side where they exist.");
                }
            }
            var xamlAdded = diff.Xaml.AddedOrRemovedNodes.Single(node => node.After is not null);
            var unchangedMatch = diff.SelectMatches(SnapshotTreeKind.Xaml, xamlAdded)
                .Single(match => match.Node.Change == SnapshotChange.None).Node;
            Require(!diff.Composition.AddedOrRemovedNodes.Contains(unchangedMatch),
                "Unchanged correlated visuals must remain navigable in the tree without appearing in the table.");

            var propertiesOnly = baseline with
            {
                CaptureId = Guid.NewGuid(),
                XamlNodes = baseline.XamlNodes.Select(node => node.Id == 2 ?
                    node with { Properties = JsonSerializer.SerializeToElement(new { opacity = 0.5 }) } : node).ToArray(),
                CompositionNodes = baseline.CompositionNodes.Select(node => node.Id == 102 ?
                    node with { Properties = JsonSerializer.SerializeToElement(new { opacity = 0.5 }) } : node).ToArray()
            };
            var updated = await SaveAsync(propertiesOnly, paths[2]);
            var updates = await SnapshotFiles.LoadAsync(before, updated, default);
            Require(updates.Xaml.Count(SnapshotChange.Properties) == 1 && updates.Composition.Count(SnapshotChange.Properties) == 1 &&
                updates.Xaml.AddedOrRemovedNodes.Count == 0 && updates.Composition.AddedOrRemovedNodes.Count == 0,
                "Property-only comparisons must keep both tables empty while retaining updated tree nodes.");
            var unchanged = await SnapshotFiles.LoadAsync(before, before, default);
            Require(unchanged.Xaml.AddedOrRemovedNodes.Count == 0 && unchanged.Composition.AddedOrRemovedNodes.Count == 0,
                "Identical snapshots must have empty tables.");
            foreach (var tree in new[] { diff.Xaml, diff.Composition })
            {
                var edited = tree.Nodes.Single(node => node.Change == SnapshotChange.Properties);
                var property = edited.GetProperties(false).Single(value => value.Name == "properties.opacity");
                Require(property.Before!.Value.GetDouble() == 1 && property.After!.Value.GetDouble() == 0.5 &&
                    edited.GetProperties(true).Count == 1,
                    "The property pane must show before/after edits and support changes-only filtering.");
            }
            var unchangedNode = unchanged.Xaml.BeforeNodes[2];
            var unchangedProperties = unchangedNode.GetProperties(false);
            Require(unchangedNode.GetProperties(true).Count == 0 &&
                unchangedProperties.Single(property => property.Name == "name").After!.Value.GetString() == "Node2" &&
                unchangedProperties.Single(property => property.Name == "properties.opacity").Before!.Value.GetDouble() == 1,
                "Selecting an unchanged node must still display its captured name and properties.");
            Console.WriteLine("Passed: snapshot diff tables include only additions/removals and retain tree identity and cross-tree correlations.");
            Console.WriteLine("Passed: selected-node properties include unchanged values, changed values, and absent snapshot sides.");
            await CheckCliAsync(baseline, before, after, paths[3], prefix);
        }
        finally
        {
            foreach (var path in paths) File.Delete(path);
        }
    }

    private static async Task CheckCliAsync(TreeSnapshotDocument baseline, SnapshotHistoryEntry before,
        SnapshotHistoryEntry after, string nullPath, string prefix)
    {
        foreach (var filter in new[] { "moved", "added" })
        {
            using var json = Invoke("snapshot", "diff", before.Saved.Path, after.Saved.Path, "--changes", filter, "--json");
            var root = json.RootElement;
            Require(root.GetProperty("schemaVersion").GetInt32() == 1, "The snapshot CLI must retain its versioned JSON contract.");
            var nodes = root.GetProperty("nodes").EnumerateArray().ToArray();
            var correlated = root.GetProperty("correlatedNodes").EnumerateArray().ToArray();
            var selected = nodes.Where(node => !node.GetProperty("context").GetBoolean()).ToArray();
            Require(selected.Length == 1 && selected[0].GetProperty("id").GetString() == (filter == "moved" ? "after:27" : "after:24"),
                "CLI change filters must select the changed node despite different capture-local IDs.");
            Require(nodes.Any(node => node.GetProperty("id").GetString() == "after:21" && node.GetProperty("context").GetBoolean()),
                "Filtered trees must retain their ancestor context.");
            CheckReferences(nodes, correlated);
            CheckReferences(correlated, nodes);
        }
        using (var json = Invoke("snapshot", "diff", before.Saved.Path, after.Saved.Path, "--node", "7",
            "--side", "before", "--changes", "moved", "--json"))
        {
            Require(json.RootElement.GetProperty("nodes").EnumerateArray().Single(node =>
                !node.GetProperty("context").GetBoolean()).GetProperty("after").GetProperty("id").GetInt32() == 27,
                "A before-side selection must resolve to the matching after-side node.");
        }
        using (var json = Invoke("snapshot", "tree", after.Saved.Path, "--node", "22", "--depth", "0", "--json"))
        {
            var nodes = json.RootElement.GetProperty("nodes");
            Require(nodes.GetArrayLength() == 1 && nodes[0].GetProperty("node").GetProperty("id").GetInt32() == 22,
                "Depth zero must show only the selected node.");
        }
        var withNull = baseline with
        {
            CaptureId = Guid.NewGuid(),
            XamlNodes = baseline.XamlNodes.Select(node => node.Id == 2 ? node with
            {
                Properties = JsonSerializer.SerializeToElement(new { opacity = 1, addedNull = (string?)null })
            } : node).ToArray()
        };
        await SaveAsync(withNull, nullPath);
        using (var json = Invoke("snapshot", "diff", before.Saved.Path, nullPath, "--properties", "changed", "--json"))
        {
            var node = json.RootElement.GetProperty("nodes").EnumerateArray().Single(node => node.GetProperty("id").GetString() == "after:2");
            var property = node.GetProperty("properties").EnumerateArray().Single(property =>
                property.GetProperty("name").GetString() == "properties.addedNull");
            Require(!property.GetProperty("beforePresent").GetBoolean() && property.GetProperty("afterPresent").GetBoolean() &&
                property.GetProperty("after").ValueKind == JsonValueKind.Null &&
                node.GetProperty("after").GetProperty("properties").ValueKind == JsonValueKind.Null,
                "Changed-only JSON must distinguish a missing property from an explicitly captured JSON null.");
        }
        var sessionPath = prefix + ".sqlite";
        try
        {
            foreach (var (pid, created, code) in new (int, long?, int)[] { (43, 100, 3), (42, 200, 3), (42, null, 3), (42, 100, 0) })
            {
                using (var writer = SessionWriter.Create(sessionPath, new(pid, "SnapshotCli",
                    created is { } time ? DateTime.FromFileTimeUtc(time) : null, 1000, 0, SessionCoverage.Trace))) { }
                using var output = new StringWriter();
                using var error = new StringWriter();
                var result = CliApplication.Run(["snapshot", "history", before.Saved.Path, "--node", "2",
                    "--session", sessionPath, "--json"], output, error, default);
                Require(result == code, $"Snapshot history must enforce process-instance identity: {error}");
                if (code == 0)
                {
                    using var json = JsonDocument.Parse(output.ToString());
                    Require(json.RootElement.GetProperty("status").GetString() == "unavailable" &&
                        json.RootElement.GetProperty("objectId").ValueKind == JsonValueKind.Null,
                        "A session without a retained correlation must report unavailable, not invent an object.");
                }
                File.Delete(sessionPath);
            }
            using (var writer = SessionWriter.Create(sessionPath, new(42, "SnapshotCli", DateTime.FromFileTimeUtc(100),
                1000, 0, SessionCoverage.Trace)))
            {
                writer.Process(1, new(5, 5, 42, 7, XamlProvider.Xaml, true, "ElementEnteredTree", 0,
                    [new("ElementId", 0x8002L), new("PeerHandle", 0x1002L), new("value", double.NaN)]));
            }
            using (var json = Invoke("snapshot", "history", before.Saved.Path, "--node", "2", "--session", sessionPath, "--json"))
                Require(json.RootElement.GetProperty("status").GetString() == "matched" && json.RootElement.GetProperty("events")
                    .GetArrayLength() == 1 && json.RootElement.GetRawText().Contains("\"NaN\""),
                    $"Snapshot history must serialize nonfinite event fields from the actual recording: {json.RootElement}");
        }
        finally
        {
            foreach (var file in new[] { sessionPath, sessionPath + "-wal", sessionPath + "-shm" }) File.Delete(file);
        }
        Console.WriteLine("Passed: snapshot CLI filters, identity, correlation references, missing/null properties and history JSON.");
    }

    private static void CheckReferences(JsonElement[] nodes, JsonElement[] matches)
    {
        var ids = nodes.Select(node => node.GetProperty("id").GetString()).ToHashSet();
        var matchIds = matches.Select(node => node.GetProperty("id").GetString()).ToHashSet();
        foreach (var node in nodes)
        {
            Require(node.GetProperty("parentId").ValueKind == JsonValueKind.Null ||
                ids.Contains(node.GetProperty("parentId").GetString()), "Every diff parent ID must resolve in its returned tree.");
            Require(node.GetProperty("matches").EnumerateArray().All(match => matchIds.Contains(match.GetProperty("id").GetString())),
                "Every cross-tree match ID must resolve in the returned correlated tree.");
        }
    }

    private static JsonDocument Invoke(params string[] args)
    {
        using var output = new StringWriter();
        using var error = new StringWriter();
        Require(CliApplication.Run(args, output, error, default) == 0, $"CLI {string.Join(' ', args)} failed: {error}");
        return JsonDocument.Parse(output.ToString());
    }

    private static TreeSnapshotDocument Capture(bool after)
    {
        ulong Id(ulong id) => id + (after ? 20UL : 0UL);
        ulong? Parent(ulong id) => id switch
        {
            1 => null,
            3 or 4 => 2,
            7 => after ? 1UL : 2UL,
            9 => 8,
            _ => 1
        };
        int Index(ulong id) => id switch
        {
            5 => after ? 2 : 1,
            6 => after ? 1 : 2,
            7 => after ? 3 : 1,
            8 => 4,
            _ => 0
        };
        ulong[] ids = [1, 2, after ? 4UL : 3UL, 5, 6, 7, 8, 9];
        var xaml = ids.Select(id => new XamlSnapshotNode(Id(id), after ? 2UL : 1UL,
            Parent(id) is { } parent ? Id(parent) : null, Index(id), "Microsoft.UI.Xaml.Controls.Grid",
            $"Node{id}", $"0x{0x1000 + id:X}", null,
            JsonSerializer.SerializeToElement(new { opacity = after && id == 2 ? 0.5 : 1 }))).ToArray();
        var composition = xaml.Select((node, index) => new CompositionSnapshotNode(node.Id + 100, node.RootId,
            node.ParentId + 100, node.ChildIndex, "Windows.UI.Composition.ContainerVisual",
            $"0x{0x2000 + ids[index]:X}", $"0x{0x3000 + ids[index]:X}", node.Properties)).ToArray();
        var correlations = xaml.Select(node => new TreeSnapshotCorrelation(node.Id, node.Id + 100)).ToList();
        correlations.Add(new(Id(after ? 4UL : 3UL), Id(after ? 9UL : 8UL) + 100));
        return new(1, Guid.NewGuid(), 42, 100, 1000, 0, 10,
            [new(after ? 2UL : 1UL, 7, 0, 10, "0x9000")], xaml, composition, correlations, []);
    }

    private static async Task<SnapshotHistoryEntry> SaveAsync(TreeSnapshotDocument document, string path)
    {
        await File.WriteAllBytesAsync(path, JsonSerializer.SerializeToUtf8Bytes(document,
            new JsonSerializerOptions { PropertyNamingPolicy = JsonNamingPolicy.CamelCase }));
        return new(new(path, document.CaptureId, document.Roots.Count, document.XamlNodes.Count,
            document.CompositionNodes.Count, document.Correlations.Count),
            new(document.ProcessId, document.CreationTimeFileTime, Guid.NewGuid()), DateTimeOffset.UtcNow, "Test", "");
    }

    private static void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}
