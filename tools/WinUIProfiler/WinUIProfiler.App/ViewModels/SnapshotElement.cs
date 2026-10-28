// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.App.ViewModels;

// A XAML snapshot node to show in the Elements tab: its process, when the snapshot was taken (QPC), and the WinRT object
// addresses the snapshot recorded for it (its IUnknown and its XAML diagnostics handle).
public sealed record SnapshotElement(uint ProcessId, long Qpc, string Description, IReadOnlyList<long> Addresses)
{
    public static SnapshotElement From(TreeSnapshotDocument document, SnapshotTreeNode node)
    {
        ArgumentNullException.ThrowIfNull(document);
        ArgumentNullException.ThrowIfNull(node);
        return new(document.ProcessId, document.EndQpc, ElementRow.Describe(node.Type, node.Name),
            [.. new[] { node.ObjectAddress, node.DiagnosticsHandle }.OfType<string>().Select(Parse).Distinct()]);
    }

    // The snapshot writes addresses in hex with a 0x prefix.
    private static long Parse(string address) =>
        long.Parse(address.StartsWith("0x", StringComparison.OrdinalIgnoreCase) ? address[2..] : address,
            NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture);
}
