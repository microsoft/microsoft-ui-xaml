using System;
using System.Collections.ObjectModel;
using System.Text;
using CommunityToolkit.Mvvm.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Windows.UI.Text;
using OSFrameworkLens.Detection;

namespace OSFrameworkLens.UI;

internal sealed partial class InspectorViewModel : ObservableObject
{
    private const string Placeholder = "—";
    private const string LiveStatus   = "Live — move the mouse. Ctrl+Shift+F to freeze.";
    private const string FrozenStatus = "Frozen — Ctrl+Shift+F to resume";

    private static readonly Brush MutedBrush  = HexColor.GetBrush("#8A93A0");
    private static readonly Brush NormalBrush = HexColor.GetBrush("#E6E6E6");

    [ObservableProperty] private string _framework = Placeholder;
    [ObservableProperty] private string _confidenceText = "";
    [ObservableProperty] private Brush _frameworkBrush = HexColor.GetBrush(null);
    [ObservableProperty] private string _islandBadge = "";
    [ObservableProperty] private Visibility _islandBadgeVisibility = Visibility.Collapsed;

    [ObservableProperty] private string _windowClass = "";
    [ObservableProperty] private string _windowTitle = "";
    [ObservableProperty] private Brush _windowTitleForeground = NormalBrush;
    [ObservableProperty] private FontStyle _windowTitleFontStyle = FontStyle.Normal;

    [ObservableProperty] private string _processName = "";
    [ObservableProperty] private string _pid = "";
    [ObservableProperty] private string _tid = "";

    [ObservableProperty] private string _hwndHex = "";
    [ObservableProperty] private string _rootHwndHex = "";
    [ObservableProperty] private string _rootClassName = "";
    [ObservableProperty] private string _boundsText = "";

    [ObservableProperty] private string _uiaFrameworkId = Placeholder;
    [ObservableProperty] private string _uiaControlType = Placeholder;
    [ObservableProperty] private string _uiaName = Placeholder;
    [ObservableProperty] private string _uiaAutomationId = Placeholder;
    [ObservableProperty] private string _uiaClassName = Placeholder;

    public ObservableCollection<string> MatchedModules { get; } = new();
    [ObservableProperty] private Visibility _matchedModulesVisibility = Visibility.Collapsed;
    [ObservableProperty] private string _modulesPlaceholder = "";
    [ObservableProperty] private Visibility _modulesPlaceholderVisibility = Visibility.Collapsed;

    [ObservableProperty] private string _evidence = "";

    [ObservableProperty] private string _statusText = LiveStatus;
    [ObservableProperty] private bool _isFrozen;

    partial void OnIsFrozenChanged(bool value) => StatusText = value ? FrozenStatus : LiveStatus;

    public ElementSnapshot? LastSnapshot { get; private set; }

    private void SetWindowTitlePlaceholder(bool isPlaceholder)
    {
        WindowTitleForeground = isPlaceholder ? MutedBrush : NormalBrush;
        WindowTitleFontStyle  = isPlaceholder ? FontStyle.Italic : FontStyle.Normal;
    }

    public void Apply(ElementSnapshot? s)
    {
        LastSnapshot = s;
        if (s is null) { Reset(); return; }

        Framework = s.FrameworkDisplayName;
        ConfidenceText = $"{s.Confidence} confidence  ·  score {s.Score}";
        FrameworkBrush = HexColor.GetBrush(s.FrameworkColorHex);

        WindowClass = Dash(s.ClassName);
        var (titleText, titleIsPlaceholder) = TitleFor(s);
        WindowTitle = titleText;
        SetWindowTitlePlaceholder(titleIsPlaceholder);

        ProcessName = Dash(s.ProcessName);
        Pid = s.Pid.ToString();
        Tid = s.Tid.ToString();

        HwndHex = $"0x{s.Hwnd.ToInt64():X}";
        RootHwndHex = $"0x{s.RootHwnd.ToInt64():X}";
        RootClassName = Dash(s.RootClassName);
        BoundsText = $"L {s.Bounds.Left}, T {s.Bounds.Top}, R {s.Bounds.Right}, B {s.Bounds.Bottom}   ({s.Bounds.Right - s.Bounds.Left} × {s.Bounds.Bottom - s.Bounds.Top})";

        IslandBadgeVisibility = s.IsIsland ? Visibility.Visible : Visibility.Collapsed;
        IslandBadge = IslandBadgeFor(s);

        UiaFrameworkId  = Dash(s.UiaFrameworkId);
        UiaControlType  = Dash(s.UiaControlType);
        UiaName         = Dash(s.UiaName);
        UiaAutomationId = Dash(s.UiaAutomationId);
        UiaClassName    = Dash(s.UiaClassName);

        MatchedModules.Clear();
        var (placeholder, listVisible) = ModulesPlaceholderFor(s);
        ModulesPlaceholder = placeholder;
        MatchedModulesVisibility    = listVisible ? Visibility.Visible : Visibility.Collapsed;
        ModulesPlaceholderVisibility = listVisible ? Visibility.Collapsed : Visibility.Visible;
        if (listVisible) foreach (var m in s.MatchedModules) MatchedModules.Add(m);

        Evidence = string.Join(Environment.NewLine, s.EvidenceLog);
    }

    private void Reset()
    {
        Framework = Placeholder;
        ConfidenceText = "";
        FrameworkBrush = HexColor.GetBrush(null);
        WindowClass = WindowTitle = ProcessName = Pid = Tid = "";
        HwndHex = RootHwndHex = RootClassName = BoundsText = "";
        UiaFrameworkId = UiaControlType = UiaName = UiaAutomationId = UiaClassName = Placeholder;
        Evidence = "";
        MatchedModules.Clear();
        MatchedModulesVisibility = ModulesPlaceholderVisibility = IslandBadgeVisibility = Visibility.Collapsed;
        ModulesPlaceholder = IslandBadge = "";
        SetWindowTitlePlaceholder(false);
    }

    private static (string text, bool isPlaceholder) TitleFor(ElementSnapshot s) => s switch
    {
        { TitleAvailable: false } => ($"title unavailable (target UI thread did not respond within {HwndInspector.TitleTimeoutMs} ms)", true),
        { Title: "" or null }     => ("no title set", true),
        _                         => (s.Title, false),
    };

    private static string IslandBadgeFor(ElementSnapshot s) => s switch
    {
        { IsIsland: false } => "",
        { IslandHostFingerprintMatched: true } =>
            $"🏝️  Embedded — leaf is {s.FrameworkDisplayName}, host is {s.IslandHostFrameworkDisplayName}  ({s.RootClassName})",
        _ => $"🏝️  Likely embedded — leaf is {s.FrameworkDisplayName}; root class '{s.RootClassName}' not in our fingerprint table."
             + (string.IsNullOrEmpty(s.IslandHostGuessText) ? "" : "\n   " + s.IslandHostGuessText),
    };

    private static (string placeholder, bool listVisible) ModulesPlaceholderFor(ElementSnapshot s) =>
        !s.ModuleListAvailable     ? (ModuleListUnavailableMsg, false)
      : s.MatchedModules.Count == 0 ? ($"None of the {s.AllModules.Count} loaded modules matched any framework fingerprint.", false)
                                    : ("", true);

    internal const string ModuleListUnavailableMsg =
        "Module list unavailable — process is likely protected/elevated, or a 32-bit target this 64-bit inspector can't enumerate.";

    private static string Dash(string s) => string.IsNullOrEmpty(s) ? Placeholder : s;

    public string BuildClipboardReport()
    {
        if (LastSnapshot is null) return "";
        var s = LastSnapshot;
        var sb = new StringBuilder();
        sb.AppendLine($"# OS Framework Lens — {s.FrameworkDisplayName} ({s.Confidence} confidence, score {s.Score})");
        sb.AppendLine();
        sb.AppendLine($"- Process    : {s.ProcessName} (PID {s.Pid}, TID {s.Tid})");
        sb.AppendLine($"- HWND       : 0x{s.Hwnd.ToInt64():X}   Root: 0x{s.RootHwnd.ToInt64():X} ({s.RootClassName})");
        sb.AppendLine($"- Class      : {s.ClassName}");
        sb.AppendLine($"- Title      : {(s.TitleAvailable ? (string.IsNullOrEmpty(s.Title) ? "(empty)" : s.Title) : $"(unavailable — target UI thread did not respond in {HwndInspector.TitleTimeoutMs} ms)")}");
        sb.AppendLine($"- Bounds     : L{s.Bounds.Left} T{s.Bounds.Top} R{s.Bounds.Right} B{s.Bounds.Bottom}");
        if (s.IsIsland)
        {
            sb.AppendLine(s.IslandHostFingerprintMatched
                ? $"- Embedded   : leaf {s.FrameworkDisplayName} hosted in {s.IslandHostFrameworkDisplayName} (root class {s.RootClassName})"
                : $"- Embedded   : leaf {s.FrameworkDisplayName}; root class {s.RootClassName} not in fingerprint table");
            if (!s.IslandHostFingerprintMatched && !string.IsNullOrEmpty(s.IslandHostGuessText))
                sb.AppendLine($"              {s.IslandHostGuessText}");
        }
        sb.AppendLine();
        sb.AppendLine("## UI Automation");
        sb.AppendLine($"- FrameworkId : {Dash(s.UiaFrameworkId)}");
        sb.AppendLine($"- ControlType : {Dash(s.UiaControlType)}");
        sb.AppendLine($"- Name        : {Dash(s.UiaName)}");
        sb.AppendLine($"- AutomationId: {Dash(s.UiaAutomationId)}");
        sb.AppendLine($"- ClassName   : {Dash(s.UiaClassName)}");
        sb.AppendLine();
        sb.AppendLine("## Matched framework-runtime modules");
        if (!s.ModuleListAvailable)
            sb.AppendLine("_module list unavailable (protected/elevated process, or 32-bit target)_");
        else if (s.MatchedModules.Count == 0)
            sb.AppendLine($"_none of {s.AllModules.Count} loaded modules matched any framework fingerprint_");
        else
            sb.AppendLine("- " + string.Join(Environment.NewLine + "- ", s.MatchedModules));
        sb.AppendLine();
        sb.AppendLine("## Evidence");
        foreach (var e in s.EvidenceLog) sb.AppendLine($"- {e}");
        return sb.ToString();
    }
}
