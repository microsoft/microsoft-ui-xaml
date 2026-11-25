namespace RenderBench;

// Numeric values match RenderingBackend::Mode in dxaml\xcp\components\base\inc\RenderingBackend.h.
public enum BackendMode
{
    Composition = 0,
    ImmediateGdiPlus = 1,
    ImmediateD2DHardware = 2,
    ImmediateD2DSoftware = 3,
}

// Numeric values match RenderingBackend::FallbackReason.
public enum FallbackReason
{
    None = 0,
    NotImplemented = 1,
    InvalidRequest = 2,
    DeviceUnavailable = 3,
    UnsupportedContent = 4,
}

public static class BackendModes
{
    public const string EnvironmentVariable = "WINUI_RENDERING_BACKEND";
    public const string RenderingBackendProvider = "Microsoft-WinUI-RenderingBackend";
    public static readonly Guid RenderingBackendProviderId = new("d18ce99c-dd66-5390-18da-068546739801");

    static readonly (BackendMode Mode, string Name, string Label)[] s_modes =
    [
        (BackendMode.Composition, "composition", "COMP"),
        (BackendMode.ImmediateGdiPlus, "gdiplus", "GDIP"),
        (BackendMode.ImmediateD2DHardware, "d2d-hw", "D2D-HW"),
        (BackendMode.ImmediateD2DSoftware, "d2d-sw", "D2D-SW"),
    ];

    public static IReadOnlyList<BackendMode> All { get; } = s_modes.Select(m => m.Mode).ToArray();

    public static bool TryParse(string? text, out BackendMode mode)
    {
        foreach (var entry in s_modes)
        {
            if (string.Equals(text, entry.Name, StringComparison.OrdinalIgnoreCase) ||
                string.Equals(text, entry.Label, StringComparison.OrdinalIgnoreCase))
            {
                mode = entry.Mode;
                return true;
            }
        }
        mode = BackendMode.Composition;
        return false;
    }

    public static string Name(BackendMode mode) => s_modes.First(m => m.Mode == mode).Name;

    public static string Label(BackendMode mode) => s_modes.First(m => m.Mode == mode).Label;
}
