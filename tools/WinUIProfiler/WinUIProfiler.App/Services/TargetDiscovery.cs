using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.InteropServices;
using Windows.Management.Deployment;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.Services;

internal static class TargetDiscovery
{
    public static Task<IReadOnlyList<TargetCandidate>> ProcessesAsync(CancellationToken token)
    {
        var candidates = new List<TargetCandidate>();
        var processes = Process.GetProcesses();
        try
        {
            foreach (var process in processes)
            {
                token.ThrowIfCancellationRequested();
                if (process.Id == Environment.ProcessId) continue;
                var pid = process.Id.ToString(CultureInfo.InvariantCulture);
                var name = pid;
                try
                {
                    name = process.ProcessName;
                    candidates.Add(new(name, pid, TargetLaunchKind.Attach, process.StartTime.ToFileTimeUtc()));
                }
                catch (Exception error) when (error is Win32Exception or InvalidOperationException or NotSupportedException)
                {
                    candidates.Add(new(name, pid, TargetLaunchKind.Attach, Error: error.Message));
                }
            }
        }
        finally { foreach (var process in processes) process.Dispose(); }
        return Task.FromResult<IReadOnlyList<TargetCandidate>>(candidates);
    }

    public static async Task<IReadOnlyList<TargetCandidate>> PackagedAppsAsync(CancellationToken token)
    {
        var candidates = new List<TargetCandidate>();
        var manager = new PackageManager();
        foreach (var package in manager.FindPackagesForUser(""))
        {
            token.ThrowIfCancellationRequested();
            if (package.IsFramework || package.IsResourcePackage || package.IsBundle) continue;
            try
            {
                var entries = await package.GetAppListEntriesAsync().AsTask(token).ConfigureAwait(false);
                foreach (var entry in entries)
                    candidates.Add(new(entry.DisplayInfo.DisplayName, entry.AppUserModelId, TargetLaunchKind.Packaged));
            }
            catch (Exception error) when (error is COMException or UnauthorizedAccessException)
            {
                candidates.Add(new(package.Id.Name, package.Id.FamilyName, TargetLaunchKind.Packaged, Error: error.Message));
            }
        }
        return candidates;
    }
}
