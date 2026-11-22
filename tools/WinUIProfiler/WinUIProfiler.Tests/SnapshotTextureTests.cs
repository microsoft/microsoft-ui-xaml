using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;

internal static class SnapshotTextureTests
{
    private static readonly DateTime Created = new(2026, 1, 1, 0, 0, 0, DateTimeKind.Utc);
    private const ulong Texture = ulong.MaxValue - 7;

    internal static void Run()
    {
        TimelineAndComparison();
        DescriptorOrder();
        AllocationEnds();
        ReorderedResourceLinks();
        ReorderedGlobalLinks();
        AllocationMetadataAtSnapshot();
        ResourceAmbiguity();
        RetainedResourceAmbiguity();
        Availability();
        Retention();
    }

    private static void TimelineAndComparison()
    {
        WithRecording([
            Direct3D(3, 10, 110),
            Direct3D(6, 20, 130),
            Direct3D(12, Texture, 200, 11),
            Allocation(33, 1000, 1000, 11, 210),
            Allocation(36, 100, 1000, 11, 220),
            Direct3D(9, 21, 240, 12),
            Allocation(36, 101, 1000, 12, 250),
            Direct3D(17, 22, 260),
            Direct3D(14, Texture, 300),
            Direct3D(13, Texture, 400),
            Direct3D(12, Texture, 450, width: 128)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path,
                [Snapshot(215), Snapshot(225), Snapshot(275), Snapshot(325), Snapshot(400), Snapshot(475)]);
            var unlinked = captures[0].Textures.Single();
            Expect(unlinked.Kind == "Texture2D" && unlinked.LinkedBytes is null && unlinked.Allocations.Count == 0,
                "A later allocation must not leak into an earlier texture snapshot, or become zero bytes.");
            var linked = captures[1].Textures.Single();
            Expect(linked.Handle == $"0x{Texture:x16}" && linked.LinkedBytes == (decimal)ulong.MaxValue &&
                linked.Allocations.Single().AdapterAllocationId.HasValue,
                "Texture identities, full unsigned allocation sizes and complete links must remain lossless.");
            using (var descriptor = JsonDocument.Parse(linked.Descriptor))
                Expect(descriptor.RootElement.GetProperty("Width").GetInt32() == 64 &&
                    descriptor.RootElement.GetProperty("Format").GetInt32() == 28,
                    "Texture snapshots must retain recorded descriptors without reading later resource metadata.");
            Expect(captures[2].Textures.Count == 3 && captures[2].KnownLinkedBytes == (decimal)ulong.MaxValue &&
                captures[2].Textures.Single(texture => texture.Kind == "Texture3D").StartQpc is null,
                "Shared adapter allocations count once; rundown creation remains unknown, and buffers/devices are not textures.");
            Expect(captures[3].Textures.Single(texture => texture.Id == linked.Id).Allocations.Count == 0 &&
                captures[2].Textures.Single(texture => texture.Id == linked.Id).Allocations.Count == 1,
                "Historical bridge values must come from their time-scoped links, not the latest bridge metadata.");
            Expect(!captures[4].Textures.Any(texture => texture.Id == linked.Id),
                "An observed Stop excludes the old texture at its exact end boundary.");
            var replacement = captures[5].Textures.Single(texture => texture.Handle == linked.Handle);
            Expect(replacement.Id != linked.Id && replacement.StartQpc == 450,
                "Reused texture handles must identify separate lifetimes.");
            using (var descriptor = JsonDocument.Parse(replacement.Descriptor))
                Expect(descriptor.RootElement.GetProperty("Width").GetInt32() == 128,
                    "Replacement texture descriptors must not borrow the previous lifetime's values.");

            var added = new SnapshotTextureComparison(captures[1], captures[2]);
            Expect(added.Comparable && added.CountDelta == 2 && added.KnownBytesDelta == 0 &&
                added.Rows.Count(row => row.Change == SnapshotTextureChange.Added) == 2 &&
                added.Rows.Single(row => row.After?.Id == linked.Id).Change == SnapshotTextureChange.Unchanged,
                "Snapshot diffs must show texture-only additions while deduplicating shared allocation bytes.");
            var reused = new SnapshotTextureComparison(captures[2], captures[5]);
            Expect(reused.HasChanges && reused.CountDelta == 0 &&
                reused.Rows.Single(row => row.Before?.Id == linked.Id).Change == SnapshotTextureChange.Removed &&
                reused.Rows.Single(row => row.After?.Id == replacement.Id).Change == SnapshotTextureChange.Added &&
                reused.Rows.Single(row => row.Before?.Id == linked.Id).Delta == -(decimal)ulong.MaxValue &&
                reused.Rows.Single(row => row.After?.Id == replacement.Id).Delta is null,
                "Equal counts and reused handles must not hide lifetime replacements or fabricate unknown byte deltas.");
            var reversed = new SnapshotTextureComparison(captures[2], captures[1]);
            Expect(reversed.CountDelta == -2, "Reversed before/after snapshots must preserve signed deltas.");
            var otherRecording = new SnapshotTextureComparison(captures[1],
                captures[1] with { SessionId = Guid.NewGuid() });
            Expect(!otherRecording.Comparable && otherRecording.CountDelta is null &&
                otherRecording.KnownBytesDelta is null && otherRecording.Rows.Count == 2 &&
                otherRecording.Rows.All(row => row.Change == SnapshotTextureChange.NotComparable && row.Delta is null),
                "Database-local IDs and equal handles from different recordings must never be treated as surviving textures.");
            var missing = new SnapshotTextureComparison(new(SnapshotTextureStatus.NotRecorded, null, []), captures[1]);
            Expect(!missing.Comparable && missing.Rows.All(row => row.Change == SnapshotTextureChange.NotComparable),
                "Missing snapshot evidence must not turn every observed texture into a claimed addition.");
        });
        Console.WriteLine("Passed: snapshot-time textures, descriptors, historical links, shared bytes, unsigned sizes and reuse-safe diffs.");
    }

    private static void AllocationEnds()
    {
        WithRecording([
            Direct3D(12, Texture, 200, 11),
            Allocation(33, 1000, 1000, 11, 210),
            Allocation(36, 100, 1000, 11, 220),
            Allocation(37, 100, 1000, 11, 280)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(275), Snapshot(285)]);
            var comparison = new SnapshotTextureComparison(captures[0], captures[1]);
            Expect(captures[0].Textures.Single().Allocations.Count == 1 &&
                captures[1].Textures.Single().Allocations.Count == 0 &&
                comparison.Rows.Single().Change == SnapshotTextureChange.Changed &&
                comparison.Rows.Single().Delta is null,
                "Allocation ends change the observed chain, not the resource lifetime; missing size is not a zero-byte free.");
        });
        Console.WriteLine("Passed: snapshot allocation membership uses link and endpoint times, independently of texture lifetime.");
    }

    private static void DescriptorOrder()
    {
        WithRecording([Direct3D(12, Texture, 200), Direct3D(14, Texture, 250, reverseFields: true)], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(225), Snapshot(275)]);
            var comparison = new SnapshotTextureComparison(captures[0], captures[1]);
            Expect(comparison.Comparable && comparison.Rows.Single().Change == SnapshotTextureChange.Unchanged,
                "Equivalent descriptors with reordered ETW fields must not produce a false texture change.");
        });
        Console.WriteLine("Passed: descriptor comparisons ignore ETW field ordering.");
    }

    private static void ReorderedResourceLinks()
    {
        WithRecording([
            Direct3D(12, Texture, 200, 11),
            Allocation(33, 1000, 1000, 11, 210, 4096),
            Allocation(36, 100, 1000, 11, 220),
            Direct3D(14, Texture, 300, 11),
            Direct3D(14, Texture, 400, 12),
            Allocation(33, 2000, 2000, 12, 240, 8192),
            Allocation(36, 101, 2000, 12, 250)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(225), Snapshot(300), Snapshot(450)]);
            Expect(captures.Select(capture => capture.Textures.Single().LinkedBytes)
                    .SequenceEqual(new decimal?[] { 4096, 4096, 8192 }) &&
                captures[1].Textures.Single().Allocations.Single().KernelResource == "0x000000000000000b",
                "A delayed allocation must not backdate the resource's future kernel-resource bridge.");
            Expect(new SnapshotTextureComparison(captures[0], captures[1]).Rows.Single().Change ==
                    SnapshotTextureChange.Unchanged &&
                new SnapshotTextureComparison(captures[1], captures[2]).Rows.Single().Delta == 4096,
                "Reordered events must not fabricate a historical byte delta or hide the later bridge change.");
        });
        WithRecording([
            Direct3D(12, Texture, 200, 11),
            Allocation(33, 1000, 1000, 11, 210, 4096),
            Allocation(36, 100, 1000, 11, 220),
            Allocation(38, 100, 1000, 12, 400),
            Direct3D(12, Texture + 1, 250, 12)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(300), Snapshot(450)]);
            var before = captures[0].Textures.Single(texture => texture.Handle == $"0x{Texture + 1:x16}");
            var after = captures[1].Textures.Single(texture => texture.Id == before.Id);
            Expect(before.LinkedBytes is null && before.Allocations.Count == 0 && after.LinkedBytes == 4096,
                "A delayed resource must not backdate its allocation endpoint's future kernel-resource bridge.");
        });
        WithRecording([
            Direct3D(12, Texture, 200, 11),
            Allocation(33, 1000, 1000, 11, 210, 4096),
            Direct3D(14, Texture, 400, 11),
            Allocation(36, 100, 1000, 11, 250)
        ], path =>
        {
            Expect(SessionFile.ReadTextureSnapshots(path, [Snapshot(300)])[0].Textures.Single().LinkedBytes == 4096,
                "A future confirmation of an unchanged bridge must not suppress its earlier recorded evidence.");
        });
        Console.WriteLine("Passed: reordered resource links require historical bridge evidence from both endpoints.");
    }

    private static void ReorderedGlobalLinks()
    {
        WithRecording([
            Direct3D(12, Texture, 200, 11),
            Allocation(33, 1000, 1000, 11, 210, 4096),
            Allocation(36, 100, 1000, 11, 220),
            Allocation(38, 100, 2000, 11, 400),
            Allocation(33, 2000, 2000, 11, 240, 8192)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(225), Snapshot(300), Snapshot(450)]);
            Expect(captures.Select(capture => capture.Textures.Single().LinkedBytes)
                    .SequenceEqual(new decimal?[] { 4096, 4096, 8192 }) &&
                captures[1].Textures.Single().Allocations.Single().GlobalAllocation == "0x00000000000003e8" &&
                new SnapshotTextureComparison(captures[0], captures[1]).Rows.Single().Change ==
                    SnapshotTextureChange.Unchanged,
                "A delayed adapter allocation must not backdate the device allocation's future global bridge.");
        });
        WithRecording([
            Direct3D(12, Texture, 200, 11),
            Allocation(36, 100, 1000, 11, 220),
            Allocation(38, 100, 2000, 11, 400),
            Allocation(33, 2000, 2000, 11, 240, 8192)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(300), Snapshot(450)]);
            var before = captures[0].Textures.Single();
            Expect(before.Allocations.Single().AdapterAllocationId is null && before.LinkedBytes is null &&
                captures[1].Textures.Single().LinkedBytes == 8192,
                "A valid resource-to-device link must remain partial when only a future global mapping is available.");
        });
        Console.WriteLine("Passed: reordered global links do not borrow future adapter-allocation mappings.");
    }

    private static void AllocationMetadataAtSnapshot()
    {
        const ulong global = ulong.MaxValue - 9;
        var texture = Direct3D(12, Texture, 200, uint.MaxValue);
        texture = texture with
        {
            Fields = texture.Fields.Select(field => field.Name == "hKMResource"
                ? new EventField(field.Name, -1) : field).ToArray()
        };
        WithRecording([
            texture,
            Allocation(33, 1000, global, uint.MaxValue, 210, 4096),
            Allocation(36, 100, global, uint.MaxValue, 220),
            Allocation(35, 1000, global, uint.MaxValue, 400),
            Allocation(35, 1000, global, uint.MaxValue, 500, 0)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path,
                [Snapshot(225), Snapshot(300), Snapshot(400), Snapshot(500)]);
            Expect(captures.Select(capture => capture.Textures.Single().LinkedBytes)
                    .SequenceEqual(new decimal?[] { 4096, 4096, (decimal)ulong.MaxValue, 0 }) &&
                captures[0].Textures.Single().Allocations.Single().GlobalAllocation == $"0x{global:x16}" &&
                new SnapshotTextureComparison(captures[0], captures[1]).Rows.Single().Change ==
                    SnapshotTextureChange.Unchanged,
                "Allocation sizes must come from the snapshot time, preserving signed handles, full unsigned values and zero.");
        });
        var allocation = Allocation(33, 1000, 1000, 11, 210);
        allocation = allocation with { Fields = allocation.Fields.Where(field => field.Name != "allocSize").ToArray() };
        WithRecording([
            Direct3D(12, Texture, 200, 11),
            allocation,
            Allocation(36, 100, 1000, 11, 220),
            Allocation(35, 1000, 1000, 11, 400, 8192)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(300), Snapshot(450)]);
            Expect(captures[0].Textures.Single().Allocations.Single().SizeBytes is null &&
                captures[0].Textures.Single().LinkedBytes is null &&
                captures[1].Textures.Single().LinkedBytes == 8192 &&
                new SnapshotTextureComparison(captures[0], captures[1]).Rows.Single().Delta is null,
                "Later size enrichment must not replace an unknown historical size or fabricate its byte delta.");
        });
        var unowned = Direct3D(12, Texture, 200, 11);
        unowned = unowned with { Fields = unowned.Fields.Where(field => field.Name != "pID3D11Device").ToArray() };
        var unknownOwner = Direct3D(14, Texture, 500, 11);
        unknownOwner = unknownOwner with
        {
            Fields = unknownOwner.Fields.Select(field => field.Name == "pID3D11Device"
                ? new EventField(field.Name, 0UL) : field).ToArray()
        };
        WithRecording([unowned, Direct3D(14, Texture, 400, 11), unknownOwner], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(300), Snapshot(450), Snapshot(500)]);
            Expect(captures[0].Textures.Single().DeviceHandle is null &&
                captures[1].Textures.Single().DeviceHandle == "0x000000000000000a" &&
                captures[2].Textures.Single().DeviceHandle == "0x000000000000000a",
                "Later ownership enrichment must not leak backward, and a zero owner must not erase the observed lifetime owner.");
        });
        Console.WriteLine("Passed: historical allocation sizes use retained point-in-time evidence rather than latest metadata.");
    }

    private static void ResourceAmbiguity()
    {
        foreach (var kind in new[] { 6, 9, 12, 15 })
        {
            WithRecording([
                Direct3D(12, Texture, 200, 11),
                Direct3D(kind, 20, 220, 11),
                Direct3D(kind + 1, 20, 400, 11),
                Allocation(33, 1000, 1000, 11, 230, 4096),
                Allocation(36, 100, 1000, 11, 250)
            ], path =>
            {
                var captures = SessionFile.ReadTextureSnapshots(path,
                    [Snapshot(225), Snapshot(300), Snapshot(399), Snapshot(400)]);
                var id = captures[0].Textures.Single(texture => texture.Handle == $"0x{Texture:x16}").Id;
                Expect(captures.Select(capture => capture.Textures.Single(texture => texture.Id == id).LinkedBytes)
                        .SequenceEqual(new decimal?[] { null, null, null, 4096 }),
                    "A future Stop must not make a shared resource handle unique at an earlier snapshot.");
                var comparison = new SnapshotTextureComparison(captures[0], captures[1]);
                Expect(comparison.Rows.Single(row => row.Before?.Id == id).Change == SnapshotTextureChange.Unchanged &&
                    comparison.KnownBytesDelta == 0,
                    "Delayed allocations must not fabricate a change while another buffer or texture claims the resource.");
            });
        }
        WithRecording([
            Direct3D(12, Texture, 200, 11),
            Direct3D(6, 20, 220, 11),
            Direct3D(8, 20, 400, 12),
            Allocation(33, 1000, 1000, 11, 230, 4096),
            Allocation(36, 100, 1000, 11, 250)
        ], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(300), Snapshot(400)]);
            Expect(captures[0].Textures.Single().LinkedBytes is null &&
                captures[1].Textures.Single().LinkedBytes == 4096,
                "A competitor's future bridge change must not establish historical resource-handle uniqueness.");
        });
        WithRecording([
            Direct3D(6, 20, 100, 11),
            Direct3D(7, 20, 190, 11),
            Direct3D(12, Texture, 200, 11),
            Allocation(33, 1000, 1000, 11, 230, 4096),
            Allocation(36, 100, 1000, 11, 250)
        ], path =>
        {
            Expect(SessionFile.ReadTextureSnapshots(path, [Snapshot(300)])[0].Textures.Single().LinkedBytes == 4096,
                "An already-ended resource must not make a later lifetime's handle ambiguous.");
        });
        Console.WriteLine("Passed: snapshot-time uniqueness includes buffers and every texture kind, with exact end and bridge boundaries.");
    }

    private static void RetainedResourceAmbiguity()
    {
        foreach (var cleared in new[] { false, true })
        {
            var events = new List<ProfilerEvent> { Direct3D(6, 20, 200, 11) };
            events.AddRange(Enumerable.Range(0, 6000).Select(index =>
                new ProfilerEvent(1000 + index, index, 42, 7, XamlProvider.Xaml, false, "Filler", 0,
                    [new("Value", new string('x', 256))])));
            events.AddRange([
                Direct3D(7, 20, 9000, 11),
                Direct3D(12, Texture, 8000, 11),
                Allocation(33, 1000, 1000, 11, 8100, 4096),
                Allocation(36, 100, 1000, 11, 8200)
            ]);
            if (cleared) events.Add(Direct3D(8, 20, 8300));
            WithRecording(events, path =>
            {
                using var connection = new SqliteConnection($"Data Source={path};Mode=ReadOnly;Pooling=False");
                connection.Open();
                Expect(Sql.Scalar(connection, """
                    SELECT COUNT(*) FROM GraphicsObjects g WHERE g.Kind = 'Buffer' AND g.Trimmed = 1
                        AND NOT EXISTS (SELECT 1 FROM GraphicsObjectEvents e
                            WHERE e.GraphicsObjectId = g.GraphicsObjectId AND e.Role = 'Start')
                    """) == 1, "The competing buffer's original bridge evidence must actually be trimmed.");
                var capture = SessionFile.ReadTextureSnapshots(path, [Snapshot(8500)])[0];
                Expect(capture.Available && capture.Trimmed && capture.Textures.Count == 1 &&
                    capture.Textures.Single().LinkedBytes == (cleared ? 4096m : (decimal?)null),
                    "Lost competing-claim evidence cannot prove uniqueness; a retained zero bridge can establish its absence.");
            }, limitBytes: 1024 * 1024);
        }
        Console.WriteLine("Passed: rolling retention keeps uncertain competing claims unknown and respects retained bridge clearing.");
    }

    private static void Availability()
    {
        Expect(SessionFile.ReadTextureSnapshots(null, [Snapshot(300)])[0].Status == SnapshotTextureStatus.NotRecorded,
            "Older history entries without a recording must remain readable with unavailable textures.");
        WithRecording([Direct3D(12, Texture, 200)], path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [
                Snapshot(300) with { ProcessId = 43 },
                Snapshot(300) with { CreationTimeFileTime = Created.AddSeconds(1).ToFileTimeUtc() },
                Snapshot(300) with { QpcFrequency = 2000 }, Snapshot(50), Snapshot(150), Snapshot(300)]);
            Expect(captures.Select(capture => capture.Status).SequenceEqual([
                SnapshotTextureStatus.DifferentProcess, SnapshotTextureStatus.DifferentProcess,
                SnapshotTextureStatus.DifferentClock, SnapshotTextureStatus.OutsideRecording,
                SnapshotTextureStatus.NoEvents, SnapshotTextureStatus.Available]),
                "Snapshot reads must verify exact process creation, live QPC clock, recording start and actual D3D evidence.");
            using var connection = new SqliteConnection($"Data Source={path};Pooling=False");
            connection.Open();
            Sql.Execute(connection, "UPDATE SessionInfo SET Source = 'Trace'");
            Expect(SessionFile.ReadTextureSnapshots(path, [Snapshot(300)])[0].Status == SnapshotTextureStatus.DifferentClock,
                "An imported trace must not be aligned to a live snapshot just because its PID and QPC frequency match.");
            Sql.Execute(connection, "DROP TABLE GraphicsLinks");
            Expect(SessionFile.ReadTextureSnapshots(path, [Snapshot(300)])[0].Status == SnapshotTextureStatus.NotRecorded,
                "Pre-graphics session files must report unavailable data without failing the tree viewer.");
        });
        using var cancellation = new CancellationTokenSource();
        cancellation.Cancel();
        try
        {
            SessionFile.ReadTextureSnapshots(null, [Snapshot(300)], cancellation.Token);
            throw new InvalidOperationException("Cancelled texture loads must stop.");
        }
        catch (OperationCanceledException) { }
        Console.WriteLine("Passed: legacy, missing, wrong-process, wrong-clock, uncovered-time and cancelled texture reads stay explicit.");
    }

    private static void Retention()
    {
        var events = new List<ProfilerEvent> { Direct3D(12, Texture, 200, 11) };
        events.AddRange(Enumerable.Range(0, 6000).Select(index =>
            new ProfilerEvent(1000 + index, index, 42, 7, XamlProvider.Xaml, false, "Filler", 0,
                [new("Value", new string('x', 256))])));
        WithRecording(events, path =>
        {
            var captures = SessionFile.ReadTextureSnapshots(path, [Snapshot(250), Snapshot(7500)]);
            Expect(captures[0].Status == SnapshotTextureStatus.HistoryTrimmed &&
                captures[0].Textures.Count == 0 && captures[1].Available && captures[1].Trimmed &&
                captures[1].Textures.Single() is { Trimmed: true, Descriptor: "{}", LinkedBytes: null },
                "Trimmed snapshot times are unavailable; surviving open textures retain identity without inventing lost descriptors or links.");
        }, limitBytes: 1024 * 1024);
        Console.WriteLine("Passed: real rolling retention marks unavailable snapshot history and preserves partial open-texture continuity.");
    }

    private static TreeSnapshotDocument Snapshot(long qpc) =>
        new(1, Guid.NewGuid(), 42, Created.ToFileTimeUtc(), 1000, qpc, qpc, [], [], [], [], []);

    private static ProfilerEvent Direct3D(int id, ulong handle, long qpc, uint resource = 0, uint width = 64,
        bool reverseFields = false)
    {
        string[] kinds = ["Device", "Buffer", "Texture1D", "Texture2D", "Texture3D"];
        string[] phases = ["Start", "Stop", "DCStart"];
        var kind = (id - 3) / 3;
        EventField[] fields = kind == 0 ? [new("pID3D11Device", handle)] :
            [new("pID3D11Resource", handle), new("pID3D11Device", 10UL), new("hKMResource", resource),
                new("Width", width), new("Height", 64U), new("Format", 28U)];
        return new(qpc, qpc, 42, 7, XamlProvider.Direct3D11, false, $"{kinds[kind]}/{phases[(id - 3) % 3]}",
            (id - 3) % 3 + 1, reverseFields ? fields.Reverse().ToArray() : fields, EventId: id, Version: 0);
    }

    private static ProfilerEvent Allocation(int id, ulong handle, ulong global, uint resource, long qpc,
        ulong size = ulong.MaxValue) =>
        new(qpc, qpc, 42, 7, XamlProvider.DxgKrnl, false, id < 36 ? "AdapterAllocation" : "DeviceAllocation",
            (id - 33) % 3 + 1,
            [new("hProcessId", 42UL), new("hVidMmAlloc", handle), new("hVidMmGlobalAlloc", global),
                new("hDevice", 10UL), new("pDxgAdapter", 20UL), new("hThunkResource", (ulong)resource),
                new("allocSize", size)], EventId: id, Version: id < 36 ? 3 : 2);

    private static void WithRecording(IEnumerable<ProfilerEvent> events, Action<string> inspect,
        long limitBytes = SessionWriter.DefaultLimitBytes)
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-SnapshotTextures-{Guid.NewGuid():N}.sqlite");
        try
        {
            using (var writer = SessionWriter.Create(path,
                new(42, "SnapshotTextures", Created, 1000, 100, SessionCoverage.Connection), limitBytes))
            {
                long sequence = 0;
                foreach (var item in events) writer.Process(++sequence, item);
                writer.Close("Test complete", lostByEtw: 0, droppedByProfiler: 0);
                if (writer.Failure is { } failure) throw new InvalidOperationException("Texture fixture recording failed.", failure);
            }
            inspect(path);
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" }) File.Delete(file);
        }
    }

    private static void Expect(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}
