using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;

namespace TableViewSampleApp;

// TableView has no context-menu API, so an app builds one from ContextRequested. The event bubbles,
// so a single handler on the table serves every cell and header and covers Shift+F10 / the Menu key
// as well as right-click. The Notes column shows the alternative: a ContextFlyout on the cell
// content, which XAML opens and marks handled, so it takes precedence for that column.
public sealed partial class ContextMenuPage : Page
{
    private readonly ObservableCollection<Item> _items = new(Data.Make(60));
    private readonly Dictionary<TableViewColumn, Func<Item, string>> _cellText = new();
    private readonly MenuFlyout _rowMenu = new();
    private readonly MenuFlyout _headerMenu = new();

    // The hit resolved by the last ContextRequested, consumed by the menu item handlers.
    private Item? _item;
    private TableViewColumn? _column;

    public ContextMenuPage()
    {
        this.InitializeComponent();

        AddColumn(SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Pixels(200)), i => i.Name);
        AddColumn(SampleColumns.Text("Role", nameof(Item.Role), SampleColumns.Pixels(120)), i => i.Role);
        AddColumn(SampleColumns.Text("City", nameof(Item.City), SampleColumns.Pixels(120)), i => i.City);
        AddColumn(
            new TableViewTemplateColumn
            {
                Header = "Notes (cell-owned menu)",
                CellTemplate = (DataTemplate)Resources["NotesCell"],
                Width = SampleColumns.Pixels(220),
            },
            i => i.Notes);

        Table.ItemsSource = _items;

        _rowMenu.Items.Add(MenuItem("Copy cell", OnCopyCell));
        _rowMenu.Items.Add(MenuItem("Copy row", OnCopyRow));
        _rowMenu.Items.Add(MenuItem("Delete row", OnDeleteRow));
        _headerMenu.Items.Add(MenuItem("Hide column", OnHideColumn));
        _headerMenu.Items.Add(MenuItem("Show all columns", OnShowAllColumns));

        SetStatus("right-click a cell, a header, or the Notes column");
    }

    private void AddColumn(TableViewColumn column, Func<Item, string> text)
    {
        _cellText[column] = text;
        Table.Columns.Add(column);
    }

    private static MenuFlyoutItem MenuItem(string text, RoutedEventHandler click)
    {
        var item = new MenuFlyoutItem { Text = text };
        item.Click += click;
        return item;
    }

    private void OnTableContextRequested(UIElement sender, ContextRequestedEventArgs args)
    {
        // Every cell wrapper and every header cell carries its column in Tag, which is how the hit
        // resolves to a column with no control API.
        var cell = FindTaggedCell(args.OriginalSource as DependencyObject, out _column);
        if (cell is null || _column is null)
        {
            // A group header, empty space or the scrollbar. Leave the event unhandled.
            SetStatus("right-click outside any cell - no menu");
            return;
        }

        // A cell sits inside a TableViewRow; a header cell does not.
        _item = FindAncestor<TableViewRow>(cell)?.DataContext as Item;
        var menu = _item is null ? _headerMenu : _rowMenu;

        if (_item is not null && Table.SelectionMode != TableViewSelectionMode.None)
        {
            Table.Select(_items.IndexOf(_item));
        }

        // TryGetPosition fails for the keyboard gestures; without this fallback the menu would be
        // reachable by pointer only.
        if (args.TryGetPosition(Table, out Point position))
        {
            menu.ShowAt(Table, new FlyoutShowOptions { Position = position });
        }
        else
        {
            menu.ShowAt(cell, new FlyoutShowOptions { Placement = FlyoutPlacementMode.BottomEdgeAlignedLeft });
        }

        args.Handled = true;
        SetStatus(_item is null ? $"header menu [{Header(_column)}]" : $"row menu {_item.Name} [{Header(_column)}]");
    }

    private void OnCopyCell(object sender, RoutedEventArgs e)
    {
        if (_item is not null && _column is not null && _cellText.TryGetValue(_column, out var text))
        {
            Copy(text(_item));
        }
    }

    private void OnCopyRow(object sender, RoutedEventArgs e)
    {
        if (_item is null) return;

        var item = _item;
        Copy(string.Join("\t", Table.Columns
            .Where(c => c.Visibility == Visibility.Visible && _cellText.ContainsKey(c))
            .Select(c => _cellText[c](item))));
    }

    private void OnDeleteRow(object sender, RoutedEventArgs e)
    {
        if (_item is null) return;

        var name = _item.Name;
        _items.Remove(_item);
        SetStatus($"deleted {name}");
    }

    // Visibility keeps the column in Columns, so "Show all columns" can bring it back.
    private void OnHideColumn(object sender, RoutedEventArgs e)
    {
        if (_column is null) return;

        _column.Visibility = Visibility.Collapsed;
        SetStatus($"hid [{Header(_column)}]");
    }

    private void OnShowAllColumns(object sender, RoutedEventArgs e)
    {
        foreach (var column in Table.Columns)
        {
            column.Visibility = Visibility.Visible;
        }

        SetStatus("all columns visible");
    }

    // The MenuFlyoutItem inherits the cell's DataContext, so the row item comes off the sender.
    private void OnNoteFlag(object sender, RoutedEventArgs e)
    {
        if ((sender as FrameworkElement)?.DataContext is not Item item) return;

        item.Notes = item.Notes.EndsWith(" *", StringComparison.Ordinal) ? item.Notes : item.Notes + " *";
        SetStatus($"cell menu flagged {item.Name}");
    }

    private void OnNoteClear(object sender, RoutedEventArgs e)
    {
        if ((sender as FrameworkElement)?.DataContext is not Item item) return;

        item.Notes = "";
        SetStatus($"cell menu cleared {item.Name}");
    }

    private FrameworkElement? FindTaggedCell(DependencyObject? node, out TableViewColumn? column)
    {
        while (node is not null && node != Table)
        {
            if (node is FrameworkElement element && element.Tag is TableViewColumn tagged)
            {
                column = tagged;
                return element;
            }

            node = VisualTreeHelper.GetParent(node);
        }

        column = null;
        return null;
    }

    private static T? FindAncestor<T>(DependencyObject? node) where T : class
    {
        while (node is not null)
        {
            if (node is T match)
            {
                return match;
            }

            node = VisualTreeHelper.GetParent(node);
        }

        return null;
    }

    private static string Header(TableViewColumn? column) => column?.Header?.ToString() ?? "?";

    private void Copy(string text)
    {
        var package = new DataPackage();
        package.SetText(text);
        Clipboard.SetContent(package);
        SetStatus($"copied \"{text}\"");
    }

    private void SetStatus(string note) => Status.Text = $"{note}   |   rows={_items.Count}";
}
