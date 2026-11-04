// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Grid lines visibility: TableView.GridLinesVisibility (None / Horizontal / Vertical / All)
/// across text and template cells, against default or custom row banding, flat or grouped.
/// </summary>
public sealed partial class GridLinesVisibilityPage : SamplePageBase
{
    private const int InitialRows = 40;

    private int _nextPersonIndex = InitialRows;

    public GridLinesVisibilityPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));

        // The actions below attach and detach the rows they add and remove themselves.
        TrackLifetime(
            () =>
            {
                foreach (var person in People)
                {
                    person.PropertyChanged += OnPersonChanged;
                }
            },
            () =>
            {
                foreach (var person in People)
                {
                    person.PropertyChanged -= OnPersonChanged;
                }
            });
    }

    // <snippet>
    public ObservableCollection<Person> People { get; } = PersonData.Take(InitialRows);
    // </snippet>

    // <snippet>
    public TableViewSource Source { get; }
    // </snippet>

    // ---- Grid lines and banding ---------------------------------------------------------

    // <snippet>
    private void OnGridLinesChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="3") before the table exists. // snippet:skip
        if (PeopleTable is null || GridLinesText is null) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        PeopleTable.GridLinesVisibility = (string)((ComboBoxItem)((ComboBox)sender).SelectedItem).Tag switch
        {
            "None" => TableViewGridLinesVisibility.None,
            "Horizontal" => TableViewGridLinesVisibility.Horizontal,
            "Vertical" => TableViewGridLinesVisibility.Vertical,
            _ => TableViewGridLinesVisibility.All,
        };
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "GridLinesVisibility -> {0}", PeopleTable.GridLinesVisibility));
    }
    // </snippet>

    // <snippet>
    private void OnRowBandingChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || RowBandingText is null) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        var custom = (string)((ComboBoxItem)((ComboBox)sender).SelectedItem).Tag == "custom";
        PeopleTable.Style = custom ? (Style)Resources["CustomBandingTableViewStyle"] : null;
        SetLastAction(custom
            ? "Row banding -> custom RowBackground / AlternatingRowBackground"
            : "Row banding -> default theme");
    }
    // </snippet>

    // ---- Header lines -------------------------------------------------------------------

    private void OnHeaderLinesChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || HeaderLinesText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(HeaderLinesSelector, "default");
        SetHeaderLinesResources(mode == "brush");
        SetColumnSeparatorsHidden(mode == "style");
        SetLastAction(mode switch
        {
            "brush" => "Header lines -> hidden: column separators and the line under the header row",
            "style" => "Header lines -> column separators hidden; the line under the header row stays",
            _ => "Header lines -> shown",
        });
    }

    // <snippet>
    private void SetColumnSeparatorsHidden(bool hidden)
    {
        PeopleTable.ColumnHeaderStyle = hidden ? (Style)Resources["NoColumnSeparatorsHeaderStyle"] : null;
    }
    // </snippet>

    private const string HeaderSeparatorBrushKey = "TabularSurfaceHeaderSeparatorBrush";

    // Demo plumbing; an app declares the brush in XAML. {ThemeResource} resolves when a template is
    // applied, so a runtime switch re-applies the table's template and then restores the scroll.
    private void SetHeaderLinesResources(bool hidden)
    {
        var overrides = (ResourceDictionary)Resources["HiddenHeaderLinesResources"];
        var tableResources = PeopleTable.Resources;
        if (hidden == tableResources.ContainsKey(HeaderSeparatorBrushKey))
        {
            return;
        }

        foreach (var (key, value) in overrides)
        {
            if (hidden)
            {
                tableResources[key] = value;
            }
            else
            {
                tableResources.Remove(key);
            }
        }

        var scroller = FindBodyScroller(PeopleTable);
        var horizontal = scroller?.HorizontalOffset ?? 0;
        var vertical = scroller?.VerticalOffset ?? 0;
        PeopleTable.Template = null;
        PeopleTable.ClearValue(Control.TemplateProperty);
        PeopleTable.UpdateLayout();
        FindBodyScroller(PeopleTable)?.ChangeView(horizontal, vertical, null, disableAnimation: true);
    }

    private static ScrollViewer? FindBodyScroller(DependencyObject parent)
    {
        for (var i = 0; i < VisualTreeHelper.GetChildrenCount(parent); i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);
            if (child is ScrollViewer { Name: "PART_BodyScroller" } scroller)
            {
                return scroller;
            }

            if (FindBodyScroller(child) is { } found)
            {
                return found;
            }
        }

        return null;
    }

    // ---- In-cell edits ------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || IsBulkUpdating)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.IsActive):
                ReapplyIfGroupedOn(e.PropertyName);
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Active -> {0} for {1}", person.IsActive ? "checked" : "unchecked", person.FullName));
                break;
            case nameof(Person.JoinDate):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Join date -> {0:d} for {1}", person.JoinDate, person.FullName));
                break;
        }
    }

    // ---- Actions ------------------------------------------------------------------------

    private void OnSelectThirdClick(object sender, RoutedEventArgs e)
    {
        if (People.Count < 3)
        {
            SetLastAction("There are fewer than three people; restore the rows first.");
            return;
        }

        var person = People[2];
        SetLastAction(SampleShaping.SelectItem(PeopleTable, person)
            ? string.Format(CultureInfo.CurrentCulture, "Selected {0}", person.FullName)
            : string.Format(CultureInfo.CurrentCulture, "{0} has no displayed row (collapsed group?); expand the groups first.", person.FullName));
    }

    private void OnMoveToNextGroupClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        // Moves along the key the Group key selector names, so the row always changes group
        // when grouped (and the same value changes when flat).
        var key = Shaping.SelectedKey;
        var from = SampleShaping.KeyOf(person, key);
        using (BeginBulkUpdate())
        {
            switch (key)
            {
                case nameof(Person.Office):
                    person.Office = SampleShaping.Next(PersonData.Offices, person.Office);
                    break;
                case nameof(Person.IsActive):
                    person.IsActive = !person.IsActive;
                    break;
                default:
                    person.Department = SampleShaping.Next(PersonData.Departments, person.Department);
                    break;
            }
        }

        // GroupBy takes a delegate, not a property path, so the control cannot re-bucket the row
        // on PropertyChanged; re-apply the grouping when the grouped-on value changed.
        ReapplyIfGroupedOn(key);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, SampleShaping.KeyOf(person, key)));
    }

    private void OnAddPersonClick(object sender, RoutedEventArgs e)
    {
        if (_nextPersonIndex >= PersonData.All.Count)
        {
            SetLastAction("No more people in the sample data.");
            return;
        }

        var person = PersonData.Take(_nextPersonIndex + 1)[_nextPersonIndex];
        _nextPersonIndex++;
        person.PropertyChanged += OnPersonChanged;
        People.Add(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} ({1})", person.FullName, person.Department));
    }

    private void OnRemovePersonClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        person.PropertyChanged -= OnPersonChanged;
        People.Remove(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0}", person.FullName));
    }

    private void OnRestoreRowsClick(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        // The same collection is refilled, so the TableViewSource and its grouping stay in place.
        People.Clear();
        foreach (var person in PersonData.Take(InitialRows))
        {
            person.PropertyChanged += OnPersonChanged;
            People.Add(person);
        }

        _nextPersonIndex = InitialRows;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored the original {0:N0} people", InitialRows));
    }
}
