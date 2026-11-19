namespace RenderBench;

public enum SceneId
{
    Settings,    // S1: static settings form
    Dashboard,   // S2: live dashboard, sparse updates
    Inbox,       // S3: 10,000-item ListView, continuous scrolling
    Reader,      // S4: RichTextBlock with mixed fonts, selection and slow scrolling
    Gallery,     // S5: 500-image GridView, scrolling and hover zoom
    Animated,    // S6: ProgressRing/ProgressBar/InfoBar/Expander/ToggleSwitch animations (animates while idle)
    Layered,     // S7: Mica, acrylic, ThemeShadow, nested opacity, translucent overlays
    DataGrid,    // S8: ItemsRepeater grid 50x1,000 with frozen header, cell updates and 2-D scrolling
    MultiWindow, // S9: N windows (or a 20-tab TabView) of S1; open/minimize/restore/close steps
    Form,        // S10: typing, focus movement, hover, ComboBox/Flyout/ToolTip popups
}

// How scripted interactions reach the app. Auto resolves at run time (see BenchRunner.ResolveInputMode).
public enum InputMode
{
    Auto,
    SendInput,
    Programmatic,
}

public enum ScreenshotMethod
{
    // Screen BitBlt; when that is one uniform color, PrintWindow; when that is uniform too (no DWM composition, e.g. a
    // disconnected remote session), XAML RenderTargetBitmap of the scene root.
    Auto,
    Screen,
    PrintWindow,
    // RenderTargetBitmap: the framework's own render of the tree, not what DWM shows (no backdrop, no other windows).
    Xaml,
}

// Per-scene defaults; applied for every option the command line doesn't set.
public sealed record SceneDefaults(double InteractionSeconds, double UpdateHz, double InputHz, double ScrollSpeed, int Items);

public sealed record SceneInfo(SceneId Id, string CliName, string PlanId, string DisplayName, SceneDefaults Defaults, string Summary);

public static class SceneCatalog
{
    // Order is the report/matrix order: S1, S2 first, then the extended scenes in the order they were added.
    public static IReadOnlyList<SceneInfo> All { get; } =
    [
        new(SceneId.Settings, "settings", "S1", "Settings", new(30, 10, 2, 0, 0), "static settings form; clicks toggle controls"),
        new(SceneId.Dashboard, "dashboard", "S2", "Dashboard", new(30, 10, 2, 0, 0), "200 tiles; 10% update at 10 Hz"),
        new(SceneId.Inbox, "inbox", "S3", "Inbox", new(20, 10, 10, 1500, 10_000), "virtualized mail list; continuous scroll"),
        new(SceneId.Reader, "reader", "S4", "Reader", new(20, 10, 2, 60, 25), "rich text; selection changes and slow scroll"),
        new(SceneId.Gallery, "gallery", "S5", "Gallery", new(20, 10, 2, 800, 500), "image grid; scroll and hover zoom"),
        new(SceneId.Animated, "animated", "S6", "Animated", new(20, 0.5, 1, 0, 0), "always-animating controls"),
        new(SceneId.Layered, "layered", "S7", "Layered", new(20, 2, 1, 0, 0), "mica, acrylic, shadows, group opacity"),
        new(SceneId.DataGrid, "datagrid", "S8", "DataGrid", new(30, 10, 0.5, 1200, 1_000), "50x1,000 cell grid; updates and scroll"),
        new(SceneId.MultiWindow, "multiwindow", "S9", "MultiWindow", new(30, 10, 1, 0, 0), "N windows of S1, lifecycle steps"),
        new(SceneId.Form, "form", "S10", "Form", new(30, 10, 10, 0, 0), "typing, focus, hover and popups"),
    ];

    public static SceneInfo Get(SceneId id) => All.First(s => s.Id == id);

    // Accepts the CLI name ("datagrid"), the enum name, or the test-plan id ("s8"); never a number.
    public static bool TryParse(string text, out SceneId id)
    {
        var match = All.FirstOrDefault(s =>
            string.Equals(s.CliName, text, StringComparison.OrdinalIgnoreCase) ||
            string.Equals(s.Id.ToString(), text, StringComparison.OrdinalIgnoreCase) ||
            string.Equals(s.PlanId, text, StringComparison.OrdinalIgnoreCase));
        id = match?.Id ?? default;
        return match is not null;
    }

    // Maps a RunStarted scene label ("Gallery-DecodeToSize") back to its scene.
    public static SceneId? FromLabel(string? label)
    {
        if (string.IsNullOrEmpty(label))
        {
            return null;
        }
        string head = label.Split('-')[0];
        return All.FirstOrDefault(s => string.Equals(s.DisplayName, head, StringComparison.OrdinalIgnoreCase))?.Id;
    }

    public static string CliNames => string.Join("|", All.Select(s => s.CliName));
}
