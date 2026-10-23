// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using OnlineXamlProfiler.Core;
using OnlineXamlProfiler.Windows;
using static OnlineXamlProfiler.Windows.ProcessStatisticsReader;

namespace OnlineXamlProfiler.Tests;

[TestClass]
public sealed class ProcessStatisticsTests
{
    [TestMethod]
    public void RollingHistoryRetainsObservedPeakAndFrozenSnapshotReading()
    {
        var history = new ProcessStatisticsHistory(12, "0000000000000010");
        var captured = Sample(0, new(100, 20));
        history.Add(captured);
        Assert.AreSame(captured, history.FindNearest(Ticks(0)));
        history.Add(Sample(1));
        Assert.IsNull(history.Latest!.GpuMemory);
        history.Add(Sample(60, new(10, 20)));
        history.Add(Sample(61, new(0, 0)));

        CollectionAssert.AreEqual(new[] { Ticks(1), Ticks(60), Ticks(61) },
            history.Samples.Select(sample => sample.CounterTimestamp).ToArray());
        Assert.AreEqual(120ul, history.ObservedGpuMemoryPeakBytes);
        Assert.AreEqual(120ul, captured.GpuMemory!.TotalBytes);
        Assert.AreEqual(0ul, history.Latest!.GpuMemory!.TotalBytes);
        Assert.IsFalse(history.Samples.Contains(captured));
        Assert.AreSame(history.Samples[0], history.FindNearest(Ticks(0)));
        Assert.AreSame(history.Samples[1], history.FindNearest(Ticks(59)));
        Assert.IsNull(history.FindNearest(Ticks(65)));
    }

    [TestMethod]
    public void StatisticsCannotMixProcessInstancesOrReorderSamples()
    {
        var history = new ProcessStatisticsHistory(12, "0000000000000010");
        history.Add(Sample(1));
        Assert.ThrowsException<ArgumentException>(() => history.Add(Sample(2) with { ProcessId = 123 }));
        Assert.ThrowsException<ArgumentException>(() => history.Add(Sample(2) with { ProcessStartTime = "0000000000000020" }));
        Assert.ThrowsException<ArgumentException>(() => history.Add(Sample(1)));
        Assert.AreEqual(1, history.Samples.Count);
        Assert.IsNull(history.ObservedGpuMemoryPeakBytes);
    }

    [TestMethod]
    public void GpuCountersArePidScopedAndUseBusiestEngineNotSum()
    {
        var dedicated = new CounterResult([
            new("pid_12_luid_a_phys_0", 10), new("pid_12_luid_b_phys_1", 20),
            new("pid_123_luid_a_phys_0", 1000),
        ]);
        var shared = new CounterResult([
            new("pid_12_luid_a_phys_0", 1), new("pid_12_luid_b_phys_1", 2),
        ]);
        var memory = SummarizeMemory(12, dedicated, shared, out string error);
        Assert.AreEqual("", error);
        Assert.AreEqual(new GpuMemoryStatistics(30, 3), memory);
        var engine = SummarizeEngine(12, new([
            new("pid_12_luid_a_eng_0_engtype_3D", 55),
            new("pid_12_luid_a_eng_1_engtype_Copy", 30),
            new("pid_123_luid_a_eng_0_engtype_3D", 99),
        ]), out error);
        Assert.AreEqual("", error);
        Assert.AreEqual(55d, engine!.UtilizationPercent);
        StringAssert.Contains(engine.Engine, "engtype_3D");
    }

    [TestMethod]
    public void MissingInvalidOrPartialGpuCountersAreNotReportedAsZero()
    {
        Assert.IsNull(SummarizeEngine(12, new([]), out string error));
        Assert.IsFalse(string.IsNullOrEmpty(error));
        Assert.IsNull(SummarizeEngine(12, new([new("pid_12_engine", 0, 0xC0000BBA)]), out error));
        StringAssert.Contains(error, "C0000BBA");
        Assert.IsNull(SummarizeEngine(12, new([new("pid_12_engine", double.NaN)]), out error));
        Assert.IsFalse(string.IsNullOrEmpty(error));
        Assert.IsNull(SummarizeMemory(12, new([new("pid_12_a", 1)]), new([new("pid_12_b", 1)]), out error));
        Assert.IsFalse(string.IsNullOrEmpty(error));
        Assert.IsNull(SummarizeMemory(12, new([], "Counter access denied."), new([]), out error));
        Assert.AreEqual("Counter access denied.", error);
        Assert.AreEqual(0d, SummarizeEngine(12, new([new("pid_12_engine", 0)]), out error)!.UtilizationPercent);
        Assert.AreEqual("", error);
    }

    [TestMethod]
    public void ReadsRealProcessMemoryAndRejectsReusedPid()
    {
        using var target = Process.GetCurrentProcess();
        uint pid = checked((uint)target.Id);
        long creation = target.StartTime.ToUniversalTime().ToFileTimeUtc();
        using var reader = new ProcessStatisticsReader(pid, creation.ToString("X16"));
        var sample = reader.Read();
        Assert.IsNotNull(sample.Memory, sample.MemoryError);
        Assert.IsTrue(sample.Memory.WorkingSetBytes > 0);
        Assert.IsTrue(sample.Memory.PeakWorkingSetBytes >= sample.Memory.WorkingSetBytes);
        Assert.IsTrue(sample.Memory.PeakPrivateCommitBytes >= sample.Memory.PrivateCommitBytes);
        Assert.AreEqual(pid, sample.ProcessId);
        Assert.IsFalse(sample.ProcessExited);
        if (sample.GpuMemory is null) { Assert.IsFalse(string.IsNullOrEmpty(sample.GpuMemoryError)); }
        if (sample.GpuEngine is null) { Assert.IsFalse(string.IsNullOrEmpty(sample.GpuEngineError)); }
        Assert.ThrowsException<InvalidOperationException>(() =>
        {
            using var wrongInstance = new ProcessStatisticsReader(pid, (creation + 1).ToString("X16"));
        });
    }

    [TestMethod]
    public void TargetExitIsNotReportedAsZeroMemory()
    {
        var start = new ProcessStartInfo(Path.Combine(Environment.SystemDirectory, "cmd.exe"))
        {
            UseShellExecute = false, CreateNoWindow = true, RedirectStandardInput = true,
        };
        foreach (string argument in new[] { "/d", "/q", "/c", "set /p input=" }) { start.ArgumentList.Add(argument); }
        using var target = Process.Start(start)!;
        try
        {
            using var reader = new ProcessStatisticsReader(checked((uint)target.Id),
                target.StartTime.ToUniversalTime().ToFileTimeUtc().ToString("X16"));
            Assert.IsNotNull(reader.Read().Memory);
            target.StandardInput.WriteLine("done");
            Assert.IsTrue(target.WaitForExit(5000), "The test-owned target did not exit.");
            var exited = reader.Read();
            Assert.IsTrue(exited.ProcessExited);
            Assert.IsNull(exited.Memory);
            Assert.IsFalse(string.IsNullOrEmpty(exited.MemoryError));
        }
        finally
        {
            if (!target.HasExited) { target.Kill(); target.WaitForExit(); }
        }
    }

    private static long Ticks(int seconds) => checked(Stopwatch.Frequency * seconds);
    private static ProcessStatisticsSample Sample(int seconds, GpuMemoryStatistics? gpu = null) =>
        new(12, "0000000000000010", DateTimeOffset.UnixEpoch.AddSeconds(seconds), Ticks(seconds),
            null, "No memory sample.", gpu, gpu is null ? "No GPU sample." : "", null, "No engine sample.");
}
