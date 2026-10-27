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
/// Per-column <c>CellContextFlyout</c> / <c>HeaderContextFlyout</c>, row and group header
/// <c>ContextFlyout</c>s from implicit styles, and <c>TableView.ContextFlyout</c> as the final fallback.
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

    // <snippet>
    public ObservableCollection<Person> People { get; } = PersonData.Take(40);
    // </snippet>

    // <snippet>
    public TableViewSource Source { get; }
    // </snippet>

    // ---- Header menus -------------------------------------------------------------------------

    // <snippet>
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
            _headerMenus[column] = menu; // snippet:skip
        }
    }
    // </snippet>

    // <snippet>
    private void HideColumn(TableViewColumn column)
    {
        column.Visibility = Visibility.Collapsed;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Hid the {0} column", column.Header));
    }
    // </snippet>

    // <snippet>
    private void ShowAllColumns()
    {
        foreach (var column in PeopleTable.Columns)
        {
            column.Visibility = Visibility.Visible;
        }

        SetLastAction("Showed all columns");
    }
    // </snippet>

    // ---- Cell and row menu items ----------------------------------------------------------------

    // <snippet>
    // MenuFlyout has no item filter: cache the XAML items and re-add this row kind's subset on each
    // open. Re-adding (not Visibility) leaves the IsActive bindings in charge.
    private List<MenuFlyoutItemBase>? _rowMenuItems;
    // </snippet>

    // <snippet>
    private void OnRowMenuOpening(object sender, object e)
    {
        var menu = (MenuFlyout)sender;
        _rowMenuItems ??= new List<MenuFlyoutItemBase>(menu.Items);

        // Target is the row, also when the request came from one of its cells. Under Hierarchy, a
        // row with reports is expandable; Manager-tagged items show only on those rows.
        var isManager = menu.Target is TableViewRow { IsExpandable: true };
        menu.Items.Clear();
        foreach (var item in _rowMenuItems)
        {
            if (item.Tag is null || (isManager && (item.Tag as string) == "Manager"))
            {
                menu.Items.Add(item);
            }
        }
    }
    // </snippet>

    // <snippet>
    // A menu item inherits the requested row item as its DataContext.
    private void OnCopyNameClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: Person person })
        {
            CopyText(person.FullName);
        }
    }
    // </snippet>

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

    private void OnEmailTeamClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: Person manager })
        {
            var reports = 0;
            foreach (var person in People)
            {
                if (person.ManagerId == manager.EmployeeId)
                {
                    reports++;
                }
            }

            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Emailed {0}'s team ({1} reports)", manager.FullName, reports));
        }
    }

    private void OnToggleActiveClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: Person person })
        {
            person.IsActive = !person.IsActive;
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} is now {1}", person.FullName, person.IsActive ? "active" : "inactive"));
        }
    }

    // <snippet>
    // A group header menu item's DataContext is the header's TableViewGroupInfo.
    private void OnEmailGroupClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: TableViewGroupInfo group })
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Emailed {0}", group.KeyText));
        }
    }
    // </snippet>

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

    // <snippet>
    // Setting a menu property to null removes that candidate; the request falls through.
    private void OnNameMenuToggled(object sender, RoutedEventArgs e)
    {
        if (!IsLoaded) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        NameColumn.CellContextFlyout = NameMenuToggle.IsOn ? _nameCellMenu : null;
        SetLastAction(NameMenuToggle.IsOn ? "Name cell menu on" : "Name cell menu off (Name cells use the row menu)");
    }
    // </snippet>

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

    private void CopyText(string text)
    {
        var package = new DataPackage();
        package.SetText(text);
        Clipboard.SetContent(package);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Copied \"{0}\"", text));
    }
}
