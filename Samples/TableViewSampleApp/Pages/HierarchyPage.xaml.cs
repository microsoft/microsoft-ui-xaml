using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;

namespace TableViewSampleApp;

// Flat parent-key hierarchy demo. One ObservableCollection<Employee> feeds two TableViewSources;
// each declares its own relation with ParentBy, so the same rows form two different trees.
// Filter, Sort and GroupBy are stages on the same sources and compose with the hierarchy.
public sealed partial class HierarchyPage : Page
{
    public const string PerfTriggerFileName = "autorun-perf";
    public const string PerfResultsFileName = "perf-results.txt";
    private const int PerfCount = 100_000;
    private const int PerfRuns = 3;

    public static bool AutoPerf { get; set; }

    // LocalFolder when the app has package identity; otherwise the exe's directory (the build
    // produces an unpackaged app, where ApplicationData.Current throws).
    public static string DataDirectory
    {
        get
        {
            try
            {
                return Windows.Storage.ApplicationData.Current.LocalFolder.Path;
            }
            catch (Exception)
            {
                return AppContext.BaseDirectory;
            }
        }
    }

    private readonly ObservableCollection<Employee> _employees = new(HierarchyData.Generate(200));
    private TableViewSource _managerSource = null!;
    private TableViewSource _mentorSource = null!;
    private int _nextId;
    private bool _ready;   // guards SelectionChanged/TextChanged that fire while XAML loads
    private bool _perfRunning;

    public HierarchyPage()
    {
        this.InitializeComponent();

        BuildColumns(ManagerTable);
        BuildColumns(MentorTable);

        _nextId = _employees.Max(e => e.Id) + 1;

        _managerSource = TableViewSource.From(_employees);
        _mentorSource = TableViewSource.From(_employees);
        ManagerTable.ItemsSource = _managerSource;
        MentorTable.ItemsSource = _mentorSource;
        ManagerTable.GroupHeaderTemplate = (DataTemplate)Resources["GroupHeader"];
        MentorTable.GroupHeaderTemplate = (DataTemplate)Resources["GroupHeader"];

        DeclareManagerRelation(_managerSource);
        DeclareMentorRelation(_mentorSource);
        ApplyLive();

        _ready = true;
        UpdateStatus();

        Loaded += async (_, _) =>
        {
            if (AutoPerf)
            {
                AutoPerf = false;
                await RunPerfAsync();
                Application.Current.Exit();
            }
        };
    }

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

    private IEnumerable<TableViewSource> Sources => new[] { _managerSource, _mentorSource };

    // ---- Shaping (applied to both sources) ----

    private void ApplyFilter()
    {
        if (!_ready)
        {
            return;
        }

        var text = FilterBox.Text?.Trim() ?? string.Empty;
        foreach (var source in Sources)
        {
            if (text.Length == 0)
            {
                source.ClearFilter();
            }
            else
            {
                // Matches NODES; ancestors of a match are kept as context rows and auto-expanded
                // while the filter is active.
                source.Filter(new TableViewPredicate(
                    item => ((Employee)item).Name.Contains(text, StringComparison.OrdinalIgnoreCase)));
            }
        }

        UpdateStatus();
    }

    private void ApplySort()
    {
        if (!_ready)
        {
            return;
        }

        foreach (var source in Sources)
        {
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
                default:
                    source.ClearSort();
                    break;
            }
        }

        UpdateStatus();
    }

    private void ApplyGroup()
    {
        if (!_ready)
        {
            return;
        }

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

        UpdateStatus();
    }

    // ---- Handlers ----

    private void Filter_Changed(object sender, TextChangedEventArgs e) => ApplyFilter();

    private void Sort_Changed(object sender, SelectionChangedEventArgs e) => ApplySort();

    private void Group_Toggled(object sender, RoutedEventArgs e) => ApplyGroup();

    // Live shaping on both sources. Under ParentBy it also tracks each row's key and parent key, so
    // a ManagerId / MentorId edit reparents.
    private void ApplyLive()
    {
        bool live = LiveToggle.IsOn;
        foreach (var source in Sources)
        {
            source.IsLiveShaping = live;
        }
    }

    private void Live_Toggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        ApplyLive();
        UpdateStatus();
    }

    private const string RenamePrefix = "zz ";

    // Toggles a prefix that sorts last, so under "Name ascending" the row jumps to the end of its
    // sibling set and back, and it flips whether it matches a name filter.
    private void Rename_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedEmployee() is { } selected)
        {
            selected.Name = selected.Name.StartsWith(RenamePrefix, StringComparison.Ordinal)
                ? selected.Name.Substring(RenamePrefix.Length)
                : RenamePrefix + selected.Name;
            UpdateStatus();
        }
    }

    // Only ROOTS are bucketed, so with "Group roots by Dept" on a root moves to another group with
    // its whole subtree; a descendant's Dept changes its cell but not its place in the tree.
    private void NextDept_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedEmployee() is { } selected)
        {
            var depts = HierarchyData.Depts;
            selected.Dept = depts[(Array.IndexOf(depts, selected.Dept) + 1) % depts.Length];
            UpdateStatus();
        }
    }

    private void ExpandAll_Click(object sender, RoutedEventArgs e)
    {
        foreach (var table in new[] { ManagerTable, MentorTable })
        {
            table.ExpandAllGroups();
            table.ExpandAllRows();
        }

        UpdateStatus();
    }

    private void CollapseAll_Click(object sender, RoutedEventArgs e)
    {
        foreach (var table in new[] { ManagerTable, MentorTable })
        {
            table.CollapseAllGroups();
            table.CollapseAllRows();
        }

        UpdateStatus();
    }

    // The new person reports to (and is mentored by) whoever is selected in either table.
    private void AddReport_Click(object sender, RoutedEventArgs e)
    {
        var selected = SelectedEmployee();
        _employees.Add(NewEmployee(selected?.Id, selected?.Id));
        UpdateStatus();
    }

    private void AddRoot_Click(object sender, RoutedEventArgs e)
    {
        _employees.Add(NewEmployee(null, null));
        UpdateStatus();
    }

    // Removing a person does not remove their reports: their parent key no longer matches any
    // row, so they become orphan roots.
    private void Remove_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedEmployee() is { } selected)
        {
            _employees.Remove(selected);
            UpdateStatus();
        }
    }

    // Moves the selected employee under a different manager. With live shaping on the edit is just
    // a property set: both sources observe ManagerId and reparent the row on the next dispatcher
    // turn. With it off, a property set would leave the row where it was, so the item is replaced
    // in the collection instead -- the collection change is what announces the new key.
    // The candidate must not be the employee or one of its reports, or the relation would cycle.
    private void Reparent_Click(object sender, RoutedEventArgs e)
    {
        if (SelectedEmployee() is not { } selected)
        {
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
                    break;
                }

                _employees[index] = new Employee
                {
                    Id = selected.Id,
                    ManagerId = candidate.Id,
                    MentorId = selected.MentorId,
                    Name = selected.Name,
                    Title = selected.Title,
                    Dept = selected.Dept,
                };
                break;
            }
        }

        UpdateStatus();
    }

    private void Reset_Click(object sender, RoutedEventArgs e)
    {
        _employees.Clear();
        foreach (var employee in HierarchyData.Generate(200))
        {
            _employees.Add(employee);
        }

        _nextId = _employees.Max(x => x.Id) + 1;
        UpdateStatus();
    }

    private Employee? SelectedEmployee() =>
        ManagerTable.SelectedItem as Employee ?? MentorTable.SelectedItem as Employee;

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

    private void UpdateStatus()
    {
        if (StatusText is null)
        {
            return;
        }

        var filter = FilterBox.Text?.Trim() ?? string.Empty;
        var sort = ((ComboBoxItem)SortCombo.SelectedItem)?.Content?.ToString() ?? "Authored order";
        var selected = SelectedEmployee();

        StatusText.Text =
            $"employees={_employees.Count}  group={(GroupToggle.IsOn ? "Dept" : "none")}  sort={sort}  " +
            $"filter={(filter.Length == 0 ? "none" : "\"" + filter + "\"")}  live={(LiveToggle.IsOn ? "on" : "off")}  " +
            $"selected={selected?.Name ?? "(none)"}";
    }

    // ---- 100k perf probe ----

    private async void Perf_Click(object sender, RoutedEventArgs e) => await RunPerfAsync();

    // Times the hierarchy pipeline (ParentBy, Sort, ExpandAllRows) against a GroupBy(Dept)
    // baseline (GroupBy, Sort, CollapseAllGroups [untimed], ExpandAllGroups) over the same 100k flat
    // rows. Each step includes a synchronous layout pass. Three runs each; per-step medians and the
    // median of per-run totals are reported. Writes perf-results.txt.
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
        try
        {
            var data = HierarchyData.Generate(PerfCount);
            var sortBy = new TableViewKeySelector(item => ((Employee)item).Name);
            var table = ManagerTable;

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

            table.ItemsSource = _managerSource;

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
            lines.Add($"PERF ERROR {ex}");
        }

        StatusText.Text = string.Join("\n", lines);
        try
        {
            File.WriteAllLines(Path.Combine(DataDirectory, PerfResultsFileName), lines, new UTF8Encoding(false));
        }
        catch (Exception ex)
        {
            StatusText.Text += "\ncould not write results file: " + ex.Message;
        }

        PerfButton.IsEnabled = true;
        _perfRunning = false;
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
