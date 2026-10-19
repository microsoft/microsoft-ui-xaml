// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace OnlineXamlProfiler.Core;

public sealed class SnapshotTreeNode
{
    public SnapshotTreeNode(uint threadId, SnapshotNode node)
    {
        ThreadId = threadId;
        Node = node;
        string type = node.Type[(node.Type.LastIndexOf('.') + 1)..];
        Label = string.IsNullOrEmpty(node.Name)
            ? $"{type} (0x{node.Id})"
            : $"{type} \"{node.Name}\" (0x{node.Id})";
    }

    public uint ThreadId { get; }
    public SnapshotNode Node { get; }
    public string Label { get; }
    public List<SnapshotTreeNode> Children { get; } = [];
}

public static class SnapshotTree
{
    public static IReadOnlyList<SnapshotTreeNode> Build(SnapshotDocument document, TreeKind kind)
    {
        document.Validate();
        var roots = new List<SnapshotTreeNode>();
        foreach (var thread in document.Threads)
        {
            var nodes = thread.Nodes.Where(node => node.Tree == kind)
                .ToDictionary(node => node.Id, node => new SnapshotTreeNode(thread.ThreadId, node));
            foreach (var node in nodes.Values.OrderBy(node => node.Node.ChildIndex))
            {
                if (node.Node.ParentId.Length == 0)
                {
                    roots.Add(node);
                }
                else
                {
                    nodes[node.Node.ParentId].Children.Add(node);
                }
            }
        }
        return roots;
    }
}
