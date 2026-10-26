// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Diagnostics;
using OnlineXamlProfiler.Core;
using OnlineXamlProfiler.Windows;

namespace OnlineXamlProfiler.Tests;

[TestClass]
public sealed class LiveSnapshotTests
{
    [DataTestMethod]
    [DataRow(false)]
    [DataRow(true)]
    [TestCategory("Live")]
    public async Task CapturesStartupTextureObservationsAndOptionalWindowsGpuEvidence(bool enableTrace)
    {
        string? executable = Environment.GetEnvironmentVariable("ONLINE_XAML_TEST_EXE");
        string? tapPath = Environment.GetEnvironmentVariable("ONLINE_XAML_TEST_TAP");
        if (string.IsNullOrWhiteSpace(executable) || string.IsNullOrWhiteSpace(tapPath))
        {
            Assert.Inconclusive("Set ONLINE_XAML_TEST_EXE and ONLINE_XAML_TEST_TAP to run the test-owned startup/cache/GPU capture.");
        }
        using var timeout = new CancellationTokenSource(TimeSpan.FromMinutes(2));
        await using var trace = enableTrace ?
            await GpuObservationSession.StartAsync(null, beforeLaunch: true, timeout.Token) : null;
        using var target = Process.Start(new ProcessStartInfo(executable)
        {
            WorkingDirectory = Path.GetDirectoryName(executable),
            UseShellExecute = false,
        }) ?? throw new InvalidOperationException("The test-owned target did not start.");
        try
        {
            trace?.SelectTarget(checked((uint)target.Id));
            while (true)
            {
                timeout.Token.ThrowIfCancellationRequested();
                target.Refresh();
                Assert.IsFalse(target.HasExited, "The test-owned target exited during startup.");
                if (target.MainWindowHandle != IntPtr.Zero && target.Responding) { break; }
                await Task.Delay(100, timeout.Token);
            }
            Assert.IsTrue(target.WaitForInputIdle(30000), "The test-owned target did not finish UI initialization.");
            await Task.Delay(500, timeout.Token);
            using var session = await ProfilerSession.ConnectAsync(checked((uint)target.Id), tapPath, timeout.Token);
            var first = await session.CaptureAsync(timeout.Token);
            var firstEvidence = trace?.Freeze(first) ?? GpuTraceSnapshot.Disabled;
            var second = await session.CaptureAsync(timeout.Token);
            var secondEvidence = trace?.Freeze(second) ?? GpuTraceSnapshot.Disabled;
            var deliveryTimeout = Stopwatch.StartNew();
            while (trace is not null && secondEvidence.State == "Collecting" &&
                !secondEvidence.Resources.Any(resource => resource.Status == GpuEvidenceStatus.Matched) &&
                deliveryTimeout.Elapsed < TimeSpan.FromSeconds(10))
            {
                await Task.Delay(100, timeout.Token);
                secondEvidence = trace.Freeze(second);
            }
            foreach (var document in new[] { first, second })
            {
                Assert.IsTrue(document.Threads.All(thread => thread.Status >= 0),
                    string.Join("; ", document.Threads.Select(thread => thread.Error)));
                Assert.IsTrue(SnapshotMemorySummary.Build(document).MeasuredCoreBytes > 0);
                Assert.IsTrue(document.Threads.All(thread => thread.Memory.Status >= 0 && thread.Memory.CacheCapacity > 0));
                Assert.IsTrue(document.Threads.SelectMany(thread => thread.Surfaces).Any(surface => surface.TextureStatus >= 0));
            }
            var observations = first.Threads.SelectMany(thread => thread.Surfaces)
                .Where(surface => surface.Status >= 0 && surface.TextureStatus >= 0)
                .Select(surface => (surface.SurfaceId, surface.Generation, surface.ResourceId, surface.ObservedQpc)).ToHashSet();
            Assert.IsTrue(second.Threads.SelectMany(thread => thread.Surfaces).Any(surface =>
                observations.Contains((surface.SurfaceId, surface.Generation, surface.ResourceId, surface.ObservedQpc))),
                "The static test app did not retain any unchanged startup texture observations across captures.");
            Console.WriteLine($"Runtime {session.RuntimePath}; core bytes {SnapshotMemorySummary.Build(first).MeasuredCoreBytes}; " +
                $"surfaces {first.Threads.Sum(thread => thread.Surfaces.Count)}; retained startup observations {observations.Count}.");
            Console.WriteLine($"GPU trace: {secondEvidence.State}; diagnostic {secondEvidence.Diagnostic}; " +
                $"presents {secondEvidence.ObservedPresentCount}; allocation matches " +
                $"{secondEvidence.Resources.Count(resource => resource.Status == GpuEvidenceStatus.Matched)}/{secondEvidence.Resources.Count}; " +
                $"lost {secondEvidence.EventsLost}; discarded {secondEvidence.DroppedObservations}; invalid {secondEvidence.InvalidPayloads}.");
            Console.WriteLine($"Event delivery QPC {secondEvidence.LastDeliveredQpc}; evidence: " +
                string.Join(", ", secondEvidence.Resources.GroupBy(resource => resource.Status)
                    .Select(group => $"{group.Key}={group.Count()}")));
            if (!enableTrace)
            {
                Assert.AreEqual("Disabled", secondEvidence.State);
            }
            else if (secondEvidence.State == "Collecting")
            {
                Assert.IsTrue(firstEvidence.StartedBeforeLaunch && secondEvidence.StartedBeforeLaunch);
                Assert.IsTrue(secondEvidence.Resources.Any(resource => resource.Status == GpuEvidenceStatus.Matched),
                    "The enabled OS providers did not correlate any observed startup textures to allocation evidence.");
            }
            else
            {
                Assert.AreEqual("Unavailable", secondEvidence.State);
                Assert.IsFalse(string.IsNullOrWhiteSpace(secondEvidence.Diagnostic));
            }
        }
        finally
        {
            if (!target.HasExited)
            {
                target.CloseMainWindow();
                using var closeTimeout = new CancellationTokenSource(TimeSpan.FromSeconds(5));
                try { await target.WaitForExitAsync(closeTimeout.Token); }
                catch (OperationCanceledException) when (closeTimeout.IsCancellationRequested) { target.Kill(); }
            }
        }
    }

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
