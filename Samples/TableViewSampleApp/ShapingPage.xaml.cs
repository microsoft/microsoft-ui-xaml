using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
// Disambiguate the real split-binary control types from the stale mock projection
// (Microsoft.UI.Xaml.Controls.TableView*) that the mock Microsoft.WinUI.dll still carries.
using SortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewKeySelector = Microsoft.UI.Xaml.Controls.Tabular.TableViewKeySelector;
using TableViewPredicate = Microsoft.UI.Xaml.Controls.Tabular.TableViewPredicate;
using TableViewSortCycle = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortCycle;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;
using TableViewSource = Microsoft.UI.Xaml.Controls.Tabular.TableViewSource;

namespace TableViewSampleApp;

// Filtering / sorting / grouping over a single TableViewSource. Filter, GroupBy and Sort are
// stages on that one source rather than three separate collections, so they compose and a reshape
// keeps row identity (selection re-anchors to the same item instead of the same index).
public sealed partial class ShapingPage : Page
{
    private readonly List<Item> _items = Data.Make();
    private TableViewSource _source = null!;
    private bool _ready;   // guards combo SelectionChanged that fires during XAML load

    // The cycle each column was built with. Kept so "As authored" can put back the per-column
    // choices - Score deliberately opens Descending - after the combo has overridden them all.
    private readonly Dictionary<TableViewColumn, TableViewSortCycle> _authoredCycles = new();

    public ShapingPage()
    {
        this.InitializeComponent();

        BuildColumns();

        // From() projects the collection once; every later verb reshapes that same projection.
        _source = TableViewSource.From(_items);
        Table.ItemsSource = _source;
        Table.GroupHeaderTemplate = (DataTemplate)Resources["GroupHeader"];

        Table.Sorted += Table_Sorted;
        Table.SelectionChanged += (s, e) => UpdateStatus();

        // Seeded here, not in markup: assigning ToggleButton.IsChecked from XAML needs
        // Nullable<Boolean> registered as a known type, which this split-binary app does not carry
        // (the same gap TabularControlsResources works around for Nullable<Double>).
        CustomHeaderToggle.IsChecked = true;
        HeaderSortToggle.IsChecked = true;

        UpdateCycleHint();

        _ready = true;
        UpdateStatus();
    }

    private void BuildColumns()
    {
        // Text columns fall back to Binding.Path for their sort key, so click-to-sort works
        // without setting SortMemberPath. Score opens Descending because the interesting rows in
        // a metric column are the largest ones.
        Table.Columns.Add(SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Auto()));
        Table.Columns.Add(SampleColumns.Text("Role", nameof(Item.Role), SampleColumns.Star()));
        Table.Columns.Add(SampleColumns.Text("City", nameof(Item.City), SampleColumns.Star()));

        var score = SampleColumns.Text("Score", nameof(Item.Score), SampleColumns.Pixels(90));
        score.SortCycle = TableViewSortCycle.DescendingAscendingNone;
        Table.Columns.Add(score);

        Table.Columns.Add(SampleColumns.Text("Joined", nameof(Item.Joined), SampleColumns.Pixels(220)));

        foreach (var column in Table.Columns)
        {
            _authoredCycles[column] = column.SortCycle;
        }
    }

    // ---- Shaping ----

    // Rebuilds all three stages from the current UI state. Each verb replaces its own stage, so
    // re-applying one never disturbs the other two.
    private void ApplyShape()
    {
        if (!_ready)
        {
            return;
        }

        var text = FilterBox.Text?.Trim() ?? string.Empty;
        var highOnly = HighScoresOnly.IsChecked == true;
        if (text.Length == 0 && !highOnly)
        {
            // ClearFilter, not a pass-everything predicate: the stage is removed rather than run
            // over every item on each rebuild.
            _source.ClearFilter();
        }
        else
        {
            _source.Filter(new TableViewPredicate(item => Matches((Item)item, text, highOnly)));
        }

        var groupKey = GroupKey();
        if (groupKey is null)
        {
            _source.ClearGroupBy();
        }
        else
        {
            // String keys use the built-in value-type group identity, so no identity selector.
            _source.GroupBy(new TableViewKeySelector(item => groupKey((Item)item)));
        }

        // Three front-ends over one sort. Sorting through the CONTROL owns the header chevron.
        // Sorting through the SOURCE by PATH is the fluent data-layer verb naming a property, so
        // the control can still find the column and light it. Sorting through the source by KEY is
        // anonymous - no property is named, so no chevron can honestly describe it.
        //
        // The control's sort replaces whatever came before it, but the source's Sort COMPOSES:
        // a second axis refines the first rather than replacing it, which is how multi-key sorts
        // are declared. Only an axis with the same token is replaced in place, and each path owns
        // its own token. This page offers a single sort, so it clears first - drop the ClearSort
        // and picking Name then Role would sort by Name, ties broken by Role.
        var sortColumn = SortColumn();
        if (sortColumn is null)
        {
            Table.ClearSort();       // clears every axis, including one declared on the source
        }
        else if (ViaCombo.SelectedIndex == 1)
        {
            _source.ClearSort();
            _source.Sort(SortPath()!, Direction());
        }
        else if (ViaCombo.SelectedIndex == 2)
        {
            var sortKey = SortKey()!;
            _source.ClearSort();
            _source.Sort(new TableViewKeySelector(item => sortKey((Item)item)), Direction());
        }
        else
        {
            Table.SortByColumn(sortColumn, Direction());
        }

        UpdateStatus();
    }

    private static bool Matches(Item item, string text, bool highOnly)
    {
        if (highOnly && item.Score < 50)
        {
            return false;
        }

        return text.Length == 0
            || item.Name.Contains(text, StringComparison.CurrentCultureIgnoreCase)
            || item.Role.Contains(text, StringComparison.CurrentCultureIgnoreCase)
            || item.City.Contains(text, StringComparison.CurrentCultureIgnoreCase);
    }

    private Func<Item, object>? GroupKey() => GroupCombo.SelectedIndex switch
    {
        1 => item => item.Role,
        2 => item => item.City,
        3 => item => ScoreBand(item.Score),
        _ => null,
    };

    // Sort row entries map 1:1 to columns, so the control can own the sort and publish the
    // chevron. Index 0 is "off"; every other index is column index + 1.
    private TableViewColumn? SortColumn() =>
        SortCombo.SelectedIndex <= 0 ? null : Table.Columns[SortCombo.SelectedIndex - 1];

    // Same selection expressed as a property path, for TableViewSource.SortByPath. These match the
    // paths the columns already bind to, which is what lets the control attribute the source's
    // sort to a column and light its chevron.
    private string? SortPath() => SortCombo.SelectedIndex switch
    {
        1 => nameof(Item.Name),
        2 => nameof(Item.Role),
        3 => nameof(Item.City),
        4 => nameof(Item.Score),
        5 => nameof(Item.Joined),
        _ => null,
    };

    // Same selection expressed as a key, for the fluent TableViewSource.Sort path.
    private Func<Item, object>? SortKey() => SortCombo.SelectedIndex switch
    {
        1 => item => item.Name,
        2 => item => item.Role,
        3 => item => item.City,
        4 => item => item.Score,
        5 => item => item.Joined,
        _ => null,
    };

    private SortDirection Direction() =>
        DirectionCombo.SelectedIndex == 1 ? SortDirection.Descending : SortDirection.Ascending;

    private static string ScoreBand(int score) => score switch
    {
        >= 80 => "80 - 100",
        >= 50 => "50 - 79",
        _ => "0 - 49",
    };

    // ---- Handlers ----

    private void Filter_Changed(object sender, TextChangedEventArgs e) => ApplyShape();

    private void Filter_Toggled(object sender, RoutedEventArgs e) => ApplyShape();

    private void ClearFilter_Click(object sender, RoutedEventArgs e)
    {
        FilterBox.Text = string.Empty;
        HighScoresOnly.IsChecked = false;
        ApplyShape();
    }

    private void Group_Changed(object sender, SelectionChangedEventArgs e) => ApplyShape();

    private void Sort_Changed(object sender, SelectionChangedEventArgs e) => ApplyShape();

    private void ExpandAll_Click(object sender, RoutedEventArgs e) => Table.ExpandAllGroups();

    private void CollapseAll_Click(object sender, RoutedEventArgs e) => Table.CollapseAllGroups();

    private void GroupHeaderTemplate_Toggled(object sender, RoutedEventArgs e)
        => Table.GroupHeaderTemplate = CustomHeaderToggle.IsChecked == true
            ? (DataTemplate)Resources["GroupHeader"]
            : null;   // null falls back to the control's built-in KeyText / ItemCountText header

    private void HeaderSort_Toggled(object sender, RoutedEventArgs e)
    {
        Table.CanUserSortColumns = HeaderSortToggle.IsChecked == true;
        UpdateCycleHint();
    }

    // SortCycle is per column and governs only what a HEADER CLICK walks through; it has no say
    // over a programmatic SortByColumn or a sort declared on the source. Index 0 restores the
    // authored per-column cycles, so Score keeps opening Descending; any other index overrides
    // every column so one cycle can be observed end to end.
    private void SortCycle_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        foreach (var column in Table.Columns)
        {
            column.SortCycle = CycleCombo.SelectedIndex switch
            {
                1 => TableViewSortCycle.AscendingDescending,
                2 => TableViewSortCycle.AscendingDescendingNone,
                3 => TableViewSortCycle.DescendingAscending,
                4 => TableViewSortCycle.DescendingAscendingNone,
                _ => _authoredCycles.TryGetValue(column, out var authored) ? authored : column.SortCycle,
            };
        }

        UpdateCycleHint();
        UpdateStatus();
    }

    private void UpdateCycleHint()
    {
        if (HeaderSortToggle.IsChecked != true)
        {
            CycleHint.Text = "click-to-sort is off, so no cycle is reachable";
            return;
        }

        CycleHint.Text = CycleCombo.SelectedIndex == 0
            ? "per column as built: Score starts Descending, the rest Ascending"
            : "applied to every column; a cycle without None never returns to unsorted by clicking";
    }

    private void ClearSort_Click(object sender, RoutedEventArgs e)
    {
        SortCombo.SelectedIndex = 0;   // re-enters ApplyShape, which clears both sort states
    }

    // Header clicks reshape the same source the Sort row drives, so the readout has to follow the
    // control's state, not just the combo.
    // A header click changes the control's sort behind the Sort row's back. Mirror it into the
    // combos, or the next filter/group change would re-enter ApplyShape and stomp the header's
    // sort with the combos' stale selection.
    private void Table_Sorted(object sender, TableViewSortedEventArgs args)
    {
        var wasReady = _ready;
        _ready = false;   // suppress the SelectionChanged re-entry into ApplyShape
        try
        {
            var column = args.Column;
            if (column is null && ViaCombo.SelectedIndex != 0)
            {
                // The control standing down because THIS page just sorted through the source with
                // a key. The Sort row still describes that sort, so leave it alone.
            }
            else
            {
                SortCombo.SelectedIndex = column is null ? 0 : Table.Columns.IndexOf(column) + 1;
            }

            if (column is not null && args.Direction != SortDirection.None)
            {
                DirectionCombo.SelectedIndex = args.Direction == SortDirection.Descending ? 1 : 0;
            }
        }
        finally
        {
            _ready = wasReady;
        }

        UpdateStatus();
    }

    private void UpdateStatus()
    {
        var text = FilterBox.Text?.Trim() ?? string.Empty;
        var highOnly = HighScoresOnly.IsChecked == true;
        var visible = _items.Where(item => Matches(item, text, highOnly)).ToList();

        var groupKey = GroupKey();
        var groupPart = groupKey is null
            ? "off"
            : $"{((ComboBoxItem)GroupCombo.SelectedItem).Content} ({visible.Select(groupKey).Distinct().Count()} groups)";

        // One sort axis, whoever declared it. A chevron means a column is attributed to that axis -
        // which the control now does for a path-declared source sort too, so the chevron alone no
        // longer says who declared it. The Via row does. A key-declared source sort names no
        // property, so it shows no chevron at all.
        var sorted = Table.Columns.FirstOrDefault(c => c.SortDirection != SortDirection.None);
        var owner = ViaCombo.SelectedIndex switch { 1 => "source by path", 2 => "source by key", _ => "control" };
        var sortPart = sorted is not null
            ? $"{sorted.Header} {sorted.SortDirection} ({owner}, chevron)"
            : SortCombo.SelectedIndex <= 0
                ? "off"
                : $"{((ComboBoxItem)SortCombo.SelectedItem).Content} {Direction()} ({owner}, no chevron)";

        var selected = Table.SelectedItem as Item;

        StatusText.Text =
            $"rows {visible.Count}/{_items.Count}   filter '{text}'{(highOnly ? " + score>=50" : "")}   " +
            $"group {groupPart}   sort {sortPart}   cycle {((ComboBoxItem)CycleCombo.SelectedItem).Content}   " +
            $"selected {(selected is null ? "none" : $"'{selected.Name}' @ {Table.SelectedIndex}")}";
    }
}
