using System.Runtime.Versioning;
using System.Text;
using System.Text.Json;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

[SupportedOSPlatform("windows")]
public sealed class SnapshotViewerTests
{
    [Fact]
    public void Viewer_has_an_inspection_model_for_saved_snapshots()
    {
        Assert.NotNull(typeof(LiveSessionViewModel).Assembly.GetType("WinUIProfiler.App.ViewModels.SnapshotViewerData"));
    }

    [Fact]
    public void Live_capture_exposes_history_instead_of_only_the_latest_path()
    {
        Assert.NotNull(typeof(LiveSessionViewModel).GetProperty("Snapshots"));
    }

    [Fact]
    public async Task History_keeps_multiple_captures_after_disconnect_and_failed_reconnect()
    {
        var model = new LiveSessionViewModel();
        var first = Saved(CoreTests.Session);
        var second = Saved(Guid.NewGuid());
        model.Snapshots.Add(first, Process(), DateTimeOffset.UtcNow);
        model.Snapshots.Add(second, Process(), DateTimeOffset.UtcNow);
        await model.StopAsync();
        await model.ConnectAsync(new(TargetLaunchKind.Unpackaged, @"C:\missing.exe", "", null, "", "", false));
        Assert.Equal([second.CaptureId, first.CaptureId], model.Snapshots.Entries.Select(e => e.Saved.CaptureId));
        Assert.All(model.Snapshots.Entries, e => Assert.Equal(Process(), e.Process));
        Assert.Throws<InvalidOperationException>(() => model.Snapshots.Add(first, Process(), DateTimeOffset.UtcNow));
        Assert.Equal(2, model.Snapshots.Entries.Count);
    }

    [Fact]
    public async Task Metrics_count_real_nodes_depth_and_visual_types_without_synthetic_groups()
    {
        await WithSnapshot(async (entry, _) =>
        {
            var data = await SnapshotViewerData.LoadAsync(entry);
            Assert.Equal(2, data.Xaml.Nodes.Count);
            Assert.Equal(3, data.Composition.Nodes.Count);
            Assert.Equal(2, data.Xaml.MaximumDepth);
            Assert.Equal(2, data.Composition.MaximumDepth);
            Assert.Equal([12UL, 11UL], data.Composition.Nodes[10].Children.Select(n => n.Id));
            Assert.Equal(2, data.VisualTypes.Single(t => t.Type == "SpriteVisual").Count);
            Assert.Equal(3, data.VisualTypes.Sum(t => t.Count));
        });
    }

    [Fact]
    public async Task Direct_bidirectional_mapping_supports_many_to_many_without_ancestor_guesses()
    {
        await WithSnapshot(async (entry, _) =>
        {
            var data = await SnapshotViewerData.LoadAsync(entry);
            Assert.Equal([10UL, 12UL], data.SelectNodes(SnapshotTreeKind.Xaml, [1]).VisualIds);
            Assert.Equal([1UL, 2UL], data.SelectNodes(SnapshotTreeKind.Composition, [10]).XamlIds);
            Assert.Empty(data.SelectNodes(SnapshotTreeKind.Composition, [11]).XamlIds);
            Assert.Equal([10UL, 12UL], data.SelectNodes(SnapshotTreeKind.Xaml, [1, 2]).VisualIds);
            Assert.Empty(data.SelectNodes(SnapshotTreeKind.Xaml, []).VisualIds);
            Assert.Throws<ArgumentException>(() => data.SelectNodes(SnapshotTreeKind.Xaml, [99]));
        });
    }

    [Theory]
    [InlineData("capture")]
    [InlineData("process")]
    [InlineData("creation")]
    public async Task Saved_viewer_rejects_foreign_capture_or_process_instances(string invalid)
    {
        await WithSnapshot(async (entry, _) =>
        {
            var foreign = invalid switch {
                "capture" => entry with { Saved = entry.Saved with { CaptureId = Guid.NewGuid() } },
                "process" => entry with { Process = new(43, 123, CoreTests.Session) },
                "creation" => entry with { Process = new(42, 124, CoreTests.Session) },
                _ => throw new ArgumentOutOfRangeException(nameof(invalid))
            };
            await Assert.ThrowsAsync<InvalidDataException>(() => SnapshotViewerData.LoadAsync(foreign));
        });
    }

    [Fact]
    public async Task Saved_viewer_reports_truncated_missing_and_oversized_files_and_cancellation()
    {
        await WithSnapshot(async (entry, _) =>
        {
            using var cancelled = new CancellationTokenSource();
            cancelled.Cancel();
            await Assert.ThrowsAnyAsync<OperationCanceledException>(() => SnapshotViewerData.LoadAsync(entry, cancelled.Token));
            await File.WriteAllTextAsync(entry.Saved.Path, "{");
            await Assert.ThrowsAsync<JsonException>(() => SnapshotViewerData.LoadAsync(entry));
            using (var file = File.OpenWrite(entry.Saved.Path)) file.SetLength(TapProtocol.MaximumSnapshotBytes + 1L);
            await Assert.ThrowsAsync<InvalidDataException>(() => SnapshotViewerData.LoadAsync(entry));
            File.Delete(entry.Saved.Path);
            await Assert.ThrowsAsync<FileNotFoundException>(() => SnapshotViewerData.LoadAsync(entry));
        });
    }

    [Fact]
    public async Task Multiple_roots_and_deep_chains_use_actual_parent_edges_without_recursion()
    {
        var document = Fixture();
        var rootNode = document.XamlNodes[0];
        var chain = Enumerable.Range(1, 5000).Select(i => rootNode with {
            Id = (ulong)i, ParentId = i == 1 ? null : (ulong)i - 1,
            ObjectAddress = "0x" + (0x1000 + i).ToString("X")
        }).Append(rootNode with { Id = 5001, RootId = 2, ParentId = null }).ToArray();
        document = document with {
            XamlNodes = chain,
            Roots = [.. document.Roots, document.Roots[0] with { Id = 2 }],
            CompositionNodes = [.. document.CompositionNodes,
                document.CompositionNodes[0] with { Id = 13, RootId = 2, ParentId = null }]
        };
        await WithSnapshot(async (entry, _) =>
        {
            var data = await SnapshotViewerData.LoadAsync(entry);
            Assert.Equal(5000, data.Xaml.MaximumDepth);
            Assert.Equal(2, data.Xaml.Roots.Count);
            Assert.Equal(2, data.Document.Roots.Count);
            Assert.Equal(5001, data.Xaml.Nodes.Count);
        }, document);
    }

    private static ProcessInstanceKey Process() => new(42, 123, CoreTests.Session);
    private static SavedTreeSnapshot Saved(Guid id) => new(@"C:\Snapshots\" + id.ToString("N") + ".trees.json", id, 1, 2, 3, 3);
    private static TreeSnapshotDocument Fixture()
    {
        var original = TreeSnapshotCodec.Decode(Encoding.UTF8.GetBytes(TapTreeSnapshotTests.SnapshotJson()),
            Process(), CoreTests.Session);
        var root = original.CompositionNodes[0];
        return original with {
            CompositionNodes = [root,
                root with { Id = 11, ParentId = 10, ChildIndex = 1, Type = "SpriteVisual" },
                root with { Id = 12, ParentId = 10, ChildIndex = 0, Type = "SpriteVisual" }],
            Correlations = [.. original.Correlations, new(1, 12)]
        };
    }

    private static async Task WithSnapshot(Func<SnapshotHistoryEntry, TreeSnapshotDocument, Task> test, TreeSnapshotDocument? document = null)
    {
        document ??= Fixture();
        var path = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N") + ".trees.json");
        try
        {
            await File.WriteAllBytesAsync(path, JsonSerializer.SerializeToUtf8Bytes(document,
                new JsonSerializerOptions { PropertyNamingPolicy = JsonNamingPolicy.CamelCase }));
            var entry = new SnapshotHistoryEntry(new(path, document.CaptureId, document.Roots.Count,
                document.XamlNodes.Count, document.CompositionNodes.Count, document.Correlations.Count),
                Process(), DateTimeOffset.UtcNow);
            await test(entry, document);
        }
        finally { File.Delete(path); }
    }
}
