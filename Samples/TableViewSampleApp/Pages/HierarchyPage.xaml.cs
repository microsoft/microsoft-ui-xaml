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
using TableViewSampleApp.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Hierarchy: TableViewSource.ParentBy over one flat list of people, by manager or by mentor, with
/// Filter, Sort, GroupBy (through the shared Shaping section) and IsLiveShaping on the same source.
/// </summary>
public sealed partial class HierarchyPage : SamplePageBase
{
    private const int RowCount = 200;
    private const int PerfCount = 100_000;
    private const int PerfRuns = 3;
    private const string RenamePrefix = "zz ";

    private int _nextId;
    private bool _perfRunning;

    public HierarchyPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // one source; ParentBy, Filter, Sort and GroupBy reshape it in place
        Source.IsLiveShaping = true;               // an edit to a key or shaped value reshapes the tree
        InitializeComponent();
        // </snippet>
        PeopleTable.SelectionChanged += OnSelectionChanged;
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source, parentRelation: ParentRelation.Manager));
        _nextId = People.Max(p => p.EmployeeId) + 1;
    }

    // <snippet>
    public ObservableCollection<Person> People { get; } = PersonData.Take(RowCount);
    // </snippet>

    // <snippet>
    public TableViewSource Source { get; }
    // </snippet>

    private Person? SelectedPerson => PeopleTable.SelectedItem as Person;

    private ParentRelation Relation => Shaping.Relation ?? ParentRelation.Manager;

    private bool IsMentorRelation => ReferenceEquals(Relation, ParentRelation.Mentor);

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args) => RefreshReadouts();

    private void OnRelationChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        var relation = SampleShaping.SelectedTag(RelationSelector, "Manager") == "Mentor" ? ParentRelation.Mentor : ParentRelation.Manager;
        Shaping.SetRelation(relation);
        SetLastAction(Shaping.IsHierarchical
            ? string.Format(CultureInfo.CurrentCulture, "Parent relation -> {0}: ParentBy declared again, every row collapsed", relation.Label)
            : string.Format(CultureInfo.CurrentCulture, "Parent relation -> {0}; choose a Hierarchy shaping mode to see it", relation.Label));
    }

    // <snippet>
    private void OnLiveToggled(object sender, RoutedEventArgs e)
    {
        if (!IsLoaded) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        Source.IsLiveShaping = ((ToggleSwitch)sender).IsOn;
        SetLastAction(Source.IsLiveShaping ? "IsLiveShaping -> True" : "IsLiveShaping -> False: edits wait for the next reshape");
    }
    // </snippet>

    // <snippet>
    private void OnFilterChanged(object sender, TextChangedEventArgs e)
    {
        var text = ((TextBox)sender).Text.Trim();
        if (text.Length == 0)
        {
            Source.ClearFilter();
        }
        else
        {
            // Matches rows; the ancestors of a match stay as ordinary rows, expanded while the filter is on.
            Source.Filter(item => ((Person)item).FullName.Contains(text, StringComparison.CurrentCultureIgnoreCase));
        }

        SetLastAction(text.Length == 0
            ? "Filter cleared"
            : string.Format(CultureInfo.CurrentCulture, "Filter -> \u201C{0}\u201D: {1:N0} matches, {2:N0} rows with their ancestors", text, People.Count(p => Matches(p, text)), Relation.CountWithAncestors(People, p => Matches(p, text))));
    }
    // </snippet>

    // <snippet>
    private void OnSortChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!IsLoaded) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        // A key-selector sort adds an axis rather than replacing one, so clear the previous axis first.
        Source.ClearSort();
        switch (((ComboBox)sender).SelectedIndex)
        {
            case 1:
                Source.Sort(item => ((Person)item).FullName, SortDirection.Ascending);
                break;
            case 2:
                Source.Sort(item => ((Person)item).FullName, SortDirection.Descending);
                break;
            case 3:
                Source.Sort(item => ((Person)item).Role, SortDirection.Ascending);
                break;
        }

        OnSortRedeclared(); // snippet:skip
        SetLastAction("Sort -> " + SortLabel);
    }
    // </snippet>

    private void OnRenameClick(object sender, RoutedEventArgs e)
    {
        if (SelectedPerson is not { } selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = selected.FullName;
        selected.FirstName = selected.FirstName.StartsWith(RenamePrefix, StringComparison.Ordinal)
            ? selected.FirstName.Substring(RenamePrefix.Length)
            : RenamePrefix + selected.FirstName;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Renamed {0} to {1}", from, selected.FullName));
    }

    // Only roots are bucketed: grouped, a root changes group with its subtree, a nested row only its cell.
    private void OnNextDepartmentClick(object sender, RoutedEventArgs e)
    {
        if (SelectedPerson is not { } selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = selected.Department;
        selected.Department = SampleShaping.Next(PersonData.Departments, from);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", selected.FullName, from, selected.Department));
    }

    private void OnAddReportClick(object sender, RoutedEventArgs e)
    {
        var selected = SelectedPerson;
        var hire = NewPerson(selected?.EmployeeId, selected?.Department);
        People.Add(hire);
        SetLastAction(selected is null
            ? string.Format(CultureInfo.CurrentCulture, "Added {0} as a root (no row selected)", hire.FullName)
            : string.Format(CultureInfo.CurrentCulture, "Added {0} under {1}", hire.FullName, selected.FullName));
    }

    private void OnAddRootClick(object sender, RoutedEventArgs e)
    {
        var hire = NewPerson(null, null);
        People.Add(hire);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} as a root", hire.FullName));
    }

    private void OnRemoveClick(object sender, RoutedEventArgs e)
    {
        if (SelectedPerson is not { } selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        var children = People.Count(p => Equals(Relation.ParentKeyOf(p), selected.EmployeeId));
        People.Remove(selected);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0}; {1:N0} child row(s) became orphan roots", selected.FullName, children));
    }

    // Moves the selected row under another parent, never under itself or its own subtree (a cycle).
    // With live shaping a key change reparents in place; without it, replace the item so the
    // collection change carries the new key.
    private void OnReparentClick(object sender, RoutedEventArgs e)
    {
        if (SelectedPerson is not { } selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        var parentOf = People.ToDictionary(p => p.EmployeeId, p => (int?)Relation.ParentKeyOf(p));
        bool IsInSubtree(int id)
        {
            int? current = id;
            for (var hops = 0; current is int key && hops <= People.Count; hops++)
            {
                if (key == selected.EmployeeId)
                {
                    return true;
                }

                current = parentOf.GetValueOrDefault(key);
            }

            return false;
        }

        var index = People.IndexOf(selected);
        var currentParent = (int?)Relation.ParentKeyOf(selected);
        for (var step = 1; step < People.Count; step++)
        {
            var candidate = People[(index + (step * 7)) % People.Count];
            if (candidate.EmployeeId == currentParent || IsInSubtree(candidate.EmployeeId))
            {
                continue;
            }

            var target = Source.IsLiveShaping ? selected : CopyOf(selected);
            if (IsMentorRelation)
            {
                target.MentorId = candidate.EmployeeId;
            }
            else
            {
                target.ManagerId = candidate.EmployeeId;
            }

            if (!Source.IsLiveShaping)
            {
                People[index] = target;
            }

            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Reparented {0} under {1}{2}", selected.FullName, candidate.FullName, Source.IsLiveShaping ? string.Empty : " (item replaced)"));
            return;
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "No other parent for {0} without a cycle", selected.FullName));
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        People.Clear();
        foreach (var person in PersonData.Take(RowCount))
        {
            People.Add(person);
        }

        _nextId = People.Max(p => p.EmployeeId) + 1;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Reset to {0:N0} people", People.Count));
    }

    private Person NewPerson(int? parentId, string? department)
    {
        var person = PersonData.Many(1, firstId: _nextId++)[0];
        person.FirstName = "New";
        person.LastName = string.Format(CultureInfo.InvariantCulture, "hire {0}", person.EmployeeId);
        person.Department = department ?? person.Department;
        person.ManagerId = parentId;
        person.MentorId = parentId;
        return person;
    }

    private static Person CopyOf(Person p) => new()
    {
        FirstName = p.FirstName,
        LastName = p.LastName,
        Email = p.Email,
        Department = p.Department,
        Role = p.Role,
        Bio = p.Bio,
        JoinDate = p.JoinDate,
        ShiftStart = p.ShiftStart,
        Salary = p.Salary,
        IsActive = p.IsActive,
        AvatarPath = p.AvatarPath,
        EmployeeId = p.EmployeeId,
        ManagerId = p.ManagerId,
        MentorId = p.MentorId,
        Office = p.Office,
    };

    // ---- 100,000-row probe ----------------------------------------------------------------

    private async void OnPerfClick(object sender, RoutedEventArgs e) => await RunPerfAsync();

    // Times ParentBy, Sort and ExpandAllRows against a GroupBy(Department) baseline, each step with a
    // synchronous layout pass, and reports the median of three runs.
    private async Task RunPerfAsync()
    {
        if (_perfRunning)
        {
            return;
        }

        _perfRunning = true;
        PerfButton.IsEnabled = false;
        Shaping.SetBusy(true);
        StatusText.Text = "Running...";
        SetLastAction("Perf probe started");

        var lines = new List<string>();
        var failed = false;
        try
        {
            var data = await Task.Run(() => PersonData.Many(PerfCount));
            var hierarchy = new List<double[]>();
            var grouped = new List<double[]>();
            for (int run = 0; run < PerfRuns + 1; run++)
            {
                // Run 0 is a warm-up (JIT, first-use costs) and is discarded.
                var h = MeasureRun(PeopleTable, data, isHierarchy: true);
                await Task.Delay(100);
                var g = MeasureRun(PeopleTable, data, isHierarchy: false);
                await Task.Delay(100);
                if (run > 0)
                {
                    hierarchy.Add(h);
                    grouped.Add(g);
                }
            }

            string[] hierarchySteps = { "ParentBy", "Sort(Name)", "ExpandAllRows" };
            string[] groupSteps = { "GroupBy(Department)", "Sort(Name)", "ExpandAllGroups" };
            lines.Add(string.Format(CultureInfo.InvariantCulture, "{0:N0} rows, Manager relation, median of {1} runs, ms ({2})", PerfCount, PerfRuns, BuildFlavor()));
            for (int i = 0; i < 3; i++)
            {
                lines.Add(string.Format(CultureInfo.InvariantCulture, "{0}: {1:F1}  vs  {2}: {3:F1}",
                    hierarchySteps[i], Median(hierarchy.Select(r => r[i])), groupSteps[i], Median(grouped.Select(r => r[i]))));
            }

            double hierarchyTotal = Median(hierarchy.Select(r => r.Sum())), groupTotal = Median(grouped.Select(r => r.Sum()));
            lines.Add(string.Format(CultureInfo.InvariantCulture, "Total: {0:F1} vs {1:F1} ({2:F2}x)", hierarchyTotal, groupTotal, hierarchyTotal / groupTotal));
        }
        catch (Exception ex)
        {
            failed = true;
            lines.Add("Perf probe failed: " + ex.Message);
        }
        finally
        {
            // MeasureRun swaps in temporary sources; give the table its own source back.
            PeopleTable.ItemsSource = Source;
            Shaping.SetBusy(false);
            PerfButton.IsEnabled = true;
            _perfRunning = false;
        }

        StatusText.Text = string.Join("\n", lines);
        SetLastAction(failed ? "Perf probe failed; see Perf probe" : "Perf probe finished; see Perf probe");
    }

    private static double[] MeasureRun(TableView table, List<Person> data, bool isHierarchy)
    {
        var source = TableViewSource.From(data);
        table.ItemsSource = source;
        table.UpdateLayout();

        var times = new double[3];
        var stopwatch = Stopwatch.StartNew();
        if (isHierarchy)
        {
            source.ParentBy(item => ((Person)item).EmployeeId, item => ((Person)item).ManagerId);
        }
        else
        {
            source.GroupBy(item => ((Person)item).Department);
        }

        table.UpdateLayout();
        times[0] = stopwatch.Elapsed.TotalMilliseconds;

        stopwatch.Restart();
        source.Sort(item => ((Person)item).FullName, SortDirection.Ascending);
        table.UpdateLayout();
        times[1] = stopwatch.Elapsed.TotalMilliseconds;

        // Groups start expanded, so the baseline collapses them (untimed) to make ExpandAllGroups
        // realize the same rows that ExpandAllRows does for the collapsed-by-default tree.
        if (!isHierarchy)
        {
            table.CollapseAllGroups();
            table.UpdateLayout();
        }

        stopwatch.Restart();
        if (isHierarchy)
        {
            table.ExpandAllRows();
        }
        else
        {
            table.ExpandAllGroups();
        }

        table.UpdateLayout();
        times[2] = stopwatch.Elapsed.TotalMilliseconds;
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
        return "sample Debug";
#else
        return "sample Release";
#endif
    }
    private void OnFirstColumnChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        var photo = SampleShaping.SelectedTag(FirstColumnSelector, "Photo") == "Photo";
        SetFirstColumn(photo);
        SetLastAction(photo ? "First column is now Photo" : "First column is now Name (text)");
    }

    // <snippet>
    // TableView.Columns is the live column collection: moving an entry moves the column. The first
    // visible column hosts the level indent and the expander chevron.
    private void SetFirstColumn(bool photoFirst)
    {
        var columns = PeopleTable.Columns;
        var from = columns.IndexOf(PhotoColumn);
        if ((from == 0) == photoFirst)
        {
            return;
        }

        // Move only Photo; the other columns keep their order.
        columns.RemoveAt(from);
        columns.Insert(photoFirst ? 0 : columns.IndexOf(NameColumn) + 1, PhotoColumn);
    }
    // </snippet>
}
