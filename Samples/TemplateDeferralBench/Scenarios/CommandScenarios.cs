using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;

namespace TemplateDeferralBench
{
    public static class CommandScenarios
    {
        public static IEnumerable<Scenario> All()
        {
            // IconRoot is only shown by the IconPlaceholder states; KeyboardAcceleratorTextBlock only by KeyboardAcceleratorTextVisible.
            yield return new Scenario
            {
                Name = "MenuFlyoutItem",
                Build = count => Bench.Rows(count, i => new MenuFlyoutItem { Text = $"Item {i}", Width = 180 }),
                DeferredPartNames = new[] { "IconRoot", "KeyboardAcceleratorTextBlock" },
                Trigger = root =>
                {
                    foreach (var item in Bench.All<MenuFlyoutItem>(root).ToList())
                    {
                        item.Icon = new SymbolIcon(Symbol.Accept);
                        item.KeyboardAcceleratorTextOverride = "Ctrl+X";
                        VisualStateManager.GoToState(item, "IconPlaceholder", false);
                        VisualStateManager.GoToState(item, "KeyboardAcceleratorTextVisible", false);
                    }
                },
                Probe = root =>
                {
                    var items = Bench.All<MenuFlyoutItem>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["IconShown"] = items.All(m => Bench.Named(m, "IconRoot") is FrameworkElement r && Bench.IsShown(r, m) && r.ActualWidth > 0),
                        ["AcceleratorTextShown"] = items.All(m => Bench.Named(m, "KeyboardAcceleratorTextBlock") is TextBlock t && Bench.IsShown(t, m) && t.Text == "Ctrl+X" && t.ActualWidth > 0),
                        // PointerOver retargets the icon foreground through IconRoot.(Viewbox.Child).
                        ["PointerOverStateApplies"] = items.All(m => VisualStateManager.GoToState(m, "PointerOver", false) && VisualStateManager.GoToState(m, "Normal", false)),
                    };
                },
            };

            yield return new Scenario
            {
                Name = "ToggleMenuFlyoutItem",
                Build = count => Bench.Rows(count, i => new ToggleMenuFlyoutItem { Text = $"Item {i}", Width = 180 }),
                DeferredPartNames = new[] { "IconRoot", "KeyboardAcceleratorTextBlock" },
                Trigger = root =>
                {
                    foreach (var item in Bench.All<ToggleMenuFlyoutItem>(root).ToList())
                    {
                        item.IsChecked = true;
                        item.Icon = new SymbolIcon(Symbol.Accept);
                        item.KeyboardAcceleratorTextOverride = "Ctrl+T";
                        VisualStateManager.GoToState(item, "CheckedWithIcon", false);
                        VisualStateManager.GoToState(item, "KeyboardAcceleratorTextVisible", false);
                    }
                },
                Probe = root =>
                {
                    var items = Bench.All<ToggleMenuFlyoutItem>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["AllChecked"] = items.All(m => m.IsChecked),
                        ["IconShown"] = items.All(m => Bench.Named(m, "IconRoot") is FrameworkElement r && Bench.IsShown(r, m) && r.ActualWidth > 0),
                        ["AcceleratorTextShown"] = items.All(m => Bench.Named(m, "KeyboardAcceleratorTextBlock") is TextBlock t && Bench.IsShown(t, m) && t.Text == "Ctrl+T"),
                        ["CheckGlyphPresent"] = items.All(m => Bench.Named(m, "CheckGlyph") != null),
                        ["PointerOverStateApplies"] = items.All(m => VisualStateManager.GoToState(m, "PointerOver", false) && VisualStateManager.GoToState(m, "Normal", false)),
                    };
                },
            };

            yield return new Scenario
            {
                Name = "MenuFlyoutSubItem",
                Build = count => Bench.Rows(count, i =>
                {
                    var subItem = new MenuFlyoutSubItem { Text = $"Item {i}", Width = 180 };
                    subItem.Items.Add(new MenuFlyoutItem { Text = "Child" });
                    return subItem;
                }),
                DeferredPartNames = new[] { "IconRoot" },
                Trigger = root =>
                {
                    foreach (var item in Bench.All<MenuFlyoutSubItem>(root).ToList())
                    {
                        item.Icon = new SymbolIcon(Symbol.Accept);
                        VisualStateManager.GoToState(item, "IconPlaceholder", false);
                    }
                },
                Probe = root =>
                {
                    var items = Bench.All<MenuFlyoutSubItem>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["IconShown"] = items.All(m => Bench.Named(m, "IconRoot") is FrameworkElement r && Bench.IsShown(r, m) && r.ActualWidth > 0),
                        ["ChevronPresent"] = items.All(m => Bench.Named(m, "SubItemChevron") != null),
                        ["PointerOverStateApplies"] = items.All(m => VisualStateManager.GoToState(m, "PointerOver", false) && VisualStateManager.GoToState(m, "Normal", false)),
                    };
                },
            };

            // Overflow label, accelerator text and overflow chevron are only used when the button is in a CommandBar overflow menu.
            yield return new Scenario
            {
                Name = "AppBarButton",
                Build = count => Bench.Rows(count, i => new AppBarButton { Label = $"Cmd {i}", Icon = new SymbolIcon(Symbol.Accept) }),
                DeferredPartNames = new[] { "OverflowTextLabel", "KeyboardAcceleratorTextLabel", "OverflowSubItemChevron" },
                Trigger = root =>
                {
                    foreach (var button in Bench.All<AppBarButton>(root).ToList())
                    {
                        var flyout = new MenuFlyout();
                        flyout.Items.Add(new MenuFlyoutItem { Text = "Child" });
                        button.Flyout = flyout;
                        button.KeyboardAcceleratorTextOverride = "Ctrl+B";
                        VisualStateManager.GoToState(button, "Overflow", false);
                        VisualStateManager.GoToState(button, "OverflowNormal", false);
                        VisualStateManager.GoToState(button, "HasFlyout", false);
                        VisualStateManager.GoToState(button, "KeyboardAcceleratorTextVisible", false);
                    }
                },
                Probe = root =>
                {
                    var buttons = Bench.All<AppBarButton>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["OverflowLabelShown"] = buttons.All(b => Bench.Named(b, "OverflowTextLabel") is TextBlock t && Bench.IsShown(t, b) && !string.IsNullOrEmpty(t.Text)),
                        ["AcceleratorTextShown"] = buttons.All(b => Bench.Named(b, "KeyboardAcceleratorTextLabel") is TextBlock t && Bench.IsShown(t, b) && t.Text == "Ctrl+B"),
                        ["OverflowChevronRealized"] = buttons.All(b => Bench.Named(b, "OverflowSubItemChevron") is FrameworkElement c && c.Visibility == Visibility.Visible),
                    };
                },
            };

            yield return new Scenario
            {
                Name = "AppBarToggleButton",
                Build = count => Bench.Rows(count, i => new AppBarToggleButton { Label = $"Cmd {i}", Icon = new SymbolIcon(Symbol.Accept) }),
                DeferredPartNames = new[] { "OverflowCheckGlyph", "OverflowTextLabel", "KeyboardAcceleratorTextLabel" },
                Trigger = root =>
                {
                    foreach (var button in Bench.All<AppBarToggleButton>(root).ToList())
                    {
                        button.IsChecked = true;
                        button.KeyboardAcceleratorTextOverride = "Ctrl+T";
                        VisualStateManager.GoToState(button, "Overflow", false);
                        VisualStateManager.GoToState(button, "OverflowChecked", false);
                        VisualStateManager.GoToState(button, "KeyboardAcceleratorTextVisible", false);
                    }
                },
                Probe = root =>
                {
                    var buttons = Bench.All<AppBarToggleButton>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["AllChecked"] = buttons.All(b => b.IsChecked == true),
                        ["CheckGlyphShown"] = buttons.All(b => Bench.Named(b, "OverflowCheckGlyph") is FrameworkElement g && Bench.IsShown(g, b) && g.ActualWidth > 0),
                        ["OverflowLabelShown"] = buttons.All(b => Bench.Named(b, "OverflowTextLabel") is TextBlock t && Bench.IsShown(t, b) && !string.IsNullOrEmpty(t.Text)),
                        ["AcceleratorTextShown"] = buttons.All(b => Bench.Named(b, "KeyboardAcceleratorTextLabel") is TextBlock t && Bench.IsShown(t, b) && t.Text == "Ctrl+T"),
                    };
                },
            };

            // OpenBorder is only shown by the CompactOpenUp/CompactOpenDown states.
            yield return new Scenario
            {
                Name = "CommandBar",
                Build = count => Bench.Rows(count, i =>
                {
                    var commandBar = new CommandBar { Width = 300, Margin = new Thickness(0, 0, 0, 60) };
                    commandBar.PrimaryCommands.Add(new AppBarButton { Label = $"Cmd {i}", Icon = new SymbolIcon(Symbol.Accept) });
                    commandBar.PrimaryCommands.Add(new AppBarButton { Label = "Other", Icon = new SymbolIcon(Symbol.Cancel) });
                    return commandBar;
                }, perRow: 4),
                DeferredPartNames = new[] { "OpenBorder" },
                Trigger = root =>
                {
                    foreach (var commandBar in Bench.All<CommandBar>(root).ToList()) commandBar.IsOpen = true;
                },
                Probe = root =>
                {
                    var bars = Bench.All<CommandBar>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["AllOpen"] = bars.All(c => c.IsOpen),
                        ["OpenBorderShown"] = bars.All(c => Bench.Named(c, "OpenBorder") is FrameworkElement b && b.Visibility == Visibility.Visible),
                    };
                },
            };
        }
    }
}
