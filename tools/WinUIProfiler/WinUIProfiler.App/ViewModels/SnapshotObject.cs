// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Snapshots;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A snapshot node to show in the Objects tab: its process, when the capture of its root ended (QPC), and the addresses the
// snapshot recorded for it. A XAML node's are its WinRT object's IUnknown and its XAML diagnostics handle; a composition
// node's is its IVisual pointer.
public sealed record SnapshotObject(SnapshotObjectIdentity Identity, string Description)
{
    public ObjectKind Kind => Identity.Kind;
    public uint ProcessId => Identity.ProcessId;

    public static SnapshotObject FromXaml(TreeSnapshotDocument document, SnapshotTreeNode node)
    {
        ArgumentNullException.ThrowIfNull(document);
        ArgumentNullException.ThrowIfNull(node);
        return new(SnapshotObjectIdentity.Create(document, node, SnapshotTreeKind.Xaml),
            ObjectRow.Describe(ObjectKind.Element, node.Type, node.Name));
    }

    public static SnapshotObject FromComposition(TreeSnapshotDocument document, SnapshotTreeNode node)
    {
        ArgumentNullException.ThrowIfNull(document);
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(node.VisualAddress);
        return new(SnapshotObjectIdentity.Create(document, node, SnapshotTreeKind.Composition),
            $"{ObjectRow.Describe(ObjectKind.Visual, node.Type, null)} ({node.VisualAddress})");
    }

}
