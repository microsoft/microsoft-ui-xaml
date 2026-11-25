// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Tests.Mcp.SessionFixtures;
using static WinUIProfiler.Tests.Mcp.ToolResults;

namespace WinUIProfiler.Tests.Mcp;

internal static class MemoryToolTests
{
    private const ulong Resource = ulong.MaxValue - 7;
    private const ulong Surface = 0xD000;
    private const ulong Visual = 0xA000;
    private const string MaxBytes = "18446744073709551615";

    public static IEnumerable<McpTestCase> Tests =>
    [
        new("memory.chain", Chain),
        new("memory.time", Time),
        new("memory.associations", Associations),
        new("memory.surface-clears", SurfaceClears),
        new("memory.resource-reuse", ResourceReuse),
        new("memory.element-reuse", ElementReuse),
        new("memory.coverage", Coverage),
        new("memory.sizes", Sizes),
        new("memory.retention", Retention),
        new("memory.recording", Recording),
        new("memory.arguments", Arguments),
    ];

    private static void Chain(McpTestContext context)
    {
        var path = WriteMemory(context, Events());
        var summary = Run(() => MemoryTools.Memory(sessionPath: path));
        Expect.Equal("summary", summary.Text("view"), "The default memory view");
        Expect.Equal(MaxBytes, summary.Property("totals").Text("knownLinkedAllocationBytes"), "Unsigned exact, deduplicated bytes");
        Expect.Equal(1L, summary.Property("totals").Long("textures"), "Process textures");
        Expect.Equal(1L, summary.Property("totals").Long("knownAdapterAllocations"), "Shared allocations count once");
        Expect.Equal(2L, summary.Long("elementsWithTextureObservations"), "Elements sharing one atlas");
        Expect.Equal(3L, summary.Long("observedLinks"), "Brush and mask paths");
        Expect.Equal(0L, summary.Long("unresolvedLinks"), "Unresolved paths");
        Expect.True(summary.Property("coverage").Bool("elementVisuals"), "The element bridge was recorded.");
        HasNote(summary, "not exclusive element backing storage");
        HasNote(summary, "do not add per-element subtotals together");
        QueryTests.CheckQueries(summary, "GPU memory summary");

        var root = Run(() => MemoryTools.Memory(sessionPath: path, address: "0x1000"));
        Expect.Equal("element", root.Text("view"), "An address selects the element view");
        Expect.Equal("Root", root.Property("element").Text("name"), "The native bridge selects the actual element");
        Expect.Equal(2L, root.Long("total"), "Brush and mask paths for Root");
        Expect.Equal(MaxBytes, root.Property("totals").Text("knownLinkedAllocationBytes"), "Repeated paths count one allocation");
        var mask = root.Items("links").Single(link => link.Text("role") == "Mask");
        Expect.Equal(-3L, mask.Property("observation").Long("offsetX"), "Signed atlas offset");
        Expect.Equal("Available", mask.Text("status"), "Verified texture lifetime");
        var texture = mask.Property("texture");
        Expect.Equal($"0x{Resource:x16}", texture.Text("handle"), "Unsigned resource identity");
        Expect.Equal(2, texture.Items("observedElementIds").Count, "The texture exposes shared element references");
        Expect.Equal(MaxBytes, texture.Items("allocations").Single().Text("sizeBytes"), "Exact allocation size");
        QueryTests.CheckQueries(root, "GPU element links");
        var page = Run(() => MemoryTools.Memory(sessionPath: path, objectId: root.Property("element").Long("objectId"), limit: 1));
        Expect.Equal(1L, page.Long("nextOffset"), "Element link pagination");

        var elements = Run(() => MemoryTools.Memory(view: "elements", sessionPath: path, limit: 1));
        Expect.Equal(2L, elements.Long("total"), "Elements with observed textures");
        Expect.Equal(1L, elements.Long("nextOffset"), "Element pagination");
        Expect.Equal(MaxBytes, elements.Items("elements").Single().Property("totals").Text("knownLinkedAllocationBytes"),
            "The ranked element subtotal");
        QueryTests.CheckQueries(elements, "GPU element ranking");
        var textures = Run(() => MemoryTools.Memory(view: "textures", sessionPath: path));
        Expect.Equal(1L, textures.Long("total"), "Texture listing");
        Expect.Equal(64L, textures.Items("textures").Single().Property("descriptor").Long("Width"), "Recorded descriptor");
        QueryTests.CheckQueries(textures, "GPU texture listing");

        var sessions = Run(() => SessionTools.Sessions(sessionPath: path));
        Expect.True(sessions.Items("analyses").Single(row => row.Text("name") == "memory").Bool("available"),
            "Session discovery advertises GPU memory evidence.");
    }

    private static void Time(McpTestContext context)
    {
        var events = Events().ToList();
        events.AddRange([
            Bind(7, Visual, 0, 0, 0), Bind(7.1, Visual, 0, 1, 0),
            Bind(8, Visual + 1, 0, 0, 0), Bind(8.1, Visual + 1, 0, 1, 0),
            Tl(9, UiThread, XamlProvider.Xaml, "End")
        ]);
        var path = WriteMemory(context, events);
        var early = Run(() => MemoryTools.Memory(sessionPath: path, atMs: 3.5));
        Expect.Equal(0L, early.Long("observedLinks"), "Later ownership must not leak backward");
        Expect.Equal(1L, early.Property("totals").Long("textures"), "Independent process texture data");
        var middle = Run(() => MemoryTools.Memory(sessionPath: path, atMs: 4.15));
        Expect.Equal(1L, middle.Long("observedLinks"), "The later mask and second element are not included");
        Expect.Equal(4.15, middle.Property("point").Double("atMs"), "The selected time");
        Expect.Equal(0L, Run(() => MemoryTools.Memory(sessionPath: path)).Long("observedLinks"), "Explicit clear removes associations");
        QueryTests.CheckQueries(middle, "Historical GPU memory");
    }

    private static void Associations(McpTestContext context)
    {
        var events = Events().ToList();
        events.AddRange([
            Bind(7, Visual, 0, 0, 0x2000),
            Bind(8, Visual, Surface, 0, 0x2000),
            Init(9, Surface),
            Observe(9.2, Surface),
            Bind(9.3, Visual, Surface, 0, 0x2000),
            Tl(10, UiThread, XamlProvider.XamlProfiler, "DCompSurfaceMemoryChanged",
                F("SurfaceId", Surface), F("IsAllocation", false)),
            Tl(11, UiThread, XamlProvider.Xaml, "End")
        ]);
        var path = WriteMemory(context, events);
        var reused = Run(() => MemoryTools.Memory(sessionPath: path, address: "0x1000", atMs: 7.5));
        Expect.Equal(0L, reused.Long("total"), "A reassigned visual does not retain its earlier element or surface bindings");
        Expect.Equal(1L, Run(() => MemoryTools.Memory(sessionPath: path, address: "0x2000", atMs: 7.5)).Long("total"),
            "The new owner must not inherit the visual's old mask binding");
        Expect.Equal(0L, Run(() => MemoryTools.Memory(sessionPath: path, atMs: 9.1)).Long("observedLinks"),
            "A reused surface address invalidates previous observations and bindings");
        Expect.Equal(1L, Run(() => MemoryTools.Memory(sessionPath: path, atMs: 9.5)).Long("observedLinks"),
            "Only new surface evidence restores the chain");
        Expect.Equal(0L, Run(() => MemoryTools.Memory(sessionPath: path)).Long("observedLinks"),
            "Surface allocation release invalidates its update observations");
    }

    private static void SurfaceClears(McpTestContext context)
    {
        var events = Events().ToList();
        events.AddRange([
            Bind(7, Visual, 0, 0, 0x1000),
            Bind(8, Visual, Surface, 0, 0x1000),
            Bind(9, Visual, 0, 1, 0x1000),
            Tl(10, UiThread, XamlProvider.Xaml, "End")
        ]);
        var path = WriteMemory(context, events);
        var mask = Run(() => MemoryTools.Memory(sessionPath: path, address: "0x1000", atMs: 7.5));
        Expect.Equal(1L, mask.Long("total"), "Clearing the brush preserves the current owner's mask");
        Expect.Equal("Mask", mask.Items("links").Single().Text("role"), "The surviving surface role");
        Expect.Equal(2L, Run(() => MemoryTools.Memory(sessionPath: path, address: "0x1000", atMs: 8.5)).Long("total"),
            "Updating a brush for the same owner must not discard the mask");
        var brush = Run(() => MemoryTools.Memory(sessionPath: path, address: "0x1000", atMs: 9.5));
        Expect.Equal(1L, brush.Long("total"), "Clearing the mask preserves the current owner's brush");
        Expect.Equal("Brush", brush.Items("links").Single().Text("role"), "The restored brush remains attributed");
    }

    private static void ResourceReuse(McpTestContext context)
    {
        var events = Events().ToList();
        events.AddRange([
            Texture(7, 13), Texture(8, 12, 12),
            Allocation(8.1, 33, 2000, 2000, 12, 8192),
            Allocation(8.2, 36, 101, 2000, 12),
            Observe(9, Surface),
            Tl(10, UiThread, XamlProvider.Xaml, "End")
        ]);
        var path = WriteMemory(context, events);
        var stopped = Run(() => MemoryTools.Memory(sessionPath: path, address: "0x2000", atMs: 7.5));
        Expect.Equal("ResourceNotAliveOrRecorded", stopped.Items("links").Single().Text("status"), "Stopped texture is not live");
        var replacement = Run(() => MemoryTools.Memory(sessionPath: path, address: "0x2000", atMs: 8.5));
        Expect.Equal("ObservationBeforeLifetime", replacement.Items("links").Single().Text("status"),
            "An old update observation must not borrow a new texture at the same pointer");
        Expect.True(replacement.Items("links").Single().IsNull("texture"), "Unverified texture identity stays unavailable");
        Expect.Equal("0", replacement.Property("totals").Text("knownLinkedAllocationBytes"), "No invented linked bytes");
        HasNote(replacement, "remain unresolved");
        var confirmed = Run(() => MemoryTools.Memory(sessionPath: path, address: "0x2000", atMs: 9.5));
        Expect.Equal("8192", confirmed.Property("totals").Text("knownLinkedAllocationBytes"), "New observation resolves the new lifetime");
    }

    private static void ElementReuse(McpTestContext context)
    {
        var events = Events().ToList();
        events.AddRange([
            LeftTree(7, 0x1000, 0),
            Enter(8, 0x1000, "Replacement", ButtonType, 202),
            Bind(8.2, Visual, Surface, 0, 0x1000),
            Tl(9, UiThread, XamlProvider.Xaml, "End")
        ]);
        var path = WriteMemory(context, events);
        var ambiguous = Run(() => MemoryTools.Memory(sessionPath: path, address: "0x1000"));
        Expect.True(ambiguous.Bool("ambiguous"), "Reused element addresses require an objectId");
        var candidates = ambiguous.Items("elements");
        Expect.Equal(2, candidates.Count, "Distinct element lifetimes");
        var old = candidates.Single(row => row.Text("name") == "Root").Long("objectId");
        var current = candidates.Single(row => row.Text("name") == "Replacement").Long("objectId");
        Expect.Equal(0L, Run(() => MemoryTools.Memory(sessionPath: path, objectId: old)).Long("total"), "Old element is not the new owner");
        Expect.Equal(1L, Run(() => MemoryTools.Memory(sessionPath: path, objectId: current)).Long("total"), "New element owns its explicit path");
    }

    private static void Coverage(McpTestContext context)
    {
        var missing = WriteMemory(context, Events().Where(item => item.Name != "VisualSurfaceChanged"));
        var partial = Run(() => MemoryTools.Memory(sessionPath: missing));
        Expect.Equal(1L, partial.Property("totals").Long("textures"), "Process textures do not require primitive bindings");
        Expect.Equal(0L, partial.Long("observedLinks"), "No ownership inferred from timing or the visual tree");
        HasNote(partial, "Element attribution needs a fresh capture");
        var noGpu = Run(() => MemoryTools.Memory(sessionPath: WriteMain(context)));
        HasNote(noGpu, "Missing textures are not zero GPU usage");
        var empty = Write(Path.Combine(context.TempDirectory, "empty.sqlite"), []);
        Expect.True(Run(() => MemoryTools.Memory(sessionPath: empty)).Property("point").IsNull("atMs"), "Empty recordings invent no time");

        using (var connection = new SqliteConnection($"Data Source={missing};Pooling=False"))
        {
            connection.Open();
            using var command = connection.CreateCommand();
            command.CommandText = "DROP TABLE GraphicsLinks; DROP TABLE GraphicsObjectEvents; DROP TABLE GraphicsObjects";
            command.ExecuteNonQuery();
        }
        HasNote(Run(() => MemoryTools.Memory(sessionPath: missing)), "no graphics lifetime index");
    }

    private static void Sizes(McpTestContext context)
    {
        var events = Events().Select(item => item.EventId == 33
            ? item with { Fields = item.Fields.Where(field => field.Name != "allocSize").ToArray() } : item).ToList();
        events.AddRange([
            Allocation(7, 35, 1000, 1000, 11, 0),
            Tl(8, UiThread, XamlProvider.Xaml, "End")
        ]);
        var path = WriteMemory(context, events);
        var unknown = Run(() => MemoryTools.Memory(view: "textures", sessionPath: path, atMs: 6));
        Expect.True(unknown.Items("textures").Single().IsNull("linkedAllocationBytes"), "Missing allocation size stays null");
        Expect.Equal(1L, unknown.Property("totals").Long("texturesWithoutCompleteAllocationSizes"), "Incomplete byte coverage");
        var zero = Run(() => MemoryTools.Memory(view: "textures", sessionPath: path));
        Expect.Equal("0", zero.Items("textures").Single().Text("linkedAllocationBytes"), "Measured zero stays a measured zero");
        Expect.Equal(0L, zero.Property("totals").Long("texturesWithoutCompleteAllocationSizes"), "Known zero is complete");
    }

    private static void Retention(McpTestContext context)
    {
        var path = Path.Combine(context.TempDirectory, "trimmed.sqlite");
        using (var recording = new FixtureRecording(path, limitBytes: 1024 * 1024))
        {
            recording.Add(Events());
            recording.Add(Enumerable.Range(0, 6000).Select(index =>
                Tl(100 + index, UiThread, XamlProvider.Xaml, "Filler", F("Value", new string('x', 256)))));
            recording.Close(EndReason);
        }
        Contains(Error(() => MemoryTools.Memory(sessionPath: path, atMs: 4)), "Trimmed time", "deleted by session retention");
        var current = Run(() => MemoryTools.Memory(sessionPath: path));
        Expect.Equal(0L, current.Long("observedLinks"), "Deleted association evidence is not reconstructed");
        HasNote(current, "oldest events");
        Expect.True(current.Property("totals").Long("texturesWithoutCompleteAllocationSizes") > 0,
            "A surviving open resource does not invent deleted allocation evidence");
    }

    private static void Recording(McpTestContext context)
    {
        var path = Path.Combine(context.TempDirectory, "recording.sqlite");
        using var recording = new FixtureRecording(path);
        recording.Add(Events());
        recording.WaitUntilStored();
        var first = Run(() => MemoryTools.Memory(sessionPath: path));
        Expect.Equal(3L, first.Long("observedLinks"), "Live transaction has the complete initial chain");
        HasNote(first, "still being recorded");
        recording.Add([
            Bind(7, Visual, 0, 0, 0), Bind(7.1, Visual, 0, 1, 0),
            Bind(8, Visual + 1, 0, 0, 0), Bind(8.1, Visual + 1, 0, 1, 0)
        ]);
        recording.WaitUntilStored();
        Expect.Equal(0L, Run(() => MemoryTools.Memory(sessionPath: path)).Long("observedLinks"), "A fresh read sees committed updates");
        recording.Close(EndReason);
    }

    private static void Arguments(McpTestContext context)
    {
        var path = WriteMemory(context, Events());
        Contains(Error(() => MemoryTools.Memory(view: "bad", sessionPath: path)), "Unknown view", "isn't a view of memory");
        Contains(Error(() => MemoryTools.Memory(view: "element", sessionPath: path)), "Missing element", "one of objectId and address");
        Contains(Error(() => MemoryTools.Memory(sessionPath: path, objectId: 1, address: "0x1000")), "Two identities", "not both");
        Contains(Error(() => MemoryTools.Memory(view: "summary", sessionPath: path, objectId: 1)), "Ignored filter", "doesn't read objectId");
        Contains(Error(() => MemoryTools.Memory(sessionPath: path, limit: 1)), "Ignored pagination", "doesn't read limit");
        Contains(Error(() => MemoryTools.Memory(view: "elements", sessionPath: path, limit: 501)), "Large page", "1 to 500");
        Contains(Error(() => MemoryTools.Memory(view: "elements", sessionPath: path, offset: -1)), "Negative page", "can't be negative");
        Contains(Error(() => MemoryTools.Memory(sessionPath: path, objectId: -1)), "Invalid identity", "must be positive");
        Contains(Error(() => MemoryTools.Memory(sessionPath: path, objectId: 9999)), "Missing identity", "No retained UI element");
        foreach (var time in new[] { -1d, double.NaN, double.PositiveInfinity })
            Contains(Error(() => MemoryTools.Memory(sessionPath: path, atMs: time)), "Invalid time", "finite and nonnegative");
        Contains(Error(() => MemoryTools.Memory(sessionPath: path, atMs: 999)), "Future time", "outside the stored recording");
        var missingOwner = Write(Path.Combine(context.TempDirectory, "missing-owner.sqlite"),
            Events().Select(item => item.Name == "VisualSurfaceChanged"
            ? item with { Fields = item.Fields.Where(field => field.Name != "UIElementId").ToArray() } : item));
        Contains(Error(() => MemoryTools.Memory(sessionPath: missingOwner)), "Missing owner field", "UIElementId");
        var unowned = Write(Path.Combine(context.TempDirectory, "unowned.sqlite"), Events().Append(Bind(7, Visual, Surface, 0, 0)));
        Contains(Error(() => MemoryTools.Memory(sessionPath: unowned)), "Unowned surface", "nonzero UIElementId");
        using var cancellation = new CancellationTokenSource();
        cancellation.Cancel();
        try
        {
            MemoryTools.Memory(sessionPath: path, cancellationToken: cancellation.Token).GetAwaiter().GetResult();
            throw new McpTestFailedException("A cancelled memory read must stop.");
        }
        catch (OperationCanceledException) { }
    }

    private static string WriteMemory(McpTestContext context, IEnumerable<FixtureEvent> events) =>
        Write(Path.Combine(context.TempDirectory, "memory.sqlite"), events);

    private static IEnumerable<FixtureEvent> Events() =>
    [
        Enter(1, 0x1000, "Root", GridType, 200), Enter(1.1, 0x2000, "Child", ButtonType, 201),
        Texture(2, 12),
        Allocation(2.1, 33, 1000, 1000, 11, ulong.MaxValue),
        Allocation(2.2, 36, 100, 1000, 11),
        Init(3, Surface), Observe(3.1, Surface),
        Init(3.2, Surface + 1), Observe(3.3, Surface + 1),
        Bind(4.1, Visual, Surface, 0, 0x1000), Bind(4.2, Visual, Surface + 1, 1, 0x1000),
        Bind(5.1, Visual + 1, Surface, 0, 0x2000),
        Tl(6, UiThread, XamlProvider.Xaml, "End")
    ];

    private static FixtureEvent Bind(double ms, ulong visual, ulong surface, uint role, ulong element) =>
        Tl(ms, UiThread, XamlProvider.XamlProfiler, "VisualSurfaceChanged",
            F("VisualId", visual), F("SurfaceId", surface), F("Role", role), F("UIElementId", element));

    private static FixtureEvent Init(double ms, ulong surface) =>
        Tl(ms, UiThread, XamlProvider.XamlProfiler, "DCompSurfaceInit", F("SurfaceId", surface));

    private static FixtureEvent Observe(double ms, ulong surface) =>
        Tl(ms, UiThread, XamlProvider.XamlProfiler, "DCompSurfaceTextureObserved", F("SurfaceId", surface),
            F("ResourceId", Resource), F("Width", 64u), F("Height", 64u), F("MipLevels", 1u), F("ArraySize", 1u),
            F("Format", 28u), F("SampleCount", 1u), F("OffsetX", -3), F("OffsetY", 7));

    private static FixtureEvent Texture(double ms, int id, uint resource = 11) =>
        Manifest(ms, UiThread, XamlProvider.Direct3D11, id == 13 ? "Texture2D/Stop" : "Texture2D/Start", id == 13 ? 2 : 1,
            F("pID3D11Resource", Resource), F("pID3D11Device", 10UL), F("hKMResource", resource),
            F("Width", 64u), F("Height", 64u), F("Format", 28u)) with
        { EventId = id, Version = 0 };

    private static FixtureEvent Allocation(double ms, int id, ulong handle, ulong global, uint resource, ulong size = 0) =>
        Manifest(ms, KernelThread, XamlProvider.DxgKrnl, id < 36 ? "AdapterAllocation" : "DeviceAllocation",
            (id - 33) % 3 + 1, F("hProcessId", (ulong)ProcessId), F("hVidMmAlloc", handle),
            F("hVidMmGlobalAlloc", global), F("hDevice", 10UL), F("pDxgAdapter", 20UL),
            F("hThunkResource", (ulong)resource), F("allocSize", size)) with
        { EventId = id, Version = id < 36 ? 3 : 2 };
}
