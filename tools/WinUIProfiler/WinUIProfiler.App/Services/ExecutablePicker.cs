using System.Runtime.InteropServices;

namespace WinUIProfiler.App.Services;

internal static class ExecutablePicker
{
    private const uint FileSystemPath = 0x80058000;
    private const int Cancelled = unchecked((int)0x800704C7);

    [Flags]
    private enum FileDialogOptions : uint
    {
        NoChangeDirectory = 0x8,
        ForceFileSystem = 0x40,
        PathMustExist = 0x800,
        FileMustExist = 0x1000,
        DoNotAddToRecent = 0x02000000
    }

    public static string? Pick(IntPtr owner)
    {
        if (owner == IntPtr.Zero) throw new ArgumentException("The executable picker requires its owning window.", nameof(owner));
        // The desktop dialog also works when the profiler is elevated; the
        // brokered Windows.Storage picker does not support that configuration.
        var type = Type.GetTypeFromCLSID(new("DC1C5A9C-E88A-4DDE-A5A1-60F82A20AEF7"), throwOnError: true)!;
        var dialog = (IFileDialog)(Activator.CreateInstance(type) ??
            throw new InvalidOperationException("Windows file selection is unavailable."));
        try
        {
            dialog.SetFileTypes(1, [new() { Name = ".exe", Pattern = "*.exe" }]);
            dialog.GetOptions(out var options);
            dialog.SetOptions(options | FileDialogOptions.ForceFileSystem | FileDialogOptions.PathMustExist |
                FileDialogOptions.FileMustExist | FileDialogOptions.NoChangeDirectory | FileDialogOptions.DoNotAddToRecent);
            var result = dialog.Show(owner);
            if (result == Cancelled) return null;
            Marshal.ThrowExceptionForHR(result);
            dialog.GetResult(out var item);
            try
            {
                item.GetDisplayName(FileSystemPath, out var path);
                if (!Path.GetExtension(path).Equals(".exe", StringComparison.OrdinalIgnoreCase))
                    throw new ArgumentException("Select a Windows executable (.exe).");
                return path;
            }
            finally { Marshal.FinalReleaseComObject(item); }
        }
        finally { Marshal.FinalReleaseComObject(dialog); }
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct FilterSpec
    {
        [MarshalAs(UnmanagedType.LPWStr)] public string Name;
        [MarshalAs(UnmanagedType.LPWStr)] public string Pattern;
    }

    [ComImport, Guid("42F85136-DB7E-439C-85F1-E4075D135FC8"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IFileDialog
    {
        [PreserveSig] int Show(IntPtr owner);
        void SetFileTypes(uint count, [In, MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0)] FilterSpec[] filters);
        void SetFileTypeIndex(uint index);
        void GetFileTypeIndex(out uint index);
        void Advise(IntPtr events, out uint cookie);
        void Unadvise(uint cookie);
        void SetOptions(FileDialogOptions options);
        void GetOptions(out FileDialogOptions options);
        void SetDefaultFolder(IShellItem item);
        void SetFolder(IShellItem item);
        void GetFolder(out IShellItem item);
        void GetCurrentSelection(out IShellItem item);
        void SetFileName([MarshalAs(UnmanagedType.LPWStr)] string name);
        void GetFileName([MarshalAs(UnmanagedType.LPWStr)] out string name);
        void SetTitle([MarshalAs(UnmanagedType.LPWStr)] string title);
        void SetOkButtonLabel([MarshalAs(UnmanagedType.LPWStr)] string text);
        void SetFileNameLabel([MarshalAs(UnmanagedType.LPWStr)] string text);
        void GetResult(out IShellItem item);
    }

    [ComImport, Guid("43826D1E-E718-42EE-BC55-A1E261C37BFE"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IShellItem
    {
        void BindToHandler(IntPtr context, ref Guid handler, ref Guid iid, out IntPtr result);
        void GetParent(out IShellItem parent);
        void GetDisplayName(uint form, [MarshalAs(UnmanagedType.LPWStr)] out string name);
        void GetAttributes(uint mask, out uint attributes);
        void Compare(IShellItem item, uint hint, out int order);
    }
}
