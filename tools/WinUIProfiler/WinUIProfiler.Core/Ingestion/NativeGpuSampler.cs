using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Ingestion.NativeMetricAbi;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record GpuAcquisition(ProcessGpuSample Sample,
ulong AcquisitionClockId,
ulong AcquisitionBegin100ns,
ulong AcquisitionEnd100ns,
uint RowCount,
uint PdhError,
uint Win32Error);
internal sealed unsafe class NativeGpuSampler : IDisposable
{
    private readonly object gate = new();
    private readonly NativeMetricLibrary module;
    private readonly CaptureGpu capture;
    private readonly NormalizeGpu normalize;
    private GpuSnapshot? previous;
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
        var request = Request(target, GpuVersion);
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(module.IsClosed, this);
            var buffer = default(GpuSnapshot);
            var result = capture(process, in request, &buffer, SizeOf<GpuSnapshot>());
            if (result != 0)
            {
                throw new COMException("The native GPU sampler rejected its ABI request.", unchecked((int)result));
            }

            var current = GpuMetricCodec.DecodeCapture(in buffer, target);
            var prior = previous.GetValueOrDefault();
            var hasBaseline = previousTarget == target && current.Status == MetricStatus.Ok && previous.HasValue;
            var normalized = default(GpuResult);
            result = normalize(hasBaseline ? &prior : null, &current, &normalized, SizeOf<GpuResult>());
            GC.KeepAlive(module);
            if (result != 0)
            {
                throw new COMException("The native GPU normalizer rejected its ABI input.", unchecked((int)result));
            }

            var sample = GpuMetricCodec.Decode(in normalized, target);
            previous = current.Status == MetricStatus.Ok && current.RowCount != 0 ? current : null;
            previousTarget = previous is null ? null : target;
            return new(sample,
(ulong)current.AcquisitionClockId,
current.Begin100ns,
current.End100ns,
current.RowCount,
current.PdhError,
current.Win32Error);
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

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint CaptureGpu(SafeProcessHandle process, in CaptureRequest request, GpuSnapshot* output, uint capacity);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint NormalizeGpu(GpuSnapshot* previousSnapshot, GpuSnapshot* currentSnapshot, GpuResult* output, uint capacity);
}
