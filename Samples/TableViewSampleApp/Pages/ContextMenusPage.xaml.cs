// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using Windows.ApplicationModel.DataTransfer;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableView's context menu properties:
/// <list type="bullet">
///   <item><description><c>TableViewColumn.CellContextFlyout</c> and <c>HeaderContextFlyout</c>:
///     per-column menus for body cells and the column header.</description></item>
///   <item><description>The row's own <c>ContextFlyout</c>, set through an implicit
///     <c>TableViewRow</c> style: the fallback for every body cell without its own cell menu.</description></item>
///   <item><description>The inherited <c>TableView.ContextFlyout</c>: the final fallback.</description></item>
/// </list>
/// The menus are not stamped on cells: a menu item's DataContext is the requested row item.
/// </summary>
public sealed partial class ContextMenusPage : SamplePageBase
{
    private readonly Dictionary<TableViewColumn, FlyoutBase> _headerMenus = new();
    private FlyoutBase? _nameCellMenu;
    private FlyoutBase? _rowMenu;
    private Style? _rowMenuStyle;

    public ContextMenusPage()
    {
        Source = TableViewSource.From(People);
        InitializeComponent();

        _nameCellMenu = NameColumn.CellContextFlyout;
        _rowMenuStyle = (Style)PeopleTable.Resources[typeof(TableViewRow)];
        _rowMenu = (FlyoutBase)((Setter)_rowMenuStyle.Setters[0]).Value;
        AttachHeaderMenus();
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource Source { get; }

    // <snippet>
    // ---- Header menus -------------------------------------------------------------------------

    // A header menu item has no row item to bind to, so each column gets its own menu and the
    // handler closes over that column.
    private void AttachHeaderMenus()
    {
        foreach (var column in new TableViewColumn[] { NameColumn, DepartmentColumn, OfficeColumn, SalaryColumn })
        {
            var hide = new MenuFlyoutItem
            {
                Text = string.Format(CultureInfo.CurrentCulture, "Hide {0}", column.Header),
                Icon = new FontIcon { Glyph = "\uED1A" },
            };
            hide.Click += (_, _) => HideColumn(column);

            var showAll = new MenuFlyoutItem { Text = "Show all columns", Icon = new FontIcon { Glyph = "\uE7B3" } };
            showAll.Click += (_, _) => ShowAllColumns();

            var menu = new MenuFlyout();
            menu.Items.Add(hide);
            menu.Items.Add(showAll);
            column.HeaderContextFlyout = menu;
            _headerMenus[column] = menu;
        }
    }

    private void HideColumn(TableViewColumn column)
    {
        column.Visibility = Visibility.Collapsed;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Hid the {0} column", column.Header));
    }

    private void ShowAllColumns()
    {
        foreach (var column in PeopleTable.Columns)
        {
            column.Visibility = Visibility.Visible;
        }

        SetLastAction("Showed all columns");
    }

    // ---- Cell and row menu items ----------------------------------------------------------------

    // A menu item inherits the requested row item as its DataContext.
    private void OnCopyNameClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: Person person })
        {
            CopyText(person.FullName);
        }
    }

    private void OnCopyEmailClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: Person person })
        {
            CopyText(person.Email);
        }
    }

    private void OnCopyRowClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: Person person })
        {
            CopyText(string.Join('\t', person.FullName, person.Department, person.Office,
                person.Salary.ToString("C0", CultureInfo.CurrentCulture)));
        }
    }

    private void OnNextDepartmentClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: Person person })
        {
            var from = person.Department;
            person.Department = SampleShaping.Next(PersonData.Departments, from);
            ReapplyIfGroupedOn(nameof(Person.Department));
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, person.Department));
        }
    }

    private void OnRemoveClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: Person person } && People.Remove(person))
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0}", person.FullName));
        }
    }

    private void OnRestoreListClick(object sender, RoutedEventArgs e)
    {
        People.Clear();
        foreach (var person in PersonData.Take(40))
        {
            People.Add(person);
        }

        SetLastAction("Restored the list");
    }

    // ---- Options ------------------------------------------------------------------------------

    // Setting a menu property to null removes that candidate; the request falls through.
    private void OnNameMenuToggled(object sender, RoutedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        NameColumn.CellContextFlyout = NameMenuToggle.IsOn ? _nameCellMenu : null;
        SetLastAction(NameMenuToggle.IsOn ? "Name cell menu on" : "Name cell menu off (Name cells use the row menu)");
    }

    // The row menu lives in an implicit row style. Removing or adding an implicit style updates
    // the realized rows on the next layout pass; recycled rows pick it up when they are reused.
    private void OnRowMenuToggled(object sender, RoutedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        PeopleTable.Resources.Remove(typeof(TableViewRow));
        if (RowMenuToggle.IsOn)
        {
            PeopleTable.Resources[typeof(TableViewRow)] = _rowMenuStyle;
        }

        SetLastAction(RowMenuToggle.IsOn ? "Row menu on" : "Row menu off (cells fall through to ContextFlyout)");
    }

    private void OnHeaderMenuToggled(object sender, RoutedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        foreach (var (column, menu) in _headerMenus)
        {
            column.HeaderContextFlyout = HeaderMenuToggle.IsOn ? menu : null;
        }

        SetLastAction(HeaderMenuToggle.IsOn ? "Header menus on" : "Header menus off (headers fall through to ContextFlyout)");
    }
    // </snippet>

    private void CopyText(string text)
    {
        var package = new DataPackage();
        package.SetText(text);
        Clipboard.SetContent(package);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Copied \"{0}\"", text));
    }
}
