using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed class NativeMetricLibrary : SafeHandleZeroOrMinusOneIsInvalid
{
    internal NativeMetricLibrary(string path) : base(true)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Native metrics require Windows.");
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        var root = Path.GetPathRoot(path);
        if (!Path.IsPathFullyQualified(path) || root is not { Length: 3 } ||
            !char.IsAsciiLetter(root[0]) || root[1] != ':')
            throw new ArgumentException("An explicit local drive-qualified native DLL path is required.", nameof(path));
        SetHandle(NativeLibrary.Load(Path.GetFullPath(path), typeof(NativeMetricLibrary).Assembly,
            DllImportSearchPath.UseDllDirectoryForDependencies | DllImportSearchPath.System32));
    }

    internal T Export<T>(string name) where T : Delegate =>
        Marshal.GetDelegateForFunctionPointer<T>(NativeLibrary.GetExport(DangerousGetHandle(), name));

    internal T? TryExport<T>(string name) where T : Delegate =>
        NativeLibrary.TryGetExport(DangerousGetHandle(), name, out var address)
            ? Marshal.GetDelegateForFunctionPointer<T>(address) : null;

    protected override bool ReleaseHandle()
    {
        NativeLibrary.Free(handle);
        return true;
    }
}
