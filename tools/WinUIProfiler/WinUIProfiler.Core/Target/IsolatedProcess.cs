using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text;
using Microsoft.Win32.SafeHandles;

namespace WinUIProfiler.Engine.Target;

[SupportedOSPlatform("windows")]
internal static class IsolatedProcess
{
    internal static Process Start(string executable, string arguments, string directory)
    {
        var security = new SecurityAttributes { Length = Marshal.SizeOf<SecurityAttributes>(), InheritHandle = true };
        using var input = CreateFileW("NUL", 0x80000000, 3, ref security, 3, 0, IntPtr.Zero);
        using var output = CreateFileW("NUL", 0x40000000, 3, ref security, 3, 0, IntPtr.Zero);
        if (input.IsInvalid || output.IsInvalid) throw new Win32Exception(Marshal.GetLastWin32Error(), "Opening isolated target streams failed.");
        nuint size = 0;
        InitializeProcThreadAttributeList(IntPtr.Zero, 1, 0, ref size);
        var attributes = Marshal.AllocHGlobal(checked((nint)size));
        var handles = Marshal.AllocHGlobal(IntPtr.Size * 2);
        var initialized = false;
        try
        {
            if (!InitializeProcThreadAttributeList(attributes, 1, 0, ref size)) throw new Win32Exception(Marshal.GetLastWin32Error());
            initialized = true;
            Marshal.WriteIntPtr(handles, input.DangerousGetHandle());
            Marshal.WriteIntPtr(handles, IntPtr.Size, output.DangerousGetHandle());
            // Inherit only NUL, never an authenticated TAP pipe or the profiler's stdout handle.
            if (!UpdateProcThreadAttribute(attributes, 0, (nuint)0x00020002, handles, (nuint)(IntPtr.Size * 2), IntPtr.Zero, IntPtr.Zero))
                throw new Win32Exception(Marshal.GetLastWin32Error());
            var startup = new StartupInfoEx
            {
                Startup = new StartupInfo
                {
                    Size = Marshal.SizeOf<StartupInfoEx>(), Flags = 0x00000100,
                    Input = input.DangerousGetHandle(), Output = output.DangerousGetHandle(), Error = output.DangerousGetHandle()
                },
                Attributes = attributes
            };
            var command = new StringBuilder($"\"{executable}\" {arguments}");
            if (!CreateProcessW(executable, command, IntPtr.Zero, IntPtr.Zero, true, 0x08080000,
                IntPtr.Zero, directory, ref startup, out var created))
                throw new Win32Exception(Marshal.GetLastWin32Error(), "Starting the isolated target failed.");
            using var processHandle = new SafeProcessHandle(created.Process, true);
            using var threadHandle = new SafeWaitHandle(created.Thread, true);
            var process = Process.GetProcessById(checked((int)created.ProcessId));
            try
            {
                _ = process.SafeHandle;
                return process;
            }
            catch
            {
                process.Dispose();
                throw;
            }
        }
        finally
        {
            if (initialized) DeleteProcThreadAttributeList(attributes);
            Marshal.FreeHGlobal(handles);
            Marshal.FreeHGlobal(attributes);
            GC.KeepAlive(input);
            GC.KeepAlive(output);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct SecurityAttributes
    {
        internal int Length;
        internal IntPtr Descriptor;
        [MarshalAs(UnmanagedType.Bool)] internal bool InheritHandle;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct StartupInfo
    {
        internal int Size;
        internal IntPtr Reserved;
        internal IntPtr Desktop;
        internal IntPtr Title;
        internal int X, Y, Width, Height, CharactersX, CharactersY, FillAttribute, Flags;
        internal short ShowWindow, ReservedSize;
        internal IntPtr ReservedBytes, Input, Output, Error;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct StartupInfoEx
    {
        internal StartupInfo Startup;
        internal IntPtr Attributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct ProcessInformation
    {
        internal IntPtr Process, Thread;
        internal uint ProcessId, ThreadId;
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true, ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern SafeFileHandle CreateFileW(string path, uint access, uint share, ref SecurityAttributes security,
        uint disposition, uint flags, IntPtr template);

    [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool InitializeProcThreadAttributeList(IntPtr list, int count, int flags, ref nuint size);

    [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool UpdateProcThreadAttribute(IntPtr list, uint flags, nuint attribute, IntPtr value, nuint size,
        IntPtr previous, IntPtr returnedSize);

    [DllImport("kernel32.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern void DeleteProcThreadAttributeList(IntPtr list);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true, ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CreateProcessW(string application, StringBuilder command, IntPtr processSecurity,
        IntPtr threadSecurity, [MarshalAs(UnmanagedType.Bool)] bool inheritHandles, uint flags, IntPtr environment,
        string directory, ref StartupInfoEx startup, out ProcessInformation process);
}
