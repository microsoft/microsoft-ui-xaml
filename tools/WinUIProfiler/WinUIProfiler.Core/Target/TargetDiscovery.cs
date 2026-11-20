using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;

namespace WinUIProfiler.Engine.Target;

public sealed record ProcessCandidate(string Name, string ProcessId, long? CreationTimeFileTime = null, string? Error = null);

public static class TargetProcessDiscovery
{
    public static IReadOnlyList<ProcessCandidate> Discover(CancellationToken token = default)
    {
        var candidates = new List<ProcessCandidate>();
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
                    candidates.Add(new(name, pid, process.StartTime.ToFileTimeUtc()));
                }
                catch (Exception error) when (error is Win32Exception or InvalidOperationException or NotSupportedException)
                {
                    candidates.Add(new(name, pid, Error: error.Message));
                }
            }
        }
        finally
        {
            foreach (var process in processes) process.Dispose();
        }
        return candidates.OrderBy(candidate => candidate.Name, StringComparer.OrdinalIgnoreCase)
            .ThenBy(candidate => candidate.ProcessId, StringComparer.Ordinal).ToArray();
    }
}
