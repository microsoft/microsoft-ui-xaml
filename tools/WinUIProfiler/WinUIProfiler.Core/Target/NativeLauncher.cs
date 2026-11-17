using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Reflection;
using System.Reflection.PortableExecutable;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Security.Principal;
using System.Text.Json;
using Microsoft.Win32.SafeHandles;

namespace WinUIProfiler.Engine.Target;

public sealed record NativeLauncherHash(string Architecture, string LauncherSHA256, string RedirectSHA256);
public sealed record NativeLauncherManifest(int SchemaVersion, IReadOnlyList<NativeLauncherHash> Launchers);

// Launches the target through the private WinUI launcher (WinUIProfiler.Launcher\README.md). The launcher never runs
// elevated: an elevated profiler starts it from the signed-in user's Explorer desktop, so the app is not elevated either.
[SupportedOSPlatform("windows")]
public static class NativeLauncher
{
    private const string Mux = "Microsoft.UI.Xaml.dll";
    private const string Muxc = "Microsoft.UI.Xaml.Controls.dll";
    private static readonly TimeSpan OwnerTimeout = TimeSpan.FromSeconds(15);

    public static async Task<NativeLaunchSession> LaunchAsync(TargetLaunchKind kind, string target, string arguments,
        PrivateRuntimeLaunch runtime, CancellationToken token)
    {
        string executable;
        string? package = null;
        string? applicationId = null;
        if (kind == TargetLaunchKind.Packaged)
        {
            (package, applicationId, executable) = TargetLauncher.GetPackagedApplication(target);
        }
        else
        {
            executable = Path.GetFullPath(target);
        }

        if (!File.Exists(executable))
        {
            throw new FileNotFoundException("Target executable does not exist.", executable);
        }

        foreach (var name in RedirectedFiles(runtime))
        {
            var path = Path.Combine(runtime.RuntimeDirectory, name);
            if (!File.Exists(path))
            {
                throw new FileNotFoundException($"The private WinUI folder does not contain {name}.", path);
            }
        }

        var deployed = Resolve(runtime.LauncherDirectory, GetArchitecture(executable));
        var directory = CreateLaunchDirectory(runtime.LaunchDirectory);
        var native = Directory.CreateDirectory(Path.Combine(directory, "native")).FullName;
        // The launcher runs from a short per-launch path: a packaged app's debugger command must fit in MAX_PATH.
        var launcher = Copy(deployed.Launcher, Path.Combine(native, "ProfilerLauncher.exe"), deployed.LauncherSHA256);
        Copy(deployed.Redirect, Path.Combine(native, "ProfilerRedirect.dll"), deployed.RedirectSHA256);
        var result = Path.Combine(directory, NativeLaunchProtocol.ResultFile);
        var log = Path.Combine(directory, NativeLaunchProtocol.LogFile);
        var commandLine = NativeLaunchProtocol.BuildArguments(executable, runtime, arguments, log, result, applicationId, package);
        using var owner = await StartAsync(launcher, commandLine, native, result + NativeLaunchProtocol.OwnerSuffix, token)
            .ConfigureAwait(false);
        var launch = await WaitForResultAsync(result, owner, token).ConfigureAwait(false);
        if (launch.Error != 0)
        {
            throw new InvalidOperationException(Describe(launch, Path.Combine(directory, NativeLaunchProtocol.LogFileFor(launch))));
        }

        if (applicationId is not null &&
            (launch.PackageFullName != package || launch.ApplicationUserModelId != applicationId))
        {
            throw new InvalidOperationException("The private WinUI launcher started a different application.");
        }

        var process = Process.GetProcessById(launch.ProcessId);
        try
        {
            // The handle keeps the ID from being reused, so this Process stays the launched instance.
            _ = process.SafeHandle;
            if (process.StartTime.ToFileTimeUtc() != launch.CreationTimeFileTime ||
                !IsSameFile(process.MainModule?.FileName, executable))
            {
                throw new InvalidOperationException($"PID {launch.ProcessId} is not the launched {executable}.");
            }

            var session = new NativeLaunchSession(process, launch, directory);
            try
            {
                if (launch.ActivationProcessId is not null)
                {
                    using var accepted = File.Create(result + NativeLaunchProtocol.AcceptedSuffix);
                }
                if (owner is not null && !owner.WaitForExit(TimeSpan.FromSeconds(5)))
                {
                    Trace.TraceWarning($"The private WinUI launcher (PID {owner.Id}) is still running after its result.");
                }
                return session;
            }
            catch
            {
                session.Dispose();
                throw;
            }
        }
        catch
        {
            process.Dispose();
            throw;
        }
    }

    // Throws unless the target loaded one Microsoft.UI.Xaml.dll, the private one, and, when it is loaded and redirected,
    // the private Microsoft.UI.Xaml.Controls.dll.
    public static void VerifyModules(Process target, PrivateRuntimeLaunch runtime)
    {
        ArgumentNullException.ThrowIfNull(target);
        ArgumentNullException.ThrowIfNull(runtime);
        target.Refresh();
        var modules = target.Modules.Cast<ProcessModule>().Select(module => (module.ModuleName, module.FileName)).ToList();
        foreach (var name in RedirectedFiles(runtime))
        {
            var loaded = modules.Where(module => module.ModuleName.Equals(name, StringComparison.OrdinalIgnoreCase))
                .Select(module => module.FileName).ToList();
            var expected = Path.Combine(runtime.RuntimeDirectory, name);
            if ((loaded.Count != 0 || name == Mux) && (loaded.Count != 1 || !IsSameFile(loaded[0], expected)))
            {
                throw new InvalidOperationException(loaded.Count == 0
                    ? $"The app did not load {name}."
                    : $"The app loaded {string.Join(", ", loaded)} instead of the private {expected}.");
            }
        }
    }

    private static string[] RedirectedFiles(PrivateRuntimeLaunch runtime) => runtime.RedirectControls ? [Mux, Muxc] : [Mux];

    private static string GetArchitecture(string executable)
    {
        using var stream = File.OpenRead(executable);
        using var reader = new PEReader(stream);
        return reader.PEHeaders.CoffHeader.Machine switch
        {
            Machine.Amd64 => "x64",
            Machine.Arm64 => "ARM64",
            var machine => throw new PlatformNotSupportedException(
                $"Private WinUI DLLs can be loaded by x64 and ARM64 apps; {Path.GetFileName(executable)} is {machine}.")
        };
    }

    private static (string Launcher, string LauncherSHA256, string Redirect, string RedirectSHA256) Resolve(string directory,
        string architecture)
    {
        var manifestPath = Path.Combine(directory, "manifest.json");
        if (!File.Exists(manifestPath))
        {
            throw new FileNotFoundException("The private WinUI launcher is not deployed. Build the profiler with scripts\\Build-App.ps1.",
                manifestPath);
        }

        if (new FileInfo(manifestPath).Length > 16384)
        {
            throw new InvalidDataException("The private WinUI launcher manifest exceeds its bound.");
        }

        var manifest = JsonSerializer.Deserialize<NativeLauncherManifest>(File.ReadAllBytes(manifestPath)) ??
            throw new InvalidDataException("The private WinUI launcher manifest is empty.");
        if (manifest.SchemaVersion != 1 ||
            manifest.Launchers is null ||
            manifest.Launchers.Any(entry => entry is null) ||
            manifest.Launchers.Select(entry => entry.Architecture).Distinct(StringComparer.Ordinal).Count() != manifest.Launchers.Count)
        {
            throw new InvalidDataException("The private WinUI launcher manifest version or architecture entries are invalid.");
        }

        var entry = manifest.Launchers.SingleOrDefault(entry => entry.Architecture == architecture) ??
            throw new FileNotFoundException($"The deployment does not contain the {architecture} private WinUI launcher.");
        return (Path.Combine(directory, architecture, "ProfilerLauncher.exe"), entry.LauncherSHA256,
            Path.Combine(directory, architecture, "ProfilerRedirect.dll"), entry.RedirectSHA256);
    }

    private static string Copy(string source, string destination, string expected)
    {
        if (expected is null || expected.Length != 64 || !expected.All(char.IsAsciiHexDigit))
        {
            throw new InvalidDataException("The private WinUI launcher hash is invalid.");
        }

        File.Copy(source, destination, overwrite: false);
        using var stream = File.OpenRead(destination);
        if (!string.Equals(Convert.ToHexString(SHA256.HashData(stream)), expected, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException($"Private WinUI launcher hash mismatch: {source}");
        }

        return destination;
    }

    // Each launch gets a new numbered folder, so a reconnection does not overwrite an earlier launch's result.
    private static string CreateLaunchDirectory(string root)
    {
        Directory.CreateDirectory(root);
        for (var number = 1; number <= 999; number++)
        {
            var path = Path.Combine(root, number.ToString(CultureInfo.InvariantCulture));
            if (!Path.Exists(path))
            {
                return Directory.CreateDirectory(path).FullName;
            }
        }

        throw new IOException($"The launch directory holds too many launches: {root}");
    }

    // Returns the launcher process, or null when it already exited before it could be opened.
    private static async Task<Process?> StartAsync(string launcher, string commandLine, string directory, string owner,
        CancellationToken token)
    {
        if (!Environment.IsPrivilegedProcess)
        {
            return Process.Start(new ProcessStartInfo(launcher, commandLine) { UseShellExecute = false, WorkingDirectory = directory }) ??
                throw new InvalidOperationException("The private WinUI launcher did not start.");
        }

        ExecuteFromDesktop(launcher, commandLine, directory);
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(token);
        timeout.CancelAfter(OwnerTimeout);
        while (!File.Exists(owner))
        {
            try
            {
                await Task.Delay(100, timeout.Token).ConfigureAwait(false);
            }
            catch (OperationCanceledException) when (!token.IsCancellationRequested)
            {
                throw new TimeoutException("The private WinUI launcher did not start from the Explorer desktop.");
            }
        }

        var (pid, created) = NativeLaunchProtocol.ParseOwner(ReadBounded(owner));
        Process process;
        try
        {
            process = Process.GetProcessById(pid);
            // Keeps a handle, so the exit code stays readable after the launcher exits and its ID is not reused.
            _ = process.SafeHandle;
        }
        catch (ArgumentException)
        {
            return null;
        }

        try
        {
            if (process.StartTime.ToFileTimeUtc() != created)
            {
                // Another creation time means the ID was reused: this launch's launcher already exited.
                process.Dispose();
                return null;
            }

            if (!IsSameFile(process.MainModule?.FileName, launcher))
            {
                throw new InvalidOperationException($"PID {pid} is not this launch's private WinUI launcher.");
            }

            return process;
        }
        catch (Exception) when (process.HasExited)
        {
            process.Dispose();
            return null;
        }
        catch
        {
            process.Dispose();
            throw;
        }
    }

    private static async Task<NativeLaunchResult> WaitForResultAsync(string result, Process? launcher, CancellationToken token)
    {
        while (true)
        {
            if (File.Exists(result))
            {
                return NativeLaunchProtocol.ParseResult(ReadBounded(result));
            }

            // The launcher writes the result before it exits.
            if (launcher is { HasExited: true } && !File.Exists(result))
            {
                throw new InvalidOperationException(
                    $"The private WinUI launcher exited with code {launcher.ExitCode} without a result: {result}");
            }

            await Task.Delay(100, token).ConfigureAwait(false);
        }
    }

    private static string ReadBounded(string path)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        if (stream.Length > NativeLaunchProtocol.MaxResultBytes)
        {
            throw new InvalidDataException($"The private WinUI launcher's file exceeds its bound: {path}");
        }

        using var reader = new StreamReader(stream);
        return reader.ReadToEnd();
    }

    private static string Describe(NativeLaunchResult launch, string log)
    {
        var reason = launch switch
        {
            { Stage: "helper_ready", Error: 1460 } => "the app did not report the private WinUI helper ready within 30 seconds",
            // ERROR_NO_UNICODE_TRANSLATION: the app's import table cannot name the helper (WinUIProfiler.Launcher\README.md).
            { Stage: "create_process" or "package_debugger", Error: 1113 } =>
                "the launch folder's path has characters outside the system's ANSI code page and no 8.3 short name, " +
                "so the app cannot load the private WinUI helper from it; use a launch folder whose path uses only " +
                "characters in that code page",
            _ => new Win32Exception(unchecked((int)launch.Error)).Message,
        };
        var helper = "";
        try
        {
            // The helper logs its failures as events, for example runtime_already_loaded when WinUI was loaded before it
            // started. A packaged app may write the log elsewhere (WinUIProfiler.Launcher\README.md).
            if (File.Exists(log))
            {
                var failures = new List<string>();
                foreach (var line in File.ReadLines(log))
                {
                    using var entry = JsonDocument.Parse(line);
                    if (entry.RootElement.TryGetProperty("event", out var name) && name.GetString() is { } value &&
                        value is not ("hook_ready" or "redirected" or "child_injected") && !failures.Contains(value))
                    {
                        failures.Add(value);
                    }
                }

                helper = failures.Count == 0 ? "" : $" Helper: {string.Join(", ", failures)}.";
            }
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException or InvalidOperationException)
        {
        }

        return $"The private WinUI launch failed at {launch.Stage} with error {launch.Error}: {reason}.{helper} Log: {log}";
    }

    private static bool IsSameFile(string? first, string second)
    {
        if (string.IsNullOrEmpty(first))
        {
            return false;
        }

        if (string.Equals(Path.GetFullPath(first), Path.GetFullPath(second), StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        try
        {
            return GetFileId(first) == GetFileId(second);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    private static (ulong Volume, Guid File) GetFileId(string path)
    {
        using var handle = File.OpenHandle(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        if (!GetFileInformationByHandleEx(handle, FileIdInfoClass, out var info, Marshal.SizeOf<FileIdInfo>()))
        {
            throw new IOException($"Cannot read the file identity of {path}.", new Win32Exception());
        }

        return (info.VolumeSerialNumber, info.FileId);
    }

    // IShellDispatch2::ShellExecute on the desktop's shell view runs the command in Explorer, as the signed-in user.
    private static void ExecuteFromDesktop(string file, string arguments, string directory)
    {
        var window = GetShellWindow();
        if (window == IntPtr.Zero || GetWindowThreadProcessId(window, out var shellId) == 0)
        {
            throw new InvalidOperationException("The Explorer desktop is not running, so the app cannot be started without administrator rights.");
        }

        using (var shell = OpenProcess(ProcessQueryLimitedInformation, false, shellId))
        {
            if (shell.IsInvalid || !OpenProcessToken(shell, TokenQuery, out var token))
            {
                throw new Win32Exception(Marshal.GetLastPInvokeError(), "Cannot read the Explorer desktop's user.");
            }

            using (token)
            using (var desktopUser = new WindowsIdentity(token.DangerousGetHandle()))
            using (var currentUser = WindowsIdentity.GetCurrent())
            {
                if (desktopUser.User != currentUser.User)
                {
                    throw new InvalidOperationException("The Explorer desktop belongs to another user. Run the profiler elevated as the signed-in user.");
                }
            }
        }

        object? windows = null;
        object? desktop = null;
        object? view = null;
        object? application = null;
        try
        {
            windows = Activator.CreateInstance(Type.GetTypeFromCLSID(new("9BA05972-F6A8-11CF-A442-00A0C90A8F39"), throwOnError: true)!) ??
                throw new InvalidOperationException("The Explorer shell windows are unavailable.");
            object location = 0; // CSIDL_DESKTOP
            object? root = null;
            Marshal.ThrowExceptionForHR(((IShellWindows)windows).FindWindowSW(ref location, ref root, SwcDesktop, out _,
                SwfoNeedDispatch, out desktop));
            view = GetProperty(desktop ?? throw new InvalidOperationException("The Explorer desktop window was not found."), "Document");
            application = GetProperty(view, "Application");
            // ShellExecute returns nothing; a failure surfaces as an exception.
            application.GetType().InvokeMember("ShellExecute", BindingFlags.InvokeMethod, null, application,
                [file, arguments, directory, "open", 1], CultureInfo.InvariantCulture);
        }
        finally
        {
            foreach (var item in new[] { application, view, desktop, windows })
            {
                if (item is not null && Marshal.IsComObject(item))
                {
                    Marshal.FinalReleaseComObject(item);
                }
            }
        }
    }

    private static object GetProperty(object target, string name) =>
        target.GetType().InvokeMember(name, BindingFlags.GetProperty, null, target, null, CultureInfo.InvariantCulture) ??
            throw new InvalidOperationException($"The Explorer desktop returned no {name}.");

    private const int FileIdInfoClass = 18;
    private const int SwcDesktop = 8;
    private const int SwfoNeedDispatch = 1;
    private const uint ProcessQueryLimitedInformation = 0x1000;
    private const uint TokenQuery = 0x0008;

    [StructLayout(LayoutKind.Sequential)]
    private struct FileIdInfo
    {
        public ulong VolumeSerialNumber;
        public Guid FileId;
    }

    // exdisp.h; only FindWindowSW is called, the earlier methods keep the vtable layout.
    [ComImport, Guid("85CB6900-4D95-11CF-960C-0080C7F4EE85"), InterfaceType(ComInterfaceType.InterfaceIsDual)]
    private interface IShellWindows
    {
        [PreserveSig]
        int GetCount(out int count);
        [PreserveSig]
        int Item(object index, [MarshalAs(UnmanagedType.IDispatch)] out object folder);
        [PreserveSig]
        int NewEnum([MarshalAs(UnmanagedType.IUnknown)] out object enumerator);
        [PreserveSig]
        int Register([MarshalAs(UnmanagedType.IDispatch)] object window, int hwnd, int windowClass, out int cookie);
        [PreserveSig]
        int RegisterPending(int threadId, ref object location, ref object root, int windowClass, out int cookie);
        [PreserveSig]
        int Revoke(int cookie);
        [PreserveSig]
        int OnNavigate(int cookie, ref object location);
        [PreserveSig]
        int OnActivated(int cookie, short active);
        [PreserveSig]
        int FindWindowSW(ref object location, ref object? root, int windowClass, out int hwnd, int options,
            [MarshalAs(UnmanagedType.IDispatch)] out object? window);
    }

    [DllImport("user32.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern IntPtr GetShellWindow();

    [DllImport("user32.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);

    [DllImport("kernel32.dll", ExactSpelling = true, SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern SafeProcessHandle OpenProcess(uint access, bool inheritHandle, uint processId);

    [DllImport("advapi32.dll", ExactSpelling = true, SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern bool OpenProcessToken(SafeProcessHandle process, uint access, out SafeAccessTokenHandle token);

    [DllImport("kernel32.dll", ExactSpelling = true, SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern bool GetFileInformationByHandleEx(SafeFileHandle file, int informationClass, out FileIdInfo information,
        int size);
}
