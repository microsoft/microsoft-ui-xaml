using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace XamlProfiler.Services;

/// <summary>
/// Pure (UI-free) helpers for locating the target app's process and waiting for it to
/// present a top-level window. Extracted from MainWindow so the process-resolution logic
/// is independently testable and free of any XAML/UI dependencies.
/// </summary>
internal static class ProcessResolver
{
    private const int ErrorSuccess = 0;
    private const int ErrorInsufficientBuffer = 122;

    /// <summary>
    /// Returns true when Windows reports that the process belongs to the package family.
    /// This works for both installed packages and loose development registrations.
    /// </summary>
    public static bool ProcessMatchesPackageFamily(Process proc, string packageFamilyName)
    {
        try
        {
            uint length = 0;
            int result = GetPackageFamilyName(proc.Handle, ref length, null);
            if (result != ErrorInsufficientBuffer || length == 0)
                return false;

            var familyName = new StringBuilder((int)length);
            result = GetPackageFamilyName(proc.Handle, ref length, familyName);
            return result == ErrorSuccess &&
                string.Equals(familyName.ToString(), packageFamilyName, StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            // Access denied for system processes — skip.
            return false;
        }
    }

    /// <summary>
    /// Finds the first process whose OS-reported package identity matches the package
    /// family name.
    /// </summary>
    public static Process? FindProcessByPackageFamily(string packageFamilyName)
    {
        foreach (var proc in Process.GetProcesses())
        {
            if (ProcessMatchesPackageFamily(proc, packageFamilyName))
                return proc;
        }
        return null;
    }

    /// <summary>
    /// Polls (up to ~12s) for a process matching <paramref name="match"/> that owns a
    /// top-level window (MainWindowHandle != 0). A live main window means the XAML core
    /// is initialized and has registered its "WinUIVisualDiagConnection1" diagnostics
    /// port, so tap injection (InitializeXamlDiagnosticsEx) finds the endpoint instead of
    /// failing with ERROR_NOT_FOUND (0x80070490). This mirrors WinUISnoop, which always
    /// targets the fully-rendered window under the cursor.
    ///
    /// If the window never appears within the timeout, falls back to the most-recently
    /// started matching process (preferring one other than the launcher stub) so behavior
    /// is no worse than trusting the launch pid.
    /// </summary>
    public static async Task<Process?> ResolveLiveWindowProcessAsync(Func<Process, bool> match, Process? launched)
    {
        const int timeoutMs = 12000;
        const int pollMs = 250;
        var sw = Stopwatch.StartNew();
        Process? fallback = null;

        while (sw.ElapsedMilliseconds < timeoutMs)
        {
            // Prefer the directly launched process if it grew a window itself.
            if (launched != null)
            {
                try
                {
                    launched.Refresh();
                    if (!launched.HasExited && match(launched))
                    {
                        if (launched.MainWindowHandle != IntPtr.Zero)
                            return launched;
                    }
                }
                catch { }
            }

            foreach (var proc in Process.GetProcesses())
            {
                try
                {
                    if (!match(proc))
                        continue;

                    if (proc.MainWindowHandle != IntPtr.Zero)
                        return proc;

                    // Remember the newest matching (windowless) process as a fallback.
                    if (launched == null || proc.Id != launched.Id)
                    {
                        if (fallback == null || proc.StartTime > fallback.StartTime)
                            fallback = proc;
                    }
                }
                catch { }
            }

            await Task.Delay(pollMs);
        }

        return fallback ?? (launched != null && match(launched) && !launched.HasExited ? launched : null);
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetPackageFamilyName(
        IntPtr process,
        ref uint packageFamilyNameLength,
        StringBuilder? packageFamilyName);
}
