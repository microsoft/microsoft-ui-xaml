using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Reflection.PortableExecutable;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Target;

public sealed record InjectorPayloads(string Helper, string Tap, string Runtime, string FrameworkUdk);
public sealed record ResolvedInjector(Architecture Architecture, InjectorPayloads Payloads);

[SupportedOSPlatform("windows")]
public static class InjectorResolver
{
    public static Architecture ResolveMachine(ushort processMachine, ushort nativeMachine) =>
        (processMachine == 0 ? nativeMachine : processMachine) switch
        {
            0x8664 => Architecture.X64,
            0xAA64 => Architecture.Arm64,
            _ => throw new PlatformNotSupportedException("Only native ARM64 and native/emulated x64 targets are supported.")
        };

    public static Architecture GetArchitecture(ProcessInstanceKey target)
    {
        target.Validate();
        if (target.CreationTimeFileTime is null)
        {
            throw new ArgumentException("Live injection requires authoritative process creation time.", nameof(target));
        }

        using var process = Process.GetProcessById(checked((int)target.ProcessId));
        if (process.StartTime.ToFileTimeUtc() != target.CreationTimeFileTime)
        {
            throw new UnauthorizedAccessException("Target creation time differs; PID reuse is not trusted.");
        }

        if (!IsWow64Process2(process.SafeHandle, out var machine, out var native))
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "Cannot determine the target's actual architecture.");
        }

        return ResolveMachine(machine, native);
    }

    public static ResolvedInjector Resolve(ProcessInstanceKey target, InjectorPayloads payloads, bool trustLocalBinaries)
    {
        if (!trustLocalBinaries)
        {
            throw new UnauthorizedAccessException("Diagnostics injection requires explicit trust of the selected local native binaries.");
        }

        ArgumentNullException.ThrowIfNull(payloads);
        var architecture = GetArchitecture(target);
        return new(architecture,
new(
            VerifyImage(payloads.Helper, architecture, dll: false),
            VerifyImage(payloads.Tap, architecture, dll: true),
            VerifyImage(payloads.Runtime, architecture, dll: true),
            VerifyImage(payloads.FrameworkUdk, architecture, dll: true)));
    }

    public static string CreateBootstrap(string pipeName, SessionScope scope, ProcessInstanceKey client,
        ReadOnlySpan<byte> nonce)
    {
        scope.Validate();
        client.Validate();
        const string prefix = "WinUIProfiler-";
        if (pipeName is null ||
!pipeName.StartsWith(prefix, StringComparison.Ordinal) ||
            !Guid.TryParseExact(pipeName.AsSpan(prefix.Length), "N", out var pipeId) ||
pipeId == Guid.Empty)
        {
            throw new ArgumentException("A local unpredictable profiler pipe name is required.", nameof(pipeName));
        }

        if (scope.Process.CreationTimeFileTime is null ||
        client.CreationTimeFileTime is null ||
                    nonce.Length != TapProtocol.NonceBytes)
        {
            throw new ArgumentException("Bootstrap requires exact process creation times and a 32-byte nonce.");
        }

        return string.Create(CultureInfo.InvariantCulture,
                    $"WUP1|{pipeName}|{scope.SessionId:D}|{scope.Process.ProcessId}|{scope.Process.CreationTimeFileTime}|" +
                    $"{scope.Process.InstanceId:D}|{client.ProcessId}|{client.CreationTimeFileTime}|{client.InstanceId:D}|" +
                    $"{Convert.ToHexString(nonce)}");
    }

    private static string VerifyImage(string path, Architecture architecture, bool dll)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        var root = Path.GetPathRoot(path);
        if (!Path.IsPathFullyQualified(path) ||
root is not { Length: 3 } ||
            !char.IsAsciiLetter(root[0]) ||
root[1] != ':')
        {
            throw new ArgumentException("An explicit local drive-qualified native image path is required.", nameof(path));
        }

        var fullPath = Path.GetFullPath(path);
        using var stream = File.OpenRead(fullPath);
        using var reader = new PEReader(stream);
        var headers = reader.PEHeaders;
        var expected = architecture == Architecture.X64 ? Machine.Amd64 : Machine.Arm64;
        if (headers.CoffHeader.Machine != expected ||
headers.PEHeader?.Magic != PEMagic.PE32Plus ||
            headers.IsDll != dll)
        {
            throw new BadImageFormatException("Native image kind/architecture does not match the selected target.", fullPath);
        }

        return fullPath;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool IsWow64Process2(SafeProcessHandle process, out ushort processMachine, out ushort nativeMachine);
}
