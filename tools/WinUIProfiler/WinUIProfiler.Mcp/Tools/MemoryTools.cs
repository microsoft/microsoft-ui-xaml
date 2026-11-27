// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using System.Text.Json;
using ModelContextProtocol;
using ModelContextProtocol.Server;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Sessions;

namespace WinUIProfiler.Mcp.Tools;

[McpServerToolType]
internal static class MemoryTools
{
    internal const string NoMemory = "No retained Direct3D11 or XAML-Profiler GPU memory evidence was recorded.";
    internal static readonly string[] Views = ["summary", "elements", "element", "textures"];

    [McpServerTool(Name = "memory", ReadOnly = true, Idempotent = true, OpenWorld = false), Description(
        "Traces observed GPU memory relationships without tree snapshots: UI element -> rendered visual -> brush/mask " +
        "surface -> observed BeginDraw update texture -> device allocation -> adapter allocation. Reads a WinUI Profiler " +
        "session, at the latest stored event or atMs on its time base. Views: summary (default), elements (ranked by known " +
        "linked allocation bytes), element (a page of one element's explicit links), textures (a page of process texture " +
        "lifetimes open at that time, including those with no element link). Element objectIds and addresses are the same " +
        "as the objects tool's. Element attribution requires fresh recordings with UIElementId on the runtime's " +
        "VisualSurfaceChanged event. Update textures can be transient and shared atlases: " +
        "linked allocation bytes are NOT exclusive per-element bytes, resident VRAM, or proof of a leak. Shared adapter " +
        "allocations are counted once within each subtotal; never add per-element subtotals together. Byte amounts are " +
        "exact decimal strings; missing sizes remain null. All results give coverage and limitations.")]
    public static Task<object> Memory(
        [Description("\"summary\", \"elements\", \"element\" or \"textures\". Omit for summary, or element when objectId or address is given.")]
        string? view = null,
        [Description(SessionCatalog.SessionPathDescription)] string? sessionPath = null,
        [Description("The element view: an Element objectId returned by objects or memory's elements view, not a graphics ID.")]
        long? objectId = null,
        [Description("The element view: a core UI element address, in hex with 0x or decimal. Reused addresses list candidates; choose an objectId.")]
        string? address = null,
        [Description("Time in milliseconds on session.timeBase. Omit for the latest stored event. Deleted history and times outside the recording are errors.")]
        double? atMs = null,
        [Description("The elements, element and textures views: how many rows to skip for the next page.")] int offset = 0,
        [Description("The elements, element and textures views: most rows to return, from 1 to 500; default 50.")] int? limit = null,
        CancellationToken cancellationToken = default)
    {
        var addressText = ToolArguments.Text(address);
        var chosen = ToolArguments.Text(view) is not null ? ToolArguments.View(view, Views, "memory")
            : objectId is not null || addressText is not null ? "element" : "summary";
        ToolArguments.OnlyFor(objectId is not null, nameof(objectId), "memory", chosen, "element");
        ToolArguments.OnlyFor(addressText is not null, nameof(address), "memory", chosen, "element");
        ToolArguments.OnlyFor(offset != 0, nameof(offset), "memory", chosen, "elements", "element", "textures");
        ToolArguments.OnlyFor(limit is not null, nameof(limit), "memory", chosen, "elements", "element", "textures");
        ToolArguments.NotNegative(offset, nameof(offset));
        var count = ToolArguments.Range(limit ?? 50, 1, 500, nameof(limit));
        if (atMs is { } time && (!double.IsFinite(time) || time < 0))
            throw new McpException("atMs must be finite and nonnegative.");
        if (objectId is <= 0) throw new McpException("objectId must be positive.");
        if (chosen == "element" && (objectId is null) == (addressText is null))
            throw new McpException("The element view needs one of objectId and address, not both or neither.");
        var wantedAddress = addressText is null ? (ulong?)null : unchecked((ulong)ToolArguments.Address(addressText));
        return SessionCatalog.RunAsync(sessionPath, path =>
            Read(path, chosen, objectId, wantedAddress, atMs, offset, count, cancellationToken), cancellationToken);
    }

    private static ToolAnswer Read(string path, string view, long? objectId, ulong? address, double? atMs,
        int offset, int count, CancellationToken token)
    {
        var memory = SessionFile.ReadGpuMemory(path, atMs, token);
        var notes = Notes(memory);
        var textures = memory.Textures.ToDictionary(texture => texture.Id);
        var header = SessionCatalog.Header(path, memory.Session);
        var point = new
        {
            atMs = EventJson.Ms(memory.AtMilliseconds),
            atQpc = memory.AtQpc?.ToString(CultureInfo.InvariantCulture),
        };
        const string Computed = "Replayed retained explicit element/visual, visual/surface and surface/update-texture " +
            "events in QPC then sequence order up to the selected time. Matched resource handles only to unambiguous " +
            "open texture lifetimes already observed at the update's time, and read their time-scoped allocation links. " +
            "Byte subtotals count each known adapter-allocation ID once, without estimating missing sizes. ";
        switch (view)
        {
            case "summary":
                return new(new
                {
                    session = header, view, point, coverage = memory.Coverage,
                    totals = Totals(memory.Textures),
                    elementsWithTextureObservations = memory.Links.Select(link => link.ObjectId).Distinct().Count(),
                    observedLinks = memory.Links.Count,
                    unresolvedLinks = memory.Links.Count(link => link.TextureId is null),
                    notes,
                }, Computed + "Counted all process textures and explicit element-to-update-texture observations.");
            case "textures":
            {
                var ordered = memory.Textures.OrderByDescending(texture => texture.LinkedBytes ?? -1)
                    .ThenBy(texture => texture.Id).ToArray();
                var page = ordered.Skip(offset).Take(count).Select(texture => TextureRow(memory, texture)).ToArray();
                return new(new
                {
                    session = header, view, point, coverage = memory.Coverage, totals = Totals(memory.Textures),
                    total = ordered.Length, offset, returned = page.Length,
                    nextOffset = ToolArguments.NextOffset(offset, page.Length, ordered.Length),
                    textures = page, notes,
                }, Computed + "Sorted process textures by known linked bytes, most first, unknown last, then texture ID; " +
                    EventJson.Page(offset, page.Length) + ".");
            }
            case "elements":
            {
                var rows = memory.Links.GroupBy(link => link.ObjectId).ToDictionary(group => group.Key, group => group.ToArray());
                var ordered = memory.Elements.Where(element => rows.ContainsKey(element.ObjectId))
                    .OrderByDescending(element => KnownBytes(LinkedTextures(rows[element.ObjectId], textures)))
                    .ThenBy(element => element.ObjectId).ToArray();
                var page = ordered.Skip(offset).Take(count).Select(element => new
                {
                    element = ElementRow(element),
                    observedLinks = rows[element.ObjectId].Length,
                    unresolvedLinks = rows[element.ObjectId].Count(link => link.TextureId is null),
                    totals = Totals(LinkedTextures(rows[element.ObjectId], textures)),
                }).ToArray();
                return new(new
                {
                    session = header, view, point, coverage = memory.Coverage,
                    total = ordered.Length, offset, returned = page.Length,
                    nextOffset = ToolArguments.NextOffset(offset, page.Length, ordered.Length),
                    elements = page, notes,
                }, Computed + "Grouped observations by element objectId, counted distinct linked textures and allocations " +
                    "per element, and sorted by known allocation bytes, most first, then objectId; " + EventJson.Page(offset, page.Length) + ".");
            }
            default:
            {
                var matches = memory.Elements.Where(element => objectId is { } id
                    ? element.ObjectId == id : element.Address == address).ToArray();
                if (matches.Length == 0)
                    throw new McpException("No retained UI element at this time matches the requested objectId or address. " +
                        "The objects tool lists elements; history may have been deleted.");
                if (matches.Length > 1)
                {
                    var candidates = matches.Skip(offset).Take(count).Select(ElementRow).ToArray();
                    return new(new
                    {
                        session = header, view, point, coverage = memory.Coverage, ambiguous = true,
                        message = "Several retained element lifetimes had this address. Call memory again with one objectId.",
                        total = matches.Length, offset, returned = candidates.Length,
                        nextOffset = ToolArguments.NextOffset(offset, candidates.Length, matches.Length),
                        elements = candidates, notes,
                    }, Computed + "Listed the retained element lifetimes at the requested address; " +
                        EventJson.Page(offset, candidates.Length) + ".");
                }
                var selected = matches[0];
                var links = memory.Links.Where(link => link.ObjectId == selected.ObjectId).ToArray();
                var page = links.Skip(offset).Take(count).Select(link => new
                {
                    visualAddress = Hex(link.VisualAddress),
                    surfaceAddress = Hex(link.SurfaceAddress),
                    role = link.Role == 0 ? "Brush" : link.Role == 1 ? "Mask" : "Unknown",
                    roleId = link.Role,
                    elementLinkMs = Ms(memory, link.ElementLinkQpc),
                    surfaceLinkMs = Ms(memory, link.SurfaceLinkQpc),
                    observation = new
                    {
                        seq = link.Observation.Sequence, ms = Ms(memory, link.Observation.Qpc),
                        resourceAddress = Hex(link.Observation.ResourceAddress),
                        width = link.Observation.Width, height = link.Observation.Height,
                        mipLevels = link.Observation.MipLevels, arraySize = link.Observation.ArraySize,
                        format = link.Observation.Format, sampleCount = link.Observation.SampleCount,
                        offsetX = link.Observation.OffsetX, offsetY = link.Observation.OffsetY,
                    },
                    status = link.Status.ToString(),
                    texture = link.TextureId is { } textureId ? TextureRow(memory, textures[textureId]) : null,
                }).ToArray();
                if (links.Length == 0)
                    notes.Add("No complete retained element/visual/surface/update-texture observation chain was found. " +
                        "This does not mean the element uses zero GPU memory.");
                return new(new
                {
                    session = header, view, point, coverage = memory.Coverage, element = ElementRow(selected),
                    totals = Totals(LinkedTextures(links, textures)),
                    total = links.Length, offset, returned = page.Length,
                    nextOffset = ToolArguments.NextOffset(offset, page.Length, links.Length),
                    links = page, notes,
                }, Computed + "Kept the requested element's observations; " + EventJson.Page(offset, page.Length) + ".");
            }
        }
    }

    private static object ElementRow(GpuMemoryElement element) => new
    {
        objectId = element.ObjectId, address = Hex(element.Address), type = element.Type, name = element.Name,
    };

    private static object TextureRow(SessionGpuMemory memory, SnapshotTexture texture) => new
    {
        textureId = texture.Id, kind = texture.Kind, handle = texture.Handle, deviceHandle = texture.DeviceHandle,
        startMs = texture.StartQpc is { } start ? Ms(memory, start) : null,
        trimmed = texture.Trimmed,
        descriptor = JsonSerializer.Deserialize<JsonElement>(texture.Descriptor),
        linkedAllocationBytes = texture.LinkedBytes?.ToString(CultureInfo.InvariantCulture),
        observedElementIds = memory.Links.Where(link => link.TextureId == texture.Id)
            .Select(link => link.ObjectId).Distinct().Order().ToArray(),
        allocations = texture.Allocations.Select(allocation => new
        {
            deviceAllocationId = allocation.DeviceAllocationId,
            deviceAllocationHandle = allocation.DeviceAllocationHandle,
            kernelResource = allocation.KernelResource, kernelDevice = allocation.KernelDevice,
            adapterAllocationId = allocation.AdapterAllocationId,
            globalAllocation = allocation.GlobalAllocation, adapter = allocation.Adapter,
            sizeBytes = allocation.SizeBytes?.ToString(CultureInfo.InvariantCulture),
            trimmed = allocation.Trimmed,
        }).ToArray(),
    };

    private static SnapshotTexture[] LinkedTextures(IEnumerable<GpuElementTextureLink> links,
        IReadOnlyDictionary<long, SnapshotTexture> textures) =>
        [.. links.Where(link => link.TextureId.HasValue).Select(link => link.TextureId!.Value)
            .Distinct().Select(id => textures[id])];

    private static decimal KnownBytes(IEnumerable<SnapshotTexture> textures) => textures
        .SelectMany(texture => texture.Allocations)
        .Where(allocation => allocation.AdapterAllocationId.HasValue && allocation.SizeBytes.HasValue)
        .DistinctBy(allocation => allocation.AdapterAllocationId)
        .Sum(allocation => (decimal)allocation.SizeBytes!.Value);

    private static object Totals(IReadOnlyCollection<SnapshotTexture> textures) => new
    {
        textures = textures.Count,
        texturesWithoutCompleteAllocationSizes = textures.Count(texture => texture.LinkedBytes is null),
        knownLinkedAllocationBytes = KnownBytes(textures).ToString(CultureInfo.InvariantCulture),
        knownAdapterAllocations = textures.SelectMany(texture => texture.Allocations)
            .Where(allocation => allocation.AdapterAllocationId.HasValue && allocation.SizeBytes.HasValue)
            .Select(allocation => allocation.AdapterAllocationId).Distinct().Count(),
    };

    private static List<string> Notes(SessionGpuMemory memory)
    {
        var notes = new List<string>(SessionCatalog.Notes(memory.Session))
        {
            "Surface observations describe transient BeginDraw update textures, not exclusive element backing storage. " +
                "Atlases and allocations can be shared. Linked allocation bytes are not resident VRAM or proof of a leak; " +
                "do not add per-element subtotals together.",
            "Only explicit retained observations are linked. Missing sizes remain unknown; known-byte subtotals exclude " +
                "unknown allocations. An open recorded resource lifetime does not prove that the resource is still alive.",
        };
        if (!memory.Coverage.GraphicsIndex)
            notes.Add("This older session has no graphics lifetime index; record a new session for texture/allocation analysis.");
        if (!memory.Coverage.Direct3D)
            notes.Add("No retained Direct3D11 resource events precede this time. Missing textures are not zero GPU usage.");
        if (!memory.Coverage.ElementVisuals)
            notes.Add("No retained VisualSurfaceChanged events with UIElementId precede this time. Element attribution needs a " +
                "fresh capture with the updated runtime, the profiler provider enabled, and rendering after collection starts.");
        if (!memory.Coverage.VisualSurfaces)
            notes.Add("No retained VisualSurfaceChanged events precede this time; visual-to-surface attribution is unavailable.");
        if (!memory.Coverage.SurfaceTextures)
            notes.Add("No retained DCompSurfaceTextureObserved events precede this time; surface-to-update-texture attribution is unavailable.");
        if (memory.Links.Any(link => link.TextureId is null))
            notes.Add("Some observed resource addresses have no unambiguous matching texture lifetime at this time. " +
                "They remain unresolved and contribute no invented allocation bytes.");
        return notes;
    }

    private static string Hex(ulong value) => $"0x{value:x16}";
    private static double? Ms(SessionGpuMemory memory, long qpc) =>
        memory.StartQpc is { } start ? EventJson.Ms((qpc - start) * 1000.0 / memory.QpcFrequency) : null;
}
