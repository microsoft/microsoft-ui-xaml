using System.Runtime.InteropServices;
using Windows.Management.Deployment;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.Services;

internal static class TargetDiscovery
{
    public static Task<IReadOnlyList<TargetCandidate>> ProcessesAsync(CancellationToken token)
    {
        var candidates = TargetProcessDiscovery.Discover(token).Select(candidate => new TargetCandidate(
            candidate.Name, candidate.ProcessId, TargetLaunchKind.Attach, candidate.CreationTimeFileTime, candidate.Error)).ToArray();
        return Task.FromResult<IReadOnlyList<TargetCandidate>>(candidates);
    }

    public static async Task<IReadOnlyList<TargetCandidate>> PackagedAppsAsync(CancellationToken token)
    {
        var candidates = new List<TargetCandidate>();
        var manager = new PackageManager();
        foreach (var package in manager.FindPackagesForUser(""))
        {
            token.ThrowIfCancellationRequested();
            if (package.IsFramework || package.IsResourcePackage || package.IsBundle)
            {
                continue;
            }

            try
            {
                var entries = await package.GetAppListEntriesAsync().AsTask(token).ConfigureAwait(false);
                foreach (var entry in entries)
                {
                    candidates.Add(new(entry.DisplayInfo.DisplayName, entry.AppUserModelId, TargetLaunchKind.Packaged));
                }
            }
            catch (Exception error) when (error is COMException or UnauthorizedAccessException)
            {
                candidates.Add(new(package.Id.Name, package.Id.FamilyName, TargetLaunchKind.Packaged, Error: error.Message));
            }
        }
        return candidates;
    }
}
