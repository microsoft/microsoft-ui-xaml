using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Snapshots;

namespace WinUIProfiler.Tests;

internal static class SnapshotDiffTests
{
    internal static void Run()
    {
        var (before, after) = SnapshotFixtures.Comparison();
        var diff = new SnapshotDiffData(SnapshotViewerData.FromDocument(before), SnapshotViewerData.FromDocument(after));
        Check(diff.Xaml.Count(SnapshotChange.Added) == 1 && diff.Xaml.Count(SnapshotChange.Removed) == 1,
            "Added/removed nodes must not be matched by capture-local IDs.");
        Check(diff.Xaml.Count(SnapshotChange.Moved) == 1 && diff.Xaml.AfterNodes[14].Change.HasFlag(SnapshotChange.Moved),
            "A surviving object with a different logical parent must be moved.");
        Check(diff.Xaml.Count(SnapshotChange.Reordered) == 2 &&
            diff.Xaml.AfterNodes[12].Change.HasFlag(SnapshotChange.Reordered) &&
            diff.Xaml.AfterNodes[13].Change.HasFlag(SnapshotChange.Reordered),
            "Reordering must compare surviving siblings, not absolute child indices.");
        Check(diff.Xaml.AfterNodes[11].Change == SnapshotChange.None && diff.Xaml.AfterNodes[11].ContainsChanges,
            "Unchanged ancestors must retain changed-subtree context.");
        Check(diff.Xaml.AfterNodes[15].Properties.Any(property => property.Name == "properties.counter" &&
            property.Before!.Value.GetUInt64() == ulong.MaxValue &&
            property.After!.Value.GetUInt64() == ulong.MaxValue - 1),
            "Property differences must preserve 64-bit JSON values.");
        Check(diff.Xaml.AfterNodes[15].Properties.Any(property => property.Name == "properties.nullable" &&
            property.Before is null && property.After!.Value.ValueKind == JsonValueKind.Null),
            "A missing property and a captured JSON null must remain distinguishable.");
        var matches = diff.SelectMatches(SnapshotTreeKind.Xaml, diff.Xaml.AfterNodes[15]);
        Check(matches.Count == 1 && matches[0].Before && matches[0].After &&
            matches[0].Node.Before!.Id == 10 && matches[0].Node.After!.Id == 20,
            "Cross-tree matches must merge both sides after identity matching.");
        var insertion = after with
        {
            XamlNodes = after.XamlNodes.Select(node => node.Id == 12 ? node with { ChildIndex = 0 } :
                node.Id == 13 ? node with { ChildIndex = 1 } : node).ToArray()
        };
        var insertionDiff = new SnapshotDiffData(SnapshotViewerData.FromDocument(before), SnapshotViewerData.FromDocument(insertion));
        Check(insertionDiff.Xaml.Count(SnapshotChange.Reordered) == 0,
            "Insertions/removals must not mark unchanged surviving sibling order as reordered.");
        Throws<InvalidDataException>(() => new SnapshotDiffData(SnapshotViewerData.FromDocument(before),
            SnapshotViewerData.FromDocument(after with { CreationTimeFileTime = after.CreationTimeFileTime + 1 })),
            "PID reuse must not permit comparison.");
        var ambiguous = before with { XamlNodes = [.. before.XamlNodes, before.XamlNodes[1] with { Id = 99 }] };
        Throws<InvalidDataException>(() => new SnapshotDiffData(SnapshotViewerData.FromDocument(ambiguous),
            SnapshotViewerData.FromDocument(after)), "Ambiguous same-root identities must be rejected.");
        using var cancelled = new CancellationTokenSource();
        cancelled.Cancel();
        Throws<OperationCanceledException>(() => SnapshotViewerData.FromDocument(before, cancelled.Token),
            "Snapshot construction must honor cancellation.");
        Console.WriteLine("Passed: shared snapshot identity matching, property precision, moves/reorders, context and correlations.");
    }

    internal static void Check(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    internal static void Throws<T>(Action action, string message) where T : Exception
    {
        try { action(); }
        catch (T) { return; }
        throw new InvalidOperationException(message);
    }
}

internal static class SnapshotFixtures
{
    internal static readonly long Created = new DateTime(2026, 1, 1, 0, 0, 0, DateTimeKind.Utc).ToFileTimeUtc();
    private static JsonElement Properties(string json = "{}") => JsonSerializer.Deserialize<JsonElement>(json);

    internal static TreeSnapshotDocument Document() => new(1, Guid.NewGuid(), 42, Created, 1_000_000, 0, 100,
        [new(1, 1, 0, 30, "0x100"), new(2, 2, 35, 70, "0x200")],
        [
            new(1, 1, null, 0, "Microsoft.UI.Xaml.Grid", null, "0x1000", null, Properties()),
            new(2, 2, null, 0, "Microsoft.UI.Xaml.Grid", "OtherRoot", "0x2000", null, Properties()),
            new(3, 2, 2, 0, "Microsoft.UI.Xaml.Button", "Action", "0x3000", "0x4000",
                Properties("""{"opacity":1,"counter":18446744073709551615,"nullable":null}"""))
        ],
        [new(10, 2, null, 0, "Microsoft.UI.Composition.SpriteVisual", "0x5000", "0xFFFFFFFFFFFFFFF0", Properties())],
        [new(3, 10)], ["Captured properties only"]);

    internal static (TreeSnapshotDocument Before, TreeSnapshotDocument After) Comparison()
    {
        XamlSnapshotNode X(ulong id, ulong root, ulong? parent, int order, ulong address, string properties = "{}") =>
            new(id, root, parent, order, "Microsoft.UI.Xaml.Grid", null, $"0x{address:X}", null, Properties(properties));
        var before = Document() with
        {
            Roots = [new(1, 1, 0, 70, "0x100")],
            XamlNodes =
            [
                X(1, 1, null, 0, 0x1000), X(2, 1, 1, 0, 0x2000), X(3, 1, 1, 1, 0x3000),
                X(4, 1, 1, 2, 0x4000), X(5, 1, 2, 0, 0x5000, """{"counter":18446744073709551615}"""),
                X(6, 1, 1, 3, 0x6000)
            ],
            CompositionNodes = [new(10, 1, null, 0, "Visual", "0x60000", "0x70000", Properties())],
            Correlations = [new(5, 10)]
        };
        var after = before with
        {
            CaptureId = Guid.NewGuid(),
            Roots = [new(9, 1, 0, 80, "0x100")],
            XamlNodes =
            [
                X(11, 9, null, 0, 0x1000), X(12, 9, 11, 1, 0x2000), X(13, 9, 11, 0, 0x3000),
                X(14, 9, 12, 0, 0x4000), X(15, 9, 12, 1, 0x5000, """{"counter":18446744073709551614,"nullable":null}"""),
                X(17, 9, 11, 2, 0x7000)
            ],
            CompositionNodes = [new(20, 9, null, 0, "Visual", "0x60000", "0x70000", Properties())],
            Correlations = [new(15, 20)]
        };
        return (before, after);
    }

    internal static async Task SaveAsync(string path, TreeSnapshotDocument document) =>
        await File.WriteAllBytesAsync(path, JsonSerializer.SerializeToUtf8Bytes(document,
            new JsonSerializerOptions { PropertyNamingPolicy = JsonNamingPolicy.CamelCase }));
}
