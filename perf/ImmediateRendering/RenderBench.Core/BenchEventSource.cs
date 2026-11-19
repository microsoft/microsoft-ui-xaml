using System.Diagnostics.Tracing;

namespace RenderBench;

public enum BenchPhase
{
    Warmup = 1,
    Steady = 2,
    Interaction = 3,
    Recovery = 4,
}

// Harness markers for the comparative runs. Self-describing (TraceLogging) format so the analyzer parses it the same
// way as the native Microsoft-WinUI-RenderingBackend provider. GUID = name hash: 114b4a9f-4cba-5c88-a321-355d52c822e6.
[EventSource(Name = ProviderName)]
public sealed class BenchEventSource : EventSource
{
    public const string ProviderName = "Microsoft-WinUI-RenderBench";
    public static readonly Guid ProviderId = new("114b4a9f-4cba-5c88-a321-355d52c822e6");
    public static readonly BenchEventSource Log = new();

    BenchEventSource() : base(EventSourceSettings.EtwSelfDescribingEventFormat) { }

    [Event(1, Level = EventLevel.Informational)]
    public void RunStarted(string Scene, string RequestedBackend, int Seed, int Width, int Height, double RasterizationScale)
        => WriteEvent(1, Scene, RequestedBackend, Seed, Width, Height, RasterizationScale);

    [Event(2, Level = EventLevel.Informational)]
    public void PhaseBegin(int Phase, string PhaseName) => WriteEvent(2, Phase, PhaseName);

    [Event(3, Level = EventLevel.Informational)]
    public void PhaseEnd(int Phase, string PhaseName) => WriteEvent(3, Phase, PhaseName);

    [Event(4, Level = EventLevel.Informational)]
    public void SceneReady(string Scene, int ElementCount) => WriteEvent(4, Scene, ElementCount);

    [Event(5, Level = EventLevel.Informational)]
    public void InputInjected(long InputId, string Kind, string Target) => WriteEvent(5, InputId, Kind, Target);

    [Event(6, Level = EventLevel.Informational)]
    public void MemorySample(
        int Phase,
        long PrivateBytes,
        long WorkingSetBytes,
        long PeakWorkingSetBytes,
        long GpuLocalBytes,
        long GpuNonLocalBytes,
        int GdiObjects,
        int UserObjects,
        double CpuUserMs,
        double CpuKernelMs)
        => WriteEvent(6, Phase, PrivateBytes, WorkingSetBytes, PeakWorkingSetBytes, GpuLocalBytes, GpuNonLocalBytes,
            GdiObjects, UserObjects, CpuUserMs, CpuKernelMs);

    [Event(7, Level = EventLevel.Verbose)]
    public void UpdateApplied(long Tick, int ChangedCount) => WriteEvent(7, Tick, ChangedCount);

    [Event(8, Level = EventLevel.Informational)]
    public void RunCompleted(int Status, string Detail) => WriteEvent(8, Status, Detail);

    [Event(9, Level = EventLevel.Informational)]
    public void ScreenshotSaved(string Path, int Width, int Height) => WriteEvent(9, Path, Width, Height);
}
