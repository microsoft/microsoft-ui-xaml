using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record GpuAcquisition(ProcessGpuSample Sample, ulong AcquisitionClockId,
    ulong AcquisitionBegin100ns, ulong AcquisitionEnd100ns, uint RowCount, uint PdhError, uint Win32Error);

internal sealed class NativeGpuSampler : IDisposable
{
    private readonly object gate = new();
    private readonly NativeMetricLibrary module;
    private readonly CaptureGpu capture;
    private readonly NormalizeGpu normalize;
    private GpuCounterSnapshot? previous;
    private ProcessInstanceKey? previousTarget;

    internal NativeGpuSampler(string path)
    {
        module = new(path);
        try
        {
            capture = module.Export<CaptureGpu>("WupCaptureGpu");
            normalize = module.Export<NormalizeGpu>("WupNormalizeGpu");
        }
        catch
        {
            module.Dispose();
            throw;
        }
    }

    internal GpuAcquisition Capture(SafeProcessHandle process, ProcessInstanceKey target)
    {
        ArgumentNullException.ThrowIfNull(process);
        target.Validate();
        if (target.CreationTimeFileTime is not > 0)
            throw new ArgumentException("Native sampling requires the target's actual creation time.", nameof(target));
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(module.IsClosed, this);
            var request = new GpuRequest(target.ProcessId, checked((ulong)target.CreationTimeFileTime.Value));
            var buffer = new byte[14424];
            var result = capture(process, in request, buffer, checked((uint)buffer.Length));
            if (result != 0)
                throw new COMException("The native GPU sampler rejected its ABI request.", unchecked((int)result));
            var current = GpuMetricCodec.DecodeCapture(buffer, target);
            var baseline = previousTarget == target && current.Status == 0 ? previous : null;
            var normalized = new byte[8336];
            result = normalize(baseline?.Buffer, current.Buffer, normalized, checked((uint)normalized.Length));
            GC.KeepAlive(module);
            if (result != 0)
                throw new COMException("The native GPU normalizer rejected its ABI input.", unchecked((int)result));
            var sample = GpuMetricCodec.Decode(normalized, target);
            previous = current.Status == 0 && current.RowCount != 0 ? current : null;
            previousTarget = previous is null ? null : target;
            return new(sample, current.AcquisitionClockId, current.AcquisitionBegin100ns,
                current.AcquisitionEnd100ns, current.RowCount, current.PdhError, current.Win32Error);
        }
    }

    public void Dispose()
    {
        lock (gate)
        {
            previous = null;
            previousTarget = null;
            module.Dispose();
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private readonly struct GpuRequest(uint pid, ulong creationTime)
    {
        public readonly uint Size = 24;
        public readonly uint Version = 1;
        public readonly uint ProcessId = pid;
        public readonly uint Reserved = 0;
        public readonly ulong CreationTime = creationTime;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint CaptureGpu(SafeProcessHandle process, in GpuRequest request,
        [Out] byte[] output, uint capacity);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint NormalizeGpu([In] byte[]? previousSnapshot,
        [In] byte[] currentSnapshot, [Out] byte[] output, uint capacity);
}
