using System.Globalization;
using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Snapshots;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Cli;

internal static class SnapshotCommands
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        NumberHandling = JsonNumberHandling.AllowNamedFloatingPointLiterals,
        WriteIndented = true
    };

    internal static async Task<int> RunAsync(string[] args, TextWriter output, TextWriter error,
        CancellationToken cancellationToken)
    {
        try
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (args is ["snapshot", "--help"])
            {
                WriteHelp(output);
                return 0;
            }
            if (args.Length < 2 || args[0] != "snapshot")
                throw new ArgumentException("Expected snapshot list|tree|node|matches|diff|resources|history.");
            var command = args[1];
            var allowed = command switch
            {
                "tree" => new[] { "--tree", "--node", "--depth" },
                "node" or "matches" => ["--tree", "--node"],
                "diff" => ["--tree", "--node", "--side", "--changes", "--properties"],
                "resources" => [],
                "list" => [],
                "history" => ["--tree", "--node", "--session"],
                _ => throw new ArgumentException($"Unknown snapshot command '{command}'.")
            };
            var options = Options.Parse(args.Skip(2), allowed);
            if (options.Help)
            {
                WriteHelp(output);
                return 0;
            }
            var expectedFiles = command == "diff" ? 2 : 1;
            if (options.Files.Count != expectedFiles &&
                !(command == "resources" && options.Files.Count == 2))
                throw new ArgumentException($"snapshot {command} requires {expectedFiles} snapshot file(s)" +
                    (command == "resources" ? " (or two to compare)." : "."));
            if (command == "list")
                return await ListAsync(options.Files[0], options.Json, output, error, cancellationToken).ConfigureAwait(false);
            var kind = options.Get("--tree") switch
            {
                null or "xaml" => SnapshotTreeKind.Xaml,
                "composition" => SnapshotTreeKind.Composition,
                _ => throw new ArgumentException("--tree must be xaml or composition.")
            };
            ulong? selected = null;
            if (options.Get("--node") is { } nodeValue)
            {
                if (!ulong.TryParse(nodeValue, NumberStyles.None, CultureInfo.InvariantCulture, out var id) || id == 0)
                    throw new ArgumentException("--node must be a positive snapshot node ID.");
                selected = id;
            }
            if (command is "node" or "history" && selected is null)
                throw new ArgumentException("--node is required.");
            var depth = int.MaxValue;
            if (options.Get("--depth") is { } depthValue &&
                (!int.TryParse(depthValue, NumberStyles.None, CultureInfo.InvariantCulture, out depth) || depth < 0))
                throw new ArgumentException("--depth must be a nonnegative integer.");
            var side = options.Get("--side") ?? "after";
            if (side is not ("before" or "after"))
                throw new ArgumentException("--side must be before or after.");
            if (options.Get("--side") is not null && selected is null)
                throw new ArgumentException("--side requires --node.");
            var allProperties = options.Get("--properties") switch
            {
                null or "all" => true,
                "changed" => false,
                _ => throw new ArgumentException("--properties must be all or changed.")
            };
            var changes = ParseChanges(options.Get("--changes"));
            var session = options.Get("--session");
            if (command == "history" && string.IsNullOrWhiteSpace(session))
                throw new ArgumentException("--session is required.");

            var data = await SnapshotViewerData.LoadAsync(options.Files[0], cancellationToken).ConfigureAwait(false);
            var tree = kind == SnapshotTreeKind.Xaml ? data.Xaml : data.Composition;
            if (command != "diff" && selected is { } requested) RequireNode(tree, requested);

            if (command == "diff")
            {
                var after = await SnapshotViewerData.LoadAsync(options.Files[1], cancellationToken).ConfigureAwait(false);
                var diff = new SnapshotDiffData(data, after, cancellationToken);
                var compared = kind == SnapshotTreeKind.Xaml ? diff.Xaml : diff.Composition;
                var target = kind == SnapshotTreeKind.Xaml ? diff.Composition : diff.Xaml;
                SnapshotNodeDiff? focus = null;
                if (selected is { } focusId &&
                    !(side == "before" ? compared.BeforeNodes : compared.AfterNodes).TryGetValue(focusId, out focus))
                    throw new InvalidDataException($"Node {focusId} does not exist in the {side} {TreeName(kind)} tree.");
                var scope = focus is null ? compared.Nodes.ToHashSet() : Descendants(focus, cancellationToken);
                var included = scope.Where(node => changes is null || (node.Change & changes.Value) != 0 ||
                    changes == SnapshotChange.None && node.Change == SnapshotChange.None).ToHashSet();
                if (focus is not null) included.Add(focus);
                AddContext(compared, included, cancellationToken);
                var matches = new HashSet<SnapshotNodeDiff>();
                foreach (var node in included)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    foreach (var match in diff.SelectMatches(kind, node)) matches.Add(match.Node);
                }
                AddContext(target, matches, cancellationToken);
                var rows = DiffRows(compared, included, matches, scope, changes, allProperties, diff, kind, cancellationToken);
                var otherKind = kind == SnapshotTreeKind.Xaml ? SnapshotTreeKind.Composition : SnapshotTreeKind.Xaml;
                var correlated = DiffRows(target, matches, included, [], changes, allProperties, diff, otherKind, cancellationToken);
                if (options.Json)
                    WriteJson(output, new
                    {
                        schemaVersion = 1, command = "snapshot diff", tree = TreeName(kind),
                        before = Metadata(data), after = Metadata(after),
                        nodes = rows, correlatedTree = TreeName(otherKind), correlatedNodes = correlated,
                        resources = ResourceOutput(data, after)
                    });
                else
                {
                    output.WriteLine($"{TreeName(kind)} comparison: {data.Document.CaptureId:D} -> {after.Document.CaptureId:D}");
                    WriteDiff(output, rows, cancellationToken);
                    output.WriteLine($"Correlated {TreeName(otherKind)} nodes:");
                    WriteDiff(output, correlated, cancellationToken);
                }
            }
            else if (command == "resources")
            {
                var after = options.Files.Count == 2
                    ? await SnapshotViewerData.LoadAsync(options.Files[1], cancellationToken).ConfigureAwait(false) : null;
                if (after is not null && (data.Document.ProcessId != after.Document.ProcessId ||
                    data.Document.CreationTimeFileTime != after.Document.CreationTimeFileTime))
                    throw new InvalidDataException("Snapshots must belong to the same process ID and creation time.");
                var result = ResourceOutput(after is null ? null : data, after ?? data);
                if (options.Json) WriteJson(output, new { schemaVersion = 1, command = "snapshot resources", resources = result });
                else
                {
                    output.WriteLine($"Resources: before={result.BeforeStatus}, after={result.AfterStatus}");
                    foreach (var row in result.Rows)
                    {
                        cancellationToken.ThrowIfCancellationRequested();
                        output.WriteLine($"{row.ResourceName ?? row.ResourceIndex?.ToString(CultureInfo.InvariantCulture) ?? "Allocator"} " +
                            $"{row.Metric}: {Number(row.Before)} -> {Number(row.After)} (delta {Number(row.Delta)})" +
                            (row.IsTracked ? "" : " [not tracked]"));
                    }
                }
            }
            else if (command == "history")
            {
                var node = tree.Nodes[selected!.Value];
                var identity = SnapshotObjectIdentity.Create(data.Document, node, kind);
                var objectId = identity.FindObject(session!, cancellationToken);
                var events = new List<HistoryEvent>();
                if (objectId is { } found)
                    SessionFile.ReadTimeline(session!, found, 0, -1,
                        (evt, fields, frame) => events.Add(new(evt.TimestampQpc, evt.RelativeMilliseconds,
                            evt.ProcessId, evt.ThreadId, evt.ProviderId, evt.ProviderName, evt.Name,
                            evt.Opcode, evt.EventId, evt.Version, evt.TraceLogging, evt.Fields, fields, frame)), cancellationToken);
                var result = new
                {
                    schemaVersion = 1, command = "snapshot history", snapshot = Metadata(data), tree = TreeName(kind),
                    node = Node(node), identity.RootId, rootEndQpc = identity.Qpc,
                    addresses = identity.Addresses.Select(address => $"0x{unchecked((ulong)address):X16}").ToArray(),
                    status = objectId.HasValue ? "matched" : "unavailable", objectId, events
                };
                if (options.Json) WriteJson(output, result);
                else
                {
                    output.WriteLine($"{Label(node)} root={identity.RootId} rootEndQpc={identity.Qpc} " +
                        $"objectId={objectId?.ToString(CultureInfo.InvariantCulture) ?? "unavailable (no stored correlation)"}");
                    foreach (var evt in events)
                        output.WriteLine($"QPC {evt.Qpc} thread={evt.ThreadId} {evt.ProviderName}/{evt.Name} " +
                            JsonSerializer.Serialize(evt.Fields, JsonOptions));
                }
            }
            else if (command == "matches")
            {
                var sources = selected is { } id ? new[] { tree.Nodes[id] } : tree.Nodes.Values.OrderBy(node => node.Id).ToArray();
                var rows = sources.Select(node =>
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    var selection = data.SelectNodes(kind, [node.Id]);
                    return new MatchOutput(node.Id, node.RootId,
                        kind == SnapshotTreeKind.Xaml ? selection.VisualIds : selection.XamlIds);
                }).ToArray();
                if (options.Json) WriteJson(output, new
                {
                    schemaVersion = 1, command = "snapshot matches", snapshot = Metadata(data),
                    tree = TreeName(kind), targetTree = TreeName(kind == SnapshotTreeKind.Xaml ? SnapshotTreeKind.Composition : SnapshotTreeKind.Xaml),
                    matches = rows
                });
                else foreach (var row in rows)
                    output.WriteLine($"{TreeName(kind)} node={row.NodeId} root={row.RootId} -> " +
                        (row.MatchingIds.Count == 0 ? "unavailable (no captured correlation)" : string.Join(", ", row.MatchingIds)));
            }
            else
            {
                var starts = selected is { } id ? new[] { tree.Nodes[id] } : tree.Roots;
                var rows = Walk(starts, command == "node" ? 0 : depth, cancellationToken)
                    .Select(item => new TreeOutput(Node(item.Node), item.Depth,
                        item.Node.Children.Select(child => child.Id).ToArray(), MatchingIds(data, kind, item.Node.Id))).ToArray();
                if (options.Json) WriteJson(output, new
                {
                    schemaVersion = 1, command = "snapshot " + command, snapshot = Metadata(data),
                    tree = TreeName(kind), nodes = rows
                });
                else
                    foreach (var row in rows)
                    {
                        cancellationToken.ThrowIfCancellationRequested();
                        WriteNode(output, row.Node, row.Depth);
                        if (command == "node") output.WriteLine(JsonSerializer.Serialize(row.Node.Properties, JsonOptions));
                    }
            }
            cancellationToken.ThrowIfCancellationRequested();
            return 0;
        }
        catch (OperationCanceledException)
        {
            await error.WriteLineAsync("Snapshot command cancelled.").ConfigureAwait(false);
            return 4;
        }
        catch (InvalidDataException ex)
        {
            await error.WriteLineAsync(ex.Message).ConfigureAwait(false);
            return 3;
        }
        catch (ArgumentException ex)
        {
            await error.WriteLineAsync(ex.Message).ConfigureAwait(false);
            return 2;
        }
        catch (Exception ex) when (ex is not OutOfMemoryException)
        {
            await error.WriteLineAsync(ex.Message).ConfigureAwait(false);
            return 3;
        }
    }

    private static string TreeName(SnapshotTreeKind tree) => tree == SnapshotTreeKind.Xaml ? "xaml" : "composition";
    private static void WriteHelp(TextWriter output)
    {
        output.WriteLine("""
            snapshot list <directory> [--json]
            snapshot tree <file.json> [--tree xaml|composition] [--node ID] [--depth N] [--json]
            snapshot node <file.json> --node ID [--tree xaml|composition] [--json]
            snapshot matches <file.json> [--node ID] [--tree xaml|composition] [--json]
            snapshot diff <before.json> <after.json> [--tree xaml|composition] [--node ID --side before|after]
              [--changes all|unchanged|added,removed,properties,moved,reordered] [--properties all|changed] [--json]
            snapshot resources <file.json> [<after.json>] [--json]
            snapshot history <file.json> --node ID --session <session.sqlite> [--tree xaml|composition] [--json]
            Node IDs are decimal, local to a capture and tree; diff --node defaults to the after side.
            Depth 0 shows only the selected node(s); the default is unlimited. JSON trees/diffs are flat.
            Diff filters retain ancestor context and cross-tree matches. Null means unavailable, not zero.
            History requires matching PID and known process creation time; a missing event correlation is unavailable.
            """);
    }

    private static async Task<int> ListAsync(string directory, bool json, TextWriter output, TextWriter error,
        CancellationToken token)
    {
        var path = Path.GetFullPath(directory);
        var entries = new List<ListEntry>();
        foreach (var file in Directory.EnumerateFiles(path, "*.json", SearchOption.TopDirectoryOnly).Order(StringComparer.OrdinalIgnoreCase))
        {
            token.ThrowIfCancellationRequested();
            try
            {
                var data = await SnapshotViewerData.LoadAsync(file, token).ConfigureAwait(false);
                entries.Add(new(file, File.GetLastWriteTimeUtc(file), Metadata(data), data.Xaml.Nodes.Count,
                    data.Composition.Nodes.Count, null));
            }
            catch (Exception ex) when (ex is not (OperationCanceledException or OutOfMemoryException))
            {
                entries.Add(new(file, null, null, null, null, ex.Message));
                await error.WriteLineAsync($"{file}: {ex.Message}").ConfigureAwait(false);
            }
        }
        token.ThrowIfCancellationRequested();
        if (json) WriteJson(output, new { schemaVersion = 1, command = "snapshot list", directory = path, snapshots = entries });
        else
            foreach (var entry in entries)
                output.WriteLine(entry.Error is { } problem ? $"{entry.Path}: invalid ({problem})" :
                    $"{entry.Path} saved={entry.LastWriteTimeUtc:O} xaml={entry.XamlNodes} composition={entry.CompositionNodes} " +
                    JsonSerializer.Serialize(entry.Snapshot, JsonOptions));
        return entries.Any(entry => entry.Error is not null) ? 3 : 0;
    }
    private static string Number(decimal? value) => value?.ToString(CultureInfo.InvariantCulture) ?? "unavailable";
    private static string Label(SnapshotTreeNode node) => $"{node.Type} {node.Name} ({node.VisualAddress ?? node.ObjectAddress})";
    private static void WriteJson(TextWriter output, object value) => output.WriteLine(JsonSerializer.Serialize(value, JsonOptions));
    private static void RequireNode(SnapshotTreeData tree, ulong id)
    {
        if (!tree.Nodes.ContainsKey(id)) throw new InvalidDataException($"Node {id} does not exist in the selected snapshot tree.");
    }

    private static object Metadata(SnapshotViewerData data) => new
    {
        data.Document.SchemaVersion, data.Document.CaptureId, data.Document.ProcessId, data.Document.CreationTimeFileTime,
        data.Document.QpcFrequency, data.Document.StartQpc, data.Document.EndQpc, data.Document.Roots, data.Document.Limitations
    };

    private static NodeOutput Node(SnapshotTreeNode node, bool properties = true) => new(node.Id, node.RootId,
        node.ParentId, node.ChildIndex, node.Type, node.Name, node.ObjectAddress, node.VisualAddress,
        node.DiagnosticsHandle, properties ? node.Properties : null);

    private static IReadOnlyList<ulong> MatchingIds(SnapshotViewerData data, SnapshotTreeKind kind, ulong id)
    {
        var selection = data.SelectNodes(kind, [id]);
        return kind == SnapshotTreeKind.Xaml ? selection.VisualIds : selection.XamlIds;
    }

    private static ResourceResult ResourceOutput(SnapshotViewerData? before, SnapshotViewerData after)
    {
        var old = before?.Document.ResourceSnapshot;
        var current = after.Document.ResourceSnapshot;
        return new(old?.Status ?? "unavailable", current?.Status ?? "unavailable", old?.HResult, current?.HResult,
            old?.StartQpc, old?.EndQpc, current?.StartQpc, current?.EndQpc,
            new SnapshotResourceComparison(old, current).Rows);
    }

    private static IEnumerable<(SnapshotTreeNode Node, int Depth)> Walk(IEnumerable<SnapshotTreeNode> roots,
        int maxDepth, CancellationToken token)
    {
        var pending = new Stack<(SnapshotTreeNode Node, int Depth)>(roots.Reverse().Select(node => (node, 0)));
        while (pending.TryPop(out var item))
        {
            token.ThrowIfCancellationRequested();
            yield return item;
            if (item.Depth >= maxDepth) continue;
            for (var index = item.Node.Children.Count - 1; index >= 0; index--)
                pending.Push((item.Node.Children[index], item.Depth + 1));
        }
    }

    private static HashSet<SnapshotNodeDiff> Descendants(SnapshotNodeDiff node, CancellationToken token)
    {
        var result = new HashSet<SnapshotNodeDiff>();
        var pending = new Stack<SnapshotNodeDiff>();
        pending.Push(node);
        while (pending.TryPop(out var next))
        {
            token.ThrowIfCancellationRequested();
            if (!result.Add(next)) continue;
            foreach (var child in next.Children) pending.Push(child);
        }
        return result;
    }

    private static void AddContext(SnapshotTreeDiff tree, HashSet<SnapshotNodeDiff> nodes, CancellationToken token)
    {
        foreach (var side in new[] { tree.BeforeNodes, tree.AfterNodes })
        {
            var visited = new HashSet<ulong>();
            foreach (var node in nodes.ToArray())
            {
                var current = ReferenceEquals(side, tree.BeforeNodes) ? node.Before : node.After;
                while (current?.ParentId is { } id && visited.Add(id))
                {
                    token.ThrowIfCancellationRequested();
                    var parent = side[id];
                    nodes.Add(parent);
                    current = ReferenceEquals(side, tree.BeforeNodes) ? parent.Before : parent.After;
                }
            }
        }
    }

    private static string DiffId(SnapshotNodeDiff node) => node.After is { } after ? $"after:{after.Id}" : $"before:{node.Before!.Id}";

    private static DiffOutput[] DiffRows(SnapshotTreeDiff tree, HashSet<SnapshotNodeDiff> included, HashSet<SnapshotNodeDiff> correlated,
        HashSet<SnapshotNodeDiff> scope, SnapshotChange? changes, bool allProperties, SnapshotDiffData diff,
        SnapshotTreeKind kind, CancellationToken token)
    {
        var result = new List<DiffOutput>();
        var pending = new Stack<(SnapshotNodeDiff Node, int Depth)>(
            tree.Roots.SelectMany(root => root.Children).Reverse().Select(node => (node, 0)));
        while (pending.TryPop(out var item))
        {
            token.ThrowIfCancellationRequested();
            var node = item.Node;
            for (var index = node.Children.Count - 1; index >= 0; index--)
                pending.Push((node.Children[index], item.Depth + 1));
            if (!included.Contains(node)) continue;
            var context = !scope.Contains(node) || changes is { } filter &&
                !(filter == SnapshotChange.None ? node.Change == SnapshotChange.None : (node.Change & filter) != 0);
            result.Add(new(DiffId(node), node.Parent is null ? null : DiffId(node.Parent), item.Depth,
                node.Before is null ? null : Node(node.Before, allProperties),
                node.After is null ? null : Node(node.After, allProperties),
                Enum.GetValues<SnapshotChange>().Where(change => change != SnapshotChange.None && node.Change.HasFlag(change))
                    .Select(change => change.ToString().ToLowerInvariant()).ToArray(),
                node.ContainsChanges, context,
                node.Properties.Select(property => new PropertyOutput(property.Name, property.Before.HasValue,
                    property.After.HasValue, property.Before, property.After)).ToArray(),
                diff.SelectMatches(kind, node).Where(match => correlated.Contains(match.Node)).Select(match => new DiffMatchOutput(DiffId(match.Node),
                    match.Node.Before?.Id, match.Node.After?.Id, match.Before, match.After)).ToArray()));
        }
        return result.ToArray();
    }

    private static SnapshotChange? ParseChanges(string? value)
    {
        if (value is null or "all") return null;
        SnapshotChange changes = 0;
        var seen = new HashSet<string>(StringComparer.Ordinal);
        foreach (var part in value.Split(','))
        {
            if (!seen.Add(part)) throw new ArgumentException("--changes contains a duplicate change.");
            changes |= part switch
            {
                "added" => SnapshotChange.Added,
                "removed" => SnapshotChange.Removed,
                "properties" => SnapshotChange.Properties,
                "moved" => SnapshotChange.Moved,
                "reordered" => SnapshotChange.Reordered,
                "unchanged" when value == "unchanged" => SnapshotChange.None,
                _ => throw new ArgumentException("--changes must be all, unchanged, or comma-separated added,removed,properties,moved,reordered.")
            };
        }
        return changes;
    }

    private static void WriteNode(TextWriter output, NodeOutput node, int depth)
    {
        var prefix = new string(' ', Math.Min(depth, 20) * 2) + (depth > 20 ? "... " : "");
        output.WriteLine($"{prefix}[depth={depth} id={node.Id} parent={node.ParentId?.ToString(CultureInfo.InvariantCulture) ?? "root"} " +
            $"root={node.RootId} index={node.ChildIndex}] {node.Type} {node.Name} " +
            $"object={node.ObjectAddress} visual={node.VisualAddress ?? "unavailable"} diagnostics={node.DiagnosticsHandle ?? "unavailable"}");
    }

    private static void WriteDiff(TextWriter output, IEnumerable<DiffOutput> rows, CancellationToken token)
    {
        foreach (var row in rows)
        {
            token.ThrowIfCancellationRequested();
            output.WriteLine($"[{row.Id} {string.Join(',', row.Changes)}{(row.Context ? " context" : "")}]");
            WriteNode(output, row.After ?? row.Before!, row.Depth);
            foreach (var property in row.Properties)
                output.WriteLine($"  {property.Name}: {(property.BeforePresent ? property.Before?.GetRawText() : "unavailable")} -> " +
                    (property.AfterPresent ? property.After?.GetRawText() : "unavailable"));
            if (row.Before?.Properties is { } beforeProperties)
                output.WriteLine("  Before properties: " + JsonSerializer.Serialize(beforeProperties, JsonOptions));
            if (row.After?.Properties is { } afterProperties)
                output.WriteLine("  After properties: " + JsonSerializer.Serialize(afterProperties, JsonOptions));
        }
    }

    private sealed record NodeOutput(ulong Id, ulong RootId, ulong? ParentId, int ChildIndex, string Type,
        string? Name, string ObjectAddress, string? VisualAddress, string? DiagnosticsHandle, JsonElement? Properties);
    private sealed record ListEntry(string Path, DateTime? LastWriteTimeUtc, object? Snapshot, int? XamlNodes,
        int? CompositionNodes, string? Error);
    private sealed record TreeOutput(NodeOutput Node, int Depth, IReadOnlyList<ulong> ChildIds, IReadOnlyList<ulong> MatchingIds);
    private sealed record MatchOutput(ulong NodeId, ulong RootId, IReadOnlyList<ulong> MatchingIds);
    private sealed record PropertyOutput(string Name, bool BeforePresent, bool AfterPresent, JsonElement? Before, JsonElement? After);
    private sealed record DiffMatchOutput(string Id, ulong? BeforeId, ulong? AfterId, bool Before, bool After);
    private sealed record DiffOutput(string Id, string? ParentId, int Depth, NodeOutput? Before, NodeOutput? After,
        IReadOnlyList<string> Changes, bool ContainsChanges, bool Context, IReadOnlyList<PropertyOutput> Properties,
        IReadOnlyList<DiffMatchOutput> Matches);
    private sealed record ResourceResult(string BeforeStatus, string AfterStatus, int? BeforeHResult, int? AfterHResult,
        long? BeforeStartQpc, long? BeforeEndQpc, long? AfterStartQpc, long? AfterEndQpc, IReadOnlyList<SnapshotResourceRow> Rows);
    private sealed record HistoryEvent(long Qpc, double RelativeMilliseconds, int ProcessId, int ThreadId,
        Guid ProviderId, string ProviderName, string Name, int Opcode, int? EventId, int? Version,
        bool TraceLogging, IReadOnlyList<EventField> Fields, IReadOnlyList<string> ObjectFields, long? FrameNumber);

    private sealed class Options
    {
        internal List<string> Files { get; } = [];
        private readonly Dictionary<string, string?> values = new(StringComparer.Ordinal);
        internal bool Json => values.ContainsKey("--json");
        internal bool Help => values.ContainsKey("--help");
        internal string? Get(string name) => values.GetValueOrDefault(name);

        internal static Options Parse(IEnumerable<string> arguments, string[] allowed)
        {
            var result = new Options();
            using var iterator = arguments.GetEnumerator();
            var positionalsOnly = false;
            while (iterator.MoveNext())
            {
                var arg = iterator.Current;
                if (positionalsOnly || !arg.StartsWith("--", StringComparison.Ordinal))
                {
                    if (!positionalsOnly && arg.StartsWith("-", StringComparison.Ordinal))
                        throw new ArgumentException($"Unknown option {arg}.");
                    result.Files.Add(arg);
                    continue;
                }
                if (arg == "--") { positionalsOnly = true; continue; }
                if (result.values.ContainsKey(arg)) throw new ArgumentException($"Duplicate option {arg}.");
                if (arg is "--json" or "--help") { result.values.Add(arg, null); continue; }
                if (!allowed.Contains(arg, StringComparer.Ordinal)) throw new ArgumentException($"Unknown option {arg}.");
                if (!iterator.MoveNext() || iterator.Current.StartsWith("--", StringComparison.Ordinal))
                    throw new ArgumentException($"{arg} requires a value.");
                result.values.Add(arg, iterator.Current);
            }
            return result;
        }
    }
}
