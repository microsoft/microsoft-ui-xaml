using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record CpuAcquisition(ProcessCpuSample Sample,
    ulong AcquisitionBegin100ns, ulong AcquisitionEnd100ns, uint ErrorCode, uint ErrorDomain);

internal sealed class NativeCpuSampler : IDisposable
{
    private readonly object gate = new();
    private readonly NativeMetricLibrary module;
    private readonly CaptureCpu capture;
    private readonly NormalizeCpu normalize;
    private CpuCounterSnapshot? previous;
    private ProcessInstanceKey? previousTarget;

    internal NativeCpuSampler(string path)
    {
        module = new(path);
        try
        {
            capture = module.Export<CaptureCpu>("WupCaptureCpu");
            normalize = module.Export<NormalizeCpu>("WupNormalizeCpu");
        }
        catch
        {
            module.Dispose();
            throw;
        }
    }

    internal CpuAcquisition Capture(SafeProcessHandle process, ProcessInstanceKey target)
    {
        ArgumentNullException.ThrowIfNull(process);
        target.Validate();
        if (target.CreationTimeFileTime is not > 0)
            throw new ArgumentException("Native sampling requires the target's actual creation time.", nameof(target));
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(module.IsClosed, this);
            var request = new CpuRequest(target.ProcessId, checked((ulong)target.CreationTimeFileTime.Value));
            var buffer = new byte[3696];
            var result = capture(process, in request, buffer, checked((uint)buffer.Length));
            if (result != 0)
                throw new COMException("The native CPU sampler rejected its ABI request.", unchecked((int)result));
            var current = CpuMetricCodec.DecodeCapture(buffer, target);
            var baseline = previousTarget == target && current.Status == 0 &&
                previous is not null && current.Timestamp100ns > previous.Timestamp100ns ? previous : null;
            var normalized = new byte[136];
            result = normalize(baseline?.Snapshot, baseline?.Groups, current.Snapshot,
                current.Groups, normalized, checked((uint)normalized.Length));
            GC.KeepAlive(module);
            if (result != 0)
                throw new COMException("The native CPU normalizer rejected its ABI input.", unchecked((int)result));
            var sample = CpuMetricCodec.Decode(normalized, target);
            if (current.Status != 0)
            {
                previous = null;
                previousTarget = null;
            }
            else if (previousTarget != target || previous is null || current.Timestamp100ns > previous.Timestamp100ns)
            {
                previous = current;
                previousTarget = target;
            }
            return new(sample, current.Begin100ns, current.End100ns, current.ErrorCode, current.ErrorDomain);
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
    private readonly struct CpuRequest(uint pid, ulong creationTime)
    {
        public readonly uint Size = 24;
        public readonly uint Version = 1;
        public readonly uint ProcessId = pid;
        public readonly uint Reserved = 0;
        public readonly ulong CreationTime = creationTime;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint CaptureCpu(SafeProcessHandle process, in CpuRequest request,
        [Out] byte[] output, uint capacity);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint NormalizeCpu([In] byte[]? previousSnapshot, [In] byte[]? previousGroups,
        [In] byte[] currentSnapshot, [In] byte[] currentGroups, [Out] byte[] output, uint capacity);
}
