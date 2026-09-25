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

namespace TableViewSampleApp;

public sealed partial class ContextMenuPage : Page
{
    private readonly TableViewTextColumn _name =
        SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Pixels(140));
    private readonly TableViewTextColumn _city =
        SampleColumns.Text("City", nameof(Item.City), SampleColumns.Pixels(120));
    private readonly TableViewTextColumn _score =
        SampleColumns.Text("Score", nameof(Item.Score), SampleColumns.Pixels(70));
    private readonly TableViewTemplateColumn _notes = new();
    private readonly List<Item> _items = Data.Make(300);
    private int _requests;
    private int _opens;
    private int _requestedFixtureIndex = -1;
    private object? _requestedItem;
    private string _lastRequest = "none";
    private string _lastOpen = "none";
    private string _apiDefaults = "UNKNOWN";
    private bool _mutateOnFocus;
    private bool _nullItemFixture;
    private bool _innerMenus = true;
    private string _nullRealized = "PENDING";
    private TableViewContextFlyoutRequestedEventArgs? _retainedArgs;

    public ContextMenuPage()
    {
        InitializeComponent();
        // GotFocus is delivered after Focus returns; GettingFocus exercises synchronous invalidation.
        Table.GettingFocus += (_, _) =>
        {
            if (!_mutateOnFocus) return;
            _mutateOnFocus = false;
            Table.ItemsSource = Data.Make(300);
            Hint.Text = "Items replaced during focus. Expect no custom request or open.";
            Report();
        };
        Table.LayoutUpdated += OnTableLayoutUpdated;
        VerifyApiDefaults();
        _name.Binding = new Binding
        {
            Path = new PropertyPath(nameof(Item.Name)),
            Mode = BindingMode.TwoWay,
        };
        _notes.Header = "Notes";
        _notes.Width = SampleColumns.Pixels(220);
        _notes.CellTemplate = (DataTemplate)Resources["AppFlyoutCell"];
        Table.ContextFlyoutRequested += OnRequest;
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
                new Binding { Path = new PropertyPath(nameof(Item.Name)) });
            menu.Items.Add(bound);
            menu.Opened += (_, _) =>
            {
                bool matches = ReferenceEquals(bound.DataContext, _requestedItem);
                _lastOpen = $"{label}; binding={(matches ? "PASS" : "FAIL")}; text={bound.Text}";
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
        _retainedArgs = null;
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
        _requests = 0;
        _opens = 0;
        _requestedItem = null;
        _requestedFixtureIndex = -1;
        _lastRequest = "none";
        _lastOpen = "none";
        _name.CellContextFlyout = Menu("CELL", true);
        _name.HeaderContextFlyout = Menu("HEADER");
        _city.HeaderContextFlyout = Menu("CITY HEADER");
        Table.RowContextFlyout = Menu("ROW", true);
        Table.ContextFlyout = Menu("NATIVE");
        Table.ItemsSource = _items;
        Mode.SelectedIndex = 0;
        Hint.Text = "Right-click Name, City or a header. Reset keeps direct element menus; navigate away/back to remove them.";
        Report();
    }

    private void OnRequest(TableView sender, TableViewContextFlyoutRequestedEventArgs args)
    {
        ++_requests;
        _requestedItem = args.Item;
        _requestedFixtureIndex = args.Item is Item item ? _items.IndexOf(item) : -1;
        _lastRequest = $"{(args.IsHeader ? "header" : "body")}; " +
            $"column={args.Column?.Header ?? "(none)"}; item={(args.Item as Item)?.Name ?? "(none)"}";
        switch (Mode.SelectedIndex)
        {
            case 1:
                args.ContextFlyout = Menu("REPLACEMENT");
                break;
            case 2:
                args.ContextFlyout = null;
                break;
            case 3:
                args.Handled = true;
                break;
            case 4:
                if (args.Column is not null)
                {
                    sender.Columns.Remove(args.Column);
                }
                break;
            case 5:
                if (args.Column is not null)
                {
                    args.Column.Visibility = Visibility.Collapsed;
                }
                break;
            case 6:
                sender.ItemsSource = Data.Make(300);
                break;
            case 7:
                sender.ItemsSource = Data.Make(300);
                args.ContextFlyout = null;
                break;
            case 8:
                Retemplate(sender);
                break;
            case 9:
                _retainedArgs = args;
                Hint.Text = "Args retained. Dismiss the menu, then choose Change retained args.";
                break;
        }

        Report();
    }

    private void Report() =>
        Status.Text = $"defaults={_apiDefaults}; requests={_requests}; opens={_opens}; selected={Table.SelectedIndex}; " +
            $"request=[{_lastRequest}]; open=[{_lastOpen}]" +
            $"; fixtureIndex={_requestedFixtureIndex}" +
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
        if (header is null) { Hint.Text = "FAIL: no Name header"; return; }
        header.ContextFlyout = Menu("DIRECT HEADER");
        Hint.Text = "Invoke the Name header. Navigate away/back afterwards.";
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
        Hint.Text = "Template replaced; invoke Name again.";
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
        var column = SampleColumns.Text("Inner", nameof(Item.Name), SampleColumns.Pixels(120));
        column.CellContextFlyout = _innerMenus ? Menu("INNER") : null;
        inner.Columns.Add(column);
        inner.ItemsSource = Data.Make(2);
    }

    private void OnNested(object sender, RoutedEventArgs e)
    {
        _innerMenus = true;
        _notes.CellTemplate = (DataTemplate)Resources["NestedTableCell"];
        Hint.Text = "Right-click an Inner body cell. Only opens increases; outer requests stays unchanged.";
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
        Hint.Text = "First row has a real null item. Invoke blank Name/City with Keep, Null, Suppress or mutation modes.";
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
        Hint.Text = "Invoke Name with Keep, Null or Suppress to compare plain Flyout routing.";
    }

    private void OnGroup(object sender, RoutedEventArgs e)
    {
        var source = TableViewSource.From(_items);
        source.GroupBy(new TableViewKeySelector(item => ((Item)item).City));
        Table.ItemsSource = source;
        Hint.Text = "Invoke a group header: NATIVE, no custom request. Reset restores ungrouped items.";
    }

    private void OnChangeRetainedArgs(object sender, RoutedEventArgs e)
    {
        if (_retainedArgs is null) { Hint.Text = "FAIL: invoke a menu with Retain args first"; return; }
        _retainedArgs.ContextFlyout = Menu("LATE REPLACEMENT");
        _retainedArgs.Handled = true;
        Hint.Text = "Retained args changed after callback. No new request or open should occur.";
        Report();
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
