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

    // The parent's ID, and its executable name without .exe while it runs; null when the child can't be queried.
    public static (int ProcessId, string? Name)? Find(Process process)
    {
        ArgumentNullException.ThrowIfNull(process);
        try
        {
            if (NtQueryInformationProcess(process.SafeHandle, ProcessBasicInformationClass, out var information,
                Marshal.SizeOf<ProcessBasicInformation>(), out _) != 0)
            {
                return null;
            }

            var parentId = (int)information.InheritedFromUniqueProcessId;
            return (parentId, FindName(parentId, process.StartTime.ToUniversalTime()));
        }
        catch (Exception error) when (error is InvalidOperationException or Win32Exception)
        {
            return null;
        }
    }

    private static string? FindName(int processId, DateTime childStartedUtc)
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
