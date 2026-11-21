using System.Runtime.InteropServices;

namespace RenderBench;

// Process-level resource sampling for M1-M3, M7, W2 (test plan). All counters are for the current process.
sealed unsafe class MemorySampler : IDisposable
{
    // GpuLocalBytes/GpuNonLocalBytes are summed over all adapters (unchanged meaning); Adapters breaks them down per adapter
    // LUID so the analyzer can report the adapter WinUI renders with (DeviceInfo.AdapterLuid).
    public readonly record struct Sample(long PrivateBytes, long WorkingSetBytes, long PeakWorkingSetBytes,
        long GpuLocalBytes, long GpuNonLocalBytes, int GdiObjects, int UserObjects, double CpuUserMs, double CpuKernelMs,
        IReadOnlyList<AdapterMemory> Adapters);

    public readonly record struct AdapterMemory(ulong Luid, long LocalBytes, long NonLocalBytes);

    // Every DXGI adapter (QueryVideoMemoryInfo reports this process's usage per adapter). The harness cannot know in
    // advance which adapter WinUI picks, so it samples all of them.
    readonly List<DxgiAdapter> _adapters = DxgiAdapters.Open(out _);

    public Sample Take()
    {
        var counters = new PROCESS_MEMORY_COUNTERS_EX { cb = (uint)sizeof(PROCESS_MEMORY_COUNTERS_EX) };
        nint process = GetCurrentProcess();
        GetProcessMemoryInfo(process, &counters, counters.cb);
        GetProcessTimes(process, out _, out _, out long kernel, out long user);

        long local = 0, nonLocal = 0;
        var perAdapter = new List<AdapterMemory>(_adapters.Count);
        foreach (var adapter in _adapters)
        {
            if (adapter.TryQueryVideoMemory(out long adapterLocal, out long adapterNonLocal))
            {
                local += adapterLocal;
                nonLocal += adapterNonLocal;
                perAdapter.Add(new AdapterMemory(adapter.Info.Luid, adapterLocal, adapterNonLocal));
            }
        }

        return new Sample((long)counters.PrivateUsage, (long)counters.WorkingSetSize, (long)counters.PeakWorkingSetSize,
            local, nonLocal, (int)GetGuiResources(process, 0), (int)GetGuiResources(process, 1), user / 10000.0, kernel / 10000.0,
            perAdapter);
    }

    public void Dispose()
    {
        foreach (var adapter in _adapters)
        {
            adapter.Dispose();
        }
        _adapters.Clear();
    }

    [StructLayout(LayoutKind.Sequential)]
    struct PROCESS_MEMORY_COUNTERS_EX
    {
        public uint cb;
        public uint PageFaultCount;
        public nuint PeakWorkingSetSize;
        public nuint WorkingSetSize;
        public nuint QuotaPeakPagedPoolUsage;
        public nuint QuotaPagedPoolUsage;
        public nuint QuotaPeakNonPagedPoolUsage;
        public nuint QuotaNonPagedPoolUsage;
        public nuint PagefileUsage;
        public nuint PeakPagefileUsage;
        public nuint PrivateUsage;
    }

    [DllImport("kernel32.dll")]
    static extern nint GetCurrentProcess();

    [DllImport("psapi.dll", SetLastError = true)]
    static extern bool GetProcessMemoryInfo(nint process, PROCESS_MEMORY_COUNTERS_EX* counters, uint size);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetProcessTimes(nint process, out long creation, out long exit, out long kernel, out long user);

    [DllImport("user32.dll")]
    static extern uint GetGuiResources(nint process, uint flags);
}
