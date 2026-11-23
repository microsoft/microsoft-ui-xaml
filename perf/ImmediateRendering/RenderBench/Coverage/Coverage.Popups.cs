using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using static RenderBench.Coverage.CoveragePageKit;

namespace RenderBench.Coverage;

// F12 popups and dialogs, each opened by Exercise. "windowed" pages set ShouldConstrainToRootBounds=false so the popup
// gets its own HWND (immediate mode paints it through the windowed-popup path; the harness composites every visible
// top-level window of the process into the capture). Coverage branch.
static class CoveragePopups
{
    const string F = "F12-popups";

    static (Grid root, Button anchor) Host(string text = "Anchor")
    {
        var anchor = new Button { Content = text, Margin = new Thickness(40, 40, 0, 0) };
        var root = Surface(ElementTheme.Light, [anchor]);
        return (root, anchor);
    }

    static CoveragePage Open(Grid root, Func<Task> open, Action? close = null) => new()
    {
        Root = root,
        Exercise = async () => { await open(); await Settle(); },
        Detach = close,
    };

    static Flyout TextFlyout(bool windowed) => new()
    {
        ShouldConstrainToRootBounds = !windowed,
        Content = new StackPanel { Spacing = 8, Width = 220, Children = {
            new TextBlock { Text = "Flyout title", FontWeight = Microsoft.UI.Text.FontWeights.SemiBold },
            new TextBlock { Text = "Flyout content with a button.", TextWrapping = TextWrapping.Wrap },
            new Button { Content = "OK" } } },
    };

    static CoveragePage FlyoutPage(bool windowed)
    {
        var (root, anchor) = Host();
        var flyout = TextFlyout(windowed);
        return Open(root, () => { flyout.ShowAt(anchor); return Task.CompletedTask; }, flyout.Hide);
    }

    static MenuFlyout Menu(bool windowed)
    {
        var menu = new MenuFlyout { ShouldConstrainToRootBounds = !windowed };
        menu.Items.Add(new MenuFlyoutItem { Text = "Cut", Icon = new SymbolIcon(Symbol.Cut), KeyboardAcceleratorTextOverride = "Ctrl+X" });
        menu.Items.Add(new MenuFlyoutItem { Text = "Copy", Icon = new SymbolIcon(Symbol.Copy) });
        menu.Items.Add(new ToggleMenuFlyoutItem { Text = "Word wrap", IsChecked = true });
        menu.Items.Add(new MenuFlyoutSeparator());
        var sub = new MenuFlyoutSubItem { Text = "Sort by" };
        sub.Items.Add(new RadioMenuFlyoutItem { Text = "Name", IsChecked = true, GroupName = "sort" });
        sub.Items.Add(new RadioMenuFlyoutItem { Text = "Date", GroupName = "sort" });
        menu.Items.Add(sub);
        return menu;
    }

    static CoveragePage MenuPage(bool windowed)
    {
        var (root, anchor) = Host();
        var menu = Menu(windowed);
        return Open(root, async () =>
        {
            menu.ShowAt(anchor);
            await Settle();
            var sub = menu.Items.OfType<MenuFlyoutSubItem>().Single();
            if (FrameworkElementAutomationPeer.CreatePeerForElement(sub)?.GetPattern(PatternInterface.ExpandCollapse)
                is Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider expander)
            {
                expander.Expand();
            }
        }, menu.Hide);
    }

    static CoveragePage CommandBarFlyoutPage(bool windowed)
    {
        var (root, anchor) = Host();
        var flyout = new CommandBarFlyout { ShouldConstrainToRootBounds = !windowed, AlwaysExpanded = true };
        flyout.PrimaryCommands.Add(new AppBarButton { Icon = new SymbolIcon(Symbol.Cut), Label = "Cut" });
        flyout.PrimaryCommands.Add(new AppBarButton { Icon = new SymbolIcon(Symbol.Copy), Label = "Copy" });
        flyout.PrimaryCommands.Add(new AppBarButton { Icon = new SymbolIcon(Symbol.Paste), Label = "Paste" });
        flyout.SecondaryCommands.Add(new AppBarButton { Label = "Select all" });
        flyout.SecondaryCommands.Add(new AppBarButton { Label = "Undo" });
        return Open(root, () =>
        {
            flyout.ShowAt(anchor, new FlyoutShowOptions { ShowMode = FlyoutShowMode.Standard });
            return Task.CompletedTask;
        }, flyout.Hide);
    }

    static CoveragePage TeachingTipPage(bool windowed)
    {
        var (root, anchor) = Host();
        var tip = new TeachingTip
        {
            Target = anchor, Title = "Teaching tip", Subtitle = "Points at the anchor button.",
            ShouldConstrainToRootBounds = !windowed, IsLightDismissEnabled = false,
            PreferredPlacement = TeachingTipPlacementMode.Bottom, ActionButtonContent = "Got it", CloseButtonContent = "Close",
        };
        root.Children.Add(tip);
        return Open(root, () => { tip.IsOpen = true; return Task.CompletedTask; }, () => tip.IsOpen = false);
    }

    static CoveragePage RawPopupPage(bool windowed)
    {
        var (root, _) = Host();
        var popup = new Popup
        {
            ShouldConstrainToRootBounds = !windowed, HorizontalOffset = 200, VerticalOffset = 120,
            Child = new Border
            {
                Width = 260, Height = 120, CornerRadius = new CornerRadius(8), BorderThickness = new Thickness(1), Padding = new Thickness(12),
                Background = ThemeBrush("SolidBackgroundFillColorBaseBrush"), BorderBrush = ThemeBrush("SurfaceStrokeColorFlyoutBrush"),
                Child = new TextBlock { Text = windowed ? "Windowed popup" : "Non-windowed popup" },
            },
        };
        root.Children.Add(popup);
        return Open(root, () => { popup.IsOpen = true; return Task.CompletedTask; }, () => popup.IsOpen = false);
    }

    [CoveragePage(F, "contentdialog")]
    public static CoveragePage ContentDialogPage(CoverageContext c)
    {
        var (root, _) = Host();
        var dialog = new ContentDialog
        {
            Title = "Save changes?", Content = "Your edits will be lost if you don't save them.",
            PrimaryButtonText = "Save", SecondaryButtonText = "Don't save", CloseButtonText = "Cancel", DefaultButton = ContentDialogButton.Primary,
        };
        return Open(root, () =>
        {
            dialog.XamlRoot = root.XamlRoot;
            _ = dialog.ShowAsync();
            return Task.CompletedTask;
        }, dialog.Hide);
    }

    [CoveragePage(F, "flyout")] public static CoveragePage FlyoutConstrained(CoverageContext c) => FlyoutPage(false);
    [CoveragePage(F, "flyout-windowed")] public static CoveragePage FlyoutWindowed(CoverageContext c) => FlyoutPage(true);
    [CoveragePage(F, "menuflyout-submenu")] public static CoveragePage MenuConstrained(CoverageContext c) => MenuPage(false);
    [CoveragePage(F, "menuflyout-submenu-windowed")] public static CoveragePage MenuWindowed(CoverageContext c) => MenuPage(true);
    [CoveragePage(F, "commandbarflyout")] public static CoveragePage CommandBarFlyoutConstrained(CoverageContext c) => CommandBarFlyoutPage(false);
    [CoveragePage(F, "commandbarflyout-windowed")] public static CoveragePage CommandBarFlyoutWindowed(CoverageContext c) => CommandBarFlyoutPage(true);
    [CoveragePage(F, "teachingtip")] public static CoveragePage TeachingTipConstrained(CoverageContext c) => TeachingTipPage(false);
    [CoveragePage(F, "teachingtip-windowed")] public static CoveragePage TeachingTipWindowed(CoverageContext c) => TeachingTipPage(true);
    [CoveragePage(F, "popup")] public static CoveragePage PopupConstrained(CoverageContext c) => RawPopupPage(false);
    [CoveragePage(F, "popup-windowed")] public static CoveragePage PopupWindowed(CoverageContext c) => RawPopupPage(true);

    [CoveragePage(F, "tooltip")]
    public static CoveragePage ToolTipPage(CoverageContext c)
    {
        var (root, anchor) = Host("Hover target");
        var tip = new ToolTip { Content = "A tooltip", Placement = PlacementMode.Bottom };
        ToolTipService.SetToolTip(anchor, tip);
        return Open(root, () => { tip.IsOpen = true; return Task.CompletedTask; }, () => tip.IsOpen = false);
    }

    [CoveragePage(F, "combobox-open")]
    public static CoveragePage ComboOpen(CoverageContext c)
    {
        var combo = new ComboBox { Width = 200, ItemsSource = Words(6), SelectedIndex = 2, Margin = new Thickness(40, 40, 0, 0) };
        return Open(Surface(ElementTheme.Light, [combo]), () => { combo.IsDropDownOpen = true; return Task.CompletedTask; }, () => combo.IsDropDownOpen = false);
    }

    [CoveragePage(F, "calendardatepicker-open")]
    public static CoveragePage CalendarOpen(CoverageContext c)
    {
        var picker = new CalendarDatePicker
        {
            Margin = new Thickness(40, 40, 0, 0), IsTodayHighlighted = false,
            Date = new DateTimeOffset(2024, 3, 15, 0, 0, 0, TimeSpan.Zero),
        };
        return Open(Surface(ElementTheme.Light, [picker]), () => { picker.IsCalendarOpen = true; return Task.CompletedTask; }, () => picker.IsCalendarOpen = false);
    }

    [CoveragePage(F, "autosuggestbox-suggestions")]
    public static CoveragePage AutoSuggest(CoverageContext c)
    {
        var box = new AutoSuggestBox { Width = 260, Margin = new Thickness(40, 40, 0, 0), PlaceholderText = "Search", QueryIcon = new SymbolIcon(Symbol.Find) };
        return Open(Surface(ElementTheme.Light, [box]), () =>
        {
            box.Text = "Ite";
            box.ItemsSource = Words(5);
            box.IsSuggestionListOpen = true;
            return Task.CompletedTask;
        }, () => box.IsSuggestionListOpen = false);
    }
}
