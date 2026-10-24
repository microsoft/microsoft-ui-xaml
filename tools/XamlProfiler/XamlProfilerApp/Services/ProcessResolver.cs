using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Xml.Linq;

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
    /// Returns the file name (for example "MyApp.exe") of the executable that the installed
    /// package's AppxManifest.xml declares for the application in
    /// <paramref name="appUserModelId"/> ("PackageFamilyName!AppId"), or null when it can't
    /// be determined (manifest not readable, or an application without an Executable).
    /// </summary>
    public static string? TryGetPackagedAppExecutableName(string? installedPath, string appUserModelId)
    {
        try
        {
            int separator = appUserModelId.IndexOf('!');
            if (string.IsNullOrEmpty(installedPath) || separator < 0)
                return null;

            string appId = appUserModelId[(separator + 1)..];
            var manifest = XDocument.Load(Path.Combine(installedPath, "AppxManifest.xml"));
            var application = manifest.Descendants().FirstOrDefault(e =>
                e.Name.LocalName == "Application" &&
                e.Parent?.Name.LocalName == "Applications" &&
                string.Equals((string?)e.Attribute("Id"), appId, StringComparison.OrdinalIgnoreCase));

            string? executable = (string?)application?.Attribute("Executable");
            return string.IsNullOrEmpty(executable) ? null : Path.GetFileName(executable);
        }
        catch
        {
            return null;
        }
    }

    /// <summary>
    /// Returns true when <paramref name="proc"/> is known to run an executable other than
    /// <paramref name="executableName"/> (a file name such as "MyApp.exe"), compared by image
    /// name, which is what ETW's executable-name filter matches. False when it runs that
    /// executable or its name can't be read (for example, because it has exited).
    /// </summary>
    public static bool RunsOtherExecutable(Process proc, string executableName)
    {
        try
        {
            return !string.Equals(
                proc.ProcessName,
                Path.GetFileNameWithoutExtension(executableName),
                StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
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
