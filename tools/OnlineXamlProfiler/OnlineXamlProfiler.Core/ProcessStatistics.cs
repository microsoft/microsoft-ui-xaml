// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;

namespace OnlineXamlProfiler.Core;

public sealed record ProcessMemoryStatistics(
    ulong WorkingSetBytes, ulong? PrivateWorkingSetBytes, ulong PrivateCommitBytes,
    ulong PeakWorkingSetBytes, ulong PeakPrivateCommitBytes, string PrivateWorkingSetError);

public sealed record GpuMemoryStatistics(ulong DedicatedBytes, ulong SharedBytes)
{
    public ulong TotalBytes => checked(DedicatedBytes + SharedBytes);
}

public sealed record GpuEngineStatistics(double UtilizationPercent, string Engine);

public sealed record ProcessStatisticsSample(
    uint ProcessId, string ProcessStartTime, DateTimeOffset TimeUtc, long CounterTimestamp,
    ProcessMemoryStatistics? Memory, string MemoryError,
    GpuMemoryStatistics? GpuMemory, string GpuMemoryError,
    GpuEngineStatistics? GpuEngine, string GpuEngineError, bool ProcessExited = false);

public sealed class ProcessStatisticsHistory(uint processId, string processStartTime)
{
    public static readonly TimeSpan Duration = TimeSpan.FromSeconds(60);
    private readonly List<ProcessStatisticsSample> _samples = [];

    public uint ProcessId { get; } = processId;
    public string ProcessStartTime { get; } = processStartTime;
    public IReadOnlyList<ProcessStatisticsSample> Samples => _samples.AsReadOnly();
    public ProcessStatisticsSample? Latest => _samples.LastOrDefault();
    public ulong? ObservedGpuMemoryPeakBytes { get; private set; }

    public void Add(ProcessStatisticsSample sample)
    {
        if (sample.ProcessId != ProcessId || sample.ProcessStartTime != ProcessStartTime)
        {
            throw new ArgumentException("The statistics belong to a different process instance.", nameof(sample));
        }
        if (Latest is { } previous && sample.CounterTimestamp <= previous.CounterTimestamp)
        {
            throw new ArgumentException("Statistics samples must have increasing timestamps.", nameof(sample));
        }
        if (sample.GpuMemory is { } gpu)
        {
            ObservedGpuMemoryPeakBytes = Math.Max(ObservedGpuMemoryPeakBytes ?? 0, gpu.TotalBytes);
        }
        _samples.Add(sample);
        _samples.RemoveAll(item => Stopwatch.GetElapsedTime(item.CounterTimestamp, sample.CounterTimestamp) > Duration);
    }

    public ProcessStatisticsSample? FindNearest(long counterTimestamp)
    {
        var nearest = _samples.MinBy(item => Math.Abs((double)item.CounterTimestamp - counterTimestamp));
        return nearest is not null &&
            Math.Abs(Stopwatch.GetElapsedTime(counterTimestamp, nearest.CounterTimestamp).TotalSeconds) <= 2
            ? nearest : null;
    }
}
