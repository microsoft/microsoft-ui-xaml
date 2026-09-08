// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Navigation;
using TableViewSampleApp.Pages;
using Windows.UI;

namespace TableViewSampleApp;

public sealed partial class MainWindow : Window
{
    private static readonly Dictionary<string, Type> s_pageMap = new()
    {
        ["Home"] = typeof(HomePage),
        ["Showcase"] = typeof(ShowcasePage),
        ["TaskManager"] = typeof(TaskManagerPage),
        ["FileExplorer"] = typeof(FileExplorerPage),
        ["FileProperties"] = typeof(FilePropertiesPage),
        ["DynamicColumns"] = typeof(DynamicColumnsPage),
        ["Selection"] = typeof(SelectionPage),
        ["ColumnResize"] = typeof(ColumnResizePage),
        ["Layout"] = typeof(LayoutPage),
        ["Sort"] = typeof(SortPage),
        ["Filter"] = typeof(FilterPage),
        // ["Groups"] = typeof(GroupsPage),  - see the comment in MainWindow.xaml.
        ["ToolTips"] = typeof(ToolTipsPage),
        ["HeadersVisibility"] = typeof(HeadersVisibilityPage),
        ["GridLinesVisibility"] = typeof(GridLinesVisibilityPage),
        ["Virtualization"] = typeof(VirtualizationPage),
        ["Performance"] = typeof(PerformancePage),
        ["KeyboardNav"] = typeof(KeyboardNavPage),
        ["ColumnReorder"] = typeof(ColumnReorderPage),
        ["MixedControls"] = typeof(MixedControlsPage),
        ["TextWrap"] = typeof(TextWrapPage),
        ["CellFlyouts"] = typeof(CellFlyoutsPage),
        ["CellEditing"] = typeof(CellEditingPage),
        ["EmptyState"] = typeof(EmptyStatePage),
        ["DensityReadOnly"] = typeof(DensityReadOnlyPage),
        ["RTLPlayground"] = typeof(RTLPlaygroundPage),
        ["Settings"] = typeof(SettingsPage),
        ["About"] = typeof(AboutPage),
    };

    private bool _showDevInfo;

    public MainWindow(string? launchArguments = null)
    {
        InitializeComponent();
        RegisterScenarioPages();
        Title = "WinUI TableView Samples";
        ContentFrame.Navigated += ContentFrame_Navigated;
        ContentFrame.NavigationFailed += ContentFrame_NavigationFailed;

        _showDevInfo = HasLaunchFlag(launchArguments, "--show-dev-info");
        if (_showDevInfo)
        {
            DevStatusBar.Visibility = Visibility.Visible;
            PopulateStatusBar();
        }

        ConfigureTitleBar();

        // Apply any persisted theme before the first navigation so pages
        // render in the right palette from frame 0. Both the title-bar
        // toggle button and the Settings page write through
        // AppSettings.ApplyAndPersist; we listen to ThemeChanged so the
        // glyph mirrors whichever surface flipped the value last.
        var savedTheme = Services.AppSettings.LoadTheme();
        if (Content is FrameworkElement rootForInit)
        {
            rootForInit.RequestedTheme = savedTheme;
        }
        OnPersistedThemeChanged(this, savedTheme);
        Services.AppSettings.ThemeChanged += OnPersistedThemeChanged;
        Closed += (_, _) => Services.AppSettings.ThemeChanged -= OnPersistedThemeChanged;

        var initialTag = ResolveInitialTag(launchArguments);
        App.AppendVerificationLog($"MainWindowCtor InitialTag={initialTag}");
        App.AppendSelectionVerificationLog($"MainWindowCtor InitialTag={initialTag}");
        DispatcherQueue.TryEnqueue(() =>
        {
            App.AppendVerificationLog($"DispatcherNavigate InitialTag={initialTag}");
            App.AppendSelectionVerificationLog($"DispatcherNavigate InitialTag={initialTag}");
            SelectNavItem(initialTag);
            Navigate(initialTag, new EntranceNavigationTransitionInfo());
        });
    }

    /// <summary>
    /// Public entry point so HomePage cards can request navigation to a sample
    /// without having to walk to MainWindow themselves.
    /// </summary>
    public void NavigateTo(string tag)
    {
        SelectNavItem(tag);
        Navigate(tag, new DrillInNavigationTransitionInfo());
    }


    private void ContentFrame_NavigationFailed(object sender, NavigationFailedEventArgs e)
    {
        App.AppendNavigationErrorLog($"NavigationFailed Source={e.SourcePageType?.FullName ?? "(null)"}\n{e.Exception}");
    }

    private static void RegisterScenarioPages()
    {
    }

    private static string ResolveInitialTag(string? launchArguments)
    {
        if (string.IsNullOrWhiteSpace(launchArguments))
        {
            return "Home";
        }

        foreach (var token in launchArguments.Split(' ', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            const string pagePrefix = "--page=";
            if (token.StartsWith(pagePrefix, StringComparison.OrdinalIgnoreCase))
            {
                var candidate = token[pagePrefix.Length..];
                if (s_pageMap.ContainsKey(candidate))
                {
                    return candidate;
                }
            }
        }

        return "Home";
    }

    private static bool HasLaunchFlag(string? launchArguments, string flag)
    {
        if (string.IsNullOrWhiteSpace(launchArguments)) return false;
        foreach (var token in launchArguments.Split(' ', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            if (string.Equals(token, flag, StringComparison.OrdinalIgnoreCase)) return true;
        }
        return false;
    }

    // ----- Custom title bar wiring -----
    //
    // Extends content into the title bar so we get the modern WinUI shell
    // (Mica backdrop + flush nav). SetTitleBar(AppTitleBar) registers our
    // Grid as the drag region. We also subscribe to NavView.DisplayModeChanged
    // so the title bar's left padding stays clear of the hamburger / back
    // buttons at narrow widths.

    private void ConfigureTitleBar()
    {
        try
        {
            ExtendsContentIntoTitleBar = true;
            SetTitleBar(AppTitleBar);

            // Unpackaged WinUI 3 apps do NOT pick up <ApplicationIcon> or the
            // Square*Logo manifest entries on the AppWindow surface (those drive
            // packaged-MSIX Start menu / tiles only). Without an explicit
            // AppWindow.SetIcon call, the taskbar and Alt-Tab show a generic
            // blank window icon. Setting it here covers every relaunch path
            // (cold start, --show-dev-info, scenario deeplinks).
            //
            // AppIcon.ico ships next to the exe via <Content CopyToOutputDirectory=
            // PreserveNewest> in the csproj. Multi-size (16/32/48/256) so it
            // renders crisply at every Windows DPI scale and at the larger
            // sizes Alt-Tab uses on hi-DPI displays.
            try
            {
                var iconPath = Path.Combine(AppContext.BaseDirectory, "Assets", "AppIcon.ico");
                if (File.Exists(iconPath))
                {
                    AppWindow.SetIcon(iconPath);
                }
                else
                {
                    App.AppendVerificationLog($"AppWindow.SetIcon skipped (icon not found): {iconPath}");
                }
            }
            catch (Exception iconEx)
            {
                App.AppendVerificationLog($"AppWindow.SetIcon failed: {iconEx.Message}");
            }

            // Read system caption-button widths from AppWindow.TitleBar instead of
            // hardcoding 144 DIPs — RightInset varies with DPI, theme, and (for
            // RTL system locales) the buttons swap to the LEFT side, leaving
            // RightInset == 0. Subscribe to Changed so the columns track DPI
            // moves and locale-driven layout flips.
            ApplyTitleBarSystemInsets();
            AppWindow.Changed += OnAppWindowChanged;

            // Caption buttons (min / max / close) are rendered by the system,
            // not by our XAML, so their glyph color does not follow the
            // ElementTheme of the XAML root. Default ButtonForegroundColor is
            // black, which becomes invisible against a dark Mica title bar.
            // Push explicit colors keyed off the XAML root's ActualTheme so
            // the buttons stay visible in both Light and Dark.
            UpdateCaptionButtonColors();
            if (Content is FrameworkElement rootForTheme)
            {
                rootForTheme.ActualThemeChanged += (s, _) => UpdateCaptionButtonColors();
            }

            NavView.DisplayModeChanged += OnNavViewDisplayModeChanged;
            UpdateTitleBarLeftInset(NavView.DisplayMode);
        }
        catch (Exception ex)
        {
            App.AppendVerificationLog($"ConfigureTitleBar failed: {ex.Message}");
        }
    }

    private void OnAppWindowChanged(AppWindow sender, AppWindowChangedEventArgs args)
    {
        // TitleBar insets can change on DPI moves and on system theme / RTL
        // toggle. Reapply on any AppWindow change — cheap, and the only event
        // surface 3.0.0-dev exposes for this.
        ApplyTitleBarSystemInsets();
    }

    private void ApplyTitleBarSystemInsets()
    {
        try
        {
            var titleBar = AppWindow.TitleBar;
            // AppWindow.TitleBar.{Right,Left}Inset are PHYSICAL PIXELS per
            // https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.windowing.appwindowtitlebar.rightinset
            // and GridLength is in DIPs. Divide by XamlRoot.RasterizationScale
            // so the padding columns are correct at non-100% display scaling.
            // XamlRoot may be null before first Activate() — the 1.0 fallback
            // matches the prior hardcoded-144 behavior on 100% scaling, and
            // OnAppWindowChanged reapplies after Activate() wires XamlRoot.
            var scale = (Content as FrameworkElement)?.XamlRoot?.RasterizationScale ?? 1.0;
            if (scale <= 0) scale = 1.0;
            // System-rendered caption buttons drive the system insets. The
            // NavView hamburger inset still composes on top of LeftInset in
            // UpdateTitleBarLeftInset (left = max(systemLeft, navInset)).
            RightPaddingColumn.Width = new GridLength(titleBar.RightInset / scale);
            m_systemLeftInset = titleBar.LeftInset / scale;
            UpdateTitleBarLeftInset(NavView.DisplayMode);
        }
        catch (Exception ex)
        {
            App.AppendVerificationLog($"ApplyTitleBarSystemInsets failed: {ex.Message}");
        }
    }

    private double m_systemLeftInset = 0;

    private void UpdateCaptionButtonColors()
    {
        try
        {
            var titleBar = AppWindow?.TitleBar;
            if (titleBar is null)
            {
                return;
            }

            var isDark = (Content as FrameworkElement)?.ActualTheme == ElementTheme.Dark;

            var fg = isDark ? Microsoft.UI.Colors.White : Microsoft.UI.Colors.Black;
            var inactiveFg = isDark
                ? Color.FromArgb(0xFF, 0x9A, 0x9A, 0x9A)
                : Color.FromArgb(0xFF, 0x60, 0x60, 0x60);
            var hoverBg = isDark
                ? Color.FromArgb(0x33, 0xFF, 0xFF, 0xFF)
                : Color.FromArgb(0x14, 0x00, 0x00, 0x00);
            var pressedBg = isDark
                ? Color.FromArgb(0x55, 0xFF, 0xFF, 0xFF)
                : Color.FromArgb(0x33, 0x00, 0x00, 0x00);

            titleBar.ButtonBackgroundColor = Microsoft.UI.Colors.Transparent;
            titleBar.ButtonInactiveBackgroundColor = Microsoft.UI.Colors.Transparent;
            titleBar.ButtonForegroundColor = fg;
            titleBar.ButtonInactiveForegroundColor = inactiveFg;
            titleBar.ButtonHoverForegroundColor = fg;
            titleBar.ButtonHoverBackgroundColor = hoverBg;
            titleBar.ButtonPressedForegroundColor = fg;
            titleBar.ButtonPressedBackgroundColor = pressedBg;
        }
        catch (Exception ex)
        {
            App.AppendVerificationLog($"UpdateCaptionButtonColors failed: {ex.Message}");
        }
    }

    private void OnNavViewDisplayModeChanged(NavigationView sender, NavigationViewDisplayModeChangedEventArgs args)
    {
        UpdateTitleBarLeftInset(args.DisplayMode);
    }

    private void UpdateTitleBarLeftInset(NavigationViewDisplayMode mode)
    {
        // Minimal mode shows both the hamburger AND the back button at the
        // top-left, occupying ~96 DIPs, so the title must clear them there.
        // Compact / Expanded keep the hamburger in the pane (below the title
        // bar), so the title sits flush-left at the standard 16 DIP inset.
        // On RTL system locales, AppWindow.TitleBar moves the caption buttons
        // to the left side, so we must also reserve m_systemLeftInset. Use the
        // larger of the two so neither layer is clipped.
        var navInset = mode == NavigationViewDisplayMode.Minimal ? 96 : 16;
        var leftInset = Math.Max(navInset, m_systemLeftInset);
        LeftPaddingColumn.Width = new GridLength(leftInset);
    }

    private void OnThemeToggleClick(object sender, RoutedEventArgs e)
    {
        if (Content is not FrameworkElement root) return;

        // Cycle: Default → Light → Dark → Default.
        var next = root.RequestedTheme switch
        {
            ElementTheme.Default => ElementTheme.Light,
            ElementTheme.Light => ElementTheme.Dark,
            _ => ElementTheme.Default,
        };

        // Apply via the shared helper so the Settings page (if currently
        // navigated to) and the persisted file in %LocalAppData% stay in
        // sync. The helper also fires ThemeChanged, which our handler in
        // the ctor uses to keep the glyph current.
        Services.AppSettings.ApplyAndPersist(root, next);
    }

    private void OnPersistedThemeChanged(object? sender, ElementTheme theme)
    {
        // Hint the NEXT state in the glyph so the title-bar button matches
        // the same Default → Light → Dark → Default cycle. Called from
        // AppSettings.ThemeChanged so both the toggle button AND the
        // Settings page radios refresh the glyph through this single path.
        if (ThemeToggleIcon is null) return;

        ThemeToggleIcon.Glyph = theme switch
        {
            ElementTheme.Light => "\uE706",   // Brightness / Sun
            ElementTheme.Dark => "\uE708",    // QuietHours / Moon-ish
            _ => "\uE793",                    // Color (system default)
        };
    }

    private void NavView_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
    {
        if (args.InvokedItemContainer is NavigationViewItem item && item.Tag is string tag)
        {
            Navigate(tag, args.RecommendedNavigationTransitionInfo);
        }
    }

    private void NavView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
    {
        // Drive navigation from SelectionChanged so that keyboard arrow keys
        // and UIA-automation drivers (SelectionItemPattern.Select, used by the
        // sample-uia-harness sweep and accessibility tools) both navigate —
        // not just mouse clicks (which fire ItemInvoked). ItemInvoked still
        // navigates for mouse clicks; we dedupe by checking the frame's
        // current SourcePageType so we don't re-navigate to the same page.
        // See commit 2a505fb5f3 — empirically validated via realized-element
        // count delta (105 chrome-only → 551 chrome + page).
        if (args.SelectedItemContainer is NavigationViewItem item && item.Tag is string tag)
        {
            if (s_pageMap.TryGetValue(tag, out var pageType) && ContentFrame.SourcePageType == pageType)
            {
                return;
            }
            Navigate(tag, args.RecommendedNavigationTransitionInfo);
        }
    }

    private void NavView_BackRequested(NavigationView sender, NavigationViewBackRequestedEventArgs args)
    {
        if (ContentFrame.CanGoBack)
        {
            ContentFrame.GoBack();
        }
    }

    private void ContentFrame_Navigated(object sender, NavigationEventArgs e)
    {
        NavView.IsBackEnabled = ContentFrame.CanGoBack;
        SyncNavViewSelectionToCurrentPage(e.SourcePageType);
        App.AppendVerificationLog($"ContentFrameNavigated Page={e.SourcePageType?.FullName ?? "(null)"}");
        App.AppendSelectionVerificationLog($"ContentFrameNavigated Page={e.SourcePageType?.FullName ?? "(null)"}");
    }

    private void SyncNavViewSelectionToCurrentPage(Type? pageType)
    {
        if (pageType is null)
        {
            return;
        }

        foreach (var kv in s_pageMap)
        {
            if (kv.Value == pageType)
            {
                SelectNavItem(kv.Key);
                return;
            }
        }
    }

    private void Navigate(string tag, NavigationTransitionInfo? transitionInfo)
    {
        if (!s_pageMap.TryGetValue(tag, out var pageType))
        {
            App.AppendVerificationLog($"NavigateMissingTag {tag}");
            App.AppendSelectionVerificationLog($"NavigateMissingTag {tag}");
            return;
        }

        var currentType = ContentFrame.CurrentSourcePageType;
        if (currentType == pageType)
        {
            App.AppendVerificationLog($"NavigateSkippedCurrent Tag={tag} CurrentType={currentType?.FullName ?? "(null)"}");
            App.AppendSelectionVerificationLog($"NavigateSkippedCurrent Tag={tag} CurrentType={currentType?.FullName ?? "(null)"}");
            return;
        }

        App.AppendVerificationLog($"NavigateBegin Tag={tag} PageType={pageType.FullName} CurrentType={currentType?.FullName ?? "(null)"}");
        App.AppendSelectionVerificationLog($"NavigateBegin Tag={tag} PageType={pageType.FullName} CurrentType={currentType?.FullName ?? "(null)"}");

        try
        {
            var navigated = ContentFrame.Navigate(pageType, this, transitionInfo);
            App.AppendVerificationLog($"NavigateReturned {navigated} CurrentTypeAfter={ContentFrame.CurrentSourcePageType?.FullName ?? "(null)"}");
            App.AppendSelectionVerificationLog($"NavigateReturned {navigated} CurrentTypeAfter={ContentFrame.CurrentSourcePageType?.FullName ?? "(null)"}");
        }
        catch (Exception ex)
        {
            App.AppendVerificationLog($"NavigateException {ex.GetType().FullName}: {ex.Message}");
            App.AppendVerificationLog(ex.ToString());
            App.AppendSelectionVerificationLog($"NavigateException {ex.GetType().FullName}: {ex.Message}");
            App.AppendSelectionVerificationLog(ex.ToString());
            throw;
        }
    }

    private void SelectNavItem(string tag)
    {
        foreach (var item in NavView.MenuItems)
        {
            if (item is NavigationViewItem navItem && (navItem.Tag as string) == tag)
            {
                NavView.SelectedItem = navItem;
                return;
            }
        }
        foreach (var item in NavView.FooterMenuItems)
        {
            if (item is NavigationViewItem navItem && (navItem.Tag as string) == tag)
            {
                NavView.SelectedItem = navItem;
                return;
            }
        }
    }

    private void PopulateStatusBar()
    {
        try
        {
            // Locate the loaded Microsoft.UI.Xaml.Controls assembly via a type
            // we actually consume (TableView lives there), so the path
            // surfaces the DLL the running app is actually bound to.
            var tableViewType = typeof(Microsoft.UI.Xaml.Controls.Tabular.TableView);
            var asm = tableViewType.Assembly;
            var location = asm.Location;
            if (string.IsNullOrEmpty(location))
            {
                StatusBarDllPathText.Text = $"Controls assembly: {asm.FullName}";
                StatusBarDllSizeText.Text = string.Empty;
                StatusBarDllTimestampText.Text = string.Empty;
                return;
            }

            StatusBarDllPathText.Text = $"Controls DLL: {location}";

            var info = new System.IO.FileInfo(location);
            if (info.Exists)
            {
                StatusBarDllSizeText.Text = $"{info.Length:N0} bytes";
                StatusBarDllTimestampText.Text = info.LastWriteTime.ToString("yyyy-MM-dd HH:mm:ss");
            }
        }
        catch (Exception ex)
        {
            StatusBarDllPathText.Text = $"Controls DLL: <error: {ex.Message}>";
            StatusBarDllSizeText.Text = string.Empty;
            StatusBarDllTimestampText.Text = string.Empty;
        }
    }
}
