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
        Assert.ThrowsException<InvalidDataException>(() => (document with { SchemaVersion = 3 }).Validate());
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

    [TestMethod]
    public void FrozenMemoryKeepsPreciseBytesAndDeduplicatesSharedSurfaces()
    {
        const ulong exactBytes = 9007199254740993;
        var nodes = new[]
        {
            Node(1) with { CoreAllocationStatus = 0, CoreAllocationBytes = exactBytes },
            Node(2, 1),
            Node(10) with { Tree = TreeKind.Visual, VisualId = Id(10) },
        };
        var document = Document(nodes);
        var surface = Surface();
        document = document with { Threads = [document.Threads[0] with { Surfaces = [surface, surface with { Role = "Brush.Mask" }] }] };
        var restored = SnapshotDocument.Deserialize(JsonSerializer.SerializeToUtf8Bytes(document));
        var summary = SnapshotMemorySummary.Build(restored);
        Assert.AreEqual(exactBytes, summary.MeasuredCoreBytes);
        Assert.AreEqual(1, summary.UnmeasuredCoreCount);
        Assert.AreEqual(8192UL, summary.EstimatedSurfaceBytes);
        var next = document with { Threads = [document.Threads[0] with { Surfaces = [surface with { Generation = 2, TextureStatus = -1, ResourceId = "", ObservedQpc = 0 }] }] };
        next.Validate();
        Assert.AreEqual("000000000000001E", restored.Threads[0].Surfaces[0].ResourceId);
        Assert.AreEqual(1UL, restored.Threads[0].Surfaces[0].Generation);
    }

    [TestMethod]
    public void RejectsInventedTextureAndHeapMeasurements()
    {
        var document = Document([Node(10) with { Tree = TreeKind.Visual, VisualId = Id(10) }]);
        var thread = document.Threads[0];
        foreach (var surface in new[]
        {
            Surface() with { VisualId = Id(99) },
            Surface() with { ObservedQpc = 21 },
            Surface() with { Generation = 0 },
            Surface() with { TextureStatus = 0, ResourceId = "" },
            Surface() with { Status = -1 },
        })
        {
            Assert.ThrowsException<InvalidDataException>(() =>
                (document with { Threads = [thread with { Surfaces = [surface] }] }).Validate());
        }
        Assert.ThrowsException<InvalidDataException>(() =>
            (document with { Threads = [thread with { Memory = thread.Memory with { OutstandingBytes = 1 } }] }).Validate());
        (document with { Threads = [thread with { Memory = thread.Memory with
        {
            Status = 0, HeapStatus = 0, HeapId = Id(100), OutstandingBytes = 0, OutstandingCount = 0,
        } }] }).Validate();
    }

    private static SnapshotSurface Surface() => new()
    {
        VisualId = Id(10), Role = "Brush", CompositionSurfaceId = Id(20), Status = 0, SurfaceId = Id(21),
        Generation = 1, Width = 64, Height = 32, Format = 87, IsVirtual = false, EstimatedBytes = 8192,
        TextureStatus = 0, ResourceId = Id(30), TextureWidth = 1024, TextureHeight = 1024,
        MipLevels = 1, ArraySize = 1, TextureFormat = 87, SampleCount = 1, OffsetX = 10, OffsetY = 20, ObservedQpc = 5,
    };

    [TestMethod]
    public void JoinsGpuEvidenceByProcessAndObservationTimeWithoutChangingFrozenSnapshots()
    {
        var document = Document([Node(10) with { Tree = TreeKind.Visual, VisualId = Id(10) }]);
        document = document with { Threads = [document.Threads[0] with { Surfaces = [Surface(), Surface() with { Role = "Mask" }] }] };
        var ledger = new GpuObservationLedger();
        ledger.Add(new(GpuObservationKind.AdapterAllocation, 1, 0, AllocationId: 50, AllocationBytes: 16384, OwnerProcessId: 123));
        ledger.Add(new(GpuObservationKind.DeviceAllocation, 2, 123, KernelResource: 40, AllocationId: 50));
        ledger.Add(new(GpuObservationKind.DeviceAllocation, 2, 999, KernelResource: 40, AllocationId: 99));
        ledger.Add(new(GpuObservationKind.Resource, 3, 123, ResourceId: 30, KernelResource: 40));
        ledger.Add(new(GpuObservationKind.Present, 4, 123));
        ledger.Add(new(GpuObservationKind.Present, 4, 999));
        var first = ledger.Freeze(document, "Collecting", "", true, 1, 0, 0);
        Assert.AreEqual(16384UL, first.DistinctReportedAllocationBytes);
        Assert.AreEqual(1, first.Resources.Count);
        Assert.AreEqual(1L, first.ObservedPresentCount);
        Assert.AreEqual(123U, first.Resources[0].OwnerProcessId);

        ledger.Add(new(GpuObservationKind.AdapterAllocation, 6, 0, AllocationId: 51, AllocationBytes: 32768, OwnerProcessId: 456));
        ledger.Add(new(GpuObservationKind.DeviceAllocation, 7, 123, KernelResource: 41, AllocationId: 51));
        ledger.Add(new(GpuObservationKind.Resource, 8, 123, ResourceId: 30, KernelResource: 41));
        var secondDocument = document with { Threads = [document.Threads[0] with { Surfaces = [Surface() with { ObservedQpc = 10 }] }] };
        var second = ledger.Freeze(secondDocument, "Collecting", "", true, 1, 2, 3);
        Assert.AreEqual(32768UL, second.DistinctReportedAllocationBytes);
        Assert.AreEqual(16384UL, first.DistinctReportedAllocationBytes);
        Assert.AreEqual(2L, second.EventsLost);
        Assert.AreEqual(3L, second.InvalidPayloads);
        Assert.AreEqual(0L, first.EventsLost);
        ledger.Add(new(GpuObservationKind.AdapterAllocation, 11, 0, AllocationId: 50, AllocationBytes: 65536, OwnerProcessId: 123));
        ledger.Add(new(GpuObservationKind.DeviceAllocation, 12, 123, KernelResource: 40, AllocationId: 50));
        ledger.Add(new(GpuObservationKind.Resource, 13, 123, ResourceId: 30, KernelResource: 40));
        var reused = document with { Threads = [document.Threads[0] with { Surfaces = [Surface() with { ObservedQpc = 15 }] }] };
        Assert.AreEqual(65536UL, ledger.Freeze(reused, "Collecting", "", true, 1, 0, 0).DistinctReportedAllocationBytes);
    }

    [TestMethod]
    public void ReportsMissingAmbiguousAndDroppedGpuEvidenceInsteadOfInventingBytes()
    {
        var document = Document([Node(10) with { Tree = TreeKind.Visual, VisualId = Id(10) }]);
        document = document with { Threads = [document.Threads[0] with { Surfaces = [Surface()] }] };
        var ledger = new GpuObservationLedger();
        ledger.Add(new(GpuObservationKind.Resource, 6, 123, ResourceId: 30, KernelResource: 40));
        var missing = ledger.Freeze(document, "Collecting", "", false, 1, 0, 0);
        Assert.AreEqual(GpuEvidenceStatus.ResourceNotObserved, missing.Resources[0].Status);
        Assert.IsNull(missing.DistinctReportedAllocationBytes);
        Assert.IsFalse(missing.StartedBeforeLaunch);
        ledger.Add(new(GpuObservationKind.Resource, 3, 123, ResourceId: 30, KernelResource: 40));
        ledger.Add(new(GpuObservationKind.DeviceAllocation, 2, 123, KernelResource: 40, AllocationId: 50));
        ledger.Add(new(GpuObservationKind.DeviceAllocation, 2, 123, KernelResource: 40, AllocationId: 51));
        var ambiguous = ledger.Freeze(document, "Collecting", "", false, 1, 0, 0);
        Assert.AreEqual(GpuEvidenceStatus.AmbiguousAllocation, ambiguous.Resources[0].Status);
        Assert.IsNull(ambiguous.DistinctReportedAllocationBytes);
        var bounded = new GpuObservationLedger(capacity: 1);
        bounded.Add(new(GpuObservationKind.Resource, 3, 123, ResourceId: 30, KernelResource: 40));
        bounded.Add(new(GpuObservationKind.Present, 4, 123));
        Assert.AreEqual(1L, bounded.Freeze(document, "Collecting", "", true, 1, 0, 0).DroppedObservations);
    }

    private static string Id(int value) => value == 0 ? "" : value.ToString("X16");

    private static SnapshotNode Node(int id, int parent = 0, uint index = 0) => new()
    {
        Tree = TreeKind.Xaml, Id = Id(id), ParentId = Id(parent), ChildIndex = index,
        Type = "Microsoft.UI.Xaml.Controls.Grid", Name = "", XamlId = Id(id), CompId = "", VisualId = "",
        Properties = [new() { Name = "ActualWidth", Value = "100", Status = 0 }],
        CoreAllocationStatus = unchecked((int)0x80004001), CoreAllocationBytes = 0,
    };

    private static SnapshotDocument Document(IReadOnlyList<SnapshotNode> nodes) => new()
    {
        SchemaVersion = 2, ProcessId = 123, ProcessStartTime = "0000000000000010", RequestId = "1",
        AdapterMemory = new()
        {
            Status = unchecked((int)0x80004001), StartQpc = 21, EndQpc = 22, QpcFrequency = 1000, Adapters = [],
        },
        Threads = [new()
        {
            ThreadId = 1, StartQpc = "10", EndQpc = "20", QpcFrequency = "1000", Status = 0, Error = "", Nodes = nodes,
            Surfaces = [], Memory = new()
            {
                Status = unchecked((int)0x80004001), HeapStatus = unchecked((int)0x80004001), HeapId = "", IsPrivateHeap = false,
                OutstandingBytes = 0, OutstandingCount = 0, CacheStatus = unchecked((int)0x80004001),
                CacheCapacity = 0, CachedSurfaceCount = 0, DroppedRegistrations = 0, CacheBytes = 0,
            },
        }],
    };
}
