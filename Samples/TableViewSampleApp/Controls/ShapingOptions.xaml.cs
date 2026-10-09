// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using SortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;
using TableViewSource = Microsoft.UI.Xaml.Controls.Tabular.TableViewSource;

namespace TableViewSampleApp.Controls;

/// <summary>
/// The Shaping section every table page carries: Flat / Grouped (plus the disabled hierarchy
/// modes), a group-key selector, Expand all and Collapse all. It reshapes the page's ONE
/// <see cref="TableViewSource"/> in place with GroupBy / ClearGroupBy (the control keeps the
/// selection across the reshape) and gates the controls that need the Grouped mode. The page
/// keeps its feature code and reacts to <see cref="ShapingApplying"/> / <see cref="ShapingApplied"/> through SamplePageBase.
/// </summary>
[ContentProperty(Name = nameof(GroupKeys))]
public sealed partial class ShapingOptions : UserControl
{
    private TableView? _table;
    private TableViewSource? _source;

    // Declaration order of the grouping and the table's sort axis (see SortOrdersGroups).
    private int _declarations;
    private int _groupDeclaredAt = -1;
    private int _sortDeclaredAt = -1;
    private TableViewColumn? _sortColumn;
    private Func<object?, string, object> _groupKeyOf = (row, key) => SampleShaping.KeyOf(row as Person, key);

    public ShapingOptions()
    {
        InitializeComponent();
        GroupKeys = new CallbackCollection<object>(InsertGroupKey, RemoveGroupKey);
    }

    /// <summary>Raised before GroupBy / ClearGroupBy.</summary>
    public event EventHandler<ShapingApplyingEventArgs>? ShapingApplying;

    /// <summary>Raised after the call returned, the selection was restored and the gating updated.</summary>
    public event EventHandler<ShapingAppliedEventArgs>? ShapingApplied;

    /// <summary>Expand all / Collapse all ran (which one, how long, and the Last action narration).</summary>
    public event EventHandler<ShapingActionEventArgs>? ActionPerformed;

    /// <summary>The fallback re-selected the item after a reshape the control did not re-anchor (see Apply).</summary>
    public event EventHandler? SelectionRestored;

    /// <summary>
    /// The table's sort changed (header click or API) and <see cref="SortOrdersGroups"/> is up to
    /// date. Raised after this control has recorded the sort, so a readout that depends on the
    /// sort-vs-GroupBy order is correct whichever Sorted handler ran first.
    /// </summary>
    public event EventHandler? SortStateChanged;

    /// <summary>The page's group keys: <c>&lt;ComboBoxItem Content="label" Tag="key"/&gt;</c> items.</summary>
    public IList<object> GroupKeys { get; }

    /// <summary>
    /// Prefix of the four AutomationIds: <c>{P}ShapingModeSelector</c>, <c>{P}GroupKeySelector</c>,
    /// <c>{P}ExpandAllGroupsButton</c>, <c>{P}CollapseAllGroupsButton</c>. Use the page's nav Tag.
    /// </summary>
    public string? AutomationIdPrefix
    {
        get => (string?)GetValue(AutomationIdPrefixProperty);
        set => SetValue(AutomationIdPrefixProperty, value);
    }

    public static readonly DependencyProperty AutomationIdPrefixProperty =
        DependencyProperty.Register(nameof(AutomationIdPrefix), typeof(string), typeof(ShapingOptions),
            new PropertyMetadata(null, (d, e) => ((ShapingOptions)d).ApplyAutomationIds((string?)e.NewValue ?? string.Empty)));

    /// <summary>The page-specific sentence under the Shaping heading.</summary>
    public string? Description
    {
        get => (string?)GetValue(DescriptionProperty);
        set => SetValue(DescriptionProperty, value);
    }

    public static readonly DependencyProperty DescriptionProperty =
        DependencyProperty.Register(nameof(Description), typeof(string), typeof(ShapingOptions),
            new PropertyMetadata(null, (d, e) => ((ShapingOptions)d).DescriptionText.Text = (string?)e.NewValue ?? string.Empty));

    /// <summary>"flat" (default) or "grouped": the mode applied when the page starts.</summary>
    public string InitialMode
    {
        get => (string)GetValue(InitialModeProperty);
        set => SetValue(InitialModeProperty, value);
    }

    public static readonly DependencyProperty InitialModeProperty =
        DependencyProperty.Register(nameof(InitialMode), typeof(string), typeof(ShapingOptions),
            new PropertyMetadata("flat", (d, e) => ((ShapingOptions)d).ShapingModeSelector.SelectedIndex = (string?)e.NewValue == "grouped" ? 1 : 0));

    /// <summary>False disables the Grouped mode too (a page with nothing to group).</summary>
    public bool IsGroupingAvailable
    {
        get => (bool)GetValue(IsGroupingAvailableProperty);
        set => SetValue(IsGroupingAvailableProperty, value);
    }

    public static readonly DependencyProperty IsGroupingAvailableProperty =
        DependencyProperty.Register(nameof(IsGroupingAvailable), typeof(bool), typeof(ShapingOptions),
            new PropertyMetadata(true, (d, e) => ((ShapingOptions)d).GroupedItem.IsEnabled = (bool)e.NewValue));

    /// <summary>False on the volume pages, which skip the re-selection fallback after a reshape.</summary>
    public bool RestoreSelection { get; set; } = true;

    /// <summary>
    /// Optional timer for the GroupBy / ClearGroupBy and Expand all / Collapse all calls: runs the
    /// call and returns its milliseconds (reported as ElapsedMilliseconds). Null: a plain Stopwatch
    /// around the call. Performance sets its Timed (settle the heap, call, synchronous layout).
    /// </summary>
    public Func<Action, long>? TimeCall { get; set; }

    public TableView? Table => _table;

    public TableViewSource? Source => _source;

    /// <summary>"flat" or "grouped". Written only after GroupBy / ClearGroupBy returned.</summary>
    public string AppliedMode { get; private set; } = "flat";

    /// <summary>The group key last applied (the ComboBoxItem Tag).</summary>
    public string AppliedKey { get; private set; } = string.Empty;

    public bool IsGrouped => AppliedMode == "grouped";

    /// <summary>
    /// True when the table's sort was declared before the current GroupBy: the source applies its
    /// verbs in declaration order, so that sort orders the rows and therefore the groups. A sort
    /// declared after GroupBy sorts within each group, and the groups keep source order. Every
    /// GroupBy (including <see cref="ReapplyIfGroupedOn"/>) is a new declaration; re-sorting the
    /// same column keeps its place, sorting another column declares a new sort.
    /// </summary>
    public bool SortOrdersGroups => _sortDeclaredAt >= 0 && _sortDeclaredAt < _groupDeclaredAt;

    /// <summary>"Flat" or "Grouped by Department".</summary>
    public string AppliedText => SampleShaping.ShapingText(IsGrouped, GroupKeySelector);

    public string SelectedMode => SampleShaping.SelectedTag(ShapingModeSelector, "flat");

    public string SelectedKey => SampleShaping.SelectedTag(GroupKeySelector, DefaultKey);

    public string SelectedKeyLabel => SampleShaping.Label(GroupKeySelector);

    // Unnamed in XAML on purpose (see ShapingOptions.xaml).
    private TextBlock DescriptionText => (TextBlock)((Panel)Content).Children[1];

    private ComboBoxItem GroupedItem => (ComboBoxItem)ShapingModeSelector.Items[1];

    private string DefaultKey => GroupKeys.Count > 0 && GroupKeys[0] is ComboBoxItem { Tag: string tag } ? tag : string.Empty;

    /// <summary>
    /// Connects the section to the page's table and its one source. Call once, after
    /// InitializeComponent, and pass the result to SamplePageBase.InitializeSample. Until then the
    /// selectors' SelectionChanged events (InitializeComponent, GroupKeys insertion) do nothing.
    /// </summary>
    /// <param name="groupKeyOf">GroupBy key for a row and a key Tag. Default: <see cref="SampleShaping.KeyOf"/> for Person.</param>
    public ShapingOptions Attach(TableView table, TableViewSource source, Func<object?, string, object>? groupKeyOf = null)
    {
        if (!ReferenceEquals(table, _table))
        {
            if (_table is not null)
            {
                _table.Sorted -= OnTableSorted;
            }

            table.Sorted += OnTableSorted;
        }

        if (!ReferenceEquals(source, _source))
        {
            _groupDeclaredAt = -1;
            _sortDeclaredAt = -1;
            _sortColumn = null;
        }

        _table = table;
        _source = source;
        if (groupKeyOf is not null)
        {
            _groupKeyOf = groupKeyOf;
        }

        AppliedKey = DefaultKey;
        return this;
    }

    // Called by SamplePageBase.InitializeSample once it listens to the events: grouped-first pages
    // apply their grouping here; flat pages only set the gating (no ClearGroupBy on a fresh source).
    internal void ApplyInitialState()
    {
        if (_source is not null && InitialMode == "grouped")
        {
            Apply(announce: false);
        }
        else
        {
            UpdateGating();
        }
    }

    /// <summary>Applies the selected mode and key to the source. <paramref name="announce"/>: narrate it as Last action.</summary>
    public void Apply(bool announce)
    {
        if (_table is null || _source is null)
        {
            return;
        }

        var mode = SelectedMode;
        var key = SelectedKey;
        var selected = RestoreSelection ? _table.SelectedItem : null;
        ShapingApplying?.Invoke(this, new ShapingApplyingEventArgs(mode, key));

        var source = _source;
        var groupKeyOf = _groupKeyOf;
        if (mode != "grouped")
        {
            mode = "flat";
        }

        var elapsedMilliseconds = Time(() => ApplyShaping(source, mode, row => groupKeyOf(row, key)));

        AppliedMode = mode;
        AppliedKey = key;
        _groupDeclaredAt = mode == "grouped" ? ++_declarations : -1;

        // The reshape raises a Reset and the control re-anchors the selection by item identity, so
        // normally there is nothing to do. Workaround: when the selected row's GROUP KEY changed
        // (an edit, then ReapplyIfGroupedOn) this build of the control drops the selection, so on
        // the next low-priority turn, once the projection is rebuilt, select the item again with
        // one index lookup. It never clears a selection the user made meanwhile.
        if (RestoreSelection && selected is not null && !ReferenceEquals(_table.SelectedItem, selected))
        {
            var table = _table;
            table.DispatcherQueue?.TryEnqueue(DispatcherQueuePriority.Low, () =>
            {
                if (table.IsLoaded && table.SelectedItem is null && SampleShaping.SelectItem(table, selected))
                {
                    SelectionRestored?.Invoke(this, EventArgs.Empty);
                }
            });
        }

        UpdateGating();
        var grouped = IsGrouped;
        var message = grouped
            ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SelectedKeyLabel)
            : "Shaping -> Flat";
        ShapingApplied?.Invoke(this, new ShapingAppliedEventArgs(mode, key, SelectedKeyLabel, announce, elapsedMilliseconds, AppliedText, message));
    }

    // <snippet Groups Sort CellEditing CellTemplating Density Filter GridLinesVisibility HeadersVisibility TextWrap Virtualization>
    // Every page reshapes its ONE TableViewSource in place: GroupBy to group, ClearGroupBy for flat.
    private static void ApplyShaping(TableViewSource source, string mode, Func<object?, object> groupKeyOf)
    {
        if (mode == "grouped")
        {
            // The key selector receives the ROW; the identity selector receives the KEY.
            source.GroupBy(row => groupKeyOf(row), SampleShaping.GroupIdentity);
        }
        else
        {
            // Hierarchical (tree) rows are not available in this release, so the "Hierarchy" and
            // "Grouped hierarchy" modes ship disabled. When hierarchy ships, it composes with the
            // GroupBy stage on this same source rather than replacing it.
            source.ClearGroupBy();
        }
    }
    // </snippet>

    /// <summary>
    /// Call after ANY write to the grouped-on property (an action or an in-cell edit): GroupBy does
    /// not move a row whose key changed, so the grouping is applied again. Returns true if it was.
    /// </summary>
    public bool ReapplyIfGroupedOn(string? propertyName)
    {
        if (IsGrouped && propertyName == AppliedKey)
        {
            Apply(announce: false);
            return true;
        }

        return false;
    }

    /// <summary>
    /// Call after the page re-declared the active sort on the source (<c>TableViewSource.Sort</c> with
    /// the column's path, to re-sort after a value change). The source then owns that axis as a NEW
    /// declaration, after any GroupBy, so a sort that used to order the groups would only sort within
    /// them. In that case GroupBy is applied again, after the sort, so the groups keep their order.
    /// Returns true if it was.
    /// </summary>
    public bool OnSortRedeclared()
    {
        var sortOrderedGroups = SortOrdersGroups;
        _sortDeclaredAt = ++_declarations;
        if (sortOrderedGroups && IsGrouped)
        {
            Apply(announce: false);
            return true;
        }

        return false;
    }

    public void ExpandAll()
    {
        var table = _table;
        var elapsedMilliseconds = Time(() => table?.ExpandAllGroups());
        ActionPerformed?.Invoke(this, new ShapingActionEventArgs(ShapingAction.ExpandedAll, elapsedMilliseconds, "Expanded all groups"));
    }

    public void CollapseAll()
    {
        var table = _table;
        var elapsedMilliseconds = Time(() => table?.CollapseAllGroups());
        ActionPerformed?.Invoke(this, new ShapingActionEventArgs(ShapingAction.CollapsedAll, elapsedMilliseconds, "Collapsed all groups"));
    }

    /// <summary>Disables the mode and key selectors while a long page action runs; false restores the gating.</summary>
    public void SetBusy(bool busy)
    {
        if (busy)
        {
            ShapingModeSelector.IsEnabled = false;
            GroupKeySelector.IsEnabled = false;
        }
        else
        {
            ShapingModeSelector.IsEnabled = true;
            UpdateGating();
        }
    }

    private long Time(Action call)
    {
        if (TimeCall is not null)
        {
            return TimeCall(call);
        }

        var stopwatch = Stopwatch.StartNew();
        call();
        stopwatch.Stop();
        return stopwatch.ElapsedMilliseconds;
    }

    private void OnTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        if (args.Column is null || args.Direction == SortDirection.None)
        {
            _sortDeclaredAt = -1;
            _sortColumn = null;
        }
        else if (_sortDeclaredAt < 0 || !ReferenceEquals(args.Column, _sortColumn))
        {
            // The control replaces the previous column's axis, so this is a new declaration.
            _sortDeclaredAt = ++_declarations;
            _sortColumn = args.Column;
        }

        SortStateChanged?.Invoke(this, EventArgs.Empty);
    }

    private void UpdateGating()
    {
        var grouped = IsGrouped;
        GroupKeySelector.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
    }

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => Apply(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => Apply(announce: true);

    private void OnExpandAllClick(object sender, RoutedEventArgs e) => ExpandAll();

    private void OnCollapseAllClick(object sender, RoutedEventArgs e) => CollapseAll();

    private void InsertGroupKey(int index, object item)
    {
        GroupKeySelector.Items.Insert(index, item);
        if (GroupKeySelector.SelectedIndex < 0)
        {
            GroupKeySelector.SelectedIndex = 0;
        }
    }

    private void RemoveGroupKey(int index) => GroupKeySelector.Items.RemoveAt(index);

    private void ApplyAutomationIds(string prefix)
    {
        AutomationProperties.SetAutomationId(ShapingModeSelector, prefix + "ShapingModeSelector");
        AutomationProperties.SetAutomationId(GroupKeySelector, prefix + "GroupKeySelector");
        AutomationProperties.SetAutomationId(ExpandAllButton, prefix + "ExpandAllGroupsButton");
        AutomationProperties.SetAutomationId(CollapseAllButton, prefix + "CollapseAllGroupsButton");
    }
}
