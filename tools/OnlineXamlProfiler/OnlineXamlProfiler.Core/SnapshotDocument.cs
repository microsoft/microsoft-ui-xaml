// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace OnlineXamlProfiler.Core;

public enum TreeKind
{
    Xaml = 1,
    Composition = 2,
    Visual = 3,
}

public sealed record SnapshotProperty
{
    public required string Name { get; init; }
    public required string? Value { get; init; }
    public required int Status { get; init; }
}

public sealed record SnapshotNode
{
    public required TreeKind Tree { get; init; }
    public required string Id { get; init; }
    public required string ParentId { get; init; }
    public required uint ChildIndex { get; init; }
    public required string Type { get; init; }
    public required string Name { get; init; }
    public required string XamlId { get; init; }
    public required string CompId { get; init; }
    public required string VisualId { get; init; }
    public required IReadOnlyList<SnapshotProperty> Properties { get; init; }
    public required int CoreAllocationStatus { get; init; }
    [JsonNumberHandling(JsonNumberHandling.AllowReadingFromString)]
    public required ulong CoreAllocationBytes { get; init; }
}

public sealed record ThreadSnapshot
{
    public required uint ThreadId { get; init; }
    public required string StartQpc { get; init; }
    public required string EndQpc { get; init; }
    public required string QpcFrequency { get; init; }
    public required int Status { get; init; }
    public required string Error { get; init; }
    public required IReadOnlyList<SnapshotNode> Nodes { get; init; }
    public required SnapshotMemory Memory { get; init; }
    public required IReadOnlyList<SnapshotSurface> Surfaces { get; init; }
}

public sealed record SnapshotDocument
{
    public const int CurrentVersion = 2;
    public const int MaximumNodeCount = 100000;

    public required int SchemaVersion { get; init; }
    public required uint ProcessId { get; init; }
    public required string ProcessStartTime { get; init; }
    public required string RequestId { get; init; }
    public required IReadOnlyList<ThreadSnapshot> Threads { get; init; }
    public required AdapterMemorySnapshot AdapterMemory { get; init; }

    public static SnapshotDocument Deserialize(ReadOnlySpan<byte> utf8Json)
    {
        SnapshotDocument document;
        try
        {
            document = JsonSerializer.Deserialize<SnapshotDocument>(utf8Json)
                ?? throw new InvalidDataException("The snapshot is null.");
        }
        catch (JsonException exception)
        {
            throw new InvalidDataException("The snapshot JSON is invalid or incomplete.", exception);
        }
        document.Validate();
        return document;
    }

    public void Validate()
    {
        if (SchemaVersion != CurrentVersion || ProcessId == 0 || Threads is null || Threads.Count == 0 ||
            !IsAddress(ProcessStartTime, allowEmpty: false) ||
            !ulong.TryParse(RequestId, NumberStyles.None, CultureInfo.InvariantCulture, out _))
        {
            throw new InvalidDataException("Invalid snapshot metadata or unsupported schema version.");
        }

        var threadIds = new HashSet<uint>();
        if (AdapterMemory is null) { throw new InvalidDataException("Missing adapter memory metadata."); }
        AdapterMemory.Validate();
        int totalNodes = 0;
        int totalSurfaces = 0;
        foreach (var thread in Threads)
        {
            if (thread is null || thread.ThreadId == 0 || !threadIds.Add(thread.ThreadId) ||
                thread.Nodes is null || thread.Error is null || thread.Memory is null || thread.Surfaces is null)
            {
                throw new InvalidDataException("Invalid or duplicate snapshot UI thread.");
            }
            totalNodes = checked(totalNodes + thread.Nodes.Count);
            totalSurfaces = checked(totalSurfaces + thread.Surfaces.Count);
            if (totalNodes > MaximumNodeCount || totalSurfaces > MaximumNodeCount)
            {
                throw new InvalidDataException("The snapshot exceeds the node limit.");
            }
            if (thread.Status < 0)
            {
                if (thread.Nodes.Count != 0 || thread.Surfaces.Count != 0 || string.IsNullOrWhiteSpace(thread.Error))
                {
                    throw new InvalidDataException("A failed thread capture must have an error and no partial tree.");
                }
                continue;
            }
            if (!long.TryParse(thread.StartQpc, NumberStyles.None, CultureInfo.InvariantCulture, out long start) ||
                !long.TryParse(thread.EndQpc, NumberStyles.None, CultureInfo.InvariantCulture, out long end) ||
                !long.TryParse(thread.QpcFrequency, NumberStyles.None, CultureInfo.InvariantCulture, out long frequency) ||
                end < start || frequency <= 0)
            {
                throw new InvalidDataException("Invalid snapshot capture interval.");
            }

            foreach (var kind in Enum.GetValues<TreeKind>())
            {
                ValidateTree(thread.Nodes, kind);
            }
            if (thread.Nodes.Any(node => !Enum.IsDefined(node.Tree)))
            {
                throw new InvalidDataException("Unknown snapshot tree kind.");
            }
            thread.Memory.Validate();
            var visuals = thread.Nodes.Where(node => node.Tree == TreeKind.Visual).Select(node => node.Id)
                .ToHashSet(StringComparer.Ordinal);
            foreach (var surface in thread.Surfaces)
            {
                if (surface is null) { throw new InvalidDataException("Null captured surface."); }
                surface.Validate(visuals, end);
            }
        }
    }

    private static void ValidateTree(IReadOnlyList<SnapshotNode> nodes, TreeKind kind)
    {
        var byId = new Dictionary<string, SnapshotNode>(StringComparer.Ordinal);
        var siblingIndices = new HashSet<(string, uint)>();
        foreach (var node in nodes)
        {
            if (node is null)
            {
                throw new InvalidDataException("Null snapshot node.");
            }
            if (node.Tree != kind)
            {
                continue;
            }
            if (!IsAddress(node.Id, allowEmpty: false) || !IsAddress(node.ParentId) ||
                !IsAddress(node.XamlId) || !IsAddress(node.CompId) || !IsAddress(node.VisualId) ||
                string.IsNullOrWhiteSpace(node.Type) || node.Name is null || node.Properties is null ||
                !byId.TryAdd(node.Id, node) || !siblingIndices.Add((node.ParentId, node.ChildIndex)))
            {
                throw new InvalidDataException("Invalid node identity, metadata, or sibling order.");
            }
            if ((node.CoreAllocationStatus >= 0 && (kind != TreeKind.Xaml || node.CoreAllocationBytes == 0)) ||
                (node.CoreAllocationStatus < 0 && node.CoreAllocationBytes != 0))
            {
                throw new InvalidDataException("Invalid core allocation measurement.");
            }
            foreach (var property in node.Properties)
            {
                if (property is null || string.IsNullOrEmpty(property.Name) ||
                    (property.Status >= 0 && property.Value is null))
                {
                    throw new InvalidDataException("Invalid captured property.");
                }
            }
        }

        var state = new Dictionary<string, byte>(StringComparer.Ordinal);
        foreach (var node in byId.Values)
        {
            var path = new List<string>();
            string current = node.Id;
            while (current.Length != 0)
            {
                if (!byId.TryGetValue(current, out var ancestor))
                {
                    throw new InvalidDataException("The snapshot contains a missing parent.");
                }
                if (state.TryGetValue(current, out byte mark))
                {
                    if (mark == 1)
                    {
                        throw new InvalidDataException("The snapshot contains a tree cycle.");
                    }
                    break;
                }
                state[current] = 1;
                path.Add(current);
                current = ancestor.ParentId;
            }
            foreach (string id in path)
            {
                state[id] = 2;
            }
        }
    }

    internal static bool IsAddress(string? text, bool allowEmpty = true) =>
        text is not null && ((allowEmpty && text.Length == 0) ||
        (text.Length == 16 && ulong.TryParse(text, NumberStyles.AllowHexSpecifier,
            CultureInfo.InvariantCulture, out ulong address) && address != 0 &&
            text == address.ToString("X16", CultureInfo.InvariantCulture)));
}
