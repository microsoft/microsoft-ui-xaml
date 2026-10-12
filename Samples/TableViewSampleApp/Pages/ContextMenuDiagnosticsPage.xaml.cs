// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

// Manual verification harness for the context menu pipeline; see README.md, "Context menu diagnostics".
public sealed partial class ContextMenuDiagnosticsPage : Page
{
    private readonly TableViewTextColumn _name =
        SampleColumns.Text("First name", nameof(Person.FirstName), SampleColumns.Pixels(140));
    private readonly TableViewTextColumn _city =
        SampleColumns.Text("Office", nameof(Person.Office), SampleColumns.Pixels(120));
    private readonly TableViewTextColumn _score =
        SampleColumns.Text("Salary", nameof(Person.Salary), SampleColumns.Pixels(90));
    private readonly TableViewTemplateColumn _notes = new();
    private readonly List<Person> _items = PersonData.Many(300);
    private int _opens;
    private int _openedFixtureIndex = -1;
    private string _lastOpen = "none";
    private string _apiDefaults = "UNKNOWN";
    private bool _mutateOnFocus;
    private bool _nullItemFixture;
    private bool _innerMenus = true;
    private string _nullRealized = "PENDING";

    public ContextMenuDiagnosticsPage()
    {
        InitializeComponent();
        // GotFocus is delivered after Focus returns; GettingFocus exercises synchronous invalidation.
        Table.GettingFocus += (_, _) =>
        {
            if (!_mutateOnFocus) return;
            _mutateOnFocus = false;
            Table.ItemsSource = PersonData.Many(300);
            Hint.Text = "Items replaced during focus. Expect no custom open; native fallback remains eligible if no custom menu was configured.";
            Report();
        };
        Table.LayoutUpdated += OnTableLayoutUpdated;
        VerifyApiDefaults();
        _name.Binding = new Binding
        {
            Path = new PropertyPath(nameof(Person.FirstName)),
            Mode = BindingMode.TwoWay,
        };
        _notes.Header = "Bio";
        _notes.Width = SampleColumns.Pixels(220);
        _notes.CellTemplate = (DataTemplate)Resources["AppFlyoutCell"];
        Table.SelectionChanged += (_, _) => Report();
        Reset();
    }

    private MenuFlyout Menu(string label, bool bindItem = false)
    {
        var menu = new MenuFlyout();
        var presenterStyle = new Style { TargetType = typeof(MenuFlyoutPresenter) };
        presenterStyle.Setters.Add(new Setter(AutomationProperties.NameProperty, label));
        menu.MenuFlyoutPresenterStyle = presenterStyle;
        var entry = new MenuFlyoutItem { Text = label };
        AutomationProperties.SetName(entry, label);
        menu.Items.Add(entry);
        if (bindItem)
        {
            var bound = new MenuFlyoutItem();
            bound.SetBinding(MenuFlyoutItem.TextProperty,
                new Binding { Path = new PropertyPath(nameof(Person.FirstName)) });
            menu.Items.Add(bound);
            menu.Opened += (_, _) =>
            {
                // The menu inherits the requested row's item; compare it with the row that was invoked.
                _openedFixtureIndex = bound.DataContext is Person item ? _items.IndexOf(item) : -1;
                _lastOpen = $"{label}; item={(bound.DataContext as Person)?.FullName ?? "(null)"}; text={bound.Text}";
            };
        }

        menu.Opened += (_, _) =>
        {
            ++_opens;
            if (!bindItem)
            {
                _lastOpen = label;
            }

            Report();
        };
        return menu;
    }

    private void VerifyApiDefaults()
    {
        List<string> failures = new();
        if (_name.CellContextFlyout is not null)
        {
            failures.Add("Name.Cell!=null");
        }

        if (_name.HeaderContextFlyout is not null)
        {
            failures.Add("Name.Header!=null");
        }

        if (Table.RowContextFlyout is not null)
        {
            failures.Add("Table.Row!=null");
        }

        if (TableViewColumn.CellContextFlyoutProperty is null)
        {
            failures.Add("CellDP=null");
        }

        if (TableViewColumn.HeaderContextFlyoutProperty is null)
        {
            failures.Add("HeaderDP=null");
        }

        if (TableView.RowContextFlyoutProperty is null)
        {
            failures.Add("RowDP=null");
        }

        _apiDefaults = failures.Count == 0
            ? "PASS"
            : "FAIL:" + string.Join(",", failures);
    }

    private void Reset()
    {
        Table.CancelEdit();
        _mutateOnFocus = false;
        _nullItemFixture = false;
        _innerMenus = true;
        _nullRealized = "PENDING";
        Table.FlowDirection = FlowDirection.LeftToRight;
        Table.HeadersVisibility = TableViewHeadersVisibility.Column;
        _name.FrozenEdge = TableViewFrozenEdge.None;
        _notes.Width = SampleColumns.Pixels(220);
        _notes.CellTemplate = (DataTemplate)Resources["AppFlyoutCell"];
        Table.Columns.Clear();
        foreach (var column in new TableViewColumn[] { _name, _city, _score, _notes })
        {
            column.Visibility = Visibility.Visible;
            Table.Columns.Add(column);
        }

        Table.RowContextFlyout = null;
        _name.CellContextFlyout = null;
        _name.HeaderContextFlyout = null;
        _city.HeaderContextFlyout = null;
        _opens = 0;
        _openedFixtureIndex = -1;
        _lastOpen = "none";
        ActionResult.Text = "Choose a cell menu action to display its row, column, and value.";
        var nameMenu = Menu("CELL", true);
        var nameAction = new MenuFlyoutItem { Text = "Show first name value" };
        nameAction.SetBinding(MenuFlyoutItem.CommandParameterProperty,
            new Binding { Path = new PropertyPath(nameof(Person.FirstName)) });
        nameAction.Click += OnShowNameValue;
        nameMenu.Items.Add(nameAction);
        nameMenu.Opened += (_, _) => nameAction.IsEnabled = nameAction.DataContext is Person;
        _name.CellContextFlyout = nameMenu;
        _name.HeaderContextFlyout = Menu("HEADER");
        _city.HeaderContextFlyout = Menu("OFFICE HEADER");
        Table.RowContextFlyout = Menu("ROW", true);
        Table.ContextFlyout = Menu("NATIVE");
        Table.ItemsSource = _items;
        Hint.Text = "Right-click First name, Office or a header. Reset keeps direct element menus; navigate away/back to remove them.";
        Report();
    }

    private void OnShowNameValue(object sender, RoutedEventArgs e)
    {
        if (sender is MenuFlyoutItem { DataContext: Person row, CommandParameter: string value })
        {
            ActionResult.Text = $"Binding: row={row.FullName}; column=FirstName; value={value}";
        }
        else
        {
            ActionResult.Text = "No bound FirstName value is available for this row.";
        }
    }

    private void Report() =>
        Status.Text = $"defaults={_apiDefaults}; opens={_opens}; selected={Table.SelectedIndex}; " +
            $"open=[{_lastOpen}]" +
            $"; fixtureIndex={_openedFixtureIndex}" +
            (_nullItemFixture ? $"; realizedNull={_nullRealized}" : "");

    private static IEnumerable<DependencyObject> Descendants(DependencyObject root)
    {
        for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); ++i)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            yield return child;
            foreach (var descendant in Descendants(child)) yield return descendant;
        }
    }

    private void OnDirectRow(object sender, RoutedEventArgs e)
    {
        var row = Descendants(Table).OfType<TableViewRow>().FirstOrDefault();
        if (row is null) { Hint.Text = "FAIL: no realized row"; return; }
        row.ContextFlyout = Menu("DIRECT ROW");
        Hint.Text = "Invoke the first realized row. Navigate away/back afterwards.";
    }

    private void OnDirectHeader(object sender, RoutedEventArgs e)
    {
        var header = Descendants(Table).OfType<Grid>().FirstOrDefault(grid =>
            ReferenceEquals(grid.Tag, _name) &&
            VisualTreeHelper.GetParent(grid) is FrameworkElement { Name: "PART_HeaderHost" });
        if (header is null) { Hint.Text = "FAIL: no First name header"; return; }
        header.ContextFlyout = Menu("DIRECT HEADER");
        Hint.Text = "Invoke the First name header. Navigate away/back afterwards.";
    }

    private static void Retemplate(TableView table)
    {
        var template = table.Template;
        if (template is null) throw new InvalidOperationException("Template unavailable");
        table.Template = null;
        table.ApplyTemplate();
        table.Template = template;
        table.ApplyTemplate();
    }

    private void OnRetemplate(object sender, RoutedEventArgs e)
    {
        if (Table.Template is null) { Hint.Text = "FAIL: template unavailable"; return; }
        Retemplate(Table);
        Hint.Text = "Template replaced; invoke First name again.";
    }

    private void OnMutateOnFocus(object sender, RoutedEventArgs e)
    {
        _mutateOnFocus = true;
        Hint.Text = "Right-click a different row without left-clicking first.";
    }

    private void OnNestedLoaded(object sender, RoutedEventArgs e)
    {
        var inner = (TableView)sender;
        if (inner.Columns.Count != 0) return;
        var column = SampleColumns.Text("Inner", nameof(Person.FirstName), SampleColumns.Pixels(120));
        column.CellContextFlyout = _innerMenus ? Menu("INNER") : null;
        inner.Columns.Add(column);
        inner.ItemsSource = PersonData.Many(2);
    }

    private void OnNested(object sender, RoutedEventArgs e)
    {
        _innerMenus = true;
        _notes.CellTemplate = (DataTemplate)Resources["NestedTableCell"];
        Hint.Text = "Right-click an Inner body cell: INNER opens, not the outer ROW menu.";
    }

    private void OnClearInnerMenus(object sender, RoutedEventArgs e)
    {
        _innerMenus = false;
        foreach (var inner in Descendants(Table).OfType<TableView>())
            foreach (var column in inner.Columns) column.CellContextFlyout = null;
        Hint.Text = "Inner candidates cleared, including future rows. Invoke Inner: native outer fallback is allowed.";
    }

    private void OnNullItem(object sender, RoutedEventArgs e)
    {
        Table.CancelEdit();
        _nullItemFixture = true;
        _nullRealized = "PENDING";
        Table.ItemsSource = new object?[] { null, _items[1] };
        Hint.Text = "First row has a real null item. Invoke its blank First name and Office cells.";
        Report();
    }

    private void OnTableLayoutUpdated(object? sender, object e)
    {
        if (!_nullItemFixture || _nullRealized != "PENDING") return;
        var repeater = Descendants(Table).OfType<ItemsRepeater>()
            .FirstOrDefault(candidate => candidate.Name == "PART_RowsRepeater");
        if (repeater?.TryGetElement(0) is not TableViewRow { IsLoaded: true } row) return;
        _nullRealized = repeater.GetElementIndex(row) == 0 &&
            repeater.ItemsSourceView.GetAt(0) is null && row.DataContext is null ? "PASS" : "FAIL";
        Report();
    }

    private void OnHideHeaders(object sender, RoutedEventArgs e) =>
        Table.HeadersVisibility = TableViewHeadersVisibility.None;

    private void OnShowHeaders(object sender, RoutedEventArgs e) =>
        Table.HeadersVisibility = TableViewHeadersVisibility.Column;

    private void OnRtl(object sender, RoutedEventArgs e)
    {
        Table.FlowDirection = FlowDirection.RightToLeft;
        _name.FrozenEdge = TableViewFrozenEdge.Leading;
        _notes.Width = SampleColumns.Pixels(1800);
        Hint.Text = "Scroll horizontally and compare RTL placement. Current frozen-column layout does not pin columns in RTL.";
    }

    private void OnPlainFlyout(object sender, RoutedEventArgs e)
    {
        var flyout = new Flyout { Content = new TextBlock { Text = "PLAIN FLYOUT" } };
        var style = new Style { TargetType = typeof(FlyoutPresenter) };
        style.Setters.Add(new Setter(AutomationProperties.NameProperty, "PLAIN FLYOUT"));
        flyout.FlyoutPresenterStyle = style;
        flyout.Opened += (_, _) =>
        {
            ++_opens;
            _lastOpen = "PLAIN FLYOUT";
            Report();
        };
        _name.CellContextFlyout = flyout;
        Hint.Text = "Invoke First name: PLAIN FLYOUT opens through the same cell routing.";
    }

    private void OnGroup(object sender, RoutedEventArgs e)
    {
        var source = TableViewSource.From(_items);
        source.GroupBy(new TableViewKeySelector(item => ((Person)item).Office));
        Table.ItemsSource = source;
        Hint.Text = "Invoke a group header: NATIVE. Reset restores ungrouped items.";
    }

    private void OnSelectionMode(object sender, RoutedEventArgs e) =>
        Table.SelectionMode = NoSelection.IsChecked == true
            ? TableViewSelectionMode.None
            : TableViewSelectionMode.Single;

    private void OnClearRow(object sender, RoutedEventArgs e)
    {
        Table.RowContextFlyout = null;
        Report();
    }

    private void OnReset(object sender, RoutedEventArgs e) => Reset();
}
