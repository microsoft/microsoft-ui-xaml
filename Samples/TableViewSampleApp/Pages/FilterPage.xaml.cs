// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSource = Microsoft.UI.Xaml.Controls.Tabular.TableViewSource;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Filtering: ONE live <see cref="TableViewSource"/> whose <c>Filter(predicate)</c> narrows the
/// rows the control renders (and <c>ClearFilter()</c> restores them) without mutating the
/// collection. Grouping is applied to that same source, so a filtered, grouped view is a single
/// projection. The columns are built in code from the shared Person cell templates.
/// </summary>
public sealed partial class FilterPage : SamplePageBase
{
    // A query no row matches: no name, department, role or email contains it.
    private const string NoMatchQuery = "Astronaut";

    private bool _refreshQueued;
    private string? _programmaticQuery;        // set when an action types the query itself

    public FilterPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; filtered and reshaped in place
        InitializeComponent();
        BuildColumns();
        ApplyFilter();
        // </snippet>
        InitializeSample(Status, Shaping.Attach(FilterTable, Source));
        TrackItems(People, OnPersonChanged);
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(60);

    public TableViewSource Source { get; }

    // <snippet>
    // ---- Columns, built in code from the shared templates -------------------------------------

    private void BuildColumns()
    {
        var resources = Application.Current.Resources;

        FilterTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Photo",
            CellTemplate = (DataTemplate)resources["AvatarTemplate"],
            CanSort = false,
            Width = Token("ColWidthAvatar"),
            HeaderToolTip = "PersonPicture: the photo when the person has one, otherwise their initials.",
        });
        FilterTable.Columns.Add(new TableViewTextColumn
        {
            Header = "Name",
            Binding = new Binding { Path = new PropertyPath(nameof(Person.FullName)) },
            Width = new GridLength(1, GridUnitType.Star),
            MinWidth = (double)resources["ColMinWidthStar"],
        });
        // A template column sorts by SortMemberPath, the value behind the template.
        FilterTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Department",
            CellTemplate = (DataTemplate)resources["DepartmentChipTemplate"],
            SortMemberPath = nameof(Person.Department),
            Width = Token("ColWidthChip"),
            HeaderToolTip = "Read-only chip. The query matches on it.",
        });
        FilterTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Role",
            CellTemplate = (DataTemplate)resources["RoleTextBoxTemplate"],
            SortMemberPath = nameof(Person.Role),
            Width = Token("ColWidthLongName"),
            HeaderToolTip = "Editable free text. The query matches on it, so retyping a role can push the row out of the filtered view.",
        });
        FilterTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Active",
            CellTemplate = (DataTemplate)resources["ActiveCheckBoxTemplate"],
            SortMemberPath = nameof(Person.IsActive),
            Width = Token("ColWidthBoolean"),
            HeaderToolTip = "Editable CheckBox. Clearing it while Active only is on removes the row from the filtered view.",
        });
    }

    // Width tokens live in App.xaml as doubles; a column width is a GridLength in pixels.
    private static GridLength Token(string key) => new((double)Application.Current.Resources[key]);

    // ---- Filtering --------------------------------------------------------------------------

    private string Query => SearchBox?.Text?.Trim() ?? string.Empty;

    private bool ActiveOnly => ActiveOnlyToggle?.IsOn == true;

    private bool HasFilter => Query.Length > 0 || ActiveOnly;

    private void OnSearchChanged(object sender, TextChangedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        // TextChanged is raised after the Text setter returns. When an action set the text, it
        // has already applied the filter and reported it.
        if (_programmaticQuery is not null && _programmaticQuery == SearchBox.Text)
        {
            _programmaticQuery = null;
            return;
        }

        ApplyFilter();
        SetLastAction(Query.Length > 0
            ? string.Format(CultureInfo.CurrentCulture, "Filter(query \u201C{0}\u201D)", Query)
            : "Query cleared");
    }

    private void OnActiveToggled(object sender, RoutedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        ApplyFilter();
        SetLastAction(ActiveOnly ? "Active only -> On" : "Active only -> Off");
    }

    /// <summary>
    /// Hands the combined predicate to the one source, or clears it. The ItemsSource is never
    /// reassigned, so selection, scroll offset and group state survive.
    /// </summary>
    private void ApplyFilter()
    {
        var query = Query;
        var activeOnly = ActiveOnly;
        if (query.Length > 0 || activeOnly)
        {
            Source.Filter(item => Matches((Person)item, query, activeOnly));
        }
        else
        {
            Source.ClearFilter();
        }

        RefreshReadouts();
    }

    private static bool Matches(Person person, string query, bool activeOnly)
    {
        if (activeOnly && !person.IsActive)
        {
            return false;
        }

        return query.Length == 0
            || Contains(person.FullName, query)
            || Contains(person.Department, query)
            || Contains(person.Role, query)
            || Contains(person.Email, query);
    }

    private static bool Contains(string? value, string query) =>
        value is not null && value.Contains(query, StringComparison.CurrentCultureIgnoreCase);

    /// <summary>
    /// An in-cell edit changes the Person but does not re-run the predicate (or the group key
    /// selector), so apply both again once the edit has settled.
    /// </summary>
    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || IsBulkUpdating || _refreshQueued
            || e.PropertyName is not (nameof(Person.Role) or nameof(Person.IsActive)))
        {
            return;
        }

        var property = e.PropertyName;
        _refreshQueued = true;
        EnqueueIfLoaded(() =>
        {
            _refreshQueued = false;
            ReapplyIfGroupedOn(property);
            ApplyFilter();
            SetLastAction(string.Format(
                CultureInfo.CurrentCulture,
                Matches(person, Query, ActiveOnly) ? "Edited {0} for {1}; the row still matches" : "Edited {0} for {1}; the row left the filtered view",
                property == nameof(Person.IsActive) ? "Active" : property,
                person.FullName));
        });
    }
    // </snippet>

    private void OnTableSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        RefreshReadouts();
    }

    // ---- Actions ----------------------------------------------------------------------------

    private void OnNoMatchClick(object sender, RoutedEventArgs e)
    {
        _programmaticQuery = SearchBox.Text == NoMatchQuery ? null : NoMatchQuery;
        SearchBox.Text = NoMatchQuery;
        ApplyFilter();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filter(query \u201C{0}\u201D): no row matches, so the EmptyTemplate shows", NoMatchQuery));
    }

    private void OnClearFilterClick(object sender, RoutedEventArgs e)
    {
        _programmaticQuery = SearchBox.Text.Length > 0 ? string.Empty : null;
        SearchBox.Text = string.Empty;
        ActiveOnlyToggle.IsOn = false;
        ApplyFilter();
        SetLastAction("ClearFilter(): every row is back");
        SearchBox.Focus(FocusState.Programmatic);

        var peer = FrameworkElementAutomationPeer.FromElement(SearchBox)
            ?? FrameworkElementAutomationPeer.CreatePeerForElement(SearchBox);
        peer?.RaiseNotificationEvent(
            AutomationNotificationKind.ActionCompleted,
            AutomationNotificationProcessing.MostRecent,
            "Filter cleared",
            "FilterCleared");
    }

    /// <summary>
    /// Changes a real value on the selected row so it stops matching: Active when Active only is
    /// on, otherwise the department or role the query matched. A person matched by name or email
    /// keeps matching, and the readout says so.
    /// </summary>
    private void OnMutateOutClick(object sender, RoutedEventArgs e)
    {
        if (FilterTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        if (!HasFilter)
        {
            SetLastAction("No filter is active, so every row matches. Type a query or turn on Active only first.");
            return;
        }

        var query = Query;
        var activeOnly = ActiveOnly;
        string change;
        using (BeginBulkUpdate())
        {
            if (activeOnly && person.IsActive)
            {
                person.IsActive = false;
                change = "cleared Active";
            }
            else if (PersonData.Departments.FirstOrDefault(d => !Contains(d, query)) is { } department
                && Contains(person.Department, query))
            {
                person.Department = department;
                change = string.Format(CultureInfo.CurrentCulture, "moved to {0}", department);
                if (Matches(person, query, activeOnly) && PersonData.Roles.FirstOrDefault(r => !Contains(r, query)) is { } role)
                {
                    person.Role = role;
                    change += string.Format(CultureInfo.CurrentCulture, " as {0}", role);
                }
            }
            else if (PersonData.Roles.FirstOrDefault(r => !Contains(r, query)) is { } role)
            {
                person.Role = role;
                change = string.Format(CultureInfo.CurrentCulture, "role changed to {0}", role);
            }
            else
            {
                change = "unchanged";
            }
        }

        // Any of Active, Department or Role may be the grouped-on value.
        ReapplyIfGroupedOn(AppliedGroupKey);
        ApplyFilter();
        SetLastAction(Matches(person, query, activeOnly)
            ? string.Format(CultureInfo.CurrentCulture, "{0} still matches by name or email ({1})", person.FullName, change)
            : string.Format(CultureInfo.CurrentCulture, "{0}: {1}, so the row left the filtered view", person.FullName, change));
    }

    private void OnMoveGroupClick(object sender, RoutedEventArgs e)
    {
        if (FilterTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var key = IsGrouped ? AppliedGroupKey : Shaping.SelectedKey;
        var from = SampleShaping.KeyOf(person, key);
        using (BeginBulkUpdate())
        {
            switch (key)
            {
                case nameof(Person.Role):
                    person.Role = SampleShaping.Next(PersonData.Roles, person.Role);
                    break;
                case nameof(Person.IsActive):
                    person.IsActive = !person.IsActive;
                    break;
                default:
                    person.Department = SampleShaping.Next(PersonData.Departments, person.Department);
                    break;
            }
        }

        // GroupBy's key selector and the filter predicate are delegates evaluated when applied, so re-apply both.
        ReapplyIfGroupedOn(key);
        ApplyFilter();
        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            Matches(person, Query, ActiveOnly) ? "Moved {0} from {1} to {2}" : "Moved {0} from {1} to {2}; the row no longer matches the filter",
            person.FullName,
            from,
            SampleShaping.KeyOf(person, key)));
    }
}
