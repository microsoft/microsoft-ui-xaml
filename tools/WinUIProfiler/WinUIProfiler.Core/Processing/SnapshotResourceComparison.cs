using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Processing;

public sealed record SnapshotResourceRow(string Metric, int? ResourceIndex, bool IsBytes, decimal? Before, decimal? After,
    string? ResourceName = null, bool IsTracked = true)
{
    public decimal? Delta => Before.HasValue && After.HasValue ? After.Value - Before.Value : null;
}

public enum SnapshotResourceGroupKind
{
    Allocations, Composition, D2D, Wrappers
}

public sealed record SnapshotResourceEntry(int? ResourceIndex, string? ResourceName,
    IReadOnlyList<SnapshotResourceRow> Metrics);

public sealed record SnapshotResourceGroup(SnapshotResourceGroupKind Kind, IReadOnlyList<SnapshotResourceEntry> Entries);

public sealed class SnapshotResourceComparison
{
    // Fixed ABI slots mirror RenderingResourceDiagnostics::ResourceKind; append new kinds, never reorder.
    private static readonly (string Name, SnapshotResourceGroupKind Group)[] ResourceKinds =
    [
        ("Composition.ContainerVisual", SnapshotResourceGroupKind.Composition),
        ("Composition.SpriteVisual", SnapshotResourceGroupKind.Composition),
        ("Composition.LayerVisual", SnapshotResourceGroupKind.Composition),
        ("Composition.ShapeVisual", SnapshotResourceGroupKind.Composition),
        ("Composition.VisualSurface", SnapshotResourceGroupKind.Composition),
        ("Composition.ColorBrush", SnapshotResourceGroupKind.Composition),
        ("Composition.SurfaceBrush", SnapshotResourceGroupKind.Composition),
        ("Composition.MaskBrush", SnapshotResourceGroupKind.Composition),
        ("Composition.NineGridBrush", SnapshotResourceGroupKind.Composition),
        ("Composition.LinearGradientBrush", SnapshotResourceGroupKind.Composition),
        ("Composition.EffectBrush", SnapshotResourceGroupKind.Composition),
        ("Composition.Surface", SnapshotResourceGroupKind.Composition),
        ("Composition.VirtualSurface", SnapshotResourceGroupKind.Composition),
        ("D2D.SolidColorBrush", SnapshotResourceGroupKind.D2D),
        ("D2D.BitmapBrush", SnapshotResourceGroupKind.D2D),
        ("D2D.LinearGradientBrush", SnapshotResourceGroupKind.D2D),
        ("D2D.RadialGradientBrush", SnapshotResourceGroupKind.D2D),
        ("WinUI.DCompSurfaceWrapper", SnapshotResourceGroupKind.Wrappers),
        ("WinUI.D2DSolidColorBrushWrapper", SnapshotResourceGroupKind.Wrappers),
        ("WinUI.D2DBitmapBrushWrapper", SnapshotResourceGroupKind.Wrappers),
        ("WinUI.D2DLinearGradientBrushWrapper", SnapshotResourceGroupKind.Wrappers),
        ("WinUI.D2DRadialGradientBrushWrapper", SnapshotResourceGroupKind.Wrappers),
        ("WinUI.D2DStrokeStyleWrapper", SnapshotResourceGroupKind.Wrappers),
        ("WinUI.D2DGeometryWrapper", SnapshotResourceGroupKind.Wrappers),
        ("WinUI.D2DBitmapWrapper", SnapshotResourceGroupKind.Wrappers)
    ];

    public IReadOnlyList<SnapshotResourceRow> Rows { get; }
    public IReadOnlyList<SnapshotResourceGroup> Groups { get; }
    public IReadOnlyList<SnapshotResourceEntry> UnknownResources { get; }
    public bool HasChanges { get; }

    public SnapshotResourceComparison(SnapshotResourceCapture? before, SnapshotResourceCapture? after)
    {
        var old = before?.Values;
        var current = after?.Values;
        var rows = new List<SnapshotResourceRow>
        {
            new("AllocCount", null, false, Available(old?.AllocCount), Available(current?.AllocCount)),
            new("AllocSize", null, true, Available(old?.AllocSize), Available(current?.AllocSize)),
            new("AllocatedMemory", null, true, Available(old?.AllocatedMemory), Available(current?.AllocatedMemory)),
            new("OutstandingAllocCount", null, false, Available(old?.OutstandingAllocCount), Available(current?.OutstandingAllocCount)),
            new("OutstandingAllocSize", null, true, Available(old?.OutstandingAllocSize), Available(current?.OutstandingAllocSize))
        };
        var oldResources = old?.Resources.ToDictionary(resource => resource.Index) ?? [];
        var newResources = current?.Resources.ToDictionary(resource => resource.Index) ?? [];
        foreach (var index in oldResources.Keys.Union(newResources.Keys).Order())
        {
            oldResources.TryGetValue(index, out var previous);
            newResources.TryGetValue(index, out var next);
            var name = index >= 0 && index < ResourceKinds.Length ? ResourceKinds[index].Name : null;
            var hasLifetime = name is null || ResourceKinds[index].Group == SnapshotResourceGroupKind.Wrappers;
            rows.Add(new("TotalCreated", index, false, previous?.TotalCreated, next?.TotalCreated, name));
            rows.Add(new("TotalDestroyed", index, false,
                hasLifetime ? previous?.TotalDestroyed : null, hasLifetime ? next?.TotalDestroyed : null,
                name, hasLifetime));
            rows.Add(new("LiveCount", index, false,
                hasLifetime ? previous?.LiveCount : null, hasLifetime ? next?.LiveCount : null,
                name, hasLifetime));
        }
        Rows = rows.AsReadOnly();
        var groups = Enum.GetValues<SnapshotResourceGroupKind>()
            .ToDictionary(kind => kind, _ => new List<SnapshotResourceEntry>());
        groups[SnapshotResourceGroupKind.Allocations].AddRange(rows.Where(row => row.ResourceIndex is null)
            .Select(row => new SnapshotResourceEntry(null, null, new[] { row })));
        var unknown = new List<SnapshotResourceEntry>();
        foreach (var resource in rows.Where(row => row.ResourceIndex.HasValue).GroupBy(row => row.ResourceIndex!.Value))
        {
            var entry = new SnapshotResourceEntry(resource.Key, resource.First().ResourceName,
                resource.Where(row => row.IsTracked).ToArray());
            if (entry.ResourceName is null)
            {
                unknown.Add(entry);
            }
            else
            {
                groups[ResourceKinds[resource.Key].Group].Add(entry);
            }
        }
        Groups = Enum.GetValues<SnapshotResourceGroupKind>()
            .Select(kind => new SnapshotResourceGroup(kind, groups[kind].AsReadOnly())).ToArray();
        UnknownResources = unknown.AsReadOnly();
        HasChanges = before?.Status != after?.Status || before?.HResult != after?.HResult ||
            rows.Any(row => row.Before != row.After);
    }

    private static decimal? Available(long? value) => value is -1 ? null : value;
}
