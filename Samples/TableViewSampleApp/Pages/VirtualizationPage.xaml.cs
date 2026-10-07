// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewRow = Microsoft.UI.Xaml.Controls.Tabular.TableViewRow;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSampleApp.Data;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates row virtualization over TableView.
///
/// The control surfaces no realized-row count, so the readout derives one by walking the
/// visual tree for <see cref="TableViewRow"/> — a public type (TableView.idl:423), not a
/// private template part. That is what makes "Total rows" and "Realized rows" diverge:
/// the whole point of the page.
/// </summary>
public sealed partial class VirtualizationPage : Page
{
    private readonly DispatcherTimer _sampler = new() { Interval = System.TimeSpan.FromMilliseconds(500) };
    private int _peakRealized;
    private TableViewSource? _source;

    // Requested shaping, straight off the pickers.
    private string _shapingMode = "flat";
    private string _groupKey = "Department";

    // Mirror the request only once GroupBy / ClearGroupBy has actually returned, so no
    // readout and no enable/disable guard can claim a grouping the source never took.
    private string _appliedMode = "flat";
    private string _appliedGroupKey = "none";

    public VirtualizationPage()
    {
        InitializeComponent();
        ApplyInMemoryAsActive();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        _sampler.Tick += OnSamplerTick;
    }

    public ObservableCollection<NumberedPerson> People { get; private set; } = new();

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        PeopleTable.SelectionChanged -= OnTableSelectionChanged;
        PeopleTable.SelectionChanged += OnTableSelectionChanged;
        if (People.Count == 0)
        {
            ApplyRowCount(10_000);
        }
        UpdateReadout();
        // Realization changes as the user scrolls and the control raises no event for it,
        // so the readout is sampled. Stopped in Unloaded.
        _sampler.Start();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        _sampler.Stop();
        PeopleTable.SelectionChanged -= OnTableSelectionChanged;
    }

    private void OnSamplerTick(object? sender, object e) => UpdateReadout();

    private void OnTableSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
        => UpdateReadout();

    private void OnRowCountChanged(object sender, SelectionChangedEventArgs e)
    {
        if (SizeSelector?.SelectedItem is ComboBoxItem item
            && int.TryParse(item.Tag?.ToString(), out int n))
        {
            ApplyRowCount(n);
            UpdateReadout();
        }
    }

    // ----- Shaping: Flat / Grouped -----
    //
    // ShapingModeSelector's XAML SelectedIndex raises SelectionChanged during
    // InitializeComponent, before GroupBySelector, the Expand/Collapse buttons and the
    // readouts below it exist; _source may not exist yet either. Every member touched
    // from here is null-guarded; OnPageLoaded re-runs UpdateReadout once all are wired.

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _shapingMode = tag;
        ApplyGrouping();
        UpdateReadout();
    }

    private void OnGroupByChanged(object sender, SelectionChangedEventArgs e)
    {
        if (GroupBySelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyGrouping();
        UpdateReadout();
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        PeopleTable?.ExpandAllGroups();
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        PeopleTable?.CollapseAllGroups();
    }

    /// <summary>
    /// Grouping over a virtualized set. This is the pairing worth watching: group headers
    /// are realized alongside rows, so "Realized rows" should still track the viewport
    /// rather than the group count. Reshapes the existing <see cref="TableViewSource"/>
    /// in place: GroupBy / ClearGroupBy mutate and return the same instance, so the
    /// source is never rebuilt for a shaping change.
    /// </summary>
    private void ApplyGrouping()
    {
        if (_source is null)
        {
            return;
        }

        if (_shapingMode != "grouped")
        {
            _source.ClearGroupBy();
            _appliedMode = "flat";
            _appliedGroupKey = "none";
        }
        else
        {
            var key = _groupKey;
            // The two delegates do NOT receive the same thing: the key selector is handed the
            // row item, while the identity selector is handed the group KEY this selector just
            // returned (TableViewSource.idl). Testing the argument against the row type here
            // would yield an empty identity, which fails fast with E_INVALIDARG.
            _source.GroupBy(
                item => (object)GroupValue(item, key),
                groupKey => groupKey?.ToString() ?? "(none)");

            // Only now is grouping genuinely applied; every readout reads these, never the
            // requested _shapingMode / _groupKey.
            _appliedMode = "grouped";
            _appliedGroupKey = key;
        }

        // case "hierarchy":
        // case "groupedhierarchy":
        //     Hierarchical (tree) rows are not available in this release, which is why the two
        //     matching ComboBoxItems ship disabled with a tooltip rather than hidden. No
        //     hierarchy verb exists on TableViewSource or TableView today — the only trace in
        //     the control source is TableViewRowInfo.h, which reserves row metadata "when
        //     hierarchical (tree) rows land" — so this stub stays prose rather than naming a
        //     member that does not exist. When hierarchy ships, apply it to this same source
        //     here, alongside the GroupBy stage above so grouping and hierarchy compose instead
        //     of replacing one another, and set the applied-mode field only after it returns.

        if (GroupBySelector is not null)
        {
            GroupBySelector.IsEnabled = _appliedMode == "grouped";
        }

        _peakRealized = 0;
    }

    // Never returns the empty string: an empty group identity is an E_INVALIDARG fail-fast.
    private static string GroupValue(object item, string key)
    {
        if (item is not NumberedPerson p)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Department" => p.Department,
            "Role" => p.Role,
            _ => null,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private void ApplyInMemoryAsActive()
    {
        if (PeopleTable != null)
        {
            // SizeSelector's XAML SelectedIndex raises SelectionChanged during
            // InitializeComponent, so ApplyRowCount has usually already built the
            // projection and assigned it. Overwriting it with the raw collection here
            // would orphan _source, and every later GroupBy would mutate a projection the
            // table no longer renders.
            PeopleTable.ItemsSource = (object?)_source ?? People;
        }
        if (SizeSelector != null) SizeSelector.IsEnabled = true;
        if (SourceModeText != null) SourceModeText.Text = "In-memory";
        UpdateReadout();
    }

    private void ApplyRowCount(int count)
    {
        // Build off the bind path first. Adding 50,000 items to a bound ObservableCollection
        // one at a time raises 50,000 CollectionChanged notifications on the UI thread, which
        // would stall the very page that exists to show that scrolling stays responsive.
        var seed = PersonData.All;
        var rows = new List<NumberedPerson>(count);
        for (int i = 0; i < count; i++)
        {
            var p = seed[i % seed.Count];
            rows.Add(new NumberedPerson
            {
                Id = i + 1,
                FirstName = p.FirstName,
                LastName = p.LastName,
                Email = p.Email,
                Department = p.Department,
                Role = p.Role,
            });
        }

        People = new ObservableCollection<NumberedPerson>(rows);
        _peakRealized = 0;
        if (PeopleTable != null)
        {
            // Wrap in a TableViewSource so grouping can be applied without rebuilding the
            // data. Filter/GroupBy mutate the projection in place and return it.
            _source = TableViewSource.From(People);
            ApplyGrouping();
            PeopleTable.ItemsSource = _source;
        }
    }

    /// <summary>
    /// Counts realized row containers. TableViewRow is public API, so this stays off the
    /// control's private template parts. Rows never nest, so recursion stops at each hit.
    /// </summary>
    private static int CountRealizedRows(DependencyObject? root, ref double firstRowHeight)
    {
        if (root is null)
        {
            return 0;
        }

        int realized = 0;
        int children = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < children; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is TableViewRow row)
            {
                realized++;
                if (firstRowHeight <= 0 && row.ActualHeight > 0)
                {
                    firstRowHeight = row.ActualHeight;
                }
            }
            else
            {
                realized += CountRealizedRows(child, ref firstRowHeight);
            }
        }

        return realized;
    }

    private void UpdateReadout()
    {
        int total = People.Count;
        if (TotalRowsText != null) TotalRowsText.Text = total.ToString("N0");

        // PeopleTable can be null while UpdateReadout runs synchronously during
        // InitializeComponent: the Options-rail ComboBoxes set SelectedIndex in
        // XAML, which raises SelectionChanged before the later-declared TableView
        // field is assigned. Guard the table-derived readouts; OnPageLoaded
        // re-runs UpdateReadout once the table is wired.
        var table = PeopleTable;
        if (SelectedRowsText != null) SelectedRowsText.Text = ((table?.SelectedItem is null ? 0 : 1)).ToString("N0");
        if (ColumnCountText != null) ColumnCountText.Text = (table?.Columns.Count ?? 0).ToString();

        double rowHeight = 0;
        int realized = CountRealizedRows(table, ref rowHeight);
        if (realized > _peakRealized) _peakRealized = realized;

        if (RealizedRowsText != null) RealizedRowsText.Text = realized.ToString("N0");
        if (PeakRealizedText != null) PeakRealizedText.Text = _peakRealized.ToString("N0");
        if (RowHeightText != null) RowHeightText.Text = rowHeight > 0 ? $"{rowHeight:F0} px" : "—";

        var grouped = _appliedMode == "grouped";
        if (GroupedByText != null) GroupedByText.Text = grouped ? _appliedGroupKey : "(none)";
        if (ExpandAllButton != null) ExpandAllButton.IsEnabled = grouped;
        if (CollapseAllButton != null) CollapseAllButton.IsEnabled = grouped;

        if (RealizedShareText != null)
        {
            RealizedShareText.Text = total > 0
                ? $"{(double)realized / total:P2} of {total:N0}"
                : "—";
        }
    }
}

public sealed class NumberedPerson
{
    public int Id { get; set; }
    public string FirstName { get; set; } = string.Empty;
    public string LastName { get; set; } = string.Empty;
    public string Email { get; set; } = string.Empty;
    public string Department { get; set; } = string.Empty;
    public string Role { get; set; } = string.Empty;
}
