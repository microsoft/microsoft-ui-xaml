using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Microsoft.Win32.SafeHandles;

namespace WinUIProfiler.Engine.Target;

// Holds every startup process object until ETW has drained, including an exited bootstrap's object. A PID therefore
// cannot be reused by an unrelated process while the capture routes events by PID.
[SupportedOSPlatform("windows")]
public sealed class NativeLaunchSession : IDisposable
{
    private readonly List<SafeProcessHandle> handles = [];

    internal NativeLaunchSession(Process process, NativeLaunchResult result, string directory)
    {
        Process = process;
        Result = result;
        Directory = directory;
        try
        {
            foreach (var entry in result.Processes)
            {
                var handle = OpenProcess(0x00101000, false, entry.ProcessId); // SYNCHRONIZE | QUERY_LIMITED_INFORMATION
                handles.Add(handle);
                if (handle.IsInvalid || !GetProcessTimes(handle, out var created, out _, out _, out _))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error(),
                        $"Could not bind startup PID {entry.ProcessId} before the native handoff expired.");
                }
                if (created != entry.CreationTimeFileTime)
                {
                    throw new InvalidDataException($"Startup PID {entry.ProcessId} no longer identifies the launched process.");
                }
            }
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    public Process Process { get; }
    public NativeLaunchResult Result { get; }
    public string Directory { get; }

    public void Dispose()
    {
        foreach (var handle in handles)
        {
            handle.Dispose();
        }
        Process.Dispose();
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern SafeProcessHandle OpenProcess(uint access, [MarshalAs(UnmanagedType.Bool)] bool inherit, int processId);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetProcessTimes(SafeProcessHandle process, out long created, out long exited,
        out long kernel, out long user);
}
