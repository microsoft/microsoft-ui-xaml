// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A snapshot node to show in the Objects tab: its process, when the snapshot was taken (QPC), and the addresses the
// snapshot recorded for it. A XAML node's are its WinRT object's IUnknown and its XAML diagnostics handle; a composition
// node's is its IVisual pointer.
public sealed record SnapshotObject(ObjectKind Kind, uint ProcessId, long Qpc, string Description, IReadOnlyList<long> Addresses)
{
    public static SnapshotObject FromXaml(TreeSnapshotDocument document, SnapshotTreeNode node)
    {
        ArgumentNullException.ThrowIfNull(document);
        ArgumentNullException.ThrowIfNull(node);
        return new(ObjectKind.Element, document.ProcessId, document.EndQpc, ObjectRow.Describe(ObjectKind.Element, node.Type, node.Name),
            [.. new[] { node.ObjectAddress, node.DiagnosticsHandle }.OfType<string>().Select(Parse).Distinct()]);
    }

    public static SnapshotObject FromComposition(TreeSnapshotDocument document, SnapshotTreeNode node)
    {
        ArgumentNullException.ThrowIfNull(document);
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(node.VisualAddress);
        return new(ObjectKind.Visual, document.ProcessId, document.EndQpc,
            $"{ObjectRow.Describe(ObjectKind.Visual, node.Type, null)} ({node.VisualAddress})", [Parse(node.VisualAddress)]);
    }

    // The snapshot writes addresses in hex with a 0x prefix.
    private static long Parse(string address) =>
        long.Parse(address.StartsWith("0x", StringComparison.OrdinalIgnoreCase) ? address[2..] : address,
            NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture);
}
