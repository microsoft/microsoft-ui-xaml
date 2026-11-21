using System.Diagnostics;
using System.Runtime.InteropServices;

namespace RenderBench;

// Per-thread CPU split for W1/W2: enumerates this process's threads (Toolhelp32), reads GetThreadTimes, and classifies
// each thread once by the module containing its Win32 start address (ThreadCategories.Classify).
sealed unsafe class ThreadCpuSampler
{
    const uint TH32CS_SNAPTHREAD = 0x4;
    const uint THREAD_QUERY_INFORMATION = 0x40;
    const int ThreadQuerySetWin32StartAddress = 9;
    const uint GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS = 0x4;
    const uint GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT = 0x2;

    static readonly string[] s_rasterModules = ["d3d10warp.dll", "d2d1.dll", "dwrite.dll", "d3d11.dll", "dxgi.dll", "gdiplus.dll"];

    readonly ThreadCpuAccumulator _accumulator = new();
    readonly Dictionary<(int, long), string?> _startModules = [];
    readonly uint _pid = (uint)Environment.ProcessId;

    // Must be called on the UI (dispatcher) thread.
    public void CaptureUiThread() => _accumulator.UiThreadId = (int)GetCurrentThreadId();

    public (double[] CategoryMs, int LiveThreads) Take()
    {
        lock (_startModules)
        {
            return _accumulator.Add(Snapshot());
        }
    }

    public IReadOnlyList<ThreadCpuAccumulator.ModuleSummary> Modules() => _accumulator.Modules();

    // Raster-stack modules loaded in this process with their file versions, for pinning WARP golden references.
    public static IEnumerable<(string Module, string Version)> RasterModuleVersions()
    {
        foreach (ProcessModule module in Process.GetCurrentProcess().Modules)
        {
            if (s_rasterModules.Contains(module.ModuleName, StringComparer.OrdinalIgnoreCase)
                || module.ModuleName.StartsWith("DWriteCore", StringComparison.OrdinalIgnoreCase))
            {
                yield return (module.ModuleName, module.FileVersionInfo.FileVersion ?? "");
            }
        }
    }

    List<ThreadCpuAccumulator.Observation> Snapshot()
    {
        var result = new List<ThreadCpuAccumulator.Observation>();
        nint snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == -1)
        {
            return result;
        }
        try
        {
            var entry = new THREADENTRY32 { dwSize = (uint)sizeof(THREADENTRY32) };
            for (bool ok = Thread32First(snapshot, &entry); ok; ok = Thread32Next(snapshot, &entry))
            {
                if (entry.th32OwnerProcessID != _pid)
                {
                    continue;
                }
                nint thread = OpenThread(THREAD_QUERY_INFORMATION, false, entry.th32ThreadID);
                if (thread == 0)
                {
                    continue;
                }
                try
                {
                    if (!GetThreadTimes(thread, out long creation, out _, out long kernel, out long user))
                    {
                        continue;
                    }
                    var key = ((int)entry.th32ThreadID, creation);
                    if (!_startModules.TryGetValue(key, out string? module))
                    {
                        module = StartModule(thread);
                        _startModules[key] = module;
                    }
                    result.Add(new((int)entry.th32ThreadID, creation, module, (kernel + user) / 10000.0));
                }
                finally
                {
                    CloseHandle(thread);
                }
            }
        }
        finally
        {
            CloseHandle(snapshot);
        }
        return result;
    }

    static string? StartModule(nint thread)
    {
        nint start;
        if (NtQueryInformationThread(thread, ThreadQuerySetWin32StartAddress, &start, (uint)sizeof(nint), null) < 0 || start == 0)
        {
            return null;
        }
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, start, out nint module))
        {
            return null;
        }
        char* path = stackalloc char[1024];
        uint length = GetModuleFileNameW(module, path, 1024);
        return length == 0 ? null : new string(path, 0, (int)length);
    }

    [StructLayout(LayoutKind.Sequential)]
    struct THREADENTRY32
    {
        public uint dwSize;
        public uint cntUsage;
        public uint th32ThreadID;
        public uint th32OwnerProcessID;
        public int tpBasePri;
        public int tpDeltaPri;
        public uint dwFlags;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern nint CreateToolhelp32Snapshot(uint flags, uint processId);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool Thread32First(nint snapshot, THREADENTRY32* entry);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool Thread32Next(nint snapshot, THREADENTRY32* entry);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern nint OpenThread(uint access, bool inherit, uint threadId);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetThreadTimes(nint thread, out long creation, out long exit, out long kernel, out long user);

    [DllImport("kernel32.dll")]
    static extern bool CloseHandle(nint handle);

    [DllImport("kernel32.dll")]
    static extern uint GetCurrentThreadId();

    [DllImport("ntdll.dll")]
    static extern int NtQueryInformationThread(nint thread, int infoClass, void* info, uint length, uint* returnLength);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetModuleHandleExW(uint flags, nint address, out nint module);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern uint GetModuleFileNameW(nint module, char* path, uint size);
}
