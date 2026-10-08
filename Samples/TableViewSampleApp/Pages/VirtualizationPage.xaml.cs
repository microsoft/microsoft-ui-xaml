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
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using Windows.Foundation;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Row virtualization over a large in-memory set. The control raises no realization event and
/// exposes no realized-row count, so the readouts walk the visual tree for TableViewRow (a public
/// type) and sample it twice a second. "Realized rows" counts the rows that intersect the
/// viewport; "Row pool" counts every container the table keeps.
/// </summary>
public sealed partial class VirtualizationPage : SamplePageBase
{
    private readonly DispatcherTimer _sampler = new() { Interval = TimeSpan.FromMilliseconds(500) };
    private ObservableCollection<Person> _people;
    private TableViewSource _source;           // one per dataset; reshaped in place, never per shaping change
    private TableViewTextColumn? _departmentTextColumn;

    public VirtualizationPage()
    {
        _people = new ObservableCollection<Person>(PersonData.Many(10_000));
        _source = TableViewSource.From(_people);
        InitializeComponent();
        PeopleTable.ItemsSource = _source;
        UpdateActionLabels();

        // No Reselect: probing tens of thousands of indexes would stall the page. A plain reshape
        // keeps the selection on its item; Rename re-selects its row by index as it scrolls.
        Shaping.RestoreSelection = false;
        InitializeSample(Status, Shaping.Attach(PeopleTable, _source));

        // Realization changes as the user scrolls and the control raises no event for it, so the
        // readout is sampled while the page is loaded.
        _sampler.Tick += OnSamplerTick;
        TrackTimer(_sampler, () => true);
    }

    private void OnSamplerTick(object? sender, object e) => RefreshReadouts();

    protected override void OnShapingApplying(ShapingApplyingEventArgs e) => _peakPool = 0;

    // ---- Dataset ------------------------------------------------------------------------

    private void OnRowCountChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="2"); the constructor already built 10,000.
        if (PeopleTable is null || !IsLoaded
            || !int.TryParse(SampleShaping.SelectedTag(RowCountSelector, "10000"), NumberStyles.Integer, CultureInfo.InvariantCulture, out var count))
        {
            return;
        }

        // <snippet>
        // There is nothing to switch on: TableView virtualizes by default. The lesson is how not
        // to defeat it.

        // DO: build the whole list first, then hand it to the table in one assignment. Adding rows
        // one at a time to a bound collection raises one notification per row.
        _people = new ObservableCollection<Person>(PersonData.Many(count));
        _source = TableViewSource.From(_people);
        PeopleTable.ItemsSource = _source;
        _peakPool = 0; // snippet:skip

        // A new source starts flat; give it the current shaping.
        Shaping.Attach(PeopleTable, _source);
        if (IsGrouped)
        {
            Shaping.Apply(announce: false);
        }
        // </snippet>

        UpdateActionLabels();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Rebuilt the source with {0:N0} rows", count));
    }

    private void OnTemplateColumnsToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || !IsLoaded)
        {
            return;
        }

        _departmentTextColumn ??= new TableViewTextColumn
        {
            Header = "Department",
            Width = new GridLength((double)Application.Current.Resources["ColWidthChip"]),
            Binding = new Microsoft.UI.Xaml.Data.Binding { Path = new PropertyPath(nameof(Person.Department)) },
            CanSort = false,
        };

        var columns = PeopleTable.Columns;
        if (TemplateColumnsToggle.IsOn)
        {
            columns[columns.IndexOf(_departmentTextColumn)] = DepartmentChipColumn;
            columns.Insert(columns.IndexOf(IdColumn) + 1, PhotoColumn);
            columns.Insert(columns.IndexOf(DepartmentChipColumn) + 1, ActiveColumn);
        }
        else
        {
            columns.Remove(PhotoColumn);
            columns.Remove(ActiveColumn);
            columns[columns.IndexOf(DepartmentChipColumn)] = _departmentTextColumn;
        }

        _peakPool = 0;
        SetLastAction(TemplateColumnsToggle.IsOn
            ? "Template columns -> On (Photo, Department chip, Active)"
            : "Template columns -> Off (text only)");
    }

    // ---- Actions: reach rows that are not realized ---------------------------------------

    private int MiddleRowNumber => Math.Max(1, _people.Count / 2);

    private int FarRowNumber => Math.Max(1, _people.Count * 4 / 5);

    private void UpdateActionLabels()
    {
        ScrollToMiddleButton.Content = string.Format(CultureInfo.CurrentCulture, "Scroll to row {0:N0}", MiddleRowNumber);
        RenameFarRowButton.Content = string.Format(CultureInfo.CurrentCulture, "Rename row {0:N0} and scroll to it", FarRowNumber);
    }

    private void OnScrollToMiddleClick(object sender, RoutedEventArgs e) =>
        ScrollToRow(MiddleRowNumber, string.Format(CultureInfo.CurrentCulture, "Scrolled to row {0:N0}", MiddleRowNumber));

    private void OnRenameFarRowClick(object sender, RoutedEventArgs e)
    {
        var number = FarRowNumber;
        var person = _people[number - 1];
        var from = person.Role;
        person.Role = SampleShaping.Next(PersonData.Roles, from);
        ReapplyIfGroupedOn(nameof(Person.Role));
        ScrollToRow(number, string.Format(CultureInfo.CurrentCulture, "Renamed row {0:N0} ({1}) from {2} to {3}, then scrolled to it", number, person.FullName, from, person.Role));
    }

    private void OnJumpToLastClick(object sender, RoutedEventArgs e) =>
        ScrollToRow(_people.Count, string.Format(CultureInfo.CurrentCulture, "Jumped to the last row ({0:N0})", _people.Count));

    private void OnJumpToFirstClick(object sender, RoutedEventArgs e) =>
        ScrollToRow(1, "Jumped to the first row");

    // <snippet>
    /// <summary>
    /// Selects row <paramref name="number"/> (1-based, source order) and scrolls to it. This
    /// release has no TableView.ScrollIntoView, so the sample scrolls the body ScrollViewer to an
    /// estimated offset, then corrects the estimate from a realized row whose display index it
    /// knows (unrealized rows only have an estimated height), and finally lets the target row
    /// bring itself into view once it is realized.
    /// </summary>
    private void ScrollToRow(int number, string message)
    {
        if (_people.Count == 0)
        {
            SetLastAction("There are no rows.");
            return;
        }

        var target = _people[number - 1];
        if (IsGrouped)
        {
            // The display index map assumes every group is expanded.
            PeopleTable.ExpandAllGroups();
        }

        var indexes = DisplayIndexes();
        var targetIndex = indexes[target];
        PeopleTable.Select(targetIndex);

        PeopleTable.UpdateLayout();
        var scroller = FindBodyScroller(PeopleTable);
        var measured = CountRows().RowHeight;
        var rowHeight = measured > 0 ? measured : _rowHeight;
        if (scroller is null || rowHeight <= 0)
        {
            SetLastAction(message + " (selected; the table is not laid out yet, so it did not scroll)");
            return;
        }

        var offset = (targetIndex * rowHeight) - (scroller.ViewportHeight / 2);
        for (var attempt = 0; attempt < 6; attempt++)
        {
            scroller.ChangeView(null, Math.Max(0, offset), null, disableAnimation: true);
            PeopleTable.UpdateLayout();

            if (FindRealizedRow(target) is { } row)
            {
                row.StartBringIntoView();
                break;
            }

            // Not realized yet: measure how far off the estimate was from any realized row.
            if (FindRealizedRow(null) is not { DataContext: Person landed } || !indexes.TryGetValue(landed, out var landedIndex))
            {
                break;
            }

            offset = scroller.VerticalOffset + ((targetIndex - landedIndex) * rowHeight);
        }

        SetLastAction(message + (ReferenceEquals(PeopleTable.SelectedItem, target) ? "; it is selected" : string.Empty));
    }
    // </snippet>

    // Display index of every row. Flat: the source index. Grouped: groups appear in
    // first-encountered source order and keep source order inside, and each group header takes
    // one index before its rows (the same indexes Select uses).
    private Dictionary<Person, int> DisplayIndexes()
    {
        var map = new Dictionary<Person, int>(_people.Count);
        if (!IsGrouped)
        {
            for (var i = 0; i < _people.Count; i++)
            {
                map[_people[i]] = i;
            }

            return map;
        }

        var groups = new Dictionary<string, List<Person>>(StringComparer.Ordinal);
        var order = new List<List<Person>>();
        foreach (var person in _people)
        {
            var identity = SampleShaping.GroupIdentity(SampleShaping.KeyOf(person, AppliedGroupKey));
            if (!groups.TryGetValue(identity, out var members))
            {
                members = new List<Person>();
                groups[identity] = members;
                order.Add(members);
            }

            members.Add(person);
        }

        var index = 0;
        foreach (var members in order)
        {
            index++;   // the group header
            foreach (var person in members)
            {
                map[person] = index++;
            }
        }

        return map;
    }

    private static ScrollViewer? FindBodyScroller(DependencyObject root)
    {
        // PART_BodyScroller is the TableView template's body ScrollViewer. Read-only use here,
        // because the control has no scroll-to-row API in this release.
        for (var i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
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

    // A row in the viewport whose item is the target, or any item when target is null.
    private TableViewRow? FindRealizedRow(object? target)
    {
        var scroller = FindBodyScroller(PeopleTable);
        return scroller is null
            ? null
            : RowContainers(scroller).FirstOrDefault(row => InViewport(row, scroller)
                && (target is null ? row.DataContext is Person : ReferenceEquals(row.DataContext, target)));
    }

    private static IEnumerable<TableViewRow> RowContainers(DependencyObject node)
    {
        for (var i = 0; i < VisualTreeHelper.GetChildrenCount(node); i++)
        {
            var child = VisualTreeHelper.GetChild(node, i);
            if (child is TableViewRow row)
            {
                yield return row;
                continue;   // rows never nest
            }

            foreach (var nested in RowContainers(child))
            {
                yield return nested;
            }
        }
    }

    // <snippet>
    // Reading realization: the control exposes no realized-row count, so the readout walks the
    // visual tree for TableViewRow (a public type). A row counts as realized when it is visible and
    // its bounds intersect the body viewport.
    private static bool InViewport(TableViewRow row, ScrollViewer scroller)
    {
        if (row.Visibility != Visibility.Visible || row.ActualHeight <= 0)
        {
            return false;
        }

        var bounds = row.TransformToVisual(scroller).TransformBounds(new Rect(0, 0, row.ActualWidth, row.ActualHeight));
        return bounds.Bottom > 0 && bounds.Top < scroller.ViewportHeight;
    }
    // </snippet>

    /// <summary>
    /// Counts TableViewRow containers. A row is "realized" when it is visible and its bounds
    /// intersect the body viewport; containers the table parks off screen or hides (for example
    /// after Collapse all) count toward the pool only.
    /// </summary>
    private (int Visible, int Pool, double RowHeight) CountRows()
    {
        var scroller = FindBodyScroller(PeopleTable);
        if (scroller is null || scroller.ViewportHeight <= 0)
        {
            return (0, 0, 0);
        }

        int visible = 0, pool = 0;
        double rowHeight = 0;
        foreach (var row in RowContainers(scroller))
        {
            pool++;
            if (InViewport(row, scroller))
            {
                visible++;
                rowHeight = rowHeight > 0 ? rowHeight : row.ActualHeight;
            }
        }

        return (visible, pool, rowHeight);
    }
}
