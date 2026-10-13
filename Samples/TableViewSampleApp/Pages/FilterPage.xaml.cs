// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
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
public sealed partial class FilterPage : Page
{
    // A query no row matches: no name, department, role or email contains it.
    private const string NoMatchQuery = "Astronaut";

    private TableViewSource? _source;          // created ONCE; filtered and reshaped in place
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private bool _isBulkUpdate;
    private bool _refreshQueued;
    private string? _programmaticQuery;        // set when an action types the query itself

    public FilterPage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();
        BuildColumns();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        ApplyFilter();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(60);

    public TableViewSource? Source => _source;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        People.CollectionChanged += OnPeopleCollectionChanged;
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        People.CollectionChanged -= OnPeopleCollectionChanged;
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }
    }

    private void OnPeopleCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        foreach (Person person in e.OldItems ?? Array.Empty<Person>())
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        foreach (Person person in e.NewItems ?? Array.Empty<Person>())
        {
            person.PropertyChanged += OnPersonChanged;
        }
    }

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
        if (_source is null)
        {
            return;
        }

        var query = Query;
        var activeOnly = ActiveOnly;
        if (query.Length > 0 || activeOnly)
        {
            _source.Filter(item => Matches((Person)item, query, activeOnly));
        }
        else
        {
            _source.ClearFilter();
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
        if (sender is not Person person || _isBulkUpdate || _refreshQueued
            || e.PropertyName is not (nameof(Person.Role) or nameof(Person.IsActive)))
        {
            return;
        }

        var property = e.PropertyName;
        _refreshQueued = true;
        DispatcherQueue.TryEnqueue(() =>
        {
            _refreshQueued = false;
            if (!IsLoaded)
            {
                return;
            }

            ReapplyIfGroupedOn(property);
            ApplyFilter();
            SetLastAction(string.Format(
                CultureInfo.CurrentCulture,
                Matches(person, Query, ActiveOnly) ? "Edited {0} for {1}; the row still matches" : "Edited {0} for {1}; the row left the filtered view",
                property == nameof(Person.IsActive) ? "Active" : property,
                person.FullName));
        });
    }

    private void OnTableSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (!SampleShaping.IsReselecting)
        {
            RefreshReadouts();
        }
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
        _isBulkUpdate = true;
        try
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
        finally
        {
            _isBulkUpdate = false;
        }

        // Any of Active, Department or Role may be the grouped-on value.
        ReapplyIfGroupedOn(_appliedKey);
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

        var key = _appliedMode == "grouped" ? _appliedKey : SampleShaping.SelectedTag(GroupKeySelector, "Department");
        var from = SampleShaping.KeyOf(person, key);
        _isBulkUpdate = true;
        try
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
        finally
        {
            _isBulkUpdate = false;
        }

        // GroupBy's key selector and the filter predicate are delegates, evaluated when they are
        // applied, so apply both again; ApplyShaping re-selects the moved row.
        ReapplyIfGroupedOn(key);
        ApplyFilter();
        SampleShaping.Reselect(FilterTable, person, People.Count + 64);
        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            Matches(person, Query, ActiveOnly) ? "Moved {0} from {1} to {2}" : "Moved {0} from {1} to {2}; the row no longer matches the filter",
            person.FullName,
            from,
            SampleShaping.KeyOf(person, key)));
    }

    // ---- Readouts ---------------------------------------------------------------------------

    private void RefreshReadouts()
    {
        if (FilterTable is null || FilterStateText is null || SelectedItemText is null || RowsText is null)
        {
            return;
        }

        var query = Query;
        var activeOnly = ActiveOnly;
        FilterStateText.Text = !HasFilter
            ? "(none)"
            : query.Length == 0
                ? "Active only"
                : string.Format(CultureInfo.CurrentCulture, "Query \u201C{0}\u201D{1}", query, activeOnly ? " + Active only" : string.Empty);

        // The projection's row count is not exposed, so count the matches app-side.
        var matched = People.Count(p => Matches(p, query, activeOnly));
        RowsText.Text = HasFilter
            ? string.Format(CultureInfo.CurrentCulture, "Showing {0:N0} of {1:N0}", matched, People.Count)
            : SampleShaping.RowCountText(People.Count);

        SelectedItemText.Text = FilterTable.SelectedItem is Person person ? person.FullName : "(none)";
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard EVERY element this path touches.
        if (_source is null || FilterTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Department");
        var selected = FilterTable.SelectedItem;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                // Applied to the same source as the filter, so only matching rows are grouped.
                _source.GroupBy(item => SampleShaping.KeyOf(item as Person, key), SampleShaping.GroupIdentity);
                break;
            // case "hierarchy":
            // case "groupedHierarchy":
            //     Hierarchical (tree) rows are not available in this release, so the two matching
            //     ComboBoxItems ship disabled. TableViewSource and TableView have no hierarchy
            //     member today. When hierarchy ships, apply it to this same source here, composed
            //     with the Filter and GroupBy stages rather than replacing them, and set
            //     _appliedMode only after the call returns.
            default:
                _source.ClearGroupBy();
                mode = "flat";
                break;
        }

        _appliedMode = mode;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(FilterTable, selected, People.Count + 64, RefreshReadouts);
        UpdateShapingGating();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
        else
        {
            RefreshReadouts();
        }
    }

    // Call after ANY write to the grouped-on property: from an action or from an in-cell edit.
    private void ReapplyIfGroupedOn(string? propertyName)
    {
        if (_appliedMode == "grouped" && propertyName == _appliedKey)
        {
            ApplyShaping(announce: false);
        }
    }

    private void UpdateShapingGating()
    {
        var grouped = _appliedMode == "grouped";
        GroupKeySelector.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ShapingModeText.Text = SampleShaping.ShapingText(grouped, GroupKeySelector);
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        FilterTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        FilterTable.CollapseAllGroups();
        SetLastAction("Collapsed all groups");
    }

    // The only writer of LastActionText.
    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }

        RefreshReadouts();
    }

    #endregion
}
