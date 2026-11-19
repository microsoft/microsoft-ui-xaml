using System.Globalization;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Snapshots;

public sealed record SnapshotObjectIdentity(ObjectKind Kind, uint ProcessId, long CreationTimeFileTime,
    ulong RootId, long Qpc, IReadOnlyList<long> Addresses)
{
    public static SnapshotObjectIdentity Create(TreeSnapshotDocument document, SnapshotTreeNode node, SnapshotTreeKind tree)
    {
        ArgumentNullException.ThrowIfNull(document);
        ArgumentNullException.ThrowIfNull(node);
        if (!Enum.IsDefined(tree)) throw new ArgumentOutOfRangeException(nameof(tree));
        var addresses = tree == SnapshotTreeKind.Xaml
            ? new[] { node.ObjectAddress, node.DiagnosticsHandle }.OfType<string>()
            : [node.VisualAddress ?? throw new ArgumentException("The selected node is not a composition visual.", nameof(node))];
        return new(tree == SnapshotTreeKind.Xaml ? ObjectKind.Element : ObjectKind.Visual,
            document.ProcessId, document.CreationTimeFileTime, node.RootId,
            document.Roots.Single(root => root.Id == node.RootId).EndQpc,
            Array.AsReadOnly(addresses.Select(Parse).Distinct().ToArray()));
    }

    // Unknown creation times cannot establish identity: PID equality alone is unsafe after PID reuse.
    public long? FindObject(string sessionPath, CancellationToken token = default)
    {
        token.ThrowIfCancellationRequested();
        var (processId, created) = SessionFile.ReadProcessIdentity(sessionPath);
        if (processId != ProcessId)
            throw new InvalidDataException("The session and snapshot belong to different process IDs.");
        if (created is null)
            throw new InvalidDataException("The session process creation time is unavailable; process instance identity cannot be verified.");
        if (created != CreationTimeFileTime)
            throw new InvalidDataException("The session and snapshot belong to different process instances.");
        token.ThrowIfCancellationRequested();
        var id = Kind == ObjectKind.Visual ? SessionFile.FindSnapshotVisual(sessionPath, Addresses[0], Qpc)
            : SessionFile.FindSnapshotElement(sessionPath, Addresses, Qpc);
        token.ThrowIfCancellationRequested();
        return id;
    }

    private static long Parse(string value) =>
        unchecked((long)ulong.Parse(value.StartsWith("0x", StringComparison.OrdinalIgnoreCase) ? value.AsSpan(2) : value,
            NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture));
}
