using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record NativeMetricBatch(ProcessInstanceKey Process, long BeginQpc, long EndQpc,
    long QpcFrequency, CpuAcquisition Cpu, ProcessMemorySample Memory, GpuAcquisition Gpu,
    IReadOnlyList<ProcessGpuMemorySample>? GpuMemory = null);

internal sealed class NativeMetricSession : IAsyncDisposable
{
    private readonly ProcessInstanceKey target;
    private readonly Func<NativeMetricBatch, bool> publish;
    private readonly SafeProcessHandle process;
    private readonly NativeCpuSampler cpu;
    private readonly NativeMemorySampler memory;
    private readonly NativeGpuSampler gpu;
    private readonly NativeGpuMemorySampler? gpuMemory;
    private readonly PhysicalAdapterKey[] gpuMemoryAdapters;
    private readonly MetricPollingWorker worker;
    private int terminationStatus = -1;
    internal string GpuMemoryCoverage { get; }

    internal NativeMetricSession(string path, ProcessInstanceKey target, TimeSpan interval,
        Func<NativeMetricBatch, bool> publish, IReadOnlyList<PhysicalAdapterKey>? gpuMemoryAdapters = null)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Native metrics require Windows.");
        target.Validate();
        if (target.CreationTimeFileTime is not > 0)
            throw new ArgumentException("Native sampling requires the target's actual creation time.", nameof(target));
        ArgumentNullException.ThrowIfNull(publish);
        this.gpuMemoryAdapters = SnapshotGpuMemoryAdapters(gpuMemoryAdapters);
        GpuMemoryCoverage = FormattableString.Invariant(
            $"GPU memory explicit physical scopes: {this.gpuMemoryAdapters.Length}; no process-wide aggregation.");
        this.target = target;
        this.publish = publish;
        worker = new(interval, Capture);
        SafeProcessHandle? opened = null;
        NativeCpuSampler? createdCpu = null;
        NativeMemorySampler? createdMemory = null;
        NativeGpuSampler? createdGpu = null;
        NativeGpuMemorySampler? createdGpuMemory = null;
        var initialized = false;
        try
        {
            opened = OpenProcess(0x00101000, false, target.ProcessId);
            if (opened.IsInvalid) throw new Win32Exception(Marshal.GetLastWin32Error(), "Opening the selected metric target failed.");
            createdCpu = new(path);
            createdMemory = new(path);
            createdGpu = new(path);
            if (gpuMemoryAdapters is null)
            {
                using var inventorySource = new NativeGpuAdapterInventory(path);
                GpuMemoryCoverage = "DXGI-visible graphics inventory unavailable: WupEnumerateGpuAdapters is absent.";
                if (inventorySource.IsSupported)
                {
                    var inventory = inventorySource.Capture();
                    this.gpuMemoryAdapters = inventory.CanConfigureMemory
                        ? SnapshotGpuMemoryAdapters(inventory.SelectMemoryAdapters(hardwareOnly: false)) : [];
                    GpuMemoryCoverage = FormattableString.Invariant(
                        $"DXGI-visible graphics inventory: status={inventory.Status}; rows={inventory.Adapters.Count}; flags={inventory.Flags}; HRESULT=0x{inventory.EnumerationHResult:X8}; Win32={inventory.Win32Error}; BasicRenderRows={inventory.Adapters.Count(adapter => adapter.IsBasicRenderDriver)}; selectedPhysicalScopes={this.gpuMemoryAdapters.Length}; excludes compute/NPU-only inventory; no process-wide aggregation.");
                }
            }
            if (this.gpuMemoryAdapters.Length != 0)
            {
                createdGpuMemory = new(path);
                if (!createdGpuMemory.IsSupported)
                    throw new NotSupportedException("Selected GPU memory sampling requires WupCaptureGpuMemory.");
            }
            process = opened;
            cpu = createdCpu;
            memory = createdMemory;
            gpu = createdGpu;
            gpuMemory = createdGpuMemory;
            initialized = true;
        }
        finally
        {
            if (!initialized)
            {
                createdGpuMemory?.Dispose();
                createdGpu?.Dispose();
                createdMemory?.Dispose();
                createdCpu?.Dispose();
                opened?.Dispose();
                worker.DisposeAsync().GetAwaiter().GetResult();
            }
        }
    }

    internal static PhysicalAdapterKey[] SnapshotGpuMemoryAdapters(IReadOnlyList<PhysicalAdapterKey>? gpuMemoryAdapters)
    {
        var count = gpuMemoryAdapters?.Count ?? 0;
        if (count > NativeMetricPayloads.MaximumGpuMemoryAdapters)
            throw new ArgumentException("GPU memory adapter selection exceeds the polling bound.", nameof(gpuMemoryAdapters));
        var snapshot = new PhysicalAdapterKey[count];
        var seen = new HashSet<PhysicalAdapterKey>();
        for (var index = 0; index < count; index++)
        {
            var adapter = gpuMemoryAdapters![index];
            if (!seen.Add(adapter))
                throw new ArgumentException("GPU memory adapter selection contains duplicate physical scopes.", nameof(gpuMemoryAdapters));
            snapshot[index] = adapter;
        }
        return snapshot;
    }

    private void Capture()
    {
        var begin = Stopwatch.GetTimestamp();
        var cpuSample = cpu.Capture(process, target);
        var memorySample = memory.Capture(process, target);
        var gpuSample = gpu.Capture(process, target);
        var memorySamples = gpuMemoryAdapters.Select(adapter => gpuMemory!.Capture(process, target, adapter)).ToArray();
        var end = Stopwatch.GetTimestamp();
        if (!publish(new(target, begin, end, Stopwatch.Frequency, cpuSample, memorySample, gpuSample,
            Array.AsReadOnly(memorySamples))))
            throw new InvalidOperationException("Metric ingress rejected the sample; polling stopped without silent loss.");
        var identityChanged = cpuSample.Sample.Status == 6 || memorySample.Status == 6 || gpuSample.Sample.Status == 6 ||
            memorySamples.Any(sample => sample.Status == 6);
        if (identityChanged || cpuSample.Sample.Status == 5 || memorySample.Status == 5 || gpuSample.Sample.Status == 5 ||
            memorySamples.Any(sample => sample.Status == 5))
        {
            Volatile.Write(ref terminationStatus, (int)(identityChanged
                ? MetricSampleStatus.IdentityChanged : MetricSampleStatus.TargetMissing));
            throw new InvalidOperationException("The selected metric target exited or its identity changed; polling stopped.");
        }
    }

    internal Task Completion => worker.Completion;
    internal MetricSampleStatus? TerminationStatus
    {
        get
        {
            var status = Volatile.Read(ref terminationStatus);
            return status < 0 ? null : (MetricSampleStatus)status;
        }
    }
    internal void Start() => worker.Start();
    internal Task StopAsync() => worker.StopAsync();

    public async ValueTask DisposeAsync()
    {
        try { await worker.DisposeAsync().ConfigureAwait(false); }
        finally
        {
            gpuMemory?.Dispose();
            gpu.Dispose();
            memory.Dispose();
            cpu.Dispose();
            process.Dispose();
        }
    }

    [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
    private static extern SafeProcessHandle OpenProcess(uint access,
        [MarshalAs(UnmanagedType.Bool)] bool inheritHandle, uint processId);
}
