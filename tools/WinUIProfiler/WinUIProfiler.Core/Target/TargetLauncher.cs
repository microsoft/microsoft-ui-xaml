using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text;
using System.Xml;
using System.Xml.Linq;

namespace WinUIProfiler.Engine.Target;

[SupportedOSPlatform("windows")]
public static class TargetLauncher
{
    public static Process LaunchUnpackaged(string executable, string arguments, string? workingDirectory)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(executable);
        ArgumentNullException.ThrowIfNull(arguments);
        if (!Path.IsPathFullyQualified(executable))
        {
            throw new ArgumentException("Select a full executable path.", nameof(executable));
        }

        var fullPath = Path.GetFullPath(executable);
        if (!File.Exists(fullPath))
        {
            throw new FileNotFoundException("Target executable does not exist.", fullPath);
        }

        var directory = string.IsNullOrWhiteSpace(workingDirectory) ? Path.GetDirectoryName(fullPath)! :
                    Path.GetFullPath(workingDirectory);
        if (!Directory.Exists(directory))
        {
            throw new DirectoryNotFoundException(directory);
        }

        return Process.Start(new ProcessStartInfo(fullPath, arguments)
        {
            UseShellExecute = false,
            WorkingDirectory = directory,
            CreateNoWindow = true
        }) ?? throw new InvalidOperationException("Target process did not start.");
    }

    public static Process LaunchPackaged(string applicationId, string arguments)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(applicationId);
        ArgumentNullException.ThrowIfNull(arguments);
        var separator = applicationId.IndexOf('!');
        if (applicationId.Length > 128 ||
separator < 1 ||
separator == applicationId.Length - 1 ||
            applicationId.LastIndexOf('!') != separator ||
applicationId.Any(char.IsWhiteSpace))
        {
            throw new ArgumentException("Enter a packaged application AUMID: PackageFamilyName!ApplicationId.", nameof(applicationId));
        }

        var type = Type.GetTypeFromCLSID(new("45BA127D-10A8-46EA-8AB7-56EA9078943C"), throwOnError: true)!;
        var activation = (IApplicationActivationManager)(Activator.CreateInstance(type) ??
            throw new InvalidOperationException("Windows application activation is unavailable."));
        try
        {
            Marshal.ThrowExceptionForHR(activation.ActivateApplication(applicationId, arguments, 0, out var pid));
            if (pid == 0)
            {
                throw new InvalidOperationException("Activation returned no target process.");
            }

            var process = Process.GetProcessById(checked((int)pid));
            try
            {
                uint length = 0;
                var result = GetApplicationUserModelId(process.SafeHandle, ref length, null);
                if (result != 122 || length is < 2 or > 129)
                {
                    throw new Win32Exception(checked((int)result), "Cannot verify the activated application identity.");
                }

                var value = new StringBuilder(checked((int)length));
                result = GetApplicationUserModelId(process.SafeHandle, ref length, value);
                if (result != 0)
                {
                    throw new Win32Exception(checked((int)result), "Cannot read the activated application identity.");
                }

                if (!string.Equals(value.ToString(), applicationId, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidOperationException("Activation returned a different application; it is not a verified target.");
                }

                return process;
            }
            catch
            {
                process.Dispose();
                throw;

            }
        }
        finally
        {
            Marshal.FinalReleaseComObject(activation);

        }
    }

    // The executable file names that activating this AUMID can start, read from the manifests of the family's
    // packages registered for the current user (the registration activation uses). Empty when the application
    // declares no executable, for example a hosted app.
    public static IReadOnlyList<string> GetPackagedExecutableNames(string applicationId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(applicationId);
        var separator = applicationId.IndexOf('!');
        if (separator < 1 || separator == applicationId.Length - 1)
        {
            throw new ArgumentException("Enter a packaged application AUMID: PackageFamilyName!ApplicationId.", nameof(applicationId));
        }

        var appId = applicationId[(separator + 1)..];
        var names = new SortedSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var fullName in GetPackageFullNames(applicationId[..separator]))
        {
            using var reader = XmlReader.Create(Path.Combine(GetPackagePath(fullName), "AppxManifest.xml"),
                new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit });
            var manifest = XDocument.Load(reader);
            var foundation = manifest.Root!.Name.Namespace;
            foreach (var application in manifest.Root.Elements(foundation + "Applications").Elements(foundation + "Application"))
            {
                if (string.Equals((string?)application.Attribute("Id"), appId, StringComparison.OrdinalIgnoreCase) &&
                    (string?)application.Attribute("Executable") is { Length: > 0 } executable)
                {
                    // The manifest path is relative to the package (for example App\Foo.exe); ETW matches the file name.
                    names.Add(Path.GetFileName(executable));
                }
            }
        }

        return [.. names];
    }

    private static string[] GetPackageFullNames(string packageFamilyName)
    {
        uint count = 0;
        uint length = 0;
        var result = GetPackagesByPackageFamily(packageFamilyName, ref count, null, ref length, null);
        if (result == 0 && count == 0)
        {
            return [];
        }

        if (result != 122)
        {
            throw new Win32Exception(result, "Cannot list the packages of the application's family.");
        }

        var buffer = new char[length];
        result = GetPackagesByPackageFamily(packageFamilyName, ref count, new IntPtr[count], ref length, buffer);
        if (result != 0)
        {
            throw new Win32Exception(result, "Cannot list the packages of the application's family.");
        }

        // The buffer holds the full names one after another, each null-terminated.
        return new string(buffer, 0, checked((int)length)).Split('\0', StringSplitOptions.RemoveEmptyEntries);
    }

    private static string GetPackagePath(string packageFullName)
    {
        uint length = 0;
        var result = GetPackagePathByFullName(packageFullName, ref length, null);
        if (result != 122)
        {
            throw new Win32Exception(result, "Cannot find the package's install folder.");
        }

        var path = new char[length];
        result = GetPackagePathByFullName(packageFullName, ref length, path);
        if (result != 0)
        {
            throw new Win32Exception(result, "Cannot find the package's install folder.");
        }

        return new string(path, 0, checked((int)length)).TrimEnd('\0');
    }

    [ComImport, Guid("2E941141-7F97-4756-BA1D-9DECDE894A3D"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IApplicationActivationManager
    {
        [PreserveSig]
        int ActivateApplication([MarshalAs(UnmanagedType.LPWStr)] string applicationId,
            [MarshalAs(UnmanagedType.LPWStr)] string arguments,
uint options,
out uint processId);
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern uint GetApplicationUserModelId(Microsoft.Win32.SafeHandles.SafeProcessHandle process,
        ref uint length, StringBuilder? applicationId);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern int GetPackagesByPackageFamily(string packageFamilyName, ref uint count,
        [Out] IntPtr[]? packageFullNames, ref uint bufferLength, [Out] char[]? buffer);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern int GetPackagePathByFullName(string packageFullName, ref uint pathLength, [Out] char[]? path);
}
