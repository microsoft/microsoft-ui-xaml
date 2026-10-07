// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    // Data item for the TableView interaction test page. Raises INotifyPropertyChanged so committed
    // edits and source mutations are observable through the control, and exposes a nested value so
    // dotted bindings can be authored in compiled markup.
    public sealed class TableViewTestPerson : INotifyPropertyChanged
    {
        private string _name;
        private int _age;
        private string _city;

        public TableViewTestPerson(int id, string name, int age, string city)
        {
            Id = id;
            _name = name;
            _age = age;
            _city = city;
        }

        public int Id { get; }

        // Deliberately NON-MONOTONIC in source order: ((id * 7) % 12) + 1 puts the maximum at source index 5,
        // neither first nor last. A column whose source order already equals its ascending order cannot
        // distinguish the trailing "None" step of a sort cycle from "Ascending", which is what
        // HeaderClicksFollowTheColumnSortCycle has to observe.
        public int Score => ((Id * 7) % 12) + 1;

        public string Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    Raise(nameof(Name));
                }
            }
        }

        public int Age
        {
            get => _age;
            set
            {
                if (_age != value)
                {
                    _age = value;
                    Raise(nameof(Age));
                }
            }
        }

        public string City
        {
            get => _city;
            set
            {
                if (_city != value)
                {
                    _city = value;
                    Raise(nameof(City));
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void Raise(string propertyName) =>
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    [TopLevelTestPage(Name = "TableView")]
    [AxeScanTestPage(Name = "TableView-Axe")]
    public sealed partial class TableViewPage : TestPage
    {
        private static readonly string[] Cities = { "Redmond", "Seattle", "Bellevue" };

        private readonly ObservableCollection<TableViewTestPerson> _basicItems = new ObservableCollection<TableViewTestPerson>();
        private readonly ObservableCollection<TableViewTestPerson> _scrollingItems = new ObservableCollection<TableViewTestPerson>();
        private readonly ObservableCollection<TableViewTestPerson> _groupedItems = new ObservableCollection<TableViewTestPerson>();
        private TableViewSource _groupedSource;
        private int _addedColumnCount;

        public TableViewPage()
        {
            // TableView's default styles live in a separate resource dictionary that the tabular
            // DLL requires to be merged into Application.Resources before any tabular type loads.
            if (!Application.Current.Resources.MergedDictionaries.OfType<TabularControlsResources>().Any())
            {
                Application.Current.Resources.MergedDictionaries.Add(new TabularControlsResources());
            }

            InitializeComponent();

            for (int i = 0; i < 12; i++)
            {
                _basicItems.Add(new TableViewTestPerson(i, "Person " + i, 20 + i, Cities[i % Cities.Length]));
            }

            for (int i = 0; i < 200; i++)
            {
                _scrollingItems.Add(new TableViewTestPerson(i, "Scroll " + i, 20 + (i % 40), Cities[i % Cities.Length]));
            }

            for (int i = 0; i < 9; i++)
            {
                _groupedItems.Add(new TableViewTestPerson(i, "Grouped " + i, 30 + i, Cities[i % Cities.Length]));
            }

            _groupedSource = TableViewSource.From(_groupedItems)
                .GroupBy(item => ((TableViewTestPerson)item).City);

            BasicTableView.ItemsSource = _basicItems;
            ScrollingTableView.ItemsSource = _scrollingItems;
            RtlTableView.ItemsSource = _scrollingItems;
            GroupedTableView.ItemsSource = _groupedSource;

            // Reports the column each edit opens on. Appends rather than overwrites so a test can tell a
            // second F2 that did nothing from one that re-reported the same column.
            BasicTableView.BeginningEdit += OnBasicTableViewBeginningEdit;
            BasicTableView.CellEditEnding += OnBasicTableViewCellEditEnding;

            _basicItems[0].PropertyChanged += OnFirstItemPropertyChanged;
            FirstItemNameTextBlock.Text = _basicItems[0].Name;

            ScrollingTableView.LayoutUpdated += OnScrollingTableViewLayoutUpdated;
        }

        private ScrollViewer _scrollingBodyScroller;

        // Resolves PART_BodyScroller lazily. The Scrolling host starts Collapsed, so the TableView's Loaded
        // fires before it is ever measured and therefore before OnApplyTemplate has created the template parts:
        // hooking Loaded reported "<no PART_BodyScroller>" forever. LayoutUpdated fires again once the host is
        // shown and the template is live, and unsubscribes itself the moment the part is found.
        private void OnScrollingTableViewLayoutUpdated(object sender, object e)
        {
            if (_scrollingBodyScroller != null)
            {
                return;
            }

            var scroller = FindDescendantByName<ScrollViewer>(ScrollingTableView, "PART_BodyScroller");
            if (scroller == null)
            {
                return;
            }

            _scrollingBodyScroller = scroller;
            ScrollingTableView.LayoutUpdated -= OnScrollingTableViewLayoutUpdated;

            _scrollingHeaderScroller = FindDescendantByName<ScrollViewer>(ScrollingTableView, "PART_HeaderScroller");

            scroller.ViewChanged += (s, args) => ReportScrollOffsets(scroller);
            ReportScrollOffsets(scroller);
        }

        private ScrollViewer _scrollingHeaderScroller;

        private void ReportScrollOffsets(ScrollViewer scroller)
        {
            ScrollOffsetTextBlock.Text = string.Format(
                "H={0:F0};V={1:F0};HeaderH={2}",
                scroller.HorizontalOffset,
                scroller.VerticalOffset,
                _scrollingHeaderScroller == null
                    ? "<none>"
                    : _scrollingHeaderScroller.HorizontalOffset.ToString("F0"));
        }

        private static T FindDescendantByName<T>(DependencyObject root, string name) where T : FrameworkElement
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is T typed && typed.Name == name)
                {
                    return typed;
                }

                var found = FindDescendantByName<T>(child, name);
                if (found != null)
                {
                    return found;
                }
            }

            return null;
        }

        private void OnFirstItemPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            FirstItemNameTextBlock.Text = _basicItems[0].Name;
        }

        private void OnBasicTableViewCellEditEnding(TableView sender, TableViewCellEditEndingEventArgs args)
        {
            EditEndReportTextBlock.Text += args.EditAction + ";";
        }

        // Reports the editor an edit opened, by an in-process visual-tree walk. Posted at Low priority: the
        // editor does not exist yet while BeginningEdit runs, and focus moves into it after layout.
        // A cell's display visual is a TextBlock, so the only TextBox under the table is the open editor.
        private void ProbeEditor()
        {
            var editor = FindDescendant<TextBox>(BasicTableView);
            if (editor == null)
            {
                EditorProbeTextBlock.Text += "no-editor;";
                return;
            }

            string automationName = AutomationProperties.GetName(editor);
            EditorProbeTextBlock.Text +=
                editor.GetType().Name + "|" +
                (string.IsNullOrEmpty(automationName) ? "-" : automationName) + "|" +
                editor.Text + "|focus=" +
                IsSelfOrDescendant(editor, FocusManager.GetFocusedElement(XamlRoot) as DependencyObject) + ";";
        }

        private static bool IsSelfOrDescendant(DependencyObject root, DependencyObject candidate)
        {
            while (candidate != null)
            {
                if (ReferenceEquals(candidate, root))
                {
                    return true;
                }

                candidate = VisualTreeHelper.GetParent(candidate);
            }

            return false;
        }

        private void OnBasicTableViewBeginningEdit(TableView sender, TableViewBeginningEditEventArgs args)
        {
            EditColumnReportTextBlock.Text += (args.Column?.Header?.ToString() ?? "null") + ";";
            DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, ProbeEditor);
        }

        // Hooks the first realized row's CommonStates group and logs every transition. Walking the visual
        // tree in-process is safe; it creates no automation peers and so does not trip finding #13.
        private void OnHookRowStatesClick(object sender, RoutedEventArgs e)
        {
            RowStateLogTextBlock.Text = string.Empty;

            var row = FindDescendant<TableViewRow>(BasicTableView);
            if (row == null)
            {
                RowStateLogTextBlock.Text = "no-row";
                return;
            }

            // CommonStates lives on the row template's root element, not on the row itself.
            var templateRoot = VisualTreeHelper.GetChildrenCount(row) > 0
                ? VisualTreeHelper.GetChild(row, 0) as FrameworkElement
                : null;
            if (templateRoot == null)
            {
                RowStateLogTextBlock.Text = "no-template-root";
                return;
            }

            foreach (var group in VisualStateManager.GetVisualStateGroups(templateRoot))
            {
                if (group.Name != "CommonStates")
                {
                    continue;
                }

                group.CurrentStateChanged -= OnRowVisualStateChanged;
                group.CurrentStateChanged += OnRowVisualStateChanged;
                RowStateLogTextBlock.Text = "hooked;";
                return;
            }

            RowStateLogTextBlock.Text = "no-common-states";
        }

        private void OnRowVisualStateChanged(object sender, VisualStateChangedEventArgs e)
        {
            RowStateLogTextBlock.Text += (e.NewState?.Name ?? "null") + ";";
        }

        // Hooks every realized group header's ToggleRequested, and the FIRST header's CommonStates group.
        // Same in-process visual-tree walk as OnHookRowStatesClick: it creates no automation peers, so it does
        // not trip finding #13.
        private void OnHookGroupHeadersClick(object sender, RoutedEventArgs e)
        {
            GroupHeaderStateLogTextBlock.Text = string.Empty;
            GroupToggleReportTextBlock.Text = string.Empty;

            var headers = new List<TableViewGroupHeader>();
            CollectDescendants(GroupedTableView, headers);
            if (headers.Count == 0)
            {
                GroupHeaderStateLogTextBlock.Text = "no-header";
                GroupToggleReportTextBlock.Text = "no-header";
                return;
            }

            foreach (var header in headers)
            {
                header.ToggleRequested -= OnGroupHeaderToggleRequested;
                header.ToggleRequested += OnGroupHeaderToggleRequested;
            }
            GroupToggleReportTextBlock.Text = "hooked;";

            var first = headers[0];
            var templateRoot = VisualTreeHelper.GetChildrenCount(first) > 0
                ? VisualTreeHelper.GetChild(first, 0) as FrameworkElement
                : null;
            if (templateRoot == null)
            {
                GroupHeaderStateLogTextBlock.Text = "no-template-root";
                return;
            }

            foreach (var group in VisualStateManager.GetVisualStateGroups(templateRoot))
            {
                if (group.Name != "CommonStates")
                {
                    continue;
                }

                group.CurrentStateChanged -= OnGroupHeaderVisualStateChanged;
                group.CurrentStateChanged += OnGroupHeaderVisualStateChanged;
                GroupHeaderStateLogTextBlock.Text = "hooked;";
                return;
            }

            GroupHeaderStateLogTextBlock.Text = "no-common-states";
        }

        private void OnGroupHeaderVisualStateChanged(object sender, VisualStateChangedEventArgs e)
        {
            GroupHeaderStateLogTextBlock.Text += (e.NewState?.Name ?? "null") + ";";
        }

        // One entry per raise, "<key>|<key>;". The key is read on entry, then the handler MUTATES the header
        // and reads the key again: TableView.idl:350-355 says the key rides on the args precisely so a
        // re-entrant handler still sees the key that was actually activated, and that is only observable if
        // the handler does re-enter.
        private void OnGroupHeaderToggleRequested(TableViewGroupHeader sender, TableViewGroupHeaderToggleRequestedEventArgs args)
        {
            GroupToggleReportTextBlock.Text += (args.GroupKey?.ToString() ?? "null") + "|";
            sender.IsExpanded = !sender.IsExpanded;
            GroupToggleReportTextBlock.Text += (args.GroupKey?.ToString() ?? "null") + ";";
        }

        private static void CollectDescendants<T>(DependencyObject root, List<T> results) where T : class
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is T match)
                {
                    results.Add(match);
                }

                CollectDescendants(child, results);
            }
        }

        private static T FindDescendant<T>(DependencyObject root) where T : class
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is T match)
                {
                    return match;
                }

                var nested = FindDescendant<T>(child);
                if (nested != null)
                {
                    return nested;
                }
            }

            return null;
        }

        // Unloads the Basic table from the tree shortly after the click, then restores it. The delay
        // exists so a test can start a pointer drag and have the unload land mid-gesture, which a
        // direct click cannot do while the control holds pointer capture.
        private void OnDelayedUnloadClick(object sender, RoutedEventArgs e)
        {
            var unloadTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1.5) };
            unloadTimer.Tick += (s, args) =>
            {
                unloadTimer.Stop();
                BasicTableHost.Children.Remove(BasicTableView);
                StatusTextBlock.Text = "Unloaded";

                var reloadTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1.5) };
                reloadTimer.Tick += (s2, args2) =>
                {
                    reloadTimer.Stop();
                    BasicTableHost.Children.Add(BasicTableView);
                    StatusTextBlock.Text = "Reloaded";
                };
                reloadTimer.Start();
            };
            unloadTimer.Start();
        }

        private void OnIsReadOnlyToggled(object sender, RoutedEventArgs e)
        {
            BasicTableView.IsReadOnly = ((CheckBox)sender).IsChecked == true;
        }

        // Guarded because IsChecked="True" in markup raises Checked during InitializeComponent, before the
        // BasicTableView field is assigned - an unguarded dereference there throws inside the parse and the
        // whole page fails to load with E_XAMLPARSEFAILED (0x802B000A).
        private void OnCanUserSortColumnsToggled(object sender, RoutedEventArgs e)
        {
            if (BasicTableView == null)
            {
                return;
            }

            BasicTableView.CanUserSortColumns = ((CheckBox)sender).IsChecked == true;
        }

        // Page switching without a Pivot: hosting the TableViews in a Pivot asserts in
        // Microsoft.UI.Xaml.Phone.dll (0xC0000420) on the second navigation to this page. These
        // buttons are reachable by AutomationId, which also keeps switching off the name-based UIA
        // search that trips product finding #13.
        private void OnGoToBasicClick(object sender, RoutedEventArgs e) => ShowTableHost(BasicTableHost);

        private void OnGoToRtlClick(object sender, RoutedEventArgs e) => ShowTableHost(RtlTableHost);

        private void OnGoToGroupedClick(object sender, RoutedEventArgs e) => ShowTableHost(GroupedTableHost);

        private void OnGoToScrollingClick(object sender, RoutedEventArgs e) => ShowTableHost(ScrollingTableHost);

        private void ShowTableHost(FrameworkElement host)
        {
            BasicTableHost.Visibility = Visibility.Collapsed;
            RtlTableHost.Visibility = Visibility.Collapsed;
            GroupedTableHost.Visibility = Visibility.Collapsed;
            ScrollingTableHost.Visibility = Visibility.Collapsed;
            host.Visibility = Visibility.Visible;
        }

        private void OnSelectionModeChanged(object sender, SelectionChangedEventArgs e)
        {
            if (BasicTableView == null)
            {
                return;
            }

            BasicTableView.SelectionMode = ((ComboBox)sender).SelectedIndex == 0
                ? TableViewSelectionMode.None
                : TableViewSelectionMode.Single;
        }

        // Drives the per-column sort cycle for HeaderClicksFollowTheColumnSortCycle. SortCycle is read at click
        // time, so flipping it between clicks needs no rebuild. Null-guarded because SelectedIndex is authored in
        // markup: the resulting SelectionChanged is raised during InitializeComponent, before ScoreColumn is set,
        // and an unguarded throw there surfaces as E_XAMLPARSEFAILED for the whole page.
        private void OnSortCycleChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ScoreColumn == null)
            {
                return;
            }

            ScoreColumn.SortCycle = ((ComboBox)sender).SelectedIndex switch
            {
                1 => TableViewSortCycle.AscendingDescendingNone,
                2 => TableViewSortCycle.DescendingAscending,
                3 => TableViewSortCycle.DescendingAscendingNone,
                _ => TableViewSortCycle.AscendingDescending,
            };
        }

        private void OnAddColumnClick(object sender, RoutedEventArgs e)
        {
            _addedColumnCount++;

            BasicTableView.Columns.Add(new TableViewTextColumn
            {
                Header = "Added " + _addedColumnCount,
                Width = new GridLength(120),
                Binding = new Binding { Path = new PropertyPath("City") },
            });
        }

        private void OnRemoveColumnClick(object sender, RoutedEventArgs e)
        {
            if (BasicTableView.Columns.Count > 0)
            {
                BasicTableView.Columns.RemoveAt(BasicTableView.Columns.Count - 1);
            }
        }

        private void OnExpandAllGroupsClick(object sender, RoutedEventArgs e)
        {
            GroupedTableView.ExpandAllGroups();
        }

        private void OnCollapseAllGroupsClick(object sender, RoutedEventArgs e)
        {
            GroupedTableView.CollapseAllGroups();
        }

        private void OnFilterSourceClick(object sender, RoutedEventArgs e)
        {
            _groupedSource.Filter(item => ((TableViewTestPerson)item).City != "Seattle");
        }

        private void OnClearFilterClick(object sender, RoutedEventArgs e)
        {
            _groupedSource.ClearFilter();
        }
    }
}
