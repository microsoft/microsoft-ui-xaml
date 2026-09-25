using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;

namespace TableViewSampleApp;

// Hierarchical rows over a single TableViewSource. WithChildren is a stage on the same source the
// other verbs live on, so it composes with them: Filter and Sort apply per sibling set, and GroupBy
// buckets the ROOTS while each root keeps its own subtree underneath its header.
public sealed partial class HierarchyPage : Page
{
    private readonly ObservableCollection<Node> _roots = new(HierarchyData.Make());
    private TableViewSource _source = null!;
    private bool _ready;   // guards SelectionChanged that fires while XAML loads

    public HierarchyPage()
    {
        this.InitializeComponent();

        BuildColumns();

        _source = TableViewSource.From(_roots);
        Table.ItemsSource = _source;
        Table.GroupHeaderTemplate = (DataTemplate)Resources["GroupHeader"];
        Table.RowIndentSize = IndentSlider.Value;

        ApplyChildren();

        _ready = true;
        UpdateStatus();
    }

    private void BuildColumns()
    {
        // The chevron and the indent are drawn OVER the first column rather than in a gutter column
        // of their own, so every cell still lines up with its header. Nothing about these column
        // declarations is hierarchy-aware.
        Table.Columns.Add(SampleColumns.Text("Name", nameof(Node.Name), SampleColumns.Star(2)));
        Table.Columns.Add(SampleColumns.Text("Kind", nameof(Node.Kind), SampleColumns.Pixels(90)));
        Table.Columns.Add(SampleColumns.Text("Owner", nameof(Node.Owner), SampleColumns.Star()));
        Table.Columns.Add(SampleColumns.Text("Size", nameof(Node.Size), SampleColumns.Pixels(90)));
        Table.Columns.Add(SampleColumns.Text("Modified", nameof(Node.ModifiedText), SampleColumns.Pixels(110)));
    }

    // ---- Shaping ----

    // Two overloads. Without the predicate, the projection has to CALL the children selector to
    // find out whether a node is expandable, which materializes every visible node's children even
    // while collapsed. With it, a folder draws its chevron from its own metadata and its contents
    // stay unread until someone opens it -- watch the "children materialized" counter.
    private void ApplyChildren()
    {
        if (HasChildrenToggle.IsChecked == true)
        {
            _source.WithChildren(
                new TableViewChildrenSelector(item => ((Node)item).Children),
                new TableViewHasChildrenPredicate(item => ((Node)item).IsFolder));
        }
        else
        {
            _source.WithChildren(new TableViewChildrenSelector(item => ((Node)item).Children));
        }

        UpdateStatus();
    }

    private void ApplyFilter()
    {
        if (!_ready)
        {
            return;
        }

        var text = FilterBox.Text?.Trim() ?? string.Empty;
        if (text.Length == 0)
        {
            _source.ClearFilter();
        }
        else
        {
            // The predicate is evaluated per sibling set at every level. It matches NODES, not
            // subtrees: a node whose name does not match is dropped even when a descendant would
            // have matched, because the descendant is only reachable through the parent that was
            // just removed.
            _source.Filter(new TableViewPredicate(
                item => ((Node)item).Name.Contains(text, StringComparison.OrdinalIgnoreCase)));
        }

        UpdateStatus();
    }

    private void ApplySort()
    {
        if (!_ready)
        {
            return;
        }

        switch (SortCombo.SelectedIndex)
        {
            case 1:
                _source.Sort(new TableViewKeySelector(item => ((Node)item).Name), SortDirection.Ascending);
                break;
            case 2:
                _source.Sort(new TableViewKeySelector(item => ((Node)item).Name), SortDirection.Descending);
                break;
            case 3:
                _source.Sort(new TableViewKeySelector(item => ((Node)item).SizeBytes), SortDirection.Descending);
                break;
            default:
                // Not a no-op sort: the stage is removed, so the authored order of each sibling
                // set comes back rather than being re-sorted by something that happens to agree.
                _source.ClearSort();
                break;
        }

        UpdateStatus();
    }

    private void ApplyGroup()
    {
        if (!_ready)
        {
            return;
        }

        var key = GroupKey();
        if (key is null)
        {
            _source.ClearGroupBy();
        }
        else
        {
            // Grouping composes with the hierarchy instead of replacing it. Only the roots are
            // bucketed; a descendant is never lifted out from under its parent to join a bucket,
            // because its depth is what makes it a descendant. So headers exist at the top level
            // and nowhere else, and expanding a root grows its bucket in place.
            _source.GroupBy(new TableViewKeySelector(item => key((Node)item)));
        }

        UpdateStatus();
    }

    private Func<Node, string>? GroupKey() => GroupCombo.SelectedIndex switch
    {
        1 => node => node.IsFolder ? "Folders" : "Files",
        2 => node => node.Owner,
        _ => null,
    };

    // ---- Handlers ----

    private void Filter_Changed(object sender, TextChangedEventArgs e) => ApplyFilter();

    private void Sort_Changed(object sender, SelectionChangedEventArgs e) => ApplySort();

    private void Group_Changed(object sender, SelectionChangedEventArgs e) => ApplyGroup();

    private void HasChildren_Toggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        ApplyChildren();
    }

    // One command drives both axes: under a composed projection it expands every group header AND
    // every expandable node, so there is no separate "expand all rows" verb to get out of sync.
    private void ExpandAll_Click(object sender, RoutedEventArgs e)
    {
        Table.ExpandAllGroups();
        UpdateStatus();
    }

    private void CollapseAll_Click(object sender, RoutedEventArgs e)
    {
        Table.CollapseAllGroups();
        UpdateStatus();
    }

    // Replaces the root objects outright. Expansion is keyed off object identity, so the new tree
    // comes back fully collapsed and the materialization counter resets - which is the honest
    // result, since none of the old nodes survive.
    private void Rebuild_Click(object sender, RoutedEventArgs e)
    {
        _roots.Clear();
        foreach (var node in HierarchyData.Make())
        {
            _roots.Add(node);
        }

        UpdateStatus();
    }

    private void Indent_Changed(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (Table is null)
        {
            return;
        }

        Table.RowIndentSize = e.NewValue;
        UpdateStatus();
    }

    private void UpdateStatus()
    {
        if (StatusText is null)
        {
            return;
        }

        var filter = FilterBox.Text?.Trim() ?? string.Empty;
        var group = GroupCombo.SelectedIndex switch { 1 => "Kind", 2 => "Owner", _ => "none" };
        var sort = ((ComboBoxItem)SortCombo.SelectedItem)?.Content?.ToString() ?? "Authored order";
        var selected = Table.SelectedItem as Node;

        StatusText.Text =
            $"roots={_roots.Count}  children materialized={Node.LoadCount}  " +
            $"hasChildren={(HasChildrenToggle.IsChecked == true ? "predicate" : "selector")}  " +
            $"group={group}  sort={sort}  " +
            $"filter={(filter.Length == 0 ? "none" : "\"" + filter + "\"")}  " +
            $"indent={Table.RowIndentSize:0}  selected={selected?.Name ?? "(none)"}";
    }
}
