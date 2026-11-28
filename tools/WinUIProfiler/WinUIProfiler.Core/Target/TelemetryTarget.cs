// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Globalization;
using System.Security.Cryptography;
using System.Text;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Target;

// The targetApp value of the usage events: a Microsoft app's name, or a short hash of another app's executable name.
public static class TelemetryTarget
{
    // Ends the package family names of Microsoft's packaged apps.
    private const string MicrosoftPublisherId = "_8wekyb3d8bbwe";

    // The process names of Microsoft apps people profile. A trace or a session file names only the process, so only these
    // names are sent as they are; any other is hashed.
    private static readonly HashSet<string> MicrosoftProcessNames = new(StringComparer.OrdinalIgnoreCase)
    {
        "CalculatorApp", "DevHome", "explorer", "Microsoft.Lightbox", "Microsoft.Media.Player", "Microsoft.Photos", "mspaint",
        "Notepad", "PowerToys", "PowerToys.Settings", "SnippingTool", "SystemSettings", "Time", "VoiceRecorder",
        "WindowsTerminal", "WinStore.App", "WinUIGallery",
    };

    public static string ForTarget(TargetLaunchKind kind, string target)
    {
        try
        {
            switch (kind)
            {
                case TargetLaunchKind.Packaged:
                    // Packages exist only on Windows. The family name comes from the package Windows finds for the AUMID,
                    // never from the text entered, which could be a path ending like a Microsoft family name.
                    if (!OperatingSystem.IsWindows())
                    {
                        return "unknown";
                    }

                    var (_, applicationId, executable) = TargetLauncher.GetPackagedApplication(target);
                    var family = applicationId[..applicationId.IndexOf('!')];
                    return family.EndsWith(MicrosoftPublisherId, StringComparison.OrdinalIgnoreCase)
                        ? family[..^MicrosoftPublisherId.Length]
                        : ForExecutable(executable);
                case TargetLaunchKind.Attach:
                    using (var process = Process.GetProcessById(int.Parse(target, CultureInfo.InvariantCulture)))
                    {
                        return MainModulePath(process) is { } path ? ForExecutable(path) : ForProcessName(process.ProcessName);
                    }
                default:
                    return ForExecutable(target.Trim('"'));
            }
        }
        catch (Exception)
        {
            return "unknown";
        }
    }

    // A recorded trace and a session file name only the process, so its publisher cannot be checked.
    public static string ForProcessName(string? name)
    {
        if (string.IsNullOrWhiteSpace(name))
        {
            return "unknown";
        }

        var process = name.Trim();
        if (process.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
        {
            process = process[..^4];
        }

        return MicrosoftProcessNames.TryGetValue(process, out var known) ? known : Hash(process);
    }

    public static string ForSessionFile(string path)
    {
        try
        {
            return ForProcessName(SessionFile.ReadProcessName(path));
        }
        catch (Exception)
        {
            return "unknown";
        }
    }

    private static string? MainModulePath(Process process)
    {
        try
        {
            return process.MainModule?.FileName;
        }
        catch (Exception)
        {
            // An elevated process, or another user's.
            return null;
        }
    }

    // An executable whose version information names Microsoft is named; any other as a trace's process is.
    private static string ForExecutable(string path) =>
        File.Exists(path) && FileVersionInfo.GetVersionInfo(path).CompanyName == "Microsoft Corporation"
            ? Path.GetFileNameWithoutExtension(path)
            : ForProcessName(Path.GetFileName(path));

    // The first 12 hex digits of the SHA-256 of the lowercase executable name, or "unknown" without a file name.
    // Process names have no extension.
    private static string Hash(string name)
    {
        var executable = Path.GetFileName(name.Trim()).ToLowerInvariant();
        if (string.IsNullOrWhiteSpace(executable))
        {
            return "unknown";
        }

        if (!executable.EndsWith(".exe", StringComparison.Ordinal))
        {
            executable += ".exe";
        }

        return Convert.ToHexStringLower(SHA256.HashData(Encoding.UTF8.GetBytes(executable)))[..12];
    }
}
