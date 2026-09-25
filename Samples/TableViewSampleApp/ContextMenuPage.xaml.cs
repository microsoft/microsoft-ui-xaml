using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;

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
    private object? _requestedItem;
    private string _lastRequest = "none";
    private string _lastOpen = "none";
    private string _apiDefaults = "UNKNOWN";

    public ContextMenuPage()
    {
        InitializeComponent();
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
        _lastRequest = "none";
        _lastOpen = "none";
        _name.CellContextFlyout = Menu("CELL", true);
        _name.HeaderContextFlyout = Menu("HEADER");
        _city.HeaderContextFlyout = Menu("CITY HEADER");
        Table.RowContextFlyout = Menu("ROW", true);
        Table.ContextFlyout = Menu("NATIVE");
        Table.ItemsSource = _items;
        Mode.SelectedIndex = 0;
        Report();
    }

    private void OnRequest(TableView sender, TableViewContextFlyoutRequestedEventArgs args)
    {
        ++_requests;
        _requestedItem = args.Item;
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
        }

        Report();
    }

    private void Report() =>
        Status.Text = $"defaults={_apiDefaults}; requests={_requests}; opens={_opens}; selected={Table.SelectedIndex}; " +
            $"request=[{_lastRequest}]; open=[{_lastOpen}]";

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
