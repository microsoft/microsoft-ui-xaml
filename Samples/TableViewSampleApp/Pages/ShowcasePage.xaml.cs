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
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSelectionMode = Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
// #44 enum unification: TableViewSortDirection was removed in favor of the shared Data enum.
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
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
    private enum ShowcaseMode
    {
        Flat,
        Grouped,
    }

    // Active preset for the cell-tint converters. They are instantiated by XAML
    // as page resources, so they read this static rather than holding a page
    // back-reference (matches the conditional-styling sample's pattern).
    public static bool Vibrant = true;


    // A few leading rows whose Salary / IsActive the live timer mutates so the
    // bound stoplight + status-chip tints re-run for just those cells — no
    // per-column rebuild, the INotifyPropertyChanged on Person drives it.
    private static readonly int[] s_liveRows = { 0, 1, 2, 3, 4 };

    private readonly ObservableCollection<Person> _people = new();
    private ShowcaseMode _mode = ShowcaseMode.Flat;

    private readonly Random _liveRandom = new();
    private DispatcherTimer? _liveTimer;

    // Re-entrancy guard so the gutter toggle and the Selection-mode / Headers-
    // visibility pickers can update one another without looping.
    private bool _syncingGutter;

    public ShowcasePage()
    {
        // Populate BEFORE InitializeComponent so the declarative IsOn / SelectedIndex
        // callbacks that fire DURING parse see real data. Those handlers still
        // null-guard later-declared fields and bail; the constructor does the
        // authoritative wiring below.
        FillPeople(100);

        InitializeComponent();
        PeopleTable.HeadersVisibility = TableViewHeadersVisibility.Column;
        PeopleTable.AlternatingRowBackground = GetBrushResource("SubtleFillColorSecondaryBrush");

        ApplyMode(ShowcaseMode.Flat);

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
        RefreshGutterToggleState();
    }

    // ----- Source mode (flat / grouped) -----

    private void OnModeRadioChecked(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || sender is not RadioButton { Tag: string tag })
        {
            return;
        }

        if (Enum.TryParse<ShowcaseMode>(tag, out var mode))
        {
            ApplyMode(mode);
        }
    }

    private void ApplyMode(ShowcaseMode mode)
    {
        _mode = mode;
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
            RowCountCombo.IsEnabled = mode == ShowcaseMode.Flat;
        }

        UpdateStatus();
    }

    private void ApplyCurrentSource()
    {
        if (PeopleTable is null)
        {
            return;
        }

        PeopleTable.ItemsSource = _mode switch
        {
            ShowcaseMode.Grouped => ApplyActiveSort(
                TableViewSource.From(_people)),
            _ => ApplyActiveSort(TableViewSource.From(_people)),
        };
    }

    private TableViewSource ApplyActiveSort(TableViewSource source)
    {
        var sortColumn = SampleShape.ActiveSortColumn(PeopleTable);
        if (PeopleTable is null ||
            sortColumn is null ||
            SampleShape.ActiveSortDirection(PeopleTable) == TableViewSortDirection.None ||
            string.IsNullOrEmpty(sortColumn.SortMemberPath))
        {
            return source;
        }

        var path = sortColumn.SortMemberPath;
        return source.Sort(item => SortKey(item, path), SampleShape.ActiveSortDirection(PeopleTable));
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
            ApplyMode(_mode);
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
            RefreshGutterToggleState();
        }
    }

    private void OnSelectAllClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || PeopleTable.SelectionMode == TableViewSelectionMode.None)
        {
            return;
        }
        // Select all is not available: selection is single-item this release.
    }

    private void OnClearSelectionClick(object sender, RoutedEventArgs e) => PeopleTable?.DeselectAll();

    // ----- Selection gutter (row checkboxes) -----

    // There is no IsSelectionGutterVisible DP: the row-checkbox gutter appears purely as a
    // function of SelectionMode (Multiple shows it; Single/None hide it), INDEPENDENT of
    // HeadersVisibility (which controls only the column-header strip). So this toggle just
    // flips SelectionMode between Multiple and Single — it must NOT touch HeadersVisibility,
    // otherwise the column headers would vanish whenever the gutter is hidden.
    private void OnGutterToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || GutterToggle is null || _syncingGutter)
        {
            return;
        }

        _syncingGutter = true;
        try
        {
            if (GutterToggle.IsOn)
            {
                if (PeopleTable.SelectionMode is TableViewSelectionMode.None or TableViewSelectionMode.Single)
                {
                    PeopleTable.SelectionMode = TableViewSelectionMode.Single;
                    SelectComboByContent(SelectionModeCombo, "Multiple");
                }
            }
            else if (PeopleTable.SelectionMode is TableViewSelectionMode.Single)
            {
                PeopleTable.SelectionMode = TableViewSelectionMode.Single;
                SelectComboByContent(SelectionModeCombo, "Single");
            }
        }
        finally
        {
            _syncingGutter = false;
        }

        UpdateSelectionCount();
        UpdateStatus();
    }

    // Reflect the live gutter state (multi-select) on the toggle when the Selection-mode
    // picker changes it. The gutter is purely SelectionMode-driven now.
    private void RefreshGutterToggleState()
    {
        if (PeopleTable is null || GutterToggle is null || _syncingGutter)
        {
            return;
        }

        _syncingGutter = true;
        try
        {
            GutterToggle.IsOn = PeopleTable.SelectionMode is TableViewSelectionMode.Single;
        }
        finally
        {
            _syncingGutter = false;
        }
    }

    private static void SelectComboByContent(ComboBox? combo, string content)
    {
        if (combo is null)
        {
            return;
        }

        for (var i = 0; i < combo.Items.Count; i++)
        {
            if (combo.Items[i] is ComboBoxItem { Content: string c } && c == content)
            {
                combo.SelectedIndex = i;
                return;
            }
        }
    }

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

    private static Brush? GetBrushResource(string key)
    {
        return Application.Current.Resources.TryGetValue(key, out var resource) && resource is Brush brush ? brush : null;
    }

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
            RefreshGutterToggleState();
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

        // There is no AutoSizeAllColumns in this release. Auto-sizing is a width INTENT, so
        // setting each column's Width to Auto is the equivalent - it sizes to the widest
        // realized cell from here on.
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

    private int CountGroups() => _people.Select(person => person.Department).Distinct(StringComparer.Ordinal).Count();

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

        ModeReadoutText.Text = _mode switch
        {
            ShowcaseMode.Grouped => $"Grouped by Department ({CountGroups()} groups)",
            _ => "Flat",
        };

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

    private static string HeaderText(TableViewColumn column) => column.Header?.ToString() ?? "(column)";

    private static string PersonIdentity(object item) => item is Person person ? person.Email : string.Empty;

    private static string GroupIdentity(object key) => key?.ToString() ?? string.Empty;

}
