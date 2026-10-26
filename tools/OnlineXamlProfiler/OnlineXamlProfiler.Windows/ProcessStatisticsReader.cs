// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using OnlineXamlProfiler.Core;

[assembly: InternalsVisibleTo("OnlineXamlProfiler.Tests")]

namespace OnlineXamlProfiler.Windows;

public sealed class ProcessStatisticsReader : IDisposable
{
    private readonly SafeProcessHandle _process;
    private readonly uint _processId;
    private readonly string _processStartTime;
    private readonly SafeQueryHandle _query;
    private readonly Counter _dedicated;
    private readonly Counter _shared;
    private readonly Counter _engine;
    private uint _memorySize = checked((uint)Marshal.SizeOf<MemoryCounters>());
    private string _privateWorkingSetError = "";
    private int _collections;
    private bool _disposed;

    public ProcessStatisticsReader(uint processId, string processStartTime)
    {
        _processId = processId;
        _processStartTime = processStartTime;
        if (!OperatingSystem.IsWindowsVersionAtLeast(10, 0, 22621, 2361))
        {
            _memorySize = checked((uint)Marshal.OffsetOf<MemoryCounters>(nameof(MemoryCounters.PrivateWorkingSet)).ToInt32());
            _privateWorkingSetError = "Private working set requires Windows build 22621.2361 or later.";
        }
        _process = OpenProcess(0x00100410, false, processId); // Synchronize, query information, VM read.
        if (_process.IsInvalid)
        {
            int error = Marshal.GetLastWin32Error();
            _process.Dispose();
            throw new Win32Exception(error, "Cannot open the target for process statistics.");
        }
        if (!GetProcessTimes(_process, out long creation, out _, out _, out _))
        {
            int error = Marshal.GetLastWin32Error();
            _process.Dispose();
            throw new Win32Exception(error, "Cannot verify the statistics process instance.");
        }
        if (creation.ToString("X16", CultureInfo.InvariantCulture) != processStartTime)
        {
            _process.Dispose();
            throw new InvalidOperationException("The process ID was reused; attach to the new process instance.");
        }

        uint status = PdhOpenQuery(null, 0, out _query);
        _dedicated = AddCounter(status, $@"\GPU Process Memory(pid_{processId}_*)\Dedicated Usage");
        _shared = AddCounter(status, $@"\GPU Process Memory(pid_{processId}_*)\Shared Usage");
        _engine = AddCounter(status, $@"\GPU Engine(pid_{processId}_*)\Utilization Percentage");
    }

    public ProcessStatisticsSample Read()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        long timestamp = Stopwatch.GetTimestamp();
        var time = DateTimeOffset.UtcNow;
        ProcessStatisticsSample Exited() => new(_processId, _processStartTime, time, timestamp,
                null, "The target process has exited.", null, "The target process has exited.",
                null, "The target process has exited.", true);
        if (HasExited()) { return Exited(); }

        var counters = new MemoryCounters { Size = _memorySize };
        bool memoryAvailable = GetProcessMemoryInfo(_process, ref counters, _memorySize);
        int memoryError = memoryAvailable ? 0 : Marshal.GetLastWin32Error();
        if (!memoryAvailable && memoryError is 24 or 87 && _privateWorkingSetError.Length == 0)
        {
            _privateWorkingSetError = $"Private working set is unavailable on this Windows version (Win32 {memoryError}).";
            _memorySize = checked((uint)Marshal.OffsetOf<MemoryCounters>(nameof(MemoryCounters.PrivateWorkingSet)).ToInt32());
            counters.Size = _memorySize;
            memoryAvailable = GetProcessMemoryInfo(_process, ref counters, _memorySize);
            memoryError = memoryAvailable ? 0 : Marshal.GetLastWin32Error();
        }
        ProcessMemoryStatistics? memory = memoryAvailable ? new(
            counters.WorkingSet, _privateWorkingSetError.Length == 0 ? (ulong)counters.PrivateWorkingSet : null,
            counters.PrivateUsage, counters.PeakWorkingSet, counters.PeakPagefileUsage, _privateWorkingSetError) : null;

        uint collect = _query.IsInvalid ? _dedicated.Status : PdhCollectQueryData(_query);
        if (collect == 0) { ++_collections; }
        var dedicated = ReadCounter(_dedicated, collect);
        var shared = ReadCounter(_shared, collect);
        var engines = ReadCounter(_engine, collect);
        var gpuMemory = SummarizeMemory(_processId, dedicated, shared, out string gpuMemoryError);
        var gpuEngine = SummarizeEngine(_processId, engines, out string gpuEngineError);
        if (_collections < 2 && gpuEngine is null && _engine.Status == 0)
        {
            gpuEngineError = $"Waiting for GPU engine samples. {gpuEngineError}";
        }
        // PDH is PID-scoped, so discard a collection if that process exited during the read.
        if (HasExited()) { return Exited(); }
        return new(_processId, _processStartTime, time, timestamp, memory,
            memoryAvailable ? "" : $"GetProcessMemoryInfo: {new Win32Exception(memoryError).Message} ({memoryError}).",
            gpuMemory, gpuMemoryError, gpuEngine, gpuEngineError);
    }

    private bool HasExited()
    {
        uint wait = WaitForSingleObject(_process, 0);
        return wait switch
        {
            0 => true,
            258 => false,
            _ => throw new Win32Exception(Marshal.GetLastWin32Error(), "Cannot query the target process lifetime."),
        };
    }

    internal sealed record CounterValue(string Instance, double Value, uint Status = 0);
    internal sealed record CounterResult(IReadOnlyList<CounterValue> Values, string Error = "");

    internal static GpuMemoryStatistics? SummarizeMemory(
        uint pid, CounterResult dedicated, CounterResult shared, out string error)
    {
        var d = ForProcess(pid, dedicated, out error);
        if (error.Length != 0) { return null; }
        var s = ForProcess(pid, shared, out error);
        if (error.Length != 0) { return null; }
        if (!d.Select(item => item.Instance).ToHashSet(StringComparer.OrdinalIgnoreCase)
            .SetEquals(s.Select(item => item.Instance)))
        {
            error = "The GPU memory counters contain different adapter instances.";
            return null;
        }
        double dedicatedBytes = d.Sum(item => item.Value);
        double sharedBytes = s.Sum(item => item.Value);
        if (dedicatedBytes + sharedBytes >= ulong.MaxValue)
        {
            error = "The GPU memory counters exceed the supported byte range.";
            return null;
        }
        return new((ulong)dedicatedBytes, (ulong)sharedBytes);
    }

    internal static GpuEngineStatistics? SummarizeEngine(uint pid, CounterResult result, out string error)
    {
        var values = ForProcess(pid, result, out error);
        if (error.Length != 0) { return null; }
        var busiest = values.MaxBy(item => item.Value)!;
        return new(Math.Min(100, busiest.Value), busiest.Instance);
    }

    private static CounterValue[] ForProcess(uint pid, CounterResult result, out string error)
    {
        error = result.Error;
        var values = result.Values.Where(item =>
            item.Instance.StartsWith($"pid_{pid}_", StringComparison.OrdinalIgnoreCase)).ToArray();
        if (error.Length == 0 && values.Length == 0)
        {
            error = $"No GPU counter instances are available for PID {pid}.";
        }
        foreach (var value in values)
        {
            if (value.Status is not (0 or 1))
            {
                error = $"GPU counter {value.Instance}: PDH 0x{value.Status:X8}.";
                break;
            }
            if (!double.IsFinite(value.Value) || value.Value < 0)
            {
                error = $"GPU counter {value.Instance} returned an invalid value.";
                break;
            }
        }
        return values;
    }

    private readonly record struct Counter(nint Handle, uint Status);

    private Counter AddCounter(uint queryStatus, string path)
    {
        if (queryStatus != 0) { return new(0, queryStatus); }
        uint status = PdhAddEnglishCounter(_query, path, 0, out nint handle);
        return new(handle, status);
    }

    private static CounterResult ReadCounter(Counter counter, uint collectionStatus)
    {
        uint status = counter.Status != 0 ? counter.Status : collectionStatus;
        if (status != 0) { return new([], $"GPU counters: PDH 0x{status:X8}."); }
        const uint format = 0x00000200 | 0x00001000 | 0x00008000; // Double, unscaled, not capped.
        for (int attempt = 0; attempt < 3; ++attempt)
        {
            uint size = 0;
            status = PdhGetFormattedCounterArray(counter.Handle, format, ref size, out _, 0);
            if (status != 0x800007D2) { return new([], $"GPU counter array: PDH 0x{status:X8}."); }
            if (size == 0 || size > 16 * 1024 * 1024)
            {
                return new([], "The GPU counter array has an invalid size.");
            }
            nint buffer = Marshal.AllocHGlobal(checked((int)size));
            try
            {
                status = PdhGetFormattedCounterArray(counter.Handle, format, ref size, out uint count, buffer);
                if (status == 0x800007D2) { continue; }
                if (status != 0) { return new([], $"GPU counter array: PDH 0x{status:X8}."); }
                int stride = Marshal.SizeOf<FormattedCounterItem>();
                if (count > size / stride) { return new([], "The GPU counter array is incomplete."); }
                var values = new List<CounterValue>();
                for (int index = 0; index < count; ++index)
                {
                    var item = Marshal.PtrToStructure<FormattedCounterItem>(buffer + index * stride);
                    values.Add(new(Marshal.PtrToStringUni(item.Name) ?? "", item.Value, item.Status));
                }
                return new(values);
            }
            finally { Marshal.FreeHGlobal(buffer); }
        }
        return new([], "The GPU counter instances changed while reading; waiting for the next sample.");
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            _disposed = true;
            _query.Dispose();
            _process.Dispose();
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct MemoryCounters
    {
        public uint Size, PageFaultCount;
        public nuint PeakWorkingSet, WorkingSet, QuotaPeakPagedPoolUsage, QuotaPagedPoolUsage;
        public nuint QuotaPeakNonPagedPoolUsage, QuotaNonPagedPoolUsage, PagefileUsage, PeakPagefileUsage;
        public nuint PrivateUsage, PrivateWorkingSet;
        public ulong SharedCommitUsage;
    }

    [StructLayout(LayoutKind.Explicit, Size = 24)]
    private struct FormattedCounterItem
    {
        [FieldOffset(0)] public nint Name;
        [FieldOffset(8)] public uint Status;
        [FieldOffset(16)] public double Value;
    }

    private sealed class SafeQueryHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        public SafeQueryHandle() : base(true) { }
        protected override bool ReleaseHandle() => PdhCloseQuery(handle) == 0;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern SafeProcessHandle OpenProcess(uint access, [MarshalAs(UnmanagedType.Bool)] bool inherit, uint pid);
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetProcessTimes(SafeProcessHandle process, out long creation, out long exit, out long kernel, out long user);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint WaitForSingleObject(SafeProcessHandle handle, uint milliseconds);
    [DllImport("psapi.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetProcessMemoryInfo(SafeProcessHandle process, ref MemoryCounters counters, uint size);
    [DllImport("pdh.dll", CharSet = CharSet.Unicode, EntryPoint = "PdhOpenQueryW")]
    private static extern uint PdhOpenQuery(string? source, nuint data, out SafeQueryHandle query);
    [DllImport("pdh.dll", CharSet = CharSet.Unicode, EntryPoint = "PdhAddEnglishCounterW")]
    private static extern uint PdhAddEnglishCounter(SafeQueryHandle query, string path, nuint data, out nint counter);
    [DllImport("pdh.dll")]
    private static extern uint PdhCollectQueryData(SafeQueryHandle query);
    [DllImport("pdh.dll", CharSet = CharSet.Unicode, EntryPoint = "PdhGetFormattedCounterArrayW")]
    private static extern uint PdhGetFormattedCounterArray(nint counter, uint format, ref uint size, out uint count, nint buffer);
    [DllImport("pdh.dll")]
    private static extern uint PdhCloseQuery(nint query);
}
