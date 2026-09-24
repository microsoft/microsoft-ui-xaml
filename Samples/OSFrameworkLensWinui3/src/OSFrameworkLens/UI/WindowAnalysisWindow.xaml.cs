using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.UI;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.ApplicationModel.DataTransfer;
using Windows.Graphics;
using WinRT.Interop;
using OSFrameworkLens.Detection;

namespace OSFrameworkLens.UI;

public partial class WindowAnalysisWindow : Window
{
    private const double BarMaxWidth = 360.0;
    private readonly WindowFrameworkAnalyzer.WindowAnalysis _analysis;

    internal WindowAnalysisWindow(WindowFrameworkAnalyzer.WindowAnalysis analysis)
    {
        InitializeComponent();
        Title = "Window Framework Breakdown";
        ConfigureAppWindow();
        _analysis = analysis;
        Render();
    }

    private void ConfigureAppWindow()
    {
        var hwnd = WindowNative.GetWindowHandle(this);
        var windowId = Win32Interop.GetWindowIdFromWindow(hwnd);
        var appWindow = AppWindow.GetFromWindowId(windowId);
        if (appWindow is null) return;

        var area = DisplayArea.GetFromWindowId(windowId, DisplayAreaFallback.Primary);
        const int width = 640, height = 640;
        appWindow.MoveAndResize(new RectInt32(
            area.WorkArea.X + (area.WorkArea.Width - width) / 2,
            area.WorkArea.Y + (area.WorkArea.Height - height) / 2,
            width, height));

        if (appWindow.Presenter is OverlappedPresenter op)
            op.IsAlwaysOnTop = true;
    }

    private void Render()
    {
        TopRootClass.Text = string.IsNullOrEmpty(_analysis.RootClassName) ? "—" : _analysis.RootClassName;
        TopProcess.Text = $"{_analysis.ProcessName}    PID {_analysis.Pid}    HWND 0x{_analysis.RootHwnd.ToInt64():X}";
        TopScanStats.Text = $"{_analysis.VisibleHwndsConsidered} visible HWNDs considered of {_analysis.TotalHwndsScanned} scanned"
                           + (_analysis.TruncatedAtLimit ? "  (truncated at scan limit)" : "");

        if (_analysis.Frameworks.Count == 0)
        {
            ShowPlaceholder(FrameworkList, NoFrameworksText, NoFrameworksText.Text);
        }
        else
        {
            var maxPercent = _analysis.Frameworks.Max(f => f.AreaPercent);
            if (maxPercent <= 0) maxPercent = 1;

            FrameworkList.ItemsSource = _analysis.Frameworks.Select(f => new FrameworkSliceVm
            {
                DisplayName = f.DisplayName,
                RightStats = $"{f.AreaPercent,5:0.0}% area  ·  {f.HwndCount} HWND{(f.HwndCount == 1 ? "" : "s")}",
                Brush = HexColor.GetBrush(f.ColorHex),
                BarWidth = BarMaxWidth * (f.AreaPercent / maxPercent),
                SampleClasses = f.SampleClasses,
                WhereDescription = f.WhereDescription
            }).ToList();
            ShowList(FrameworkList, NoFrameworksText);
        }

        if (!_analysis.ModuleListAvailable)
            ShowPlaceholder(LoadedButNotSeenList, LoadedButNotSeenEmpty,
                "Process module list unavailable — cannot compute the 'loaded but not seen' tail (process is likely protected/elevated, or a 32-bit target this 64-bit inspector can't enumerate).");
        else if (_analysis.LoadedButNotSeenFrameworks.Count == 0)
            ShowPlaceholder(LoadedButNotSeenList, LoadedButNotSeenEmpty,
                "None — every framework whose runtime is loaded in this process produced at least one HWND in this window.");
        else
        {
            LoadedButNotSeenList.ItemsSource = _analysis.LoadedButNotSeenFrameworks;
            ShowList(LoadedButNotSeenList, LoadedButNotSeenEmpty);
        }
    }

    private static void ShowList(UIElement list, TextBlock placeholder)
    {
        list.Visibility = Visibility.Visible;
        placeholder.Visibility = Visibility.Collapsed;
    }

    private static void ShowPlaceholder(UIElement list, TextBlock placeholder, string text)
    {
        list.Visibility = Visibility.Collapsed;
        placeholder.Text = text;
        placeholder.Visibility = Visibility.Visible;
    }

    private void OnCopyClicked(object sender, RoutedEventArgs e)
    {
        try
        {
            var pkg = new DataPackage();
            pkg.SetText(BuildReport());
            Clipboard.SetContent(pkg);
        }
        catch (COMException) { /* clipboard occasionally throws when another app holds it */ }
    }

    private string BuildReport()
    {
        var sb = new StringBuilder();
        sb.AppendLine($"# Window Framework Breakdown — {_analysis.RootClassName}");
        sb.AppendLine();
        sb.AppendLine($"- Process : {_analysis.ProcessName} (PID {_analysis.Pid})");
        sb.AppendLine($"- Root    : 0x{_analysis.RootHwnd.ToInt64():X} ({_analysis.RootClassName})");
        sb.AppendLine($"- Scanned : {_analysis.VisibleHwndsConsidered} visible / {_analysis.TotalHwndsScanned} total HWNDs"
                      + (_analysis.TruncatedAtLimit ? " (truncated at scan limit)" : ""));
        sb.AppendLine();
        sb.AppendLine("## Frameworks present");
        if (_analysis.Frameworks.Count == 0)
        {
            sb.AppendLine("_no visible HWNDs found_");
        }
        else
        {
            foreach (var f in _analysis.Frameworks)
            {
                sb.AppendLine($"- {f.DisplayName,-32}  {f.AreaPercent,5:0.0}% area, {f.HwndCount} HWND{(f.HwndCount == 1 ? "" : "s")}   [{string.Join(", ", f.SampleClasses)}]");
                if (!string.IsNullOrEmpty(f.WhereDescription))
                    sb.AppendLine($"    {f.WhereDescription}");
            }
        }
        sb.AppendLine();
        sb.AppendLine("## Loaded but not seen in this window");
        if (!_analysis.ModuleListAvailable)
            sb.AppendLine("_module list unavailable (protected/elevated process, or 32-bit target)_");
        else if (_analysis.LoadedButNotSeenFrameworks.Count == 0)
            sb.AppendLine("_none_");
        else
            foreach (var s in _analysis.LoadedButNotSeenFrameworks) sb.AppendLine($"- {s}");
        sb.AppendLine();
        sb.AppendLine("_Caveats: counts are per HWND (XAML islands count as 1). Area % is share of root window pixels, capped at 100% per slice; overlap can push sums above 100%. Invisible HWNDs are excluded._");
        return sb.ToString();
    }

    private void OnCloseClicked(object sender, RoutedEventArgs e) => Close();
}