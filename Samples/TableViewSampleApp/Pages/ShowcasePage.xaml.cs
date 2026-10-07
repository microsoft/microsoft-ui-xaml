// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSelectionMode = Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
// #44 enum unification: TableViewSortDirection was removed in favor of the shared Data enum.
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Navigation;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Flagship composite page that exercises the aligned TableView feature set
/// with TableViewSource-backed flat and grouped modes.
/// </summary>
public sealed partial class ShowcasePage : Page
{
    // Active preset for the cell-tint converters. They are instantiated by XAML
    // as page resources, so they read this static rather than holding a page
    // back-reference (matches the conditional-styling sample's pattern).
    public static bool Vibrant = true;


    // A few leading rows whose Salary / IsActive the live timer mutates so the
    // bound stoplight + status-chip tints re-run for just those cells — no
    // per-column rebuild, the INotifyPropertyChanged on Person drives it.
    private static readonly int[] s_liveRows = { 0, 1, 2, 3, 4 };

    private readonly ObservableCollection<Person> _people = new();

    // Reshaped IN PLACE: GroupBy / ClearGroupBy / Sort mutate and return the same TableViewSource
    // (TableViewSource.cpp:49-50). Rebuilding the source per change - which this page used to do -
    // drops selection, scroll offset and group expansion on every toggle.
    private TableViewSource? _source;
    private string _mode = "flat";          // requested
    private string _appliedMode = "flat";   // applied - every readout and guard reads THIS
    private string _groupKey = "Department";

    private readonly Random _liveRandom = new();
    private DispatcherTimer? _liveTimer;

    public ShowcasePage()
    {
        // Populate BEFORE InitializeComponent so the declarative IsOn / SelectedIndex
        // callbacks that fire DURING parse see real data. Those handlers still
        // null-guard later-declared fields and bail; the constructor does the
        // authoritative wiring below.
        FillPeople(100);

        InitializeComponent();
        PeopleTable.HeadersVisibility = TableViewHeadersVisibility.Column;

        ApplyMode();

        Vibrant = VibrantToggle?.IsOn ?? true;
        UpdateStatus();

        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
    }

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        // Live updates ship ON: the declarative IsOn="True" fired OnLiveToggled
        // during parse before the table existed (so it bailed). Start the loop
        // here, once the table is realized. StartLiveUpdates is idempotent.
        if (LiveToggle?.IsOn == true)
        {
            StartLiveUpdates();
        }
        UpdateStatus();
        UpdateStatus();
    }

    // ----- Source shaping (flat / grouped) -----

    private void OnModeSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null ||
            ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _mode = tag;
        ApplyMode();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "Shaping mode -> {0}", _appliedMode == "grouped" ? "Grouped" : "Flat"));
    }

    private void OnShapingGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || ShapingGroupKeyCombo?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyMode();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "Group key -> {0}", GroupKeyLabel(tag)));
    }

    private void ApplyMode()
    {
        if (PeopleTable is null)
        {
            return;
        }

        if (FirstNameColumn is not null)
        {
            FirstNameColumn.Header = "First name";
            FirstNameColumn.CellTemplate = (DataTemplate)Resources["ShowcaseNameTemplate"];
        }

        foreach (var column in CanonicalOrder())
        {
            if (column is not null)
            {
                column.CanSort = true;
            }
        }

        ApplyCurrentSource();

        if (RowCountCombo is not null)
        {
            RowCountCombo.IsEnabled = _appliedMode != "grouped";
        }

        UpdateStatus();
    }

    private void ApplyCurrentSource()
    {
        if (PeopleTable is null)
        {
            return;
        }

        if (_source is null)
        {
            _source = TableViewSource.From(_people);
            PeopleTable.ItemsSource = _source;
        }

        switch (_mode)
        {
            case "grouped":
                var key = _groupKey;
                // The two delegates receive DIFFERENT things despite both parameters being named
                // `item`: TableViewKeySelector gets the ROW ITEM, TableViewIdentitySelector gets
                // the GROUP KEY (TableViewSource.idl:12-16). The old item-typed identity lambda
                // here could return the empty string, which is an E_INVALIDARG fail-fast.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");
                break;

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release, and no hierarchy API exists
            //     on TableViewSource or TableView yet, so there is deliberately no call written
            //     here to copy. When the control ships hierarchy support, apply it to this same
            //     source instance alongside the GroupBy stage above so the two compose, and drop
            //     the IsEnabled="False" from the matching options in the XAML.
            //     break;

            default:
                _source.ClearGroupBy();
                break;
        }

        // Set ONLY after the shaping call returns: a readout driven by the REQUESTED mode lies
        // about the table whenever GroupBy throws.
        _appliedMode = _mode;

        ApplyActiveSort(_source);
    }

    // Never returns string.Empty: an empty group identity is an E_INVALIDARG fail-fast.
    private static string GroupValue(object item, string key)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Role" => person.Role,
            "Active" => person.IsActive ? "Active" : "Inactive",
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private static string GroupKeyLabel(string key) => key switch
    {
        "Role" => "Role",
        "Active" => "Active status",
        _ => "Department",
    };

    private void ApplyActiveSort(TableViewSource source)
    {
        // Clear first: the keySelector Sort overload declares an ANONYMOUS axis, so re-declaring
        // one without clearing would stack a second axis rather than replace the first.
        source.ClearSort();

        var sortColumn = SampleShape.ActiveSortColumn(PeopleTable);
        if (PeopleTable is null ||
            sortColumn is null ||
            SampleShape.ActiveSortDirection(PeopleTable) == TableViewSortDirection.None ||
            string.IsNullOrEmpty(sortColumn.SortMemberPath))
        {
            return;
        }

        var path = sortColumn.SortMemberPath;
        source.Sort(item => SortKey(item, path), SampleShape.ActiveSortDirection(PeopleTable));
    }

    // ----- Actions -----

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        PeopleTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        PeopleTable.CollapseAllGroups();
        SetLastAction("Collapsed all groups");
    }

    private void OnMoveSelectedGroupClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable?.SelectedItem is not Person selected)
        {
            return;
        }

        var before = GroupValue(selected, _groupKey);
        switch (_groupKey)
        {
            case "Role":
                selected.Role = NextInRing(_people.Select(p => p.Role).Distinct(StringComparer.Ordinal).OrderBy(r => r, StringComparer.Ordinal).ToList(), selected.Role);
                break;
            case "Active":
                selected.IsActive = !selected.IsActive;
                break;
            default:
                selected.Department = NextInRing(PersonData.Departments, selected.Department);
                break;
        }

        // Re-running the shaping stage re-reads every key, so the mutated row re-buckets.
        ApplyCurrentSource();
        UpdateStatus();
        SetLastAction(string.Format(
            CultureInfo.InvariantCulture,
            "{0} {1}: {2} -> {3}",
            selected.FirstName,
            selected.LastName,
            before,
            GroupValue(selected, _groupKey)));
    }

    private void OnBumpSalaryClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable?.SelectedItem is not Person selected)
        {
            return;
        }

        var before = selected.Salary;
        selected.Salary = Math.Round(before * 2, 0);

        // Re-applying the sort axis re-reads the key, so a row sorted on Salary re-positions.
        ApplyCurrentSource();
        UpdateStatus();
        SetLastAction(string.Format(
            CultureInfo.InvariantCulture,
            "{0} {1} salary {2:N0} -> {3:N0}",
            selected.FirstName,
            selected.LastName,
            before,
            selected.Salary));
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable?.SelectedItem is not Person selected)
        {
            return;
        }

        var group = GroupValue(selected, _groupKey);
        if (_people.Remove(selected))
        {
            ApplyCurrentSource();
            UpdateStatus();
            SetLastAction(string.Format(CultureInfo.InvariantCulture, "Removed {0} {1} from {2}", selected.FirstName, selected.LastName, group));
        }
    }

    private static string NextInRing(IReadOnlyList<string> ring, string current)
    {
        if (ring.Count == 0)
        {
            return current;
        }

        var index = -1;
        for (var i = 0; i < ring.Count; i++)
        {
            if (string.Equals(ring[i], current, StringComparison.Ordinal))
            {
                index = i;
                break;
            }
        }

        return ring[(index + 1 + ring.Count) % ring.Count];
    }

    private void SetLastAction(string text)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = text;
        }
    }

    private static object? SortKey(object item, string path) => item switch
    {
        Person p => path switch
        {
            "FirstName" => p.FirstName,
            "LastName" => p.LastName,
            "Email" => p.Email,
            "Department" => p.Department,
            "Role" => p.Role,
            "JoinDate" => p.JoinDate,
            "Salary" => p.Salary,
            "IsActive" => p.IsActive,
            _ => null,
        },
        _ => null,
    };

    private void OnRowCountChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null)
        {
            return;
        }

        if (RowCountCombo?.SelectedItem is ComboBoxItem { Tag: string tag } &&
            int.TryParse(tag, NumberStyles.Integer, CultureInfo.InvariantCulture, out var count))
        {
            FillPeople(count);
            ApplyMode();
        }
    }

    private void FillPeople(int count)
    {
        _people.Clear();
        foreach (var person in PersonData.Take(count))
        {
            _people.Add(person);
        }
    }

    // ----- Selection -----

    private void OnSelectionModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null)
        {
            return;
        }

        if (SelectionModeCombo?.SelectedItem is ComboBoxItem item &&
            item.Content is string name &&
            Enum.TryParse<TableViewSelectionMode>(name, out var mode))
        {
            PeopleTable.SelectionMode = mode;
            UpdateSelectionCount();
            UpdateStatus();
        }
    }

    private void OnSelectFirstClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || PeopleTable.SelectionMode == TableViewSelectionMode.None)
        {
            return;
        }

        PeopleTable.Select(0);
    }

    private void OnClearSelectionClick(object sender, RoutedEventArgs e) => PeopleTable?.DeselectAll();

    private static void SelectComboByTag(ComboBox? combo, string tag)
    {
        if (combo is null)
        {
            return;
        }

        for (var i = 0; i < combo.Items.Count; i++)
        {
            if (combo.Items[i] is ComboBoxItem { Tag: string t } && t == tag)
            {
                combo.SelectedIndex = i;
                return;
            }
        }
    }

    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        UpdateSelectionCount();
        UpdateStatus();
    }

    private void UpdateSelectionCount()
    {
        if (PeopleTable is null || SelectionCountText is null)
        {
            return;
        }

        var count = (PeopleTable.SelectedItem is null ? 0 : 1);
        SelectionCountText.Text = count switch
        {
            0 => "No rows selected",
            1 => "1 row selected",
            _ => $"{count} rows selected",
        };
    }

    // ----- Columns: headers visibility, show / hide, auto-size -----

    private void OnHeadersVisibilityChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null)
        {
            return;
        }

        if (HeadersVisibilityCombo?.SelectedItem is ComboBoxItem { Tag: string tag } &&
            Enum.TryParse<TableViewHeadersVisibility>(tag, out var visibility))
        {
            PeopleTable.HeadersVisibility = visibility;
            UpdateStatus();
        }
    }

    private void OnEmailColumnToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || EmailColumn is null || EmailColumnToggle is null)
        {
            return;
        }

        SetColumnVisible(EmailColumn, EmailColumnToggle.IsOn);
        UpdateStatus();
    }

    private void OnRoleColumnToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || RoleColumn is null || RoleColumnToggle is null)
        {
            return;
        }

        SetColumnVisible(RoleColumn, RoleColumnToggle.IsOn);
        UpdateStatus();
    }

    private void OnAutosizeAllClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null)
        {
            return;
        }

        // Width carries sizing INTENT (TableView.idl:123-126). Auto tracks the widest
        // *realized* cell continuously and is shrink-capable, so widths re-derive on every
        // measure pass as rows scroll in and out - it is a persistent mode, not a one-shot
        // fit-to-content command. Prefer Pixel/Star for large virtualized grids.
        foreach (var column in PeopleTable.Columns)
        {
            column.Width = new GridLength(1, GridUnitType.Auto);
        }
    }

    // The canonical left-to-right column order. Used to re-insert a toggled
    // column at a sensible slot (before the first still-present successor) so a
    // show / hide preserves the current order of the other columns.
    private TableViewColumn[] CanonicalOrder() => new TableViewColumn[]
    {
        FirstNameColumn,
        DepartmentColumn,
        ActiveColumn,
        SalaryColumn,
        LastNameColumn,
        EmailColumn,
        RoleColumn,
        JoinDateColumn,
    };

    private void SetColumnVisible(TableViewColumn column, bool show)
    {
        if (PeopleTable is null || column is null)
        {
            return;
        }

        var present = PeopleTable.Columns.IndexOf(column) >= 0;
        if (show && !present)
        {
            var order = CanonicalOrder();
            var canonicalIndex = Array.IndexOf(order, column);
            var insertAt = PeopleTable.Columns.Count;
            for (var i = canonicalIndex + 1; i < order.Length; i++)
            {
                var idx = PeopleTable.Columns.IndexOf(order[i]);
                if (idx >= 0)
                {
                    insertAt = idx;
                    break;
                }
            }

            PeopleTable.Columns.Insert(insertAt, column);
        }
        else if (!show && present)
        {
            var idx = PeopleTable.Columns.IndexOf(column);
            if (idx >= 0)
            {
                PeopleTable.Columns.RemoveAt(idx);
            }
        }
    }

    // ----- Sort -----

    private void OnPeopleTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        ApplyCurrentSource();
        UpdateStatus();
    }

    // Column reordering is not part of this release.

    // ----- Vibrant cell tints -----

    private void OnVibrantToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || VibrantToggle is null)
        {
            return;
        }

        Vibrant = VibrantToggle.IsOn;
        ReevaluateTintedColumns();
        UpdateStatus();
    }

    private void ReevaluateTintedColumns()
    {
        // The tint converters are pure functions of (value, Vibrant). Toggling
        // Vibrant touches no bound value, so already-realized cells won't re-run
        // on their own. Clearing + restoring the CellTemplate re-realizes just the
        // tinted columns once — a per-click cost, never per-tick.
        ReassignCellTemplate(DepartmentColumn);
        ReassignCellTemplate(ActiveColumn);
        ReassignCellTemplate(SalaryColumn);
    }

    private static void ReassignCellTemplate(TableViewTemplateColumn? column)
    {
        if (column is null)
        {
            return;
        }

        var template = column.CellTemplate;
        column.CellTemplate = null;
        column.CellTemplate = template;
    }

    // ----- Live updates -----

    private void OnLiveToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || LiveToggle is null)
        {
            return;
        }

        if (LiveToggle.IsOn)
        {
            StartLiveUpdates();
        }
        else
        {
            StopLiveUpdates();
        }

        UpdateStatus();
    }

    private void StartLiveUpdates()
    {
        StopLiveUpdates();
        _liveTimer = new DispatcherTimer
        {
            Interval = TimeSpan.FromMilliseconds(UpdateIntervalSlider?.Value ?? 700),
        };
        _liveTimer.Tick += OnLiveTimerTick;
        _liveTimer.Start();
    }

    private void StopLiveUpdates()
    {
        if (_liveTimer is null)
        {
            return;
        }

        _liveTimer.Stop();
        _liveTimer.Tick -= OnLiveTimerTick;
        _liveTimer = null;
    }

    private void OnLiveTimerTick(object? sender, object e)
    {
        var targets = GetLiveTargets();
        if (targets.Count == 0)
        {
            return;
        }

        // Keep the cadence responsive to the slider between ticks.
        if (_liveTimer is not null && UpdateIntervalSlider is not null)
        {
            _liveTimer.Interval = TimeSpan.FromMilliseconds(UpdateIntervalSlider.Value);
        }

        foreach (var i in s_liveRows)
        {
            if (i >= targets.Count)
            {
                continue;
            }

            var row = targets[i];

            // Nudge Salary within the dataset band; flip Active occasionally.
            // INPC on Person re-runs just these cells' bound tint converters — no per-column rebuild.
            var delta = (_liveRandom.NextDouble() - 0.5) * 12_000;
            var next = Math.Clamp(GetSalary(row) + delta, 110_000, 230_000);
            SetSalary(row, Math.Round(next / 100.0) * 100.0);

            if (_liveRandom.NextDouble() < 0.15)
            {
                ToggleActive(row);
            }
        }

        UpdateStatus();
    }

    // The rows whose tints the live timer animates.
    private IReadOnlyList<object> GetLiveTargets() => _people.Cast<object>().ToList();

    private int CountGroups()
    {
        var key = _groupKey;
        return _people.Select(person => GroupValue(person, key)).Distinct(StringComparer.Ordinal).Count();
    }

    private static double GetSalary(object row) => row is Person p ? p.Salary : 0;

    private static void SetSalary(object row, double value)
    {
        if (row is Person p)
        {
            p.Salary = value;
        }
    }

    private static void ToggleActive(object row)
    {
        if (row is Person p)
        {
            p.IsActive = !p.IsActive;
        }
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e) => StopLiveUpdates();

    protected override void OnNavigatedFrom(NavigationEventArgs e)
    {
        StopLiveUpdates();
        base.OnNavigatedFrom(e);
    }

    // ----- Status readout -----

    private void UpdateStatus()
    {
        // Bail until the readout TextBlocks are inflated (declarative callbacks
        // fire mid-parse before the Options pane exists).
        if (ModeReadoutText is null || RowsReadoutText is null || SelectionReadoutText is null ||
            SortReadoutText is null || ColumnsReadoutText is null)
        {
            return;
        }

        var grouped = _appliedMode == "grouped";
        var hasSelection = PeopleTable?.SelectedItem is Person;

        SetActionState(ExpandAllButton, grouped, "Expand every group.");
        SetActionState(CollapseAllButton, grouped, "Collapse every group.");
        SetActionState(MoveGroupButton, hasSelection, "Rewrites the selected row's group key, then re-applies GroupBy so the row moves between groups.", "Select a row first.");
        SetActionState(BumpSalaryButton, hasSelection, "Doubles Salary, then re-applies the sort axis - sort by Salary first to watch the row re-position.", "Select a row first.");
        SetActionState(RemoveRowButton, hasSelection, "Removes the row; its group disappears when it was the last one.", "Select a row first.");

        if (ShapingGroupKeyCombo is not null)
        {
            ShapingGroupKeyCombo.IsEnabled = grouped;
            ToolTipService.SetToolTip(ShapingGroupKeyCombo, grouped
                ? "Change the GroupBy key while grouped - expansion and selection survive."
                : "Available once Grouped mode is selected.");
        }

        // Readouts describe the APPLIED mode, never the requested one.
        ModeReadoutText.Text = grouped
            ? $"Grouped by {GroupKeyLabel(_groupKey)} ({CountGroups()} groups)"
            : "Flat";

        var rowCount = _people.Count;
        RowsReadoutText.Text = rowCount.ToString(CultureInfo.InvariantCulture);

        var selectionMode = PeopleTable?.SelectionMode ?? TableViewSelectionMode.Single;
        var selectionCount = (PeopleTable?.SelectedItem is null ? 0 : 1);
        SelectionReadoutText.Text = $"{selectionCount} ({selectionMode})";

        var sortColumn = SampleShape.ActiveSortColumn(PeopleTable);
        SortReadoutText.Text = sortColumn is null || SampleShape.ActiveSortDirection(PeopleTable) == TableViewSortDirection.None
            ? "(none)"
            : $"{HeaderText(sortColumn)} ({SampleShape.ActiveSortDirection(PeopleTable)})";


        if (PeopleTable is not null)
        {
            var visible = CanonicalOrder()
                .Where(c => c is not null && PeopleTable.Columns.IndexOf(c) >= 0)
                .Select(HeaderText);
            ColumnsReadoutText.Text = string.Join(", ", visible);
        }
    }

    private static void SetActionState(Button? button, bool enabled, string enabledTip, string disabledTip = "Available once Grouped mode is selected.")
    {
        if (button is null)
        {
            return;
        }

        button.IsEnabled = enabled;
        ToolTipService.SetToolTip(button, enabled ? enabledTip : disabledTip);
    }

    private static string HeaderText(TableViewColumn column) => column.Header?.ToString() ?? "(column)";

    private static string PersonIdentity(object item) => item is Person person ? person.Email : string.Empty;

}
