// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
using TableViewSource = Microsoft.UI.Xaml.Controls.Tabular.TableViewSource;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates the aligned TableView filtering surface: ONE live
/// <see cref="TableViewSource"/> whose <c>Filter(predicate)</c> narrows the rows the
/// control renders (and <c>ClearFilter()</c> restores them) without mutating the source
/// collection. Grouping is applied to that same source, so a filtered, grouped view is a
/// single projection rather than two passes.
/// </summary>
public sealed partial class FilterPage : Page
{
    private const string NoMatchQuery = "zzz-no-such-row";

    private readonly ObservableCollection<Person> _people;

    // One source for the lifetime of the page. Filter / ClearFilter / GroupBy / ClearGroupBy
    // mutate and return this same instance (TableViewSource.cpp:49-50), so the projection is
    // reshaped in place. Constructing a new TableViewSource and reassigning ItemsSource per
    // keystroke — which this page used to do — discards selection, scroll offset and any group
    // state on every character typed.
    private TableViewSource? _source;

    // Requested vs applied. Every readout and every enable/disable guard reads the *applied*
    // fields, which are assigned only after the shaping call returns.
    private string _shapeMode = "flat";
    private string _appliedShapeMode = "flat";
    private string _groupKey = "Department";
    private string _appliedGroupKey = "none";

    private string _lastAction = "(not exercised yet)";
    private bool _refreshQueued;

    public FilterPage()
    {
        _people = new ObservableCollection<Person>(PersonData.Take(60));
        foreach (var person in _people)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        InitializeComponent();
        BuildColumns();

        _source = TableViewSource.From(_people);
        FilterTable.ItemsSource = _source;

        ApplyShaping();
    }

    private void BuildColumns()
    {
        FilterTable.Columns.Add(Col("First name", nameof(Person.FirstName), new GridLength(1, GridUnitType.Auto)));
        FilterTable.Columns.Add(Col("Last name", nameof(Person.LastName), new GridLength(1, GridUnitType.Auto)));
        // Templated cell: a rounded chip whose tint and dot come from cached, page-private
        // converters (no per-Convert brush allocation). SortMemberPath keeps the header
        // sortable even though the cell is a template rather than text.
        FilterTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Department",
            CellTemplate = (DataTemplate)Resources["FilterDepartmentChipTemplate"],
            SortMemberPath = nameof(Person.Department),
            CanSort = true,
            Width = new GridLength(1, GridUnitType.Auto),
        });
        FilterTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Role",
            CellTemplate = (DataTemplate)Resources["FilterRoleEditorTemplate"],
            SortMemberPath = nameof(Person.Role),
            CanSort = true,
            HeaderToolTip = "Editable free text. The filter matches on it, so retyping a role can push the row out of the filtered view.",
            Width = new GridLength(1, GridUnitType.Star),
        });
        FilterTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Active",
            CellTemplate = (DataTemplate)Resources["FilterActiveTemplate"],
            SortMemberPath = nameof(Person.IsActive),
            CanSort = true,
            HeaderToolTip = "Whether this person is a currently active employee. Clearing it while \u201CActive only\u201D is on removes the row from the filtered view.",
            Width = new GridLength(70),
        });
        FilterTable.Columns.Add(Col("Email", nameof(Person.Email), new GridLength(2, GridUnitType.Star)));
    }

    private static TableViewTextColumn Col(string header, string path, GridLength width) =>
        new()
        {
            Header = header,
            Binding = new Binding { Path = new PropertyPath(path) },
            Width = width,
        };

    // ---- Filtering --------------------------------------------------------------------------

    private void OnSearchChanged(object sender, TextChangedEventArgs e)
    {
        if (FilterTable is null) return;
        ApplyFilter();
    }

    private void OnActiveToggled(object sender, RoutedEventArgs e)
    {
        if (FilterTable is null) return;
        ApplyFilter();
    }

    private void OnNoMatchClick(object sender, RoutedEventArgs e)
    {
        SearchBox.Text = NoMatchQuery;
        _lastAction = "Applied a query no row matches — the EmptyTemplate should be showing.";
        ApplyFilter();
    }

    private void OnClearFilterClicked(object sender, RoutedEventArgs e)
    {
        SearchBox.Text = string.Empty;
        ActiveOnlyToggle.IsOn = false;
        _lastAction = "ClearFilter() — every row is back.";
        ApplyFilter();
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
    /// Mutates the selected row so it stops matching the active predicate and drops out of the
    /// projection — the live-reshaping counterpart to retyping the query.
    /// </summary>
    private void OnMutateOutClick(object sender, RoutedEventArgs e)
    {
        if (FilterTable.SelectedItem is not Person selected || !HasFilter)
        {
            return;
        }

        string name = $"{selected.FirstName} {selected.LastName}";

        if (ActiveOnlyToggle.IsOn && selected.IsActive)
        {
            selected.IsActive = false;
        }
        else
        {
            selected.FirstName = "Zzz";
            selected.LastName = "Filtered-out";
            selected.Department = "Operations";
            selected.Role = "No longer matching";
            selected.Email = "filtered.out@contoso.com";
        }

        // Re-run the predicate over the mutated collection so the projection catches up.
        ApplyFilter();
        _lastAction = string.Format(
            CultureInfo.InvariantCulture,
            "Mutated {0} so it no longer matches; it has left the filtered view.",
            name);
        RefreshReadout();
    }

    private bool HasFilter => (SearchBox?.Text?.Trim().Length ?? 0) > 0 || ActiveOnlyToggle?.IsOn == true;

    /// <summary>
    /// An in-cell edit mutates the Person but does not by itself re-evaluate the predicate, so
    /// re-apply the filter once the edit has settled. The row then leaves — or rejoins — the
    /// projection without the source ever being rebuilt.
    /// </summary>
    private void OnPersonChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (_refreshQueued)
        {
            return;
        }

        _refreshQueued = true;
        DispatcherQueue.TryEnqueue(() =>
        {
            _refreshQueued = false;
            // Re-apply grouping as well as the filter: an edit to the group-key property must
            // re-bucket the row, and GroupBy does not observe PropertyChanged on its own.
            if (_appliedGroupKey != "none")
            {
                ApplyShaping(announce: false);
            }
            ApplyFilter();
        });
    }

    private void ApplyFilter()
    {
        if (_source is null) return;

        string query = SearchBox?.Text?.Trim() ?? string.Empty;
        bool activeOnly = ActiveOnlyToggle?.IsOn == true;
        bool filtered = query.Length > 0 || activeOnly;

        // Reshape in place. Filter(predicate) narrows the projection; ClearFilter() removes it.
        // The ItemsSource is never reassigned, so selection, scroll offset and group state all
        // survive.
        if (filtered)
        {
            _source.Filter(item => Match((Person)item, query, activeOnly));
        }
        else
        {
            _source.ClearFilter();
        }

        // This re-runs the predicate app-side purely to print "Showing N of M": it duplicates
        // work the projection has already done. A TableViewSource.Count (or a projected row
        // count on the control) would let the readout come straight off the projection and make
        // this second pass unnecessary.
        int matched = _people.Count(p => Match(p, query, activeOnly));

        ReadoutText.Text = filtered
            ? string.Format(CultureInfo.InvariantCulture, "Showing {0} of {1} rows (filtered)", matched, _people.Count)
            : string.Format(CultureInfo.InvariantCulture, "Showing all {0} rows", _people.Count);

        RefreshReadout();
    }

    private static bool Match(Person person, string query, bool activeOnly)
    {
        if (activeOnly && !person.IsActive)
        {
            return false;
        }

        if (query.Length == 0)
        {
            return true;
        }

        return Contains(person.FirstName, query)
            || Contains(person.LastName, query)
            || Contains(person.Department, query)
            || Contains(person.Role, query)
            || Contains(person.Email, query);
    }

    private static bool Contains(string? value, string query) =>
        value is not null && value.Contains(query, StringComparison.OrdinalIgnoreCase);

    // ---- Shaping (Flat / Grouped; Hierarchy modes are present but unavailable) ---------------

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FilterTable is null || ShapingModeSelector?.SelectedItem is not ComboBoxItem item) return;
        _shapeMode = item.Tag as string ?? "flat";
        ApplyShaping();
    }

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FilterTable is null || GroupKeyCombo?.SelectedItem is not ComboBoxItem item) return;
        _groupKey = item.Tag as string ?? "Department";
        ApplyShaping();
    }

    private void ApplyShaping(bool announce = true)
    {
        if (_source is null || FilterTable is null) return;

        // Flat mode pins the key to "none" so the applied key can never claim a grouping the
        // projection does not have.
        var requestedKey = _shapeMode == "grouped" ? _groupKey : "none";
        bool wasGrouped = _appliedGroupKey != "none";

        switch (_shapeMode)
        {
            case "grouped":
            {
                var key = requestedKey;
                // The two delegates receive DIFFERENT things despite both parameters being named
                // `item` (TableViewSource.idl:12-16):
                //   TableViewKeySelector(Object item)      -> receives the ROW ITEM
                //   TableViewIdentitySelector(Object item) -> receives the GROUP KEY
                // An item-typed identity lambda returns the empty string, which is an
                // unresolvable identity: GroupBy fails fast with E_INVALIDARG and grouping
                // silently never applies. Keep the identity selector key-based.
                //
                // Note this is applied to the SAME source the filter is applied to, so the
                // filtered, grouped view is one projection and empty groups simply do not
                // appear.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");
                break;
            }

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release, which is why the two
            //     matching ComboBoxItems ship IsEnabled="False" with a tooltip. No call is
            //     written here on purpose: as of this commit TableViewSource.idl has no
            //     hierarchy verb and TableView.idl has no hierarchy property, so any code here
            //     would be naming a member that does not exist. The intended shape, per
            //     TableViewRowInfo.h:27 ("when hierarchical (tree) rows land"), is to apply the
            //     hierarchy stage to THIS SAME _source alongside the Filter and GroupBy stages
            //     so all three compose into one projection rather than three passes. When that
            //     ships, write the real call here and drop IsEnabled="False" from the two items.
            //     break;

            default:
                _source.ClearGroupBy();
                break;
        }

        // Applied state is committed only once the shaping call has returned.
        _appliedGroupKey = requestedKey;
        _appliedShapeMode = requestedKey == "none" ? "flat" : "grouped";
        if (announce)
        {
            _lastAction = requestedKey == "none"
                ? "ClearGroupBy() — the projection is flat again."
                : string.Format(CultureInfo.InvariantCulture, "GroupBy({0}) applied on the same filtered source.", GroupLabel(requestedKey));
        }

        if (_appliedGroupKey != "none" && !wasGrouped)
        {
            DispatcherQueue.TryEnqueue(() =>
            {
                if (_appliedGroupKey != "none")
                {
                    FilterTable.ExpandAllGroups();
                }
            });
        }

        RefreshReadout();
    }

    /// <summary>
    /// Group key for a row item. Never returns <see cref="string.Empty"/>: the empty string is
    /// not a usable group identity and makes the projection fail fast.
    /// </summary>
    private static string GroupValue(object item, string key)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        string? raw = key switch
        {
            "Role" => person.Role,
            "Active" => person.IsActive ? "Active" : "Inactive",
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(raw) ? "(none)" : raw;
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedGroupKey == "none") return;
        FilterTable.ExpandAllGroups();
        _lastAction = "ExpandAllGroups() — every group header is now expanded.";
        RefreshReadout();
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedGroupKey == "none") return;
        FilterTable.CollapseAllGroups();
        _lastAction = "CollapseAllGroups() — only group headers remain visible.";
        RefreshReadout();
    }

    /// <summary>
    /// Mutates the selected row's group-key property while grouping is applied, so the row has
    /// to leave one group and join another inside the already-filtered projection.
    /// </summary>
    private void OnMoveSelectedGroupClick(object sender, RoutedEventArgs e)
    {
        if (_appliedGroupKey == "none" || FilterTable.SelectedItem is not Person selected)
        {
            return;
        }

        string from = GroupValue(selected, _appliedGroupKey);

        switch (_appliedGroupKey)
        {
            case "Active":
                selected.IsActive = !selected.IsActive;
                break;
            case "Role":
                selected.Role = from == "Group mover" ? "Principal Engineer" : "Group mover";
                break;
            default:
                var departments = PersonData.Departments;
                int next = (departments.ToList().IndexOf(selected.Department) + 1) % departments.Count;
                selected.Department = departments[next];
                break;
        }

        string to = GroupValue(selected, _appliedGroupKey);

        // The row may also have fallen in or out of the active filter. Re-apply the grouping
        // stage too: GroupBy's key selector is evaluated when the projection is built and does
        // not observe PropertyChanged, so without this the row keeps its old bucket.
        ApplyShaping(announce: false);
        ApplyFilter();

        _lastAction = string.Format(
            CultureInfo.InvariantCulture,
            "Moved {0} {1} from \u201C{2}\u201D to \u201C{3}\u201D.",
            selected.FirstName,
            selected.LastName,
            from,
            to);
        RefreshReadout();
    }

    private void OnTableSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args) => RefreshReadout();

    private void RefreshReadout()
    {
        if (FilterTable is null || ShapeModeText is null) return;

        bool grouped = _appliedGroupKey != "none";

        ShapeModeText.Text = grouped
            ? string.Format(CultureInfo.InvariantCulture, "Grouped by {0}", GroupLabel(_appliedGroupKey))
            : "Flat";

        GroupedByText.Text = grouped
            ? string.Format(
                CultureInfo.InvariantCulture,
                "{0} — {1} groups in the filtered set",
                GroupLabel(_appliedGroupKey),
                VisibleGroupCount(_appliedGroupKey))
            : "(not grouped)";

        FilterStateText.Text = HasFilter
            ? string.Format(
                CultureInfo.InvariantCulture,
                "query \u201C{0}\u201D{1}",
                SearchBox?.Text?.Trim(),
                ActiveOnlyToggle?.IsOn == true ? " + active only" : string.Empty)
            : "(none)";

        SelectedItemText.Text = FilterTable.SelectedItem is Person person
            ? $"{person.FirstName} {person.LastName}"
            : "(none)";

        LastActionText.Text = _lastAction;

        GroupKeyCombo.IsEnabled = _appliedShapeMode == "grouped";
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ClearFilterButton.IsEnabled = HasFilter;
        DropOutButton.IsEnabled = HasFilter && FilterTable.SelectedItem is not null;
        MoveGroupButton.IsEnabled = grouped && FilterTable.SelectedItem is not null;
    }

    /// <summary>
    /// Groups the reader can actually see: the group count over the rows that currently pass
    /// the filter, which is what makes "empty groups disappear" observable.
    /// </summary>
    private int VisibleGroupCount(string key)
    {
        string query = SearchBox?.Text?.Trim() ?? string.Empty;
        bool activeOnly = ActiveOnlyToggle?.IsOn == true;

        return _people
            .Where(p => Match(p, query, activeOnly))
            .Select(p => GroupValue(p, key))
            .Distinct(StringComparer.Ordinal)
            .Count();
    }

    private static string GroupLabel(string key) => key switch
    {
        "Role" => "Role",
        "Active" => "Active status",
        "Department" => "Department",
        _ => "(none)",
    };
}

// Page-private chip converters. Deliberately NOT the shared Pages/ShowcaseConverters.cs set:
// those allocate a new SolidColorBrush on every Convert call, which under virtualization is a
// per-cell allocation on every realization, and they have no HighContrast path. Everything
// here is cached in static readonly state and built exactly once.
internal static class FilterChipPalette
{
    // Read once. In HighContrast the chip drops its tint entirely and relies on the theme
    // foreground plus the chip's border for shape.
    internal static readonly bool HighContrast = new Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;

    internal static readonly SolidColorBrush TransparentBrush = new(Colors.Transparent);

    private static readonly IReadOnlyDictionary<string, SolidColorBrush> s_tints = Build(0x40);
    private static readonly IReadOnlyDictionary<string, SolidColorBrush> s_dots = Build(0xFF);
    private static readonly SolidColorBrush s_fallbackTint = new(ColorHelper.FromArgb(0x40, 0x64, 0x74, 0x8B));
    private static readonly SolidColorBrush s_fallbackDot = new(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B));

    private static IReadOnlyDictionary<string, SolidColorBrush> Build(byte alpha) =>
        new Dictionary<string, SolidColorBrush>(StringComparer.Ordinal)
        {
            ["Engineering"] = new(ColorHelper.FromArgb(alpha, 0x00, 0x78, 0xD4)),
            ["Sales"] = new(ColorHelper.FromArgb(alpha, 0x14, 0xB8, 0xA6)),
            ["Marketing"] = new(ColorHelper.FromArgb(alpha, 0xA8, 0x55, 0xF7)),
            ["HR"] = new(ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B)),
            ["Operations"] = new(ColorHelper.FromArgb(alpha, 0xEF, 0x44, 0x44)),
            ["Design"] = new(ColorHelper.FromArgb(alpha, 0xEC, 0x48, 0x99)),
            ["Product"] = new(ColorHelper.FromArgb(alpha, 0x0E, 0xA5, 0xE9)),
            ["Finance"] = new(ColorHelper.FromArgb(alpha, 0x22, 0xC5, 0x5E)),
        };

    internal static SolidColorBrush Tint(object? value) =>
        HighContrast ? TransparentBrush : Lookup(s_tints, value, s_fallbackTint);

    internal static SolidColorBrush Dot(object? value) =>
        HighContrast ? TransparentBrush : Lookup(s_dots, value, s_fallbackDot);

    private static SolidColorBrush Lookup(IReadOnlyDictionary<string, SolidColorBrush> map, object? value, SolidColorBrush fallback) =>
        value is string key && map.TryGetValue(key, out var brush) ? brush : fallback;
}

/// <summary>Chip background for the templated Department cell. Never allocates.</summary>
public sealed partial class FilterChipTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        FilterChipPalette.Tint(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

/// <summary>Decorative chip dot for the templated Department cell. Never allocates.</summary>
public sealed partial class FilterChipDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        FilterChipPalette.Dot(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

/// <summary>
/// Builds a per-row accessible name for an interactive cell from the row's identity, so a
/// column of sixty checkboxes or text boxes does not announce the same string sixty times.
/// The format comes from ConverterParameter, e.g. "Role for {0}".
/// </summary>
public sealed partial class FilterRowCellNameConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        var rowName = value?.ToString();
        if (string.IsNullOrWhiteSpace(rowName))
        {
            rowName = "row";
        }

        return parameter is string format && format.Length > 0
            ? string.Format(CultureInfo.CurrentCulture, format, rowName)
            : rowName;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}
