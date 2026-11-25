using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;

namespace WinUIProfiler.Engine.Metrics;

internal sealed class ProcessMetricSession : IAsyncDisposable
{
    private readonly ProcessInstanceKey target;
    private readonly MetricHistory history;
    private readonly SafeProcessHandle process;
    private readonly NativeCpuSampler cpu;
    private readonly NativeMemorySampler memory;
    private readonly NativeGpuSampler gpu;
    private readonly MetricPollingWorker worker;
    private readonly TaskCompletionSource firstSample = new(TaskCreationOptions.RunContinuationsAsynchronously);
    internal ProcessMetricSession(string nativeLibrary, ProcessInstanceKey target, MetricHistory history)
    {
        target.Validate();
        this.target = target;
        this.history = history;
        process = OpenProcess(0x00101000, false, target.ProcessId);
        if (process.IsInvalid)
        {
            var error = Marshal.GetLastWin32Error();
            process.Dispose();
            throw new Win32Exception(error, "Opening the selected metric target failed.");
        }

        NativeCpuSampler? createdCpu = null;
        NativeMemorySampler? createdMemory = null;
        NativeGpuSampler? createdGpu = null;
        try
        {
            createdCpu = new(nativeLibrary);
            createdMemory = new(nativeLibrary);
            createdGpu = new(nativeLibrary);
            cpu = createdCpu;
            memory = createdMemory;
            gpu = createdGpu;
            worker = new(MetricHistory.SamplingInterval, Capture);
        }
        catch
        {
            createdGpu?.Dispose();
            createdMemory?.Dispose();
            createdCpu?.Dispose();
            process.Dispose();
            throw;
        }
    }

    internal Task FirstSample => firstSample.Task;
    internal Task Completion => worker.Completion;

    internal void Start() => worker.Start();
    private void Capture()
    {
        var cpuSample = cpu.Capture(process, target);
        var memorySample = memory.Capture(process, target);
        var gpuSample = gpu.Capture(process, target);
        if (cpuSample.Sample.Status is 5 or 6 || memorySample.Status is 5 or 6 || gpuSample.Sample.Status is 5 or 6)
        {
            throw new IOException("The selected metric target exited or its process identity changed.");
        }

        history.Add(new(target,
Stopwatch.GetTimestamp(),
[ToValue(ProcessMetric.Cpu, cpuSample.Sample.ProcessUtilization, cpuSample.Sample.Source), ToValue(ProcessMetric.Gpu, gpuSample.Sample.Utilization, gpuSample.Sample.Source), ToValue(ProcessMetric.WorkingSet, memorySample.TotalWorkingSet, memorySample.Source), ToValue(ProcessMetric.PrivateWorkingSet, memorySample.PrivateWorkingSet, memorySample.Source), ToValue(ProcessMetric.PrivateCommit, memorySample.PrivateCommit, memorySample.Source)]));
        firstSample.TrySetResult();
    }

    internal static MetricValue ToValue(ProcessMetric metric, TypedValue value, string source)
    {
        Require.Enum(metric);
        value.Validate();
        var isPercent = metric <= ProcessMetric.Gpu;
        if (value.Unit != (isPercent ? ValueUnit.Percent : ValueUnit.Bytes))
        {
            throw new ArgumentException("The metric's source unit does not match its graph.");
        }

        var number = value.Kind switch
        {
            ValueKind.Integer => (double?)value.Integer,
            ValueKind.Number => value.Number,
            ValueKind.Missing => null,
            _ => throw new ArgumentException("Process metrics require numeric or explicitly missing values.")
        };
        return new(metric,
isPercent ? number : number / (1024 * 1024),
source,
number.HasValue ? value.Quality.Reason : value.Quality.Reason ?? "Unavailable");
    }

    public async ValueTask DisposeAsync()
    {
        try
        {
            await worker.DisposeAsync().ConfigureAwait(false);
        }
        finally
        {
            gpu.Dispose();
            memory.Dispose();
            cpu.Dispose();
            process.Dispose();
        }
    }

    [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern SafeProcessHandle OpenProcess(uint access, [MarshalAs(UnmanagedType.Bool)] bool inheritHandle, uint processId);
}
