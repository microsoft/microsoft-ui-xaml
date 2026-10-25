using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed class NativeGpuMemorySampler : IDisposable
{
    private readonly object gate = new();
    private readonly NativeMetricLibrary module;
    private readonly CaptureGpuMemory? capture;

    internal NativeGpuMemorySampler(string path)
    {
        module = new(path);
        try
        {
            capture = module.TryExport<CaptureGpuMemory>("WupCaptureGpuMemory");
        }
        catch
        {
            module.Dispose();
            throw;
        }
    }

    internal bool IsSupported => capture is not null;

    internal ProcessGpuMemorySample Capture(SafeProcessHandle process, ProcessInstanceKey target,
        PhysicalAdapterKey adapter)
    {
        ArgumentNullException.ThrowIfNull(process);
        target.Validate();
        if (target.CreationTimeFileTime is not > 0)
            throw new ArgumentException("Native sampling requires the target's actual creation time.", nameof(target));
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(module.IsClosed, this);
            if (capture is null)
                throw new NotSupportedException("The native metric library does not export WupCaptureGpuMemory.");
            var request = new GpuMemoryRequest(target, adapter);
            var buffer = new byte[152];
            var result = capture(process, in request, buffer, checked((uint)buffer.Length));
            GC.KeepAlive(module);
            if (result != 0)
                throw new COMException("The native GPU memory sampler rejected its ABI request.", unchecked((int)result));
            return GpuMemoryMetricCodec.Decode(buffer, target, adapter);
        }
    }

    public void Dispose()
    {
        lock (gate) module.Dispose();
    }

    [StructLayout(LayoutKind.Sequential)]
    private readonly struct GpuMemoryRequest(ProcessInstanceKey target, PhysicalAdapterKey adapter)
    {
        public readonly uint Size = 40;
        public readonly uint Version = 1;
        public readonly uint ProcessId = target.ProcessId;
        public readonly uint Reserved = 0;
        public readonly ulong CreationTime = checked((ulong)target.CreationTimeFileTime!.Value);
        public readonly uint LuidHigh = unchecked((uint)adapter.LuidHighPart);
        public readonly uint LuidLow = adapter.LuidLowPart;
        public readonly uint PhysicalIndex = adapter.PhysicalAdapterIndex;
        public readonly uint Reserved2 = 0;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint CaptureGpuMemory(SafeProcessHandle process, in GpuMemoryRequest request,
        [Out] byte[] output, uint capacity);
}
