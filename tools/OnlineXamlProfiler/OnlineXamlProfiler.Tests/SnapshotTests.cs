// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using OnlineXamlProfiler.Core;

namespace OnlineXamlProfiler.Tests;

[TestClass]
public sealed class SnapshotTests
{
    [TestMethod]
    public void BuildsOrderedTreesAndKeepsSnapshotsIndependent()
    {
        var xaml = Node(1) with { CompId = Id(10) };
        var comp = Node(10) with { Tree = TreeKind.Composition, XamlId = Id(1), VisualId = Id(20) };
        var visual = Node(20) with { Tree = TreeKind.Visual, XamlId = Id(1), CompId = Id(10), VisualId = Id(20) };
        var first = Document([xaml, Node(3, 1, 1), comp, visual, Node(2, 1, 0)]);
        var treeA = SnapshotTree.Build(SnapshotDocument.Deserialize(JsonSerializer.SerializeToUtf8Bytes(first)), TreeKind.Xaml);
        CollectionAssert.AreEqual(new[] { Id(2), Id(3) }, treeA.Single().Children.Select(child => child.Node.Id).ToArray());
        Assert.AreEqual(Id(10), treeA.Single().Node.CompId);
        Assert.AreEqual(Id(20), SnapshotTree.Build(first, TreeKind.Composition).Single().Node.VisualId);

        var second = Document([xaml with { Name = "Changed" }]) with { RequestId = "2" };
        var treeB = SnapshotTree.Build(second, TreeKind.Xaml);
        Assert.AreEqual("", treeA.Single().Node.Name);
        Assert.AreEqual(2, treeA.Single().Children.Count);
        Assert.AreEqual("Changed", treeB.Single().Node.Name);
        Assert.AreEqual(0, treeB.Single().Children.Count);
    }

    [TestMethod]
    public void RejectsCyclesMissingParentsAndDuplicateSiblings()
    {
        Assert.ThrowsException<InvalidDataException>(() => Document([Node(1, 2), Node(2, 1)]).Validate());
        Assert.ThrowsException<InvalidDataException>(() => Document([Node(1, 99)]).Validate());
        Assert.ThrowsException<InvalidDataException>(() => Document([Node(1), Node(2, 1), Node(3, 1)]).Validate());
        Assert.ThrowsException<InvalidDataException>(() => Document([Node(1), Node(1)]).Validate());
    }

    [TestMethod]
    public void ValidatesFailedThreadsAndCaptureIntervals()
    {
        var document = Document([]);
        var thread = document.Threads[0];
        (document with { Threads = [thread with { Status = -1, Error = "Unavailable" }] }).Validate();
        Assert.ThrowsException<InvalidDataException>(() =>
            (document with { Threads = [thread with { Status = -1, Error = "Unavailable", Nodes = [Node(1)] }] }).Validate());
        Assert.ThrowsException<InvalidDataException>(() =>
            (document with { Threads = [thread with { EndQpc = "9" }] }).Validate());
        Assert.ThrowsException<InvalidDataException>(() => (document with { SchemaVersion = 2 }).Validate());
        Assert.ThrowsException<InvalidDataException>(() => SnapshotDocument.Deserialize("{}"u8));
    }

    [TestMethod]
    public void DeepTreesDoNotRequireRecursiveValidationOrBuilding()
    {
        var nodes = Enumerable.Range(1, 12000).Select(value => Node(value, value - 1)).ToArray();
        var root = SnapshotTree.Build(Document(nodes), TreeKind.Xaml).Single();
        int count = 1;
        while (root.Children.Count != 0)
        {
            root = root.Children.Single();
            ++count;
        }
        Assert.AreEqual(nodes.Length, count);
    }

    private static string Id(int value) => value == 0 ? "" : value.ToString("X16");

    private static SnapshotNode Node(int id, int parent = 0, uint index = 0) => new()
    {
        Tree = TreeKind.Xaml, Id = Id(id), ParentId = Id(parent), ChildIndex = index,
        Type = "Microsoft.UI.Xaml.Controls.Grid", Name = "", XamlId = Id(id), CompId = "", VisualId = "",
        Properties = [new() { Name = "ActualWidth", Value = "100", Status = 0 }],
    };

    private static SnapshotDocument Document(IReadOnlyList<SnapshotNode> nodes) => new()
    {
        SchemaVersion = 1, ProcessId = 123, ProcessStartTime = "0000000000000010", RequestId = "1",
        Threads = [new() { ThreadId = 1, StartQpc = "10", EndQpc = "20", QpcFrequency = "1000", Status = 0, Error = "", Nodes = nodes }],
    };
}
