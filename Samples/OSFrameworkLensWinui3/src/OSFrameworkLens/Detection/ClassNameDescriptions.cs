using System;
using System.Collections.Generic;

namespace OSFrameworkLens.Detection;

/// <summary>
/// Maps known Windows HWND class names to a short natural-language description
/// of what kind of UI surface that class typically represents. Used by the
/// per-window analyzer to translate raw class names (e.g. "SysTreeView32") into
/// developer- and leadership-friendly descriptions (e.g. "tree view, e.g. the
/// left navigation pane"). Unknown classes are simply not described — we don't
/// invent meaning.
///
/// Sources: documented Windows shell internals, Microsoft Learn (UWP / WinUI 3),
/// Chromium / WebView2 conventions. Where a description applies only to a
/// specific app (e.g. CabinetWClass is Explorer-specific), the description says
/// so to avoid misleading users on lookalike windows.
/// </summary>
internal static class ClassNameDescriptions
{
    private static readonly Dictionary<string, string> s_exact = new(StringComparer.OrdinalIgnoreCase)
    {
        // ── Explorer / shell ──────────────────────────────────────────
        { "CabinetWClass", "Explorer top-level window frame" },
        { "ExploreWClass", "Explorer top-level window frame (legacy)" },
        { "ShellTabWindowClass", "Explorer tab container host" },
        { "TITLE_BAR_SCAFFOLDING_WINDOW_CLASS", "Win11 title-bar scaffolding (hosts the modern tab strip)" },
        { "TopLevelWindowForOverflowXamlIsland", "shell XAML island overflow host" },
        { "NamespaceTreeControl", "shell namespace tree control" },
        { "DUIViewWndClassName", "DirectUI shell view container" },
        { "DetailsPaneHwndHostClass", "Details pane host (right side of Explorer)" },
        { "SHELLDLL_DefView", "shell default view (folder content host)" },
        { "DirectUIHWND", "DirectUI content host (items list)" },
        { "CtrlNotifySink", "DirectUI control notify sink" },

        // ── Modern XAML hosts (WinUI 2 / 3) ──────────────────────────
        { "Microsoft.UI.Content.DesktopChildSiteBridge", "WinUI 3 XAML island (tab strip, command bar, etc.)" },
        { "Microsoft.UI.Content.PopupWindowSiteBridge", "WinUI 3 popup (context menu, flyout)" },
        { "WinUIDesktopWin32WindowClass", "WinUI 3 desktop top-level window" },
        { "Windows.UI.Core.CoreWindow", "UWP / XAML core window" },
        { "ApplicationFrameWindow", "UWP application frame host" },

        // ── Taskbar / Start / desktop ────────────────────────────────
        { "Shell_TrayWnd", "the taskbar" },
        { "Shell_SecondaryTrayWnd", "secondary-monitor taskbar" },
        { "Progman", "desktop manager (program manager)" },
        { "WorkerW", "desktop worker (icon layer / wallpaper)" },
        { "TaskListThumbnailWnd", "taskbar thumbnail preview" },

        // ── Common Controls ──────────────────────────────────────────
        { "SysTreeView32", "tree view (e.g., Explorer left nav pane)" },
        { "SysListView32", "list view" },
        { "SysHeader32", "list-view column header" },
        { "SysTabControl32", "classic tab control" },
        { "SysLink", "hyperlink control" },
        { "ToolbarWindow32", "classic toolbar" },
        { "ReBarWindow32", "rebar (toolbar container)" },
        { "msctls_statusbar32", "status bar (bottom of window)" },
        { "msctls_progress32", "progress bar" },
        { "msctls_trackbar32", "slider / trackbar" },
        { "msctls_updown32", "up-down (spinner) control" },
        { "tooltips_class32", "tooltip" },

        // ── Classic Win32 / dialogs ──────────────────────────────────
        { "Button", "classic Win32 button" },
        { "Edit", "classic Win32 edit field" },
        { "Static", "classic Win32 label" },
        { "ListBox", "classic Win32 list box" },
        { "ComboBox", "classic Win32 combo box" },
        { "ComboLBox", "classic Win32 combo dropdown" },
        { "ScrollBar", "scroll bar" },
        { "#32770", "classic Win32 dialog" },
        { "#32768", "classic Win32 menu" },
        { "#32769", "desktop window" },
        { "#32771", "task-switcher window" },
        { "Notepad", "classic Notepad (Win10-style)" },
        { "ConsoleWindowClass", "classic console window" },

        // ── Chromium / WebView2 ──────────────────────────────────────
        { "Chrome_WidgetWin_0", "Chromium widget host" },
        { "Chrome_WidgetWin_1", "Chromium top-level window" },
        { "Chrome_WidgetWin_2", "Chromium child widget" },
        { "Chrome_RenderWidgetHostHWND", "Chromium renderer surface" },
        { "Intermediate D3D Window", "Chromium D3D presentation surface" },
    };

    private static readonly (string Prefix, string Description)[] s_prefixes = new[]
    {
        ("HwndWrapper[",   "WPF hosted control"),
        ("WindowsForms10", "WinForms control"),
        ("Afx:",           "MFC window"),
        ("NetUI",          "Office NetUI surface"),
        ("MsoCommandBar",  "Office command bar"),
    };

    /// <summary>
    /// Returns a short natural-language description for a given HWND class name,
    /// or null if the class isn't known. Never makes up a description.
    /// </summary>
    public static string? TryDescribe(string className)
    {
        if (string.IsNullOrEmpty(className)) return null;
        if (s_exact.TryGetValue(className, out var d)) return d;
        foreach (var (prefix, desc) in s_prefixes)
            if (className.StartsWith(prefix, StringComparison.Ordinal))
                return desc;
        return null;
    }
}
