// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// One ObservableCollection<Employee> feeds two TableViewSources; each declares its own relation with
// ParentBy, so the same rows form two different trees. Filter, Sort and GroupBy are stages on the
// same sources and compose with the hierarchy.
public sealed partial class HierarchyPage : SamplePageBase
{
    private const int PerfCount = 100_000;
    private const int PerfRuns = 3;
    private const string RenamePrefix = "zz ";

    private readonly ObservableCollection<Employee> _employees = new(HierarchyData.Generate(200));
    private readonly TableViewSource _managerSource;
    private readonly TableViewSource _mentorSource;
    private int _nextId;
    private bool _ready;   // guards handlers that fire while InitializeComponent applies initial values
    private bool _perfRunning;
    private TableView? _activeTable;   // the table the user selected in last

    public HierarchyPage()
    {
        InitializeComponent();

        // <snippet>
        BuildColumns(ManagerTable);
        BuildColumns(MentorTable);

        _managerSource = TableViewSource.From(_employees);
        _mentorSource = TableViewSource.From(_employees);
        ManagerTable.ItemsSource = _managerSource;
        MentorTable.ItemsSource = _mentorSource;
        ManagerTable.GroupHeaderTemplate = (DataTemplate)Resources["GroupHeader"];
        MentorTable.GroupHeaderTemplate = (DataTemplate)Resources["GroupHeader"];

        DeclareManagerRelation(_managerSource);
        DeclareMentorRelation(_mentorSource);
        // </snippet>
        ApplyLive();

        _nextId = _employees.Max(e => e.Id) + 1;
        _ready = true;
        InitializeSample(Status);
    }

    // <snippet>
    private static void BuildColumns(TableView table)
    {
        // The chevron and the indent are drawn over the first column; nothing about these column
        // declarations is hierarchy-aware.
        table.Columns.Add(SampleColumns.Text("Name", nameof(Employee.Name), SampleColumns.Star(2)));
        table.Columns.Add(SampleColumns.Text("Title", nameof(Employee.Title), SampleColumns.Star()));
        table.Columns.Add(SampleColumns.Text("Dept", nameof(Employee.Dept), SampleColumns.Pixels(100)));
        table.Columns.Add(SampleColumns.Text("Id", nameof(Employee.Id), SampleColumns.Pixels(60)));
    }

    // ---- Relations ----

    // A null parent key makes a root; a key with no matching Id makes an orphan, which is also a root.
    private static void DeclareManagerRelation(TableViewSource source) =>
        source.ParentBy(
            new TableViewKeySelector(item => ((Employee)item).Id),
            new TableViewKeySelector(item => ((Employee)item).ManagerId!));

    private static void DeclareMentorRelation(TableViewSource source) =>
        source.ParentBy(
            new TableViewKeySelector(item => ((Employee)item).Id),
            new TableViewKeySelector(item => ((Employee)item).MentorId!));
    // </snippet>

    private IEnumerable<TableViewSource> Sources => new[] { _managerSource, _mentorSource };

    // <snippet>
    // ---- Shaping (applied to both sources) ----

    private void ApplyFilter()
    {
        var text = FilterBox.Text?.Trim() ?? string.Empty;
        foreach (var source in Sources)
        {
            if (text.Length == 0)
            {
                source.ClearFilter();
            }
            else
            {
                // Matches NODES; ancestors of a match are kept as ordinary rows and expanded while the
                // filter is active.
                source.Filter(new TableViewPredicate(
                    item => ((Employee)item).Name.Contains(text, StringComparison.OrdinalIgnoreCase)));
            }
        }
    }

    private void ApplySort()
    {
        foreach (var source in Sources)
        {
            // A key-selector sort is an anonymous axis: a new Sort(keySelector) is added behind the
            // previous one instead of replacing it, so clear the old axis first.
            source.ClearSort();
            switch (SortCombo.SelectedIndex)
            {
                case 1:
                    source.Sort(new TableViewKeySelector(item => ((Employee)item).Name), SortDirection.Ascending);
                    break;
                case 2:
                    source.Sort(new TableViewKeySelector(item => ((Employee)item).Name), SortDirection.Descending);
                    break;
                case 3:
                    source.Sort(new TableViewKeySelector(item => ((Employee)item).Title), SortDirection.Ascending);
                    break;
            }
        }
    }

    private void ApplyGroup()
    {
        foreach (var source in Sources)
        {
            if (GroupToggle.IsOn)
            {
                // Only the roots are bucketed; a descendant stays under its parent.
                source.GroupBy(new TableViewKeySelector(item => ((Employee)item).Dept));
            }
            else
            {
                source.ClearGroupBy();
            }
        }
    }

    // Under ParentBy, live shaping also tracks keys, so a ManagerId/MentorId edit reparents.
    private void ApplyLive()
    {
        bool live = LiveToggle.IsOn;
        foreach (var source in Sources)
        {
            source.IsLiveShaping = live;
        }
    }
    // </snippet>

    // ---- Handlers ----

    private void Filter_Changed(object sender, TextChangedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        ApplyFilter();
        var text = FilterText;
        SetLastAction(text.Length == 0
            ? "Filter cleared"
            : string.Format(CultureInfo.CurrentCulture, "Filter -> \u201C{0}\u201D: {1:N0} matching employees", text, _employees.Count(x => Matches(x, text))));
    }

    private void Sort_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        ApplySort();
        SetLastAction("Sort -> " + SortLabel);
    }

    private void Group_Toggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        ApplyGroup();
        SetLastAction(GroupToggle.IsOn ? "Grouped the roots by department" : "Grouping off");
    }

    private void Live_Toggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        ApplyLive();
        SetLastAction(LiveToggle.IsOn ? "Live shaping -> On" : "Live shaping -> Off");
    }

    // The tables select independently; the actions act on the row selected last, and the other
    // table's selection is cleared so only one row reads as selected.
    private void OnTableSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (sender.SelectedItem is not null && !ReferenceEquals(sender, _activeTable))
        {
            var other = ReferenceEquals(sender, ManagerTable) ? MentorTable : ManagerTable;
            _activeTable = sender;
            other.DeselectAll();
        }

        RefreshReadouts();
    }

    // The prefix sorts last and flips name-filter matching.
    private void Rename_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedEmployee() is not { } selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = selected.Name;
        selected.Name = from.StartsWith(RenamePrefix, StringComparison.Ordinal)
            ? from.Substring(RenamePrefix.Length)
            : RenamePrefix + from;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Renamed {0} to {1}", from, selected.Name));
    }

    // Only roots are bucketed: a root changes group with its subtree; a descendant only its cell.
    private void NextDept_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedEmployee() is not { } selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = selected.Dept;
        var depts = HierarchyData.Depts;
        selected.Dept = depts[(Array.IndexOf(depts, selected.Dept) + 1) % depts.Length];
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", selected.Name, from, selected.Dept));
    }

    private void ExpandAll_Click(object sender, RoutedEventArgs e)
    {
        foreach (var table in new[] { ManagerTable, MentorTable })
        {
            table.ExpandAllGroups();
            table.ExpandAllRows();
        }

        SetLastAction("Expanded every group and row in both tables");
    }

    private void CollapseAll_Click(object sender, RoutedEventArgs e)
    {
        foreach (var table in new[] { ManagerTable, MentorTable })
        {
            table.CollapseAllGroups();
            table.CollapseAllRows();
        }

        SetLastAction("Collapsed every group and row in both tables");
    }

    // The new person reports to (and is mentored by) whoever is selected.
    private void AddReport_Click(object sender, RoutedEventArgs e)
    {
        var selected = SelectedEmployee();
        var hire = NewEmployee(selected?.Id, selected?.Id);
        _employees.Add(hire);
        SetLastAction(selected is null
            ? string.Format(CultureInfo.CurrentCulture, "Added {0} as a root (no row selected)", hire.Name)
            : string.Format(CultureInfo.CurrentCulture, "Added {0} under {1} in both tables", hire.Name, selected.Name));
    }

    private void AddRoot_Click(object sender, RoutedEventArgs e)
    {
        var hire = NewEmployee(null, null);
        _employees.Add(hire);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} as a root", hire.Name));
    }

    // Orphaned reports become roots (parent key no longer matches).
    private void Remove_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedEmployee() is not { } selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        var reports = _employees.Count(x => x.ManagerId == selected.Id);
        _employees.Remove(selected);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0}; {1:N0} direct report(s) became roots in the Manager table", selected.Name, reports));
    }

    // <snippet>
    // Moves selection under another manager (never itself or a report, which would cycle). Live: a
    // property set reparents. Off: the item is replaced so the collection change carries the new key.
    private void Reparent_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedEmployee() is not { } selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        int index = _employees.IndexOf(selected);
        var managerOf = _employees.ToDictionary(x => x.Id, x => x.ManagerId);
        bool IsUnderSelected(int? id)
        {
            for (int hops = 0; id is int current && hops <= _employees.Count; hops++)
            {
                if (current == selected.Id)
                {
                    return true;
                }

                id = managerOf.GetValueOrDefault(current);
            }

            return false;
        }

        for (int step = 1; step < _employees.Count; step++)
        {
            var candidate = _employees[(index + step * 7) % _employees.Count];
            if (candidate.Id != selected.ManagerId && !IsUnderSelected(candidate.Id))
            {
                if (LiveToggle.IsOn)
                {
                    selected.ManagerId = candidate.Id;
                }
                else
                {
                    _employees[index] = new Employee
                    {
                        Id = selected.Id,
                        ManagerId = candidate.Id,
                        MentorId = selected.MentorId,
                        Name = selected.Name,
                        Title = selected.Title,
                        Dept = selected.Dept,
                    };
                }

                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Reparented {0} under {1}{2}", selected.Name, candidate.Name, LiveToggle.IsOn ? string.Empty : " (item replaced)"));
                return;
            }
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "No other manager for {0} without a cycle", selected.Name));
    }
    // </snippet>

    private void Reset_Click(object sender, RoutedEventArgs e)
    {
        _employees.Clear();
        foreach (var employee in HierarchyData.Generate(200))
        {
            _employees.Add(employee);
        }

        _nextId = _employees.Max(x => x.Id) + 1;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Reset to {0:N0} employees", _employees.Count));
    }

    private Employee? SelectedEmployee() => _activeTable?.SelectedItem as Employee;

    private Employee NewEmployee(int? managerId, int? mentorId)
    {
        int id = _nextId++;
        return new Employee
        {
            Id = id,
            ManagerId = managerId,
            MentorId = mentorId,
            Name = $"New hire {id}",
            Title = "Associate",
            Dept = "Engineering",
        };
    }

    // ---- 100k perf probe ----

    private async void Perf_Click(object sender, RoutedEventArgs e) => await RunPerfAsync();

    // Times ParentBy/Sort/ExpandAllRows vs a GroupBy(Dept) baseline over 100k rows, each step with
    // a sync layout pass. Shows medians of 3 runs in the Perf probe readout.
    private async Task RunPerfAsync()
    {
        if (_perfRunning)
        {
            return;
        }

        _perfRunning = true;
        PerfButton.IsEnabled = false;
        StatusText.Text = "perf running...";

        var lines = new List<string>();
        var failed = false;
        var table = ManagerTable;
        try
        {
            var data = HierarchyData.Generate(PerfCount);
            var sortBy = new TableViewKeySelector(item => ((Employee)item).Name);

            var hier = new List<double[]>();
            var group = new List<double[]>();
            for (int run = 0; run < PerfRuns + 1; run++)
            {
                // Run 0 is a warm-up (JIT, first-use costs) and is discarded.
                var h = MeasureRun(table, data, isHierarchy: true, sortBy);
                await Task.Delay(100);
                var g = MeasureRun(table, data, isHierarchy: false, sortBy);
                await Task.Delay(100);
                if (run > 0)
                {
                    hier.Add(h);
                    group.Add(g);
                }
            }

            string[] hierSteps = { "ParentBy", "Sort(Name)", "ExpandAllRows" };
            string[] groupSteps = { "GroupBy(Dept)", "Sort(Name)", "ExpandAllGroups" };
            lines.Add($"items={PerfCount} branching={HierarchyData.Branching} runs={PerfRuns} (median, warm-up discarded) build={BuildFlavor()} clr={Environment.Version} time=ms");
            double hierTotal = 0, groupTotal = 0;
            for (int i = 0; i < 3; i++)
            {
                double hm = Median(hier.Select(r => r[i])), gm = Median(group.Select(r => r[i]));
                hierTotal += hm;
                groupTotal += gm;
                lines.Add($"hierarchy {hierSteps[i]}: {hm:F1}   groupby {groupSteps[i]}: {gm:F1}");
            }

            double hierTotalMedian = Median(hier.Select(r => r.Sum())), groupTotalMedian = Median(group.Select(r => r.Sum()));
            lines.Add($"SUM OF STEP MEDIANS hierarchy={hierTotal:F1} groupby={groupTotal:F1} ratio={hierTotal / groupTotal:F2}x");
            lines.Add($"MEDIAN OF PER-RUN TOTALS hierarchy={hierTotalMedian:F1} groupby={groupTotalMedian:F1} ratio={hierTotalMedian / groupTotalMedian:F2}x");
            lines.Add("RAW hierarchy runs: " + string.Join(" | ", hier.Select(r => string.Join(",", r.Select(v => v.ToString("F1"))))));
            lines.Add("RAW groupby runs: " + string.Join(" | ", group.Select(r => string.Join(",", r.Select(v => v.ToString("F1"))))));
        }
        catch (Exception ex)
        {
            failed = true;
            lines.Add($"PERF ERROR {ex}");
        }
        finally
        {
            // MeasureRun swaps in temporary sources; always give the table its own source back.
            table.ItemsSource = _managerSource;
            PerfButton.IsEnabled = true;
            _perfRunning = false;
        }

        StatusText.Text = string.Join("\n", lines);
        SetLastAction(failed ? "Perf probe failed; see Perf probe" : "Perf probe finished; see Perf probe");
    }

    private static double[] MeasureRun(TableView table, List<Employee> data, bool isHierarchy, TableViewKeySelector sortBy)
    {
        var source = TableViewSource.From(data);
        table.ItemsSource = source;
        table.UpdateLayout();

        var times = new double[3];
        var sw = new Stopwatch();

        sw.Restart();
        if (isHierarchy)
        {
            DeclareManagerRelation(source);
        }
        else
        {
            source.GroupBy(new TableViewKeySelector(item => ((Employee)item).Dept));
        }

        table.UpdateLayout();
        times[0] = sw.Elapsed.TotalMilliseconds;

        sw.Restart();
        source.Sort(sortBy, SortDirection.Ascending);
        table.UpdateLayout();
        times[1] = sw.Elapsed.TotalMilliseconds;

        // Groups start expanded, so the baseline collapses them (untimed) to make ExpandAllGroups
        // realize the same 100k rows that ExpandAllRows does for the (collapsed-by-default) tree.
        if (!isHierarchy)
        {
            table.CollapseAllGroups();
            table.UpdateLayout();
        }

        sw.Restart();
        if (isHierarchy)
        {
            table.ExpandAllRows();
        }
        else
        {
            table.ExpandAllGroups();
        }

        table.UpdateLayout();
        times[2] = sw.Elapsed.TotalMilliseconds;

        return times;
    }

    private static double Median(IEnumerable<double> values)
    {
        var sorted = values.OrderBy(v => v).ToArray();
        return sorted[sorted.Length / 2];
    }

    private static string BuildFlavor()
    {
#if DEBUG
        return "sample Debug (product chk/amd64chk)";
#else
        return "sample Release (product chk/amd64chk)";
#endif
    }
}
