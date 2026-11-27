namespace WinUIProfiler.Engine.Processing;

public enum SnapshotTextureStatus
{
    Available, NotRecorded, NoEvents, DifferentProcess, DifferentClock, OutsideRecording, HistoryTrimmed, Unavailable
}

public sealed record SnapshotTextureAllocation(long DeviceAllocationId, string DeviceAllocationHandle,
    string KernelResource, string? KernelDevice, long? AdapterAllocationId, string? GlobalAllocation,
    string? Adapter, ulong? SizeBytes, bool Trimmed);

public sealed record SnapshotTexture(long Id, string Kind, string Handle, string? DeviceHandle,
    long? StartQpc, long? EndQpc, string? EndReason, bool Trimmed, string Descriptor,
    IReadOnlyList<SnapshotTextureAllocation> Allocations)
{
    public long KnownFromQpc { get; init; }

    public decimal? LinkedBytes => Allocations.Count == 0 ||
        Allocations.Any(allocation => allocation.AdapterAllocationId is null || allocation.SizeBytes is null)
            ? null : Allocations.DistinctBy(allocation => allocation.AdapterAllocationId)
                .Sum(allocation => (decimal)allocation.SizeBytes!.Value);
}

public sealed record SnapshotTextureCapture(SnapshotTextureStatus Status, Guid? SessionId,
    IReadOnlyList<SnapshotTexture> Textures)
{
    public long AtQpc { get; init; }
    public long StartQpc { get; init; }
    public long QpcFrequency { get; init; }
    public bool RecordingActive { get; init; }
    public bool Trimmed { get; init; }
    public int? LostByEtw { get; init; }
    public long? DroppedByProfiler { get; init; }
    public string? Error { get; init; }
    public bool Available => Status == SnapshotTextureStatus.Available;
    public decimal? KnownLinkedBytes => !Available ? null : Textures.SelectMany(texture => texture.Allocations)
        .Where(allocation => allocation.AdapterAllocationId.HasValue && allocation.SizeBytes.HasValue)
        .DistinctBy(allocation => allocation.AdapterAllocationId).Sum(allocation => (decimal)allocation.SizeBytes!.Value);
}

public enum SnapshotTextureChange { Unchanged, Added, Removed, Changed, NotComparable }

public sealed record SnapshotTextureDifference(SnapshotTexture? Before, SnapshotTexture? After,
    SnapshotTextureChange Change)
{
    public decimal? Delta => Change == SnapshotTextureChange.NotComparable ? null
        : (After is null ? 0m : After.LinkedBytes) - (Before is null ? 0m : Before.LinkedBytes);
}

public sealed class SnapshotTextureComparison
{
    public SnapshotTextureCapture? Before { get; }
    public SnapshotTextureCapture After { get; }
    public bool Comparable { get; }
    public IReadOnlyList<SnapshotTextureDifference> Rows { get; }
    public bool HasChanges => Rows.Any(row => row.Change is SnapshotTextureChange.Added or
        SnapshotTextureChange.Removed or SnapshotTextureChange.Changed);
    public int? CountDelta => Comparable ? After.Textures.Count - Before!.Textures.Count : null;
    public decimal? KnownBytesDelta => Comparable ? After.KnownLinkedBytes - Before!.KnownLinkedBytes : null;

    public SnapshotTextureComparison(SnapshotTextureCapture? before, SnapshotTextureCapture after)
    {
        ArgumentNullException.ThrowIfNull(after);
        Before = before;
        After = after;
        Comparable = before?.Available == true && after.Available &&
            before.SessionId.HasValue && before.SessionId == after.SessionId;
        var rows = new List<SnapshotTextureDifference>();
        if (Comparable)
        {
            var old = before!.Textures.ToDictionary(texture => texture.Id);
            foreach (var texture in after.Textures)
            {
                old.Remove(texture.Id, out var previous);
                rows.Add(new(previous, texture, previous is null ? SnapshotTextureChange.Added
                    : Same(previous, texture) ? SnapshotTextureChange.Unchanged : SnapshotTextureChange.Changed));
            }
            rows.AddRange(old.Values.Select(texture =>
                new SnapshotTextureDifference(texture, null, SnapshotTextureChange.Removed)));
        }
        else
        {
            rows.AddRange(after.Textures.Select(texture =>
                new SnapshotTextureDifference(null, texture, SnapshotTextureChange.NotComparable)));
            if (before is not null)
                rows.AddRange(before.Textures.Select(texture =>
                    new SnapshotTextureDifference(texture, null, SnapshotTextureChange.NotComparable)));
        }
        Rows = rows.OrderBy(row => row.Change).ThenBy(row => (row.After ?? row.Before!).Kind, StringComparer.Ordinal)
            .ThenBy(row => (row.After ?? row.Before!).Handle, StringComparer.Ordinal)
            .ThenBy(row => (row.After ?? row.Before!).Id).ToArray();
    }

    private static bool Same(SnapshotTexture before, SnapshotTexture after) =>
        before.Kind == after.Kind && before.Handle == after.Handle && before.DeviceHandle == after.DeviceHandle &&
        before.Descriptor == after.Descriptor &&
        before.Allocations.OrderBy(allocation => (allocation.DeviceAllocationId, allocation.AdapterAllocationId))
            .SequenceEqual(after.Allocations.OrderBy(allocation => (allocation.DeviceAllocationId, allocation.AdapterAllocationId)));
}
