// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using OnlineXamlProfiler.Core;
using OnlineXamlProfiler.Windows;

namespace OnlineXamlProfiler.Tests;

[TestClass]
public sealed class LiveSnapshotTests
{
    [TestMethod]
    [TestCategory("Live")]
    public async Task CapturesAllThreeTreesTwiceFromOneTarget()
    {
        string? pidText = Environment.GetEnvironmentVariable("ONLINE_XAML_TEST_PID");
        string? tapPath = Environment.GetEnvironmentVariable("ONLINE_XAML_TEST_TAP");
        if (string.IsNullOrWhiteSpace(pidText) || string.IsNullOrWhiteSpace(tapPath))
        {
            Assert.Inconclusive("Set ONLINE_XAML_TEST_PID and ONLINE_XAML_TEST_TAP for an opted-in live target using the modified runtime.");
        }
        using var timeout = new CancellationTokenSource(TimeSpan.FromMinutes(2));
        uint pid = uint.Parse(pidText);
        using var session = await ProfilerSession.ConnectAsync(pid, tapPath, timeout.Token);
        var first = await session.CaptureAsync(timeout.Token);
        var second = await session.CaptureAsync(timeout.Token);
        Assert.AreEqual(pid, first.ProcessId);
        Assert.AreEqual(first.ProcessId, second.ProcessId);
        Assert.AreEqual(first.ProcessStartTime, second.ProcessStartTime);
        Assert.AreNotEqual(first.RequestId, second.RequestId);
        foreach (var snapshot in new[] { first, second })
        {
            Assert.IsTrue(snapshot.Threads.All(thread => thread.Status >= 0),
                string.Join("; ", snapshot.Threads.Select(thread => thread.Error)));
            foreach (var kind in Enum.GetValues<TreeKind>())
            {
                var roots = SnapshotTree.Build(snapshot, kind);
                Assert.IsTrue(roots.Count > 0, $"Missing {kind} tree.");
                var nodes = snapshot.Threads.SelectMany(thread => thread.Nodes).Where(node => node.Tree == kind).ToList();
                Assert.IsTrue(nodes.Any(node => node.ParentId.Length != 0), $"The {kind} capture is flat, not a hierarchy.");
                Console.WriteLine($"Request {snapshot.RequestId}, {kind}: {nodes.Count} nodes, {roots.Count} roots, " +
                    $"{roots.Sum(node => node.Children.Count)} immediate root children.");
            }
            var xamlIds = snapshot.Threads.SelectMany(thread => thread.Nodes).Where(node => node.Tree == TreeKind.Xaml)
                .Select(node => node.Id).ToHashSet();
            Assert.IsTrue(snapshot.Threads.SelectMany(thread => thread.Nodes).Any(node =>
                node.Tree == TreeKind.Composition && xamlIds.Contains(node.XamlId) && node.VisualId.Length != 0),
                "No direct XAML/HWCompNode/IVisual cross-link was captured.");
        }
        Console.WriteLine($"Captured requests {first.RequestId}, {second.RequestId} from PID {pid}, runtime {session.RuntimePath}.");
    }
}
