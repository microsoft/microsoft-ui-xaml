// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Input;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Navigation;
using TableViewSampleApp.Pages;
using Windows.UI;
using Windows.UI.ViewManagement;

namespace TableViewSampleApp;

public sealed partial class MainWindow : Window
{
    // Tags are matched case-insensitively (so --page=keyboardnav works); the key as written
    // here is the canonical Tag, which is also the NavigationViewItem.Tag.
    private static readonly Dictionary<string, Type> s_pageMap = new(StringComparer.OrdinalIgnoreCase)
    {
        ["Home"] = typeof(HomePage),
        ["Showcase"] = typeof(ShowcasePage),
        ["TaskManager"] = typeof(TaskManagerPage),
        ["FileExplorer"] = typeof(FileExplorerPage),
        ["FileProperties"] = typeof(FilePropertiesPage),
        ["ColumnLifecycle"] = typeof(ColumnLifecyclePage),
        ["Selection"] = typeof(SelectionPage),
        ["ColumnLayout"] = typeof(ColumnLayoutPage),
        ["Sort"] = typeof(SortPage),
        ["Filter"] = typeof(FilterPage),
        ["Groups"] = typeof(GroupsPage),
        ["ToolTips"] = typeof(ToolTipsPage),
        ["HeadersVisibility"] = typeof(HeadersVisibilityPage),
        ["GridLinesVisibility"] = typeof(GridLinesVisibilityPage),
        ["Virtualization"] = typeof(VirtualizationPage),
        ["Performance"] = typeof(PerformancePage),
        ["KeyboardNav"] = typeof(KeyboardNavPage),
        ["Hierarchy"] = typeof(HierarchyPage),
        ["TextWrap"] = typeof(TextWrapPage),
        ["CellTemplating"] = typeof(CellTemplatingPage),
        ["CellEditing"] = typeof(CellEditingPage),
        ["EmptyState"] = typeof(EmptyStatePage),
        ["Density"] = typeof(DensityPage),
        ["RightToLeft"] = typeof(RightToLeftPage),
        ["Settings"] = typeof(SettingsPage),
        ["About"] = typeof(AboutPage),
    };

    // Retired Tags that existing --page= deep links still use, mapped to the current Tag.
    private static readonly Dictionary<string, string> s_tagAliases = new(StringComparer.OrdinalIgnoreCase)
    {
        ["CellFlyouts"] = "CellTemplating",
        ["DynamicColumns"] = "ColumnLifecycle",
        ["Layout"] = "ColumnLayout",
        ["DensityReadOnly"] = "Density",
        ["RTLPlayground"] = "RightToLeft",
    };

    private bool _showDevInfo;
    private bool _isUpdatingSelection;
    private bool _shouldFocusContentAfterNavigation;
    private FocusState _contentNavigationFocusState = FocusState.Programmatic;
    private readonly AccessibilitySettings _accessibilitySettings = new();
    private readonly UISettings _uiSettings = new();

    public MainWindow(IReadOnlyList<string>? launchArguments = null)
    {
        InitializeComponent();
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

        // The caption buttons are drawn by the system; follow a Contrast theme switched on or off
        // while the app runs, not only the state at startup. AccessibilitySettings.
        // HighContrastChanged throws ELEMENT_NOT_FOUND in a desktop (non-CoreWindow) app, so listen
        // to UISettings.ColorValuesChanged, which a Contrast switch also raises, and re-read
        // AccessibilitySettings.HighContrast in UpdateCaptionButtonColors.
        _uiSettings.ColorValuesChanged += OnColorValuesChanged;
        Closed += (_, _) => _uiSettings.ColorValuesChanged -= OnColorValuesChanged;

        var initialTag = ResolveInitialTag(launchArguments);
        App.AppendVerificationLog($"MainWindowCtor InitialTag={initialTag}");
        App.AppendSelectionVerificationLog($"MainWindowCtor InitialTag={initialTag}");
        DispatcherQueue.TryEnqueue(() =>
        {
            App.AppendVerificationLog($"DispatcherNavigate InitialTag={initialTag}");
            App.AppendSelectionVerificationLog($"DispatcherNavigate InitialTag={initialTag}");
            SelectNavItem(initialTag);
            // Focus the page's first control, not the title bar's Theme button, which is the
            // first tab stop in the window and would otherwise take initial focus.
            Navigate(initialTag, new EntranceNavigationTransitionInfo(), true);
        });
    }

    /// <summary>
    /// Public entry point so HomePage cards can request navigation to a sample
    /// without having to walk to MainWindow themselves.
    /// </summary>
    public void NavigateTo(string tag)
    {
        if (TryGetCanonicalTag(tag, out var canonical))
        {
            tag = canonical;
        }

        SelectNavItem(tag);
        Navigate(tag, new DrillInNavigationTransitionInfo(), true);
    }

    /// <summary>Resolves a tag in any casing, or a retired alias, to the canonical s_pageMap key.</summary>
    private static bool TryGetCanonicalTag(string? tag, out string canonical)
    {
        canonical = string.Empty;
        if (!string.IsNullOrEmpty(tag) && s_tagAliases.TryGetValue(tag, out var aliasTarget))
        {
            tag = aliasTarget;
        }

        if (string.IsNullOrEmpty(tag) || !s_pageMap.ContainsKey(tag))
        {
            return false;
        }

        foreach (var key in s_pageMap.Keys)
        {
            if (string.Equals(key, tag, StringComparison.OrdinalIgnoreCase))
            {
                canonical = key;
                return true;
            }
        }

        return false;
    }

    // Raised on a background thread. A Contrast switch also recolours the shared chip palette in
    // place, so realized cells follow it without re-conversion.
    private void OnColorValuesChanged(UISettings sender, object args) =>
        DispatcherQueue.TryEnqueue(() =>
        {
            UpdateCaptionButtonColors();
            Converters.ChipBrushes.Refresh();
        });

    private void ContentFrame_NavigationFailed(object sender, NavigationFailedEventArgs e)
    {
        App.AppendNavigationErrorLog($"NavigationFailed Source={e.SourcePageType?.FullName ?? "(null)"}\n{e.Exception}");
    }

    private static string ResolveInitialTag(IReadOnlyList<string>? launchArguments)
    {
        if (launchArguments is null || launchArguments.Count == 0)
        {
            return "Home";
        }

        foreach (var token in launchArguments)
        {
            const string pagePrefix = "--page=";
            if (token.StartsWith(pagePrefix, StringComparison.OrdinalIgnoreCase))
            {
                var candidate = token[pagePrefix.Length..];
                if (TryGetCanonicalTag(candidate, out var canonical))
                {
                    return canonical;
                }

                // An unknown tag opens Home, but leaves a trace instead of failing silently.
                App.AppendVerificationLog($"NavigateMissingTag {candidate}");
                App.AppendSelectionVerificationLog($"NavigateMissingTag {candidate}");
                return "Home";
            }
        }

        return "Home";
    }

    private static bool HasLaunchFlag(IReadOnlyList<string>? launchArguments, string flag)
    {
        if (launchArguments is null || launchArguments.Count == 0) return false;
        foreach (var token in launchArguments)
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

            // SetTitleBar makes the whole AppTitleBar rect a caption (drag) region, which also
            // swallows pointer input on the theme button inside it; carve the button out as a
            // passthrough region. Each event is hooked once: the button moves when the title bar
            // resizes or the caption-button inset changes (OnAppWindowChanged), and DPI changes
            // raise XamlRoot.Changed. (LayoutUpdated would run this on every layout pass in the
            // window, e.g. every frame of TableView scrolling.)
            ThemeToggleButton.SizeChanged += (_, _) => UpdateTitleBarPassthrough();
            AppTitleBar.SizeChanged += (_, _) => UpdateTitleBarPassthrough();
            ThemeToggleButton.Loaded += OnThemeToggleButtonLoaded;

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

    private Windows.Graphics.RectInt32[] m_titleBarPassthrough = Array.Empty<Windows.Graphics.RectInt32>();
    private bool m_xamlRootChangedHooked;

    private void OnThemeToggleButtonLoaded(object sender, RoutedEventArgs e)
    {
        if (!m_xamlRootChangedHooked && ThemeToggleButton.XamlRoot is { } xamlRoot)
        {
            m_xamlRootChangedHooked = true;
            xamlRoot.Changed += (_, _) => UpdateTitleBarPassthrough();
        }

        UpdateTitleBarPassthrough();
    }

    // SetRegionRects REPLACES every passthrough rect of the window, so the complete list of
    // interactive title-bar elements is built here, in one place.
    private void UpdateTitleBarPassthrough()
    {
        try
        {
            if (ThemeToggleButton.XamlRoot is not { } xamlRoot || ThemeToggleButton.ActualWidth <= 0)
            {
                return;
            }

            var rects = new[] { ClientRectOf(ThemeToggleButton, xamlRoot) };
            if (System.Linq.Enumerable.SequenceEqual(rects, m_titleBarPassthrough))
            {
                return;
            }

            m_titleBarPassthrough = rects;
            InputNonClientPointerSource.GetForWindowId(AppWindow.Id)
                .SetRegionRects(NonClientRegionKind.Passthrough, rects);
        }
        catch (Exception ex)
        {
            App.AppendVerificationLog($"Title bar passthrough update failed: {ex.Message}");
        }
    }

    // XAML works in DIPs relative to the window content; the non-client API takes physical client
    // pixels. TransformToVisual(null) already includes any FlowDirection=RightToLeft mirroring of
    // the content, so the rect is in unmirrored client coordinates and needs no RTL flip (the WinUI
    // TitleBar control computes its passthrough rects the same way). The caption buttons swapping
    // sides for an RTL system locale is handled by the insets (ApplyTitleBarSystemInsets).
    private static Windows.Graphics.RectInt32 ClientRectOf(FrameworkElement element, XamlRoot xamlRoot)
    {
        var scale = xamlRoot.RasterizationScale;
        var bounds = element.TransformToVisual(null).TransformBounds(
            new Windows.Foundation.Rect(0, 0, element.ActualWidth, element.ActualHeight));
        return new Windows.Graphics.RectInt32(
            (int)Math.Round(bounds.X * scale),
            (int)Math.Round(bounds.Y * scale),
            (int)Math.Round(bounds.Width * scale),
            (int)Math.Round(bounds.Height * scale));
    }

    private void OnAppWindowChanged(AppWindow sender, AppWindowChangedEventArgs args)
    {
        // TitleBar insets can change on DPI moves and on system theme / RTL
        // toggle. Reapply on any AppWindow change — cheap, and the only event
        // surface 3.0.0-dev exposes for this.
        var rightInset = RightPaddingColumn.Width;
        ApplyTitleBarSystemInsets();

        // A new caption-button inset moves the theme button without resizing it or the title
        // bar; recompute its passthrough rect once, after the next layout pass.
        if (!rightInset.Equals(RightPaddingColumn.Width))
        {
            void OnLayoutUpdated(object? s, object e)
            {
                ThemeToggleButton.LayoutUpdated -= OnLayoutUpdated;
                UpdateTitleBarPassthrough();
            }

            ThemeToggleButton.LayoutUpdated += OnLayoutUpdated;
        }
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

            if (_accessibilitySettings.HighContrast)
            {
                // Undo only our sample-owned Light/Dark overrides. Let Windows
                // high-contrast resources provide the caption-button colors.
                titleBar.ButtonBackgroundColor = null;
                titleBar.ButtonInactiveBackgroundColor = null;
                titleBar.ButtonForegroundColor = null;
                titleBar.ButtonInactiveForegroundColor = null;
                titleBar.ButtonHoverForegroundColor = null;
                titleBar.ButtonHoverBackgroundColor = null;
                titleBar.ButtonPressedForegroundColor = null;
                titleBar.ButtonPressedBackgroundColor = null;
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
        RaiseThemeChangedNotification(next);
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

        var current = GetThemeName(theme);
        var next = GetThemeName(GetNextTheme(theme));

        AutomationProperties.SetName(ThemeToggleButton, $"Theme: {current}. Switch to {next}.");
        AutomationProperties.SetHelpText(ThemeToggleButton, $"The current theme is {current}. Activating this button switches to {next}.");
        ToolTipService.SetToolTip(ThemeToggleButton, $"Theme: {current}. Switch to {next}.");
    }

    private void NavView_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
    {
        if (args.InvokedItemContainer is NavigationViewItem item && item.Tag is string tag)
        {
            Navigate(tag, args.RecommendedNavigationTransitionInfo, true);
        }
    }

    private void NavView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
    {
        // Drive navigation from SelectionChanged so that keyboard arrow keys
        // and UIA-automation drivers (SelectionItemPattern.Select, used by the
        // sample-uia-harness sweep and accessibility tools) both navigate —
        // not just mouse clicks (which fire ItemInvoked). ItemInvoked still
        // navigates for mouse clicks; we dedupe by checking the frame's
        // CurrentSourcePageType so we don't re-navigate to the same page.
        // See commit 2a505fb5f3 — empirically validated via realized-element
        // count delta (105 chrome-only → 551 chrome + page).
        if (_isUpdatingSelection)
        {
            return;
        }

        if (args.SelectedItemContainer is NavigationViewItem item && item.Tag is string tag)
        {
            if (s_pageMap.TryGetValue(tag, out var pageType) && ContentFrame.CurrentSourcePageType == pageType)
            {
                return;
            }
            Navigate(tag, args.RecommendedNavigationTransitionInfo, true);
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
        FocusContentAfterNavigationIfRequested();
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

    private void Navigate(string tag, NavigationTransitionInfo? transitionInfo, bool focusContentAfterNavigation)
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
            if (focusContentAfterNavigation)
            {
                _shouldFocusContentAfterNavigation = true;
                _contentNavigationFocusState = GetNavigationFocusState();
            }

            var navigated = ContentFrame.Navigate(pageType, this, transitionInfo);
            if (!navigated && focusContentAfterNavigation)
            {
                _shouldFocusContentAfterNavigation = false;
            }
            App.AppendVerificationLog($"NavigateReturned {navigated} CurrentTypeAfter={ContentFrame.CurrentSourcePageType?.FullName ?? "(null)"}");
            App.AppendSelectionVerificationLog($"NavigateReturned {navigated} CurrentTypeAfter={ContentFrame.CurrentSourcePageType?.FullName ?? "(null)"}");
        }
        catch (Exception ex)
        {
            App.AppendVerificationLog($"NavigateException {ex.GetType().FullName}: {ex.Message}");
            App.AppendVerificationLog(ex.ToString());
            App.AppendSelectionVerificationLog($"NavigateException {ex.GetType().FullName}: {ex.Message}");
            App.AppendSelectionVerificationLog(ex.ToString());
            _shouldFocusContentAfterNavigation = false;
            throw;
        }
    }

    private FocusState GetNavigationFocusState()
    {
        // XamlRoot is still null when the first navigation runs at startup, and
        // GetFocusedElement(null) throws E_INVALIDARG.
        if (ContentFrame.XamlRoot is { } root &&
            FocusManager.GetFocusedElement(root) is Control { FocusState: FocusState.Keyboard })
        {
            return FocusState.Keyboard;
        }

        return FocusState.Programmatic;
    }

    private void FocusContentAfterNavigationIfRequested()
    {
        if (!_shouldFocusContentAfterNavigation)
        {
            return;
        }

        _shouldFocusContentAfterNavigation = false;

        if (ContentFrame.Content is not FrameworkElement page)
        {
            return;
        }

        // Land focus on the first focusable element INSIDE the page rather than on
        // the page root, so Narrator starts reading at a real control and the first
        // Tab continues from there instead of restarting at the title bar.
        var focusState = _contentNavigationFocusState;
        if (page.IsLoaded)
        {
            FocusFirstElement(page, focusState, attemptsLeft: 10);
            return;
        }

        void OnPageLoaded(object sender, RoutedEventArgs args)
        {
            page.Loaded -= OnPageLoaded;
            FocusFirstElement(page, focusState, attemptsLeft: 10);
        }

        page.Loaded += OnPageLoaded;
    }

    // Synchronous on purpose: the previous page (holding the focused element) stays in
    // the tree until the navigation transition ends, and its removal clears a still
    // pending FocusManager.TryFocusAsync. Retry on the dispatcher while the incoming
    // page is still realizing its first focusable child.
    private void FocusFirstElement(FrameworkElement page, FocusState focusState, int attemptsLeft)
    {
        if (FocusManager.FindFirstFocusableElement(page) is UIElement first &&
            first.Focus(focusState))
        {
            return;
        }

        if (attemptsLeft > 0)
        {
            DispatcherQueue.TryEnqueue(
                DispatcherQueuePriority.Low,
                () => FocusFirstElement(page, focusState, attemptsLeft - 1));
        }
    }

    private static ElementTheme GetNextTheme(ElementTheme theme)
    {
        return theme switch
        {
            ElementTheme.Default => ElementTheme.Light,
            ElementTheme.Light => ElementTheme.Dark,
            _ => ElementTheme.Default,
        };
    }

    private static string GetThemeName(ElementTheme theme)
    {
        return theme switch
        {
            ElementTheme.Light => "Light",
            ElementTheme.Dark => "Dark",
            _ => "Use system setting",
        };
    }

    private void RaiseThemeChangedNotification(ElementTheme theme)
    {
        var peer = FrameworkElementAutomationPeer.FromElement(ThemeToggleButton)
            ?? FrameworkElementAutomationPeer.CreatePeerForElement(ThemeToggleButton);
        peer?.RaiseNotificationEvent(
            AutomationNotificationKind.ActionCompleted,
            AutomationNotificationProcessing.ImportantMostRecent,
            $"Theme: {GetThemeName(theme)}.",
            "ThemeChanged");
    }

    private void SelectNavItem(string tag)
    {
        try
        {
            _isUpdatingSelection = true;
            foreach (var item in NavView.MenuItems)
            {
                if (item is NavigationViewItem navItem && string.Equals(navItem.Tag as string, tag, StringComparison.OrdinalIgnoreCase))
                {
                    NavView.SelectedItem = navItem;
                    return;
                }
            }
            foreach (var item in NavView.FooterMenuItems)
            {
                if (item is NavigationViewItem navItem && string.Equals(navItem.Tag as string, tag, StringComparison.OrdinalIgnoreCase))
                {
                    NavView.SelectedItem = navItem;
                    return;
                }
            }
        }
        finally
        {
            _isUpdatingSelection = false;
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
