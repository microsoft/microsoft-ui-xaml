// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Microsoft.Win32.SafeHandles;

namespace WinUIProfiler.Engine.Target;

// The process that created another, its parent. Windows keeps only the parent's ID, which a process started after the
// parent exited can reuse, so a process with that ID is named as the parent only if it started before the child.
[SupportedOSPlatform("windows")]
internal static class ProcessParent
{
    private const int ProcessBasicInformationClass = 0;
    private const uint QueryLimitedInformation = 0x1000;

    // The parent's ID, and its executable name without .exe while it runs; null when the child can't be queried. The ID is
    // returned even when the parent has exited, or another process has its ID now, as the launch events it wrote carry it.
    public static (int ProcessId, string? Name)? Find(Process process)
    {
        ArgumentNullException.ThrowIfNull(process);
        try
        {
            return Find(process.SafeHandle, process.StartTime.ToUniversalTime());
        }
        catch (Exception error) when (error is InvalidOperationException or Win32Exception)
        {
            return null;
        }
    }

    // As Find(Process), for the process instance with this ID and creation time, a UTC FILETIME; null when another
    // instance has the ID. An exited process can still be queried while a handle to it is open.
    public static (int ProcessId, string? Name)? Find(int processId, long creationTimeFileTime)
    {
        using var process = OpenProcess(QueryLimitedInformation, false, processId);
        return !process.IsInvalid && GetProcessTimes(process, out var created, out _, out _, out _) &&
            created == creationTimeFileTime ? Find(process, DateTime.FromFileTimeUtc(created)) : null;
    }

    private static (int ProcessId, string? Name)? Find(SafeProcessHandle process, DateTime startedUtc)
    {
        if (NtQueryInformationProcess(process, ProcessBasicInformationClass, out var information,
            Marshal.SizeOf<ProcessBasicInformation>(), out _) != 0)
        {
            return null;
        }

        var parentId = (int)information.InheritedFromUniqueProcessId;
        return (parentId, FindName(parentId, startedUtc));
    }

    // Null for a process that started after the child, which only reused the parent's ID.
    internal static string? FindName(int processId, DateTime childStartedUtc)
    {
        try
        {
            using var parent = Process.GetProcessById(processId);
            return parent.StartTime.ToUniversalTime() <= childStartedUtc ? parent.ProcessName : null;
        }
        catch (Exception error) when (error is ArgumentException or InvalidOperationException or Win32Exception)
        {
            return null;
        }
    }

    [DllImport("ntdll.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern int NtQueryInformationProcess(SafeProcessHandle process, int informationClass,
        out ProcessBasicInformation information, int length, out int returnLength);

    [DllImport("kernel32.dll", ExactSpelling = true, SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern SafeProcessHandle OpenProcess(uint access, [MarshalAs(UnmanagedType.Bool)] bool inherit, int processId);

    [DllImport("kernel32.dll", ExactSpelling = true, SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetProcessTimes(SafeProcessHandle process, out long created, out long exited,
        out long kernel, out long user);

    // winternl.h's PROCESS_BASIC_INFORMATION; ExitStatus and BasePriority are pointer-sized to keep 64-bit padding.
    private struct ProcessBasicInformation
    {
        public IntPtr ExitStatus;
        public IntPtr PebBaseAddress;
        public UIntPtr AffinityMask;
        public IntPtr BasePriority;
        public UIntPtr UniqueProcessId;
        public UIntPtr InheritedFromUniqueProcessId;
    }
}
