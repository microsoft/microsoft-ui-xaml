using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal static class NativeMetricPayloads
{
    internal const int MaximumGpuMemoryAdapters = 256;
    internal const int MaximumPayloadCount = 13 + 256 + 4 * MaximumGpuMemoryAdapters;

    internal static IReadOnlyList<MetricPayload> Create(ProcessGpuMemorySample sample,
        TimeRange acquisitionQpc, QpcMapping clock)
    {
        ArgumentNullException.ThrowIfNull(sample);
        ArgumentNullException.ThrowIfNull(clock);
        sample.Process.Validate();
        var interval = new TimeRange(clock.Convert(acquisitionQpc.StartQpc), clock.Convert(acquisitionQpc.EndQpc));
        var identity = new GpuSampleIdentity(sample.Adapter, GpuMetricScope.PerAdapterMemory, null);
        var payloads = new List<MetricPayload>(4);
        void Add(MetricKind kind, TypedValue value, uint hresult)
        {
            var error = sample.Win32Error != 0 ? sample.Win32Error : hresult;
            var domain = error == 0 ? MetricErrorDomain.None :
                sample.Win32Error != 0 ? MetricErrorDomain.Win32 : MetricErrorDomain.HResult;
            var status = value.Kind == ValueKind.Integer ? MetricSampleStatus.Measured :
                sample.Status is 5 or 6 ? (MetricSampleStatus)sample.Status : MetricSampleStatus.Unavailable;
            var payload = new MetricPayload(kind, value, interval, sample.Source, sample.SourceVersion, identity)
            {
                SourceObservation = new(MetricClock.Boot100ns, 10_000_000,
                    sample.Begin100ns, sample.End100ns, status, sample.Begin100ns, sample.End100ns,
                    error, domain, 0)
            };
            EventValidation.Validate(payload, sample.Process);
            payloads.Add(payload);
        }
        Add(MetricKind.GpuDedicatedCommittedBytes, sample.DedicatedCommitted, sample.DedicatedHResult);
        Add(MetricKind.GpuSharedCommittedBytes, sample.SharedCommitted, sample.SharedHResult);
        Add(MetricKind.GpuDedicatedResidentBytes, sample.DedicatedResident, sample.DedicatedHResult);
        Add(MetricKind.GpuSharedResidentBytes, sample.SharedResident, sample.SharedHResult);
        return payloads.AsReadOnly();
    }

    internal static IReadOnlyList<MetricPayload> Create(NativeMetricBatch batch, QpcMapping clock)
    {
        ArgumentNullException.ThrowIfNull(batch);
        ArgumentNullException.ThrowIfNull(clock);
        batch.Process.Validate();
        var cpu = batch.Cpu.Sample;
        var memory = batch.Memory;
        var gpu = batch.Gpu.Sample;
        var gpuMemory = batch.GpuMemory ?? [];
        if (cpu.Process != batch.Process || memory.Process != batch.Process || gpu.Process != batch.Process ||
            batch.QpcFrequency != clock.SourceFrequency || gpu.Engines.Count > 256 ||
            batch.Gpu.AcquisitionClockId != 2 || gpuMemory.Count > MaximumGpuMemoryAdapters)
            throw new ArgumentException("Native metric samples must share the selected identity and their recorded clocks.");
        if (gpuMemory.Any(sample => sample is null || sample.Process != batch.Process) ||
            gpuMemory.Select(sample => sample.Adapter).Distinct().Count() != gpuMemory.Count)
            throw new ArgumentException("Native GPU memory must use this target and distinct physical adapter scopes.");
        var interval = new TimeRange(clock.Convert(batch.BeginQpc), clock.Convert(batch.EndQpc));
        var payloads = new List<MetricPayload>(13 + gpu.Engines.Count + 4 * gpuMemory.Count);
        var cpuObservation = Observation(cpu.ClockId, cpu.Begin100ns, cpu.End100ns, cpu.Status,
            batch.Cpu.AcquisitionBegin100ns, batch.Cpu.AcquisitionEnd100ns,
            batch.Cpu.ErrorCode, batch.Cpu.ErrorDomain);
        var memoryObservation = Observation(memory.ClockId, memory.Begin100ns, memory.End100ns, memory.Status,
            memory.Begin100ns, memory.End100ns, memory.NativeError, memory.NativeError == 0 ? 0u : 1u);
        var gpuObservation = Observation(gpu.ClockId, gpu.Begin100ns, gpu.End100ns, gpu.Status,
            batch.Gpu.AcquisitionBegin100ns, batch.Gpu.AcquisitionEnd100ns,
            batch.Gpu.Win32Error, batch.Gpu.Win32Error == 0 ? 0u : 1u, batch.Gpu.PdhError);

        void Add(MetricKind metric, TypedValue value, string source, string version,
            MetricSourceObservation observation, GpuSampleIdentity? identity = null)
        {
            var payload = new MetricPayload(metric, value, interval, source, version, identity)
            {
                SourceObservation = observation
            };
            EventValidation.Validate(payload, batch.Process);
            payloads.Add(payload);
        }

        Add(MetricKind.CpuUtilization, cpu.ProcessUtilization, cpu.Source, cpu.SourceVersion, cpuObservation);
        Add(MetricKind.SystemCpuUtilization, cpu.SystemUtilization, cpu.Source, cpu.SourceVersion, cpuObservation);
        Add(MetricKind.InterruptCpuUtilization, cpu.InterruptUtilization, cpu.Source, cpu.SourceVersion, cpuObservation);
        Add(MetricKind.OtherOrUnattributedCpuUtilization, cpu.OtherOrUnattributedUtilization, cpu.Source, cpu.SourceVersion, cpuObservation);
        Add(MetricKind.ProcessCpuUtility, cpu.ProcessUtility, cpu.Source, cpu.SourceVersion, cpuObservation);
        Add(MetricKind.ProcessPrivateWorkingSet, memory.PrivateWorkingSet, memory.Source, memory.SourceVersion, memoryObservation);
        Add(MetricKind.ProcessWorkingSet, memory.TotalWorkingSet, memory.Source, memory.SourceVersion, memoryObservation);
        Add(MetricKind.ProcessPrivateCommit, memory.PrivateCommit, memory.Source, memory.SourceVersion, memoryObservation);

        static GpuSampleIdentity Identity(GpuEngineSample engine, GpuMetricScope scope) =>
            new(new PhysicalAdapterKey(engine.LuidLow, unchecked((int)engine.LuidHigh), engine.Physical),
                scope, new GpuEngineKey(engine.Engine, GpuEngineClass.Unknown));
        var winner = gpu.Engines.MaxBy(engine => engine.Utilization.Number);
        Add(MetricKind.GpuUtilization, gpu.Utilization, gpu.Source, gpu.SourceVersion, gpuObservation,
            winner is null ? null : Identity(winner, GpuMetricScope.OverallBusiestEngine));
        foreach (var engine in gpu.Engines)
            Add(MetricKind.GpuUtilization, engine.Utilization, gpu.Source, gpu.SourceVersion, gpuObservation,
                Identity(engine, GpuMetricScope.PerEngine));
        Add(MetricKind.GpuDedicatedCommittedBytes, gpu.DedicatedCommitted, gpu.Source, gpu.SourceVersion, gpuObservation);
        Add(MetricKind.GpuSharedCommittedBytes, gpu.SharedCommitted, gpu.Source, gpu.SourceVersion, gpuObservation);
        Add(MetricKind.GpuDedicatedResidentBytes, gpu.DedicatedResident, gpu.Source, gpu.SourceVersion, gpuObservation);
        Add(MetricKind.GpuSharedResidentBytes, gpu.SharedResident, gpu.Source, gpu.SourceVersion, gpuObservation);
        foreach (var sample in gpuMemory)
            payloads.AddRange(Create(sample, new(batch.BeginQpc, batch.EndQpc), clock));
        return payloads.AsReadOnly();
    }

    private static MetricSourceObservation Observation(ulong clock, ulong begin, ulong end, uint status,
        ulong acquisitionBegin, ulong acquisitionEnd, uint error, uint domain, uint pdhError = 0) =>
        new(checked((MetricClock)(int)clock), 10_000_000, begin, end, checked((MetricSampleStatus)(int)status),
            acquisitionBegin, acquisitionEnd, error, checked((MetricErrorDomain)(int)domain), pdhError);
}
