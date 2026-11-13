using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Ingestion.NativeMetricAbi;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed unsafe class NativeMemorySampler : IDisposable
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
        var request = Request(target, MemoryVersion);
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(module.IsClosed, this);
            var buffer = default(MemoryResult);
            var result = capture(process, in request, &buffer, SizeOf<MemoryResult>());
            GC.KeepAlive(module);
            if (result != 0)
            {
                throw new COMException("The native memory sampler rejected its ABI request.", unchecked((int)result));
            }

            return MetricSampler.DecodeMemory(in buffer, target);
        }
    }

    public void Dispose()
    {
        lock (gate)
        {
            module.Dispose();
        }
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint CaptureMemory(SafeProcessHandle process, in CaptureRequest request, MemoryResult* output, uint capacity);
}
