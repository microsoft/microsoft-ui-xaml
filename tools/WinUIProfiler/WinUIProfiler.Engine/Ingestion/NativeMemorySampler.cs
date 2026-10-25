using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed class NativeMemorySampler : IDisposable
{
    private readonly object gate = new();
    private readonly NativeMetricLibrary module;
    private readonly CaptureMemory capture;

    internal NativeMemorySampler(string path)
    {
        module = new(path);
        try
        {
            capture = module.Export<CaptureMemory>("WupCaptureProcessMemory");
        }
        catch
        {
            module.Dispose();
            throw;
        }
    }

    internal ProcessMemorySample Capture(SafeProcessHandle process, ProcessInstanceKey target)
    {
        ArgumentNullException.ThrowIfNull(process);
        target.Validate();
        if (target.CreationTimeFileTime is not > 0)
            throw new ArgumentException("Native sampling requires the target's actual creation time.", nameof(target));
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(module.IsClosed, this);
            var request = new MemoryRequest(target.ProcessId, checked((ulong)target.CreationTimeFileTime.Value));
            var buffer = new byte[112];
            var result = capture(process, in request, buffer, checked((uint)buffer.Length));
            GC.KeepAlive(module);
            if (result != 0)
                throw new COMException("The native memory sampler rejected its ABI request.", unchecked((int)result));
            return MetricSampler.DecodeMemory(buffer, target);
        }
    }

    public void Dispose()
    {
        lock (gate) module.Dispose();
    }

    [StructLayout(LayoutKind.Sequential)]
    private readonly struct MemoryRequest(uint pid, ulong creationTime)
    {
        public readonly uint Size = 24;
        public readonly uint Version = 1;
        public readonly uint ProcessId = pid;
        public readonly uint Reserved = 0;
        public readonly ulong CreationTime = creationTime;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint CaptureMemory(SafeProcessHandle process, in MemoryRequest request,
        [Out] byte[] output, uint capacity);

}
