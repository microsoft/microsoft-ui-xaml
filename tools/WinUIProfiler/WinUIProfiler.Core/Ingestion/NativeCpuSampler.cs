using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Ingestion.NativeMetricAbi;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record CpuAcquisition(ProcessCpuSample Sample, ulong AcquisitionBegin100ns, ulong AcquisitionEnd100ns, uint ErrorCode, uint ErrorDomain);
internal sealed unsafe class NativeCpuSampler : IDisposable
{
    private readonly object gate = new();
    private readonly NativeMetricLibrary module;
    private readonly CaptureCpu capture;
    private readonly NormalizeCpu normalize;
    private CpuCaptureResult? previous;
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
        var request = Request(target, CaptureVersion);
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(module.IsClosed, this);
            var buffer = default(CpuCaptureResult);
            var result = capture(process, in request, &buffer, SizeOf<CpuCaptureResult>());
            if (result != 0)
            {
                throw new COMException("The native CPU sampler rejected its ABI request.", unchecked((int)result));
            }

            var current = CpuMetricCodec.DecodeCapture(in buffer, target);
            var prior = previous.GetValueOrDefault();
            var hasBaseline = previousTarget == target && current.Snapshot.Status == MetricStatus.Ok && previous.HasValue && current.Snapshot.Timestamp100ns > prior.Snapshot.Timestamp100ns;
            var normalized = default(CpuResult);
            result = normalize(hasBaseline ? &prior.Snapshot : null,
hasBaseline ? &prior.Groups[0] : null,
&current.Snapshot,
&current.Groups[0],
&normalized,
SizeOf<CpuResult>());
            GC.KeepAlive(module);
            if (result != 0)
            {
                throw new COMException("The native CPU normalizer rejected its ABI input.", unchecked((int)result));
            }

            var sample = CpuMetricCodec.Decode(in normalized, target);
            if (current.Snapshot.Status != MetricStatus.Ok)
            {
                previous = null;
                previousTarget = null;
            }
            else if (previousTarget != target || !previous.HasValue || current.Snapshot.Timestamp100ns > prior.Snapshot.Timestamp100ns)
            {
                previous = current;
                previousTarget = target;
            }

            return new(sample, current.Begin100ns, current.End100ns, current.ErrorCode, (uint)current.ErrorDomain);
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
    private delegate uint CaptureCpu(SafeProcessHandle process, in CaptureRequest request, CpuCaptureResult* output, uint capacity);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint NormalizeCpu(CpuSnapshot* previousSnapshot,
CpuGroup* previousGroups,
CpuSnapshot* currentSnapshot,
CpuGroup* currentGroups,
CpuResult* output,
uint capacity);
}
