// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json.Serialization;

namespace OnlineXamlProfiler.Core;

[JsonNumberHandling(JsonNumberHandling.AllowReadingFromString)]
public sealed record SnapshotMemory
{
    public required int Status { get; init; }
    public required int HeapStatus { get; init; }
    public required string HeapId { get; init; }
    public required bool IsPrivateHeap { get; init; }
    public required ulong OutstandingBytes { get; init; }
    public required ulong OutstandingCount { get; init; }
    public required int CacheStatus { get; init; }
    public required uint CacheCapacity { get; init; }
    public required uint CachedSurfaceCount { get; init; }
    public required ulong DroppedRegistrations { get; init; }
    public required ulong CacheBytes { get; init; }

    internal void Validate()
    {
        if (!SnapshotDocument.IsAddress(HeapId) || CachedSurfaceCount > CacheCapacity ||
            (Status >= 0 && HeapStatus >= 0 && HeapId.Length == 0) ||
            (Status < 0 && (OutstandingBytes != 0 || OutstandingCount != 0 || CachedSurfaceCount != 0)) ||
            (HeapStatus < 0 && (OutstandingBytes != 0 || OutstandingCount != 0)))
        {
            throw new InvalidDataException("Invalid captured heap or observation-cache metadata.");
        }
    }
}

[JsonNumberHandling(JsonNumberHandling.AllowReadingFromString)]
public sealed record SnapshotSurface
{
    public required string VisualId { get; init; }
    public required string Role { get; init; }
    public required string CompositionSurfaceId { get; init; }
    public required int Status { get; init; }
    public required string SurfaceId { get; init; }
    public required ulong Generation { get; init; }
    public required uint Width { get; init; }
    public required uint Height { get; init; }
    public required uint Format { get; init; }
    public required bool IsVirtual { get; init; }
    public required ulong EstimatedBytes { get; init; }
    public required int TextureStatus { get; init; }
    public required string ResourceId { get; init; }
    public required uint TextureWidth { get; init; }
    public required uint TextureHeight { get; init; }
    public required uint MipLevels { get; init; }
    public required uint ArraySize { get; init; }
    public required uint TextureFormat { get; init; }
    public required uint SampleCount { get; init; }
    public required int OffsetX { get; init; }
    public required int OffsetY { get; init; }
    public required long ObservedQpc { get; init; }

    internal void Validate(HashSet<string> visuals, long endQpc)
    {
        if (!SnapshotDocument.IsAddress(VisualId, false) || !visuals.Contains(VisualId) ||
            string.IsNullOrWhiteSpace(Role) || Role.Length > 512 ||
            !SnapshotDocument.IsAddress(CompositionSurfaceId, false) ||
            !SnapshotDocument.IsAddress(SurfaceId) || !SnapshotDocument.IsAddress(ResourceId) ||
            ObservedQpc < 0 || ObservedQpc > endQpc ||
            (Status >= 0 && (SurfaceId.Length == 0 || Generation == 0)) ||
            (Status < 0 && (SurfaceId.Length != 0 || Generation != 0 || EstimatedBytes != 0 || TextureStatus >= 0)) ||
            (TextureStatus >= 0 && (Status < 0 || ResourceId.Length == 0 || ObservedQpc == 0 ||
                TextureWidth == 0 || TextureHeight == 0 || MipLevels == 0 || ArraySize == 0 || SampleCount == 0)))
        {
            throw new InvalidDataException("Invalid captured surface reference or texture observation.");
        }
    }
}

[JsonNumberHandling(JsonNumberHandling.AllowReadingFromString)]
public sealed record AdapterMemorySegment
{
    public required string AdapterLuid { get; init; }
    public required string Description { get; init; }
    public required uint Segment { get; init; }
    public required int Status { get; init; }
    public required ulong CurrentUsage { get; init; }
    public required ulong Budget { get; init; }
    public required ulong CurrentReservation { get; init; }
    public required ulong AvailableForReservation { get; init; }
}

[JsonNumberHandling(JsonNumberHandling.AllowReadingFromString)]
public sealed record AdapterMemorySnapshot
{
    public required int Status { get; init; }
    public required long StartQpc { get; init; }
    public required long EndQpc { get; init; }
    public required long QpcFrequency { get; init; }
    public required IReadOnlyList<AdapterMemorySegment> Adapters { get; init; }

    internal void Validate()
    {
        if (Adapters is null || Adapters.Count > 128 || StartQpc <= 0 || EndQpc < StartQpc || QpcFrequency <= 0)
        {
            throw new InvalidDataException("Invalid adapter memory metadata.");
        }
        var identities = new HashSet<(string, uint)>();
        foreach (var adapter in Adapters)
        {
            if (adapter is null || !SnapshotDocument.IsAddress(adapter.AdapterLuid) ||
                string.IsNullOrWhiteSpace(adapter.Description) || adapter.Segment > 1 ||
                !identities.Add((adapter.AdapterLuid, adapter.Segment)) ||
                (adapter.Status < 0 && (adapter.CurrentUsage != 0 || adapter.Budget != 0 ||
                    adapter.CurrentReservation != 0 || adapter.AvailableForReservation != 0)))
            {
                throw new InvalidDataException("Invalid or duplicate adapter memory segment.");
            }
        }
    }
}

public sealed record CoreAllocationGroup(string Type, int ObjectCount, int MeasuredCount, ulong MeasuredBytes);

public sealed record SnapshotMemorySummary(
    IReadOnlyList<CoreAllocationGroup> CoreByType, ulong MeasuredCoreBytes,
    int UnmeasuredCoreCount, ulong EstimatedSurfaceBytes, int UnknownSurfaceReferenceCount)
{
    public static SnapshotMemorySummary Build(SnapshotDocument document)
    {
        document.Validate();
        var core = document.Threads.Where(thread => thread.Status >= 0)
            .SelectMany(thread => thread.Nodes).Where(node => node.Tree == TreeKind.Xaml);
        var byType = core.GroupBy(node => node.Type).Select(group => new CoreAllocationGroup(
            group.Key, group.Count(), group.Count(node => node.CoreAllocationStatus >= 0),
            Sum(group.Where(node => node.CoreAllocationStatus >= 0).Select(node => node.CoreAllocationBytes))))
            .OrderByDescending(group => group.MeasuredBytes).ThenBy(group => group.Type, StringComparer.Ordinal).ToArray();
        var surfaces = document.Threads.Where(thread => thread.Status >= 0).SelectMany(thread => thread.Surfaces).ToArray();
        return new(byType, Sum(byType.Select(group => group.MeasuredBytes)),
            byType.Sum(group => group.ObjectCount - group.MeasuredCount),
            Sum(surfaces.Where(surface => surface.Status >= 0)
                .DistinctBy(surface => (surface.SurfaceId, surface.Generation)).Select(surface => surface.EstimatedBytes)),
            surfaces.Count(surface => surface.Status < 0));
    }

    private static ulong Sum(IEnumerable<ulong> values) =>
        values.Aggregate(0UL, (total, value) => checked(total + value));
}
