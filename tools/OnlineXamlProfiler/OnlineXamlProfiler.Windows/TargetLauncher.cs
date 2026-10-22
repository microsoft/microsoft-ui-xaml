// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Runtime.InteropServices;

namespace OnlineXamlProfiler.Windows;

public static class TargetLauncher
{
    public static uint LaunchExecutable(string path, string arguments)
    {
        string fullPath = Path.GetFullPath(path);
        if (!File.Exists(fullPath))
        {
            throw new FileNotFoundException("The target executable does not exist.", fullPath);
        }
        using var process = Process.Start(new ProcessStartInfo(fullPath, arguments)
        {
            UseShellExecute = false,
            WorkingDirectory = Path.GetDirectoryName(fullPath)!,
        }) ?? throw new InvalidOperationException("The executable did not start.");
        return checked((uint)process.Id);
    }

    public static uint LaunchPackaged(string aumid, string arguments)
    {
        if (string.IsNullOrWhiteSpace(aumid) || !aumid.Contains('!'))
        {
            throw new ArgumentException("Enter a package application user model ID (PackageFamilyName!ApplicationId).");
        }
        var manager = (IApplicationActivationManager)new ApplicationActivationManager();
        try
        {
            Marshal.ThrowExceptionForHR(manager.ActivateApplication(aumid, arguments, 0, out uint processId));
            return processId;
        }
        finally
        {
            Marshal.ReleaseComObject(manager);
        }
    }

    [ComImport, Guid("45BA127D-10A8-46EA-8AB7-56EA9078943C")]
    private class ApplicationActivationManager { }

    [ComImport, Guid("2E941141-7F97-4756-BA1D-9DECDE894A3D"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IApplicationActivationManager
    {
        [PreserveSig]
        int ActivateApplication([MarshalAs(UnmanagedType.LPWStr)] string appUserModelId,
            [MarshalAs(UnmanagedType.LPWStr)] string arguments, uint options, out uint processId);
    }
}
