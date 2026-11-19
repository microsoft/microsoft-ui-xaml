using System.Runtime.InteropServices;

namespace RenderBench;

// Process-level resource sampling for M1-M3, M7, W2 (test plan). All counters are for the current process.
sealed unsafe class MemorySampler : IDisposable
{
    public readonly record struct Sample(long PrivateBytes, long WorkingSetBytes, long PeakWorkingSetBytes,
        long GpuLocalBytes, long GpuNonLocalBytes, int GdiObjects, int UserObjects, double CpuUserMs, double CpuKernelMs);

    readonly List<nint> _adapters = [];

    public MemorySampler()
    {
        // IDXGIFactory1 -> EnumAdapters1 -> QI IDXGIAdapter3; QueryVideoMemoryInfo reports this process's usage per adapter.
        Guid factoryId = new("770aae78-f26f-4dba-a829-253c83d1b387");
        if (CreateDXGIFactory1(&factoryId, out nint factory) < 0)
        {
            return;
        }
        var vtable = *(nint**)factory;
        var enumAdapters1 = (delegate* unmanaged[Stdcall]<nint, uint, nint*, int>)vtable[12];
        Guid adapter3Id = new("645967A4-1392-4310-A798-8053CE3E93FD");
        for (uint i = 0; ; i++)
        {
            nint adapter;
            if (enumAdapters1(factory, i, &adapter) < 0)
            {
                break;
            }
            if (Marshal.QueryInterface(adapter, in adapter3Id, out nint adapter3) >= 0)
            {
                _adapters.Add(adapter3);
            }
            Marshal.Release(adapter);
        }
        Marshal.Release(factory);
    }

    public Sample Take()
    {
        var counters = new PROCESS_MEMORY_COUNTERS_EX { cb = (uint)sizeof(PROCESS_MEMORY_COUNTERS_EX) };
        nint process = GetCurrentProcess();
        GetProcessMemoryInfo(process, &counters, counters.cb);
        GetProcessTimes(process, out _, out _, out long kernel, out long user);

        long local = 0, nonLocal = 0;
        foreach (var adapter in _adapters)
        {
            var query = (delegate* unmanaged[Stdcall]<nint, uint, int, VideoMemoryInfo*, int>)(*(nint**)adapter)[14];
            VideoMemoryInfo info;
            if (query(adapter, 0, 0, &info) >= 0)
            {
                local += (long)info.CurrentUsage;
            }
            if (query(adapter, 0, 1, &info) >= 0)
            {
                nonLocal += (long)info.CurrentUsage;
            }
        }

        return new Sample((long)counters.PrivateUsage, (long)counters.WorkingSetSize, (long)counters.PeakWorkingSetSize,
            local, nonLocal, (int)GetGuiResources(process, 0), (int)GetGuiResources(process, 1), user / 10000.0, kernel / 10000.0);
    }

    public void Dispose()
    {
        foreach (var adapter in _adapters)
        {
            Marshal.Release(adapter);
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

    [StructLayout(LayoutKind.Sequential)]
    struct VideoMemoryInfo
    {
        public ulong Budget;
        public ulong CurrentUsage;
        public ulong AvailableForReservation;
        public ulong CurrentReservation;
    }

    [DllImport("dxgi.dll")]
    static extern int CreateDXGIFactory1(Guid* riid, out nint factory);

    [DllImport("kernel32.dll")]
    static extern nint GetCurrentProcess();

    [DllImport("psapi.dll", SetLastError = true)]
    static extern bool GetProcessMemoryInfo(nint process, PROCESS_MEMORY_COUNTERS_EX* counters, uint size);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetProcessTimes(nint process, out long creation, out long exit, out long kernel, out long user);

    [DllImport("user32.dll")]
    static extern uint GetGuiResources(nint process, uint flags);
}
