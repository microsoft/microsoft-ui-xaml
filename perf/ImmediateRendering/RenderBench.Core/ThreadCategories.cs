namespace RenderBench;

// Per-thread CPU categories for the W1/W2 CPU split. D2D-SW and GDI+ move rasterization from the GPU to CPU threads,
// so process CPU alone can't say where the work went. Values are persisted in metrics.json by name.
public enum ThreadCategory
{
    Ui,
    WarpRasterizer,
    D2DDWrite,
    Composition,
    Other,
}

public static class ThreadCategories
{
    public static readonly IReadOnlyList<ThreadCategory> All = Enum.GetValues<ThreadCategory>();

    // Classifies a thread by the module containing its start address (NtQueryInformationThread
    // ThreadQuerySetWin32StartAddress). The UI thread is identified by id, since it starts in the harness executable.
    // Thread-pool workers start in ntdll and land in Other even when they run WARP or D2D work; WPA CPU sampling
    // (Timing profile) attributes those by stack.
    public static ThreadCategory Classify(string? modulePath, bool isUiThread)
    {
        if (isUiThread)
        {
            return ThreadCategory.Ui;
        }
        string module = ModuleName(modulePath);
        if (module.Length == 0)
        {
            return ThreadCategory.Other;
        }
        if (Is(module, "d3d10warp.dll"))
        {
            return ThreadCategory.WarpRasterizer;
        }
        if (Is(module, "d2d1.dll") || Is(module, "dwrite.dll") || StartsWith(module, "DWriteCore"))
        {
            return ThreadCategory.D2DDWrite;
        }
        // dwmcorei.dll hosts the in-process (lifted) compositor threads in WinAppSDK apps; observed in smoke runs.
        if (StartsWith(module, "dcomp") || StartsWith(module, "Microsoft.UI.Composition") || StartsWith(module, "CoreMessaging")
            || Is(module, "dwmcorei.dll"))
        {
            return ThreadCategory.Composition;
        }
        return ThreadCategory.Other;
    }

    public static string ModuleName(string? modulePath)
    {
        if (string.IsNullOrWhiteSpace(modulePath))
        {
            return "";
        }
        int slash = modulePath.LastIndexOfAny(['\\', '/']);
        return slash >= 0 ? modulePath[(slash + 1)..] : modulePath;
    }

    static bool Is(string module, string name) => string.Equals(module, name, StringComparison.OrdinalIgnoreCase);

    static bool StartsWith(string module, string prefix) => module.StartsWith(prefix, StringComparison.OrdinalIgnoreCase);
}
