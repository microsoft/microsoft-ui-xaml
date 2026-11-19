using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;

namespace TemplateDeferralBench
{
    public static class ItemScenarios
    {
        private const int TreeItemsPerView = 28;
        private const int LeftNavItemsPerView = 22;
        private const int TopNavItemsPerView = 10;

        public static IEnumerable<Scenario> All()
        {
            yield return new Scenario
            {
                Name = "TreeViewItem",
                Build = BuildTrees,
                DeferredPartNames = new[] { "SelectionIndicator", "MultiSelectCheckBox", "MultiArrangeOverlayTextBorder" },
                Trigger = root =>
                {
                    // Multiple selection shows MultiSelectCheckBox on every realized item.
                    var trees = Bench.All<TreeView>(root).ToList();
                    foreach (var tree in trees) tree.SelectionMode = TreeViewSelectionMode.Multiple;
                    root.UpdateLayout();

                    // Checking a box must still select its node (Checked handler hooked on the lazily fetched box).
                    var first = trees[0];
                    if (first.ContainerFromNode(first.RootNodes[0]) is TreeViewItem firstItem &&
                        Bench.Named(firstItem, "MultiSelectCheckBox") is CheckBox box)
                    {
                        box.IsChecked = true;
                    }

                    // Selected shows SelectionIndicator; MultipleDraggingPrimary (odd items) shows the drag count overlay.
                    var items = Bench.All<TreeViewItem>(root).ToList();
                    for (int i = 0; i < items.Count; i++)
                    {
                        VisualStateManager.GoToState(items[i], "Selected", false);
                        if (i % 2 == 1) VisualStateManager.GoToState(items[i], "MultipleDraggingPrimary", false);
                    }
                },
                Probe = root =>
                {
                    var items = Bench.All<TreeViewItem>(root).ToList();
                    var first = Bench.All<TreeView>(root).First();
                    bool Shown(TreeViewItem item, string name) =>
                        Bench.Named(item, name) is FrameworkElement part && Bench.IsShown(part, item) && part.ActualWidth > 0 && part.ActualHeight > 0;
                    return new Dictionary<string, object>
                    {
                        ["Items"] = items.Count,
                        ["HasItems"] = items.Count > 0,
                        ["AllHaveCheckBox"] = items.All(item => Bench.Named(item, "MultiSelectCheckBox") is CheckBox),
                        ["CheckBoxShown"] = items.Where((_, i) => i % 2 == 0).All(item => Shown(item, "MultiSelectCheckBox")),
                        ["SelectionIndicatorShown"] = items.All(item => Shown(item, "SelectionIndicator")),
                        ["DragOverlayShown"] = items.Where((_, i) => i % 2 == 1).All(item => Shown(item, "MultiArrangeOverlayTextBorder")),
                        ["CheckBoxSelectsNode"] = first.SelectedNodes.Count == 1 && first.SelectedNodes[0] == first.RootNodes[0],
                    };
                },
            };

            yield return new Scenario
            {
                Name = "NavigationViewItemLeft",
                Build = count => BuildNavs(count, top: false),
                DeferredPartNames = new[] { "SelectionIndicator" },
                Trigger = root =>
                {
                    // SelectionIndicator is only needed by the selected item; select every realized item in turn.
                    foreach (var nav in Bench.All<NavigationView>(root).ToList())
                    {
                        foreach (var item in Bench.All<NavigationViewItem>(nav).ToList()) nav.SelectedItem = item;
                    }
                },
                Probe = root =>
                {
                    var items = Bench.All<NavigationViewItem>(root).ToList();
                    var selected = Bench.All<NavigationView>(root)
                        .Select(nav => nav.SelectedItem is NavigationViewItem item ? Bench.Named(item, "SelectionIndicator") : null).ToList();
                    return new Dictionary<string, object>
                    {
                        ["Items"] = items.Count,
                        ["HasItems"] = items.Count > 0,
                        ["AllIndicatorsRealized"] = items.All(item => Bench.Named(item, "SelectionIndicator") != null),
                        ["SelectedIndicatorLaidOut"] = selected.All(indicator => indicator != null && indicator.ActualWidth > 0 && indicator.ActualHeight > 0),
                        ["SelectedIndicatorMinOpacity"] = selected.Count == 0 ? 0.0 : selected.Min(indicator => indicator?.Opacity ?? 0.0),
                    };
                },
            };

            yield return new Scenario
            {
                Name = "NavigationViewItemTop",
                Build = count => BuildNavs(count, top: true),
                DeferredPartNames = new[] { "PointerRectangle", "ExpandCollapseChevron" },
                Trigger = root =>
                {
                    // Children make the chevron needed; PointerOver targets PointerRectangle.
                    foreach (var nav in Bench.All<NavigationView>(root).ToList())
                    {
                        foreach (var item in nav.MenuItems.OfType<NavigationViewItem>())
                        {
                            item.MenuItems.Add(new NavigationViewItem { Content = "Child" });
                        }
                    }
                    root.UpdateLayout();
                    foreach (var presenter in Bench.All<NavigationViewItemPresenter>(root).ToList())
                    {
                        VisualStateManager.GoToState(presenter, "PointerOver", false);
                    }
                },
                Probe = root =>
                {
                    var presenters = Bench.All<NavigationViewItemPresenter>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["Items"] = presenters.Count,
                        ["HasItems"] = presenters.Count > 0,
                        ["PointerRectangleFillSet"] = presenters.All(p =>
                            Bench.Named(p, "PointerRectangle") is Microsoft.UI.Xaml.Shapes.Rectangle r &&
                            r.Fill is SolidColorBrush brush && brush.Color != Colors.Transparent),
                        ["ChevronShown"] = presenters.All(p =>
                            Bench.Named(p, "ExpandCollapseChevron") is FrameworkElement chevron &&
                            Bench.IsShown(chevron, p) && chevron.ActualWidth > 0),
                    };
                },
            };
        }

        private static FrameworkElement BuildTrees(int count)
        {
            int trees = Math.Max(1, (count + TreeItemsPerView - 1) / TreeItemsPerView);
            double width = Math.Max(150, 1380.0 / trees);
            var panel = new StackPanel { Orientation = Orientation.Horizontal, Background = new SolidColorBrush(Colors.White) };
            for (int t = 0; t < trees; t++)
            {
                var tree = new TreeView { Width = width, Height = 960 };
                for (int i = t * TreeItemsPerView; i < Math.Min(count, (t + 1) * TreeItemsPerView); i++)
                {
                    tree.RootNodes.Add(new TreeViewNode { Content = $"Item {i}" });
                }
                panel.Children.Add(tree);
            }
            return panel;
        }

        private static FrameworkElement BuildNavs(int count, bool top)
        {
            int perView = top ? TopNavItemsPerView : LeftNavItemsPerView;
            int navs = Math.Max(1, (count + perView - 1) / perView);
            double width = top ? 1380 : Math.Max(150, 1380.0 / navs);
            var panel = new StackPanel
            {
                Orientation = top ? Orientation.Vertical : Orientation.Horizontal,
                Background = new SolidColorBrush(Colors.White),
            };
            for (int n = 0; n < navs; n++)
            {
                var nav = new NavigationView
                {
                    PaneDisplayMode = top ? NavigationViewPaneDisplayMode.Top : NavigationViewPaneDisplayMode.Left,
                    IsPaneOpen = true,
                    IsSettingsVisible = false,
                    IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                    IsPaneToggleButtonVisible = false,
                    Width = width,
                    Height = top ? 48 : 960,
                };
                if (!top) nav.OpenPaneLength = width;
                for (int i = n * perView; i < Math.Min(count, (n + 1) * perView); i++)
                {
                    // An icon keeps top items in IconOnLeft (ContentOnly targets the chevron).
                    nav.MenuItems.Add(new NavigationViewItem { Content = $"Item {i}", Icon = new SymbolIcon(Symbol.Document) });
                }
                panel.Children.Add(nav);
            }
            return panel;
        }
    }
}