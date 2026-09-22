using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp;

// Filtering / sorting / grouping over a single TableViewSource. Filter, GroupBy and Sort are
// stages on that one source rather than three separate collections, so they compose and a reshape
// keeps row identity (selection re-anchors to the same item instead of the same index).
public sealed partial class ShapingPage : Page
{
    // Observable so the page can mutate the collection after binding. That is what makes a Reset
    // reach the TableView on an ItemsSourceView it already holds - the shaping verbs each swap in a
    // new view instead, so they never exercise that path.
    private readonly ObservableCollection<Item> _items = new(Data.Make());
    private TableViewSource _source = null!;
    private bool _ready;   // guards combo SelectionChanged that fires during XAML load

    // Every SelectionChanged the control raises, in order. A Reset that the control absorbs cleanly
    // shows as one entry; a Reset it reacts to late shows the transient clear as a separate entry
    // before the restore, which is the only externally visible difference between the two.
    private readonly List<string> _selLog = new();

    // Free-form diagnostic line appended to the status text, so a UIA driver can read whatever the
    // last diagnostic action measured.
    private string _diag = "";

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
        Table.SelectionChanged += Table_SelectionChanged;

        Table.CanUserSortColumns = HeaderSortToggle.IsChecked == true;
        UpdateCycleHint();

        // Seeded from the checkboxes rather than assumed, so the control and the UI cannot start
        // out disagreeing. All four are off/read-only by default: live shaping is opt-in, and
        // TableView.IsReadOnly defaults to true.
        ApplyLiveShaping();
        Table.IsReadOnly = EditableToggle.IsChecked != true;

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

    // Each verb is an independent stage on the one source, so a change only re-declares its own.
    // Re-declaring a stage that did not change is not free: every declaration re-mints the
    // description id the engine diffs on, so reapplying GroupBy and Sort on every keystroke would
    // force a re-bucket and a re-sort that the filter change never needed.
    private void ApplyFilter()
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

        UpdateStatus();
    }

    private void ApplyGroup()
    {
        if (!_ready)
        {
            return;
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

        UpdateStatus();
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
    private void ApplySort()
    {
        if (!_ready)
        {
            return;
        }

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

    private void Filter_Changed(object sender, TextChangedEventArgs e) => ApplyFilter();

    private void Filter_Toggled(object sender, RoutedEventArgs e) => ApplyFilter();

    private void ClearFilter_Click(object sender, RoutedEventArgs e)
    {
        FilterBox.Text = string.Empty;
        HighScoresOnly.IsChecked = false;
        ApplyFilter();
    }

    private void Group_Changed(object sender, SelectionChangedEventArgs e) => ApplyGroup();

    private void Sort_Changed(object sender, SelectionChangedEventArgs e) => ApplySort();

    private void ExpandAll_Click(object sender, RoutedEventArgs e)
    {
        _diag = "focusAtClick=" + FocusedName();
        Table.ExpandAllGroups();
        UpdateStatus();
    }

    private void CollapseAll_Click(object sender, RoutedEventArgs e)
    {
        _diag = "focusAtClick=" + FocusedName();
        Table.CollapseAllGroups();
        UpdateStatus();
    }

    // A button steals focus before its Click handler runs, which destroys the very state the bulk
    // expand/collapse focus restore exists to preserve. An accelerator leaves focus where it is,
    // so this is the path that exercises the real scenario.
    private void CollapseAccel_Invoked(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args)
    {
        _diag = "focusAtAccel=" + FocusedName();
        Table.CollapseAllGroups();
        UpdateStatus();
        args.Handled = true;
    }

    private void ExpandAccel_Invoked(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args)
    {
        _diag = "focusAtAccel=" + FocusedName();
        Table.ExpandAllGroups();
        UpdateStatus();
        args.Handled = true;
    }

    private string FocusedName()
    {
        // Read through the same API the control uses, so this reports exactly what the control's
        // focus capture would have seen at the moment the bulk command ran. The XamlRoot overload
        // is the one that works in WinUI 3; the parameterless one returns null.
        return FocusManager.GetFocusedElement(XamlRoot) is FrameworkElement fe
            ? $"{fe.GetType().Name}/{(fe as Control)?.FocusState.ToString() ?? "n-a"}"
            : "null";
    }

    // Reads banding straight off the realized containers instead of sampling pixels: no DPI, z-order
    // or screenshot timing to get wrong. Emits one character per container in visual order.
    private void DumpBanding_Click(object sender, RoutedEventArgs e)
    {
        var found = new List<(double Y, char C)>();
        Collect(Table, found);
        var ordered = found.OrderBy(t => t.Y).Select(t => t.C).ToArray();
        _diag = "band " + new string(ordered);
        UpdateStatus();
    }

    private void Collect(DependencyObject node, List<(double Y, char C)> found)
    {
        var count = VisualTreeHelper.GetChildrenCount(node);
        for (var i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(node, i);
            var name = child.GetType().Name;
            if (name == "TableViewRow" || name == "TableViewGroupHeader")
            {
                var fe = (FrameworkElement)child;
                double y;
                try { y = fe.TransformToVisual(Table).TransformPoint(new Windows.Foundation.Point(0, 0)).Y; }
                catch { continue; }

                var c = '|';
                if (name == "TableViewRow")
                {
                    var fill = (child as Control)?.Background as SolidColorBrush;
                    c = fill is null ? '-'
                        : fill.Color.R > 200 && fill.Color.G < 100 ? 'R'
                        : fill.Color.R > 200 && fill.Color.G > 200 ? 'W'
                        : '?';
                }
                found.Add((y, c));
            }

            Collect(child, found);
        }
    }

    private void GroupHeaderTemplate_Toggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;   // fires while XAML seeds IsChecked, before Table exists
        }

        // null falls back to the control's built-in KeyText / ItemCountText header
        Table.GroupHeaderTemplate = CustomHeaderToggle.IsChecked == true
            ? (DataTemplate)Resources["GroupHeader"]
            : null;
    }

    private void HeaderSort_Toggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

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
        SortCombo.SelectedIndex = 0;   // re-enters ApplySort, which clears both sort states
    }

    // Header clicks reshape the same source the Sort row drives, so the readout has to follow the
    // control's state, not just the combo.
    // A header click changes the control's sort behind the Sort row's back. Mirror it into the
    // combos, or the next change in the Sort row would apply the combos' stale selection over the
    // sort the header just established.
    private void Table_Sorted(object sender, TableViewSortedEventArgs args)
    {
        var wasReady = _ready;
        _ready = false;   // suppress the SelectionChanged re-entry into ApplySort
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

    private void Repump_Click(object sender, RoutedEventArgs e)
    {
        // Detach the TableView, then re-attach it on the next tick. Doing both in one tick would let
        // XAML coalesce the tree change and raise neither Unloaded nor Loaded, so the detach has to
        // settle first. The ItemsSource is untouched, so the reload re-runs the rows pipeline against
        // the very same ItemsSourceView - the case where the selection subscriptions must not
        // re-register, or they fall behind SelectionModel and stop seeing a Reset before it does.
        if (TableHost.Child is null)
        {
            return;
        }

        TableHost.Child = null;
        DispatcherQueue.TryEnqueue(() =>
        {
            TableHost.Child = Table;
            UpdateStatus();
        });
    }

    private void Table_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        _selLog.Add($"#{_selLog.Count + 1} idx={Table.SelectedIndex} +{e.AddedItems.Count}-{e.RemovedItems.Count}");
        UpdateStatus();
    }

    private void ClearSelLog_Click(object sender, RoutedEventArgs e)
    {
        _selLog.Clear();
        UpdateStatus();
    }

    // Alternating banding is index-derived, so grouping is what exposes whether the parity counts
    // synthesized header rows. Opaque, saturated fills so a screenshot can be sampled per row.
    private void Banding_Changed(object sender, RoutedEventArgs e)
    {
        if (BandingToggle.IsChecked == true)
        {
            Table.RowBackground = new SolidColorBrush(Microsoft.UI.Colors.White);
            Table.AlternatingRowBackground = new SolidColorBrush(Microsoft.UI.Colors.Red);
        }
        else
        {
            Table.RowBackground = null;
            Table.AlternatingRowBackground = null;
        }
    }

    private void AddItem_Click(object sender, RoutedEventArgs e)
    {
        // Mutates the bound collection, so the projection re-publishes over the ItemsSourceView the
        // TableView already holds rather than handing it a new one. With grouping on there is no
        // incremental path, so this arrives as a Reset - the case the selection reset detector
        // exists for, and the only one that exposes a subscription order inversion.
        var n = _items.Count + 1;
        _items.Add(new Item($"Added {n}", "Engineer", "Seattle", 42, "", DateTimeOffset.Now, "", null));

        UpdateStatus();
    }

    // ---- Live shaping ----------------------------------------------------------------------
    //
    // A collection change always reshapes. A PROPERTY change on a row already in the projection
    // only reshapes when the matching live flag is on - otherwise the row keeps the position,
    // bucket and filter verdict it had when it was last shaped, and goes stale in place. These
    // handlers exist to make that difference observable side by side.

    private const string RenamePrefix = "zz ";

    private void ApplyLiveShaping()
    {
        _source.IsLiveSorting = LiveSortToggle.IsChecked == true;
        _source.IsLiveFiltering = LiveFilterToggle.IsChecked == true;
        _source.IsLiveGrouping = LiveGroupToggle.IsChecked == true;
    }

    private void LiveShaping_Toggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        ApplyLiveShaping();
        UpdateStatus();
    }

    private void Editable_Toggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        // Editing a cell commits through the column's own two-way editor, so it raises exactly the
        // same PropertyChanged the mutate buttons do - the same live-shaping trigger, reached by
        // typing instead of clicking.
        Table.IsReadOnly = EditableToggle.IsChecked != true;
        UpdateStatus();
    }

    // Selection is reported as the underlying item even while grouped, so a mutation lands on the
    // data row regardless of how the projection is arranged.
    private Item? MutationTarget()
    {
        if (Table.SelectedItem is Item item)
        {
            return item;
        }

        _diag = "mutate: no row selected";
        UpdateStatus();
        return null;
    }

    private void MutateRole_Click(object sender, RoutedEventArgs e)
    {
        if (MutationTarget() is not { } item)
        {
            return;
        }

        // Cycles within the authored value set, so the row moves to a bucket that already exists
        // rather than creating one - the case where a stale live grouping is most obvious.
        var next = Data.Roles[(Array.IndexOf(Data.Roles, item.Role) + 1) % Data.Roles.Length];
        _diag = $"mutate: '{item.Name}' Role '{item.Role}' -> '{next}'";
        item.Role = next;

        UpdateStatus();
    }

    private void MutateCity_Click(object sender, RoutedEventArgs e)
    {
        if (MutationTarget() is not { } item)
        {
            return;
        }

        var next = Data.Cities[(Array.IndexOf(Data.Cities, item.City) + 1) % Data.Cities.Length];
        _diag = $"mutate: '{item.Name}' City '{item.City}' -> '{next}'";
        item.City = next;

        UpdateStatus();
    }

    private void MutateName_Click(object sender, RoutedEventArgs e)
    {
        if (MutationTarget() is not { } item)
        {
            return;
        }

        // Toggling a prefix that sorts last moves the row across the whole range under a Name sort
        // and flips whether it matches a text filter, and clicking again puts it back.
        var next = item.Name.StartsWith(RenamePrefix, StringComparison.Ordinal)
            ? item.Name.Substring(RenamePrefix.Length)
            : RenamePrefix + item.Name;
        _diag = $"mutate: Name '{item.Name}' -> '{next}'";
        item.Name = next;

        UpdateStatus();
    }

    private void MutateScore_Click(object sender, RoutedEventArgs e)
    {
        if (MutationTarget() is not { } item)
        {
            return;
        }

        // A step that is not a divisor of the range keeps crossing the 50 threshold the filter uses
        // and the band boundaries the grouping uses, instead of settling into a short orbit.
        var next = (item.Score + 30) % 101;
        _diag = $"mutate: '{item.Name}' Score {item.Score} -> {next}";
        item.Score = next;

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

        var livePart =
            $"{(LiveSortToggle.IsChecked == true ? "sort" : "-")}/" +
            $"{(LiveFilterToggle.IsChecked == true ? "filter" : "-")}/" +
            $"{(LiveGroupToggle.IsChecked == true ? "group" : "-")}";

        StatusText.Text =
            $"rows {visible.Count}/{_items.Count}   filter '{text}'{(highOnly ? " + score>=50" : "")}   " +
            $"group {groupPart}   sort {sortPart}   cycle {((ComboBoxItem)CycleCombo.SelectedItem).Content}   " +
            $"selected {(selected is null ? "none" : $"'{selected.Name}' @ {Table.SelectedIndex}")}" +
            $"\nlive {livePart}   edit {(Table.IsReadOnly ? "off" : "on")}" +
            $"\nselLog[{_selLog.Count}] {string.Join(" | ", _selLog)}" +
            $"\ndiag {_diag}";
    }
}
