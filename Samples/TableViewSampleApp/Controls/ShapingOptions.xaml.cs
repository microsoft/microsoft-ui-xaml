// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
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

[ContentProperty(Name = nameof(GroupKeys))]
public sealed partial class ShapingOptions : UserControl
{
    private const string NoRelationReason = "These rows have no parent relation.";

    // ShapingModeSelector item order.
    private static readonly string[] Modes = { "flat", "grouped", "hierarchy", "groupedHierarchy" };

    private TableView? _table;
    private TableViewSource? _source;

    private int _declarations;
    private int _groupDeclaredAt = -1;
    private int _sortDeclaredAt = -1;
    private TableViewColumn? _sortColumn;
    private Func<object?, string, object> _groupKeyOf = (row, key) => SampleShaping.KeyOf(row as Person, key);
    private ParentRelation? _relation;
    private bool _parentDeclared;
    private bool _revertingMode;
    private TableViewColumn? _treeColumn;
    private readonly List<TableViewColumn> _movedLeadColumns = new();
    private GridLength _treeColumnWidth;
    private double _treeColumnMinWidth;

    // Chevron (24) plus about five 16-px levels, and the group header offset under Grouped hierarchy.
    private const double HierarchyIndentReserve = 112;

    public ShapingOptions()
    {
        InitializeComponent();
        AutomationProperties.SetName(this, "Shaping");
        GroupKeys = new CallbackCollection<object>(InsertGroupKey, RemoveGroupKey);
        UpdateHierarchyItems();
    }

    // Report the section as a Group like RailSection does; the inner StackPanel has no peer.
    protected override AutomationPeer OnCreateAutomationPeer() =>
        new RailSection.GroupAutomationPeer(this, nameof(ShapingOptions));

    public event EventHandler<ShapingApplyingEventArgs>? ShapingApplying;

    public event EventHandler<ShapingAppliedEventArgs>? ShapingApplied;

    public event EventHandler<ShapingActionEventArgs>? ActionPerformed;

    public event EventHandler? SelectionRestored;

    public event EventHandler? SortStateChanged;

    public IList<object> GroupKeys { get; }

    public string? AutomationIdPrefix
    {
        get => (string?)GetValue(AutomationIdPrefixProperty);
        set => SetValue(AutomationIdPrefixProperty, value);
    }

    public static readonly DependencyProperty AutomationIdPrefixProperty =
        DependencyProperty.Register(nameof(AutomationIdPrefix), typeof(string), typeof(ShapingOptions),
            new PropertyMetadata(null, (d, e) => ((ShapingOptions)d).ApplyAutomationIds((string?)e.NewValue ?? string.Empty)));

    public string? Description
    {
        get => (string?)GetValue(DescriptionProperty);
        set => SetValue(DescriptionProperty, value);
    }

    public static readonly DependencyProperty DescriptionProperty =
        DependencyProperty.Register(nameof(Description), typeof(string), typeof(ShapingOptions),
            new PropertyMetadata(null, (d, e) => ((ShapingOptions)d).DescriptionText.Text = (string?)e.NewValue ?? string.Empty));

    public string InitialMode
    {
        get => (string)GetValue(InitialModeProperty);
        set => SetValue(InitialModeProperty, value);
    }

    public static readonly DependencyProperty InitialModeProperty =
        DependencyProperty.Register(nameof(InitialMode), typeof(string), typeof(ShapingOptions),
            new PropertyMetadata("flat", (d, e) => ((ShapingOptions)d).ShapingModeSelector.SelectedIndex = Array.IndexOf(Modes, (string?)e.NewValue) is var i and >= 0 ? i : 0));

    // Why this page's rows cannot nest (shown on the disabled Hierarchy items). Setting it opts the page
    // out of the default Person manager relation.
    public string? HierarchyUnavailableReason
    {
        get => (string?)GetValue(HierarchyUnavailableReasonProperty);
        set => SetValue(HierarchyUnavailableReasonProperty, value);
    }

    public static readonly DependencyProperty HierarchyUnavailableReasonProperty =
        DependencyProperty.Register(nameof(HierarchyUnavailableReason), typeof(string), typeof(ShapingOptions),
            new PropertyMetadata(null, (d, e) => ((ShapingOptions)d).UpdateHierarchyItems()));

    public bool IsGroupingAvailable
    {
        get => (bool)GetValue(IsGroupingAvailableProperty);
        set => SetValue(IsGroupingAvailableProperty, value);
    }

    public static readonly DependencyProperty IsGroupingAvailableProperty =
        DependencyProperty.Register(nameof(IsGroupingAvailable), typeof(bool), typeof(ShapingOptions),
            new PropertyMetadata(true, (d, e) =>
            {
                var options = (ShapingOptions)d;
                options.GroupedItem.IsEnabled = (bool)e.NewValue;
                options.UpdateHierarchyItems();
            }));

    public bool RestoreSelection { get; set; } = true;

    // Header of the column that hosts the chevron and indent in the Hierarchy modes.
    public string HierarchyColumnHeader { get; set; } = "Name";

    public Func<Action, long>? TimeCall { get; set; }

    public TableView? Table => _table;

    public TableViewSource? Source => _source;

    public string AppliedMode { get; private set; } = "flat";

    public string AppliedKey { get; private set; } = string.Empty;

    public bool IsGrouped => AppliedMode is "grouped" or "groupedHierarchy";

    public bool IsHierarchical => AppliedMode is "hierarchy" or "groupedHierarchy";

    public ParentRelation? Relation => _relation;

    // TableViewSource applies its verbs in declaration order: a sort declared before GroupBy orders the
    // groups; one declared after it only sorts within each group.
    public bool SortOrdersGroups => _sortDeclaredAt >= 0 && _sortDeclaredAt < _groupDeclaredAt;

    public string AppliedText => ShapingText(AppliedMode, SelectedKeyLabel, _relation?.Label);

    public string SelectedMode => SampleShaping.SelectedTag(ShapingModeSelector, "flat");

    public string SelectedKey => SampleShaping.SelectedTag(GroupKeySelector, DefaultKey);

    public string SelectedKeyLabel => SampleShaping.Label(GroupKeySelector);

    // Unnamed in XAML on purpose (see ShapingOptions.xaml).
    private TextBlock DescriptionText => (TextBlock)((Panel)Content).Children[1];

    private ComboBoxItem GroupedItem => (ComboBoxItem)ShapingModeSelector.Items[1];

    private ComboBoxItem HierarchyItem => (ComboBoxItem)ShapingModeSelector.Items[2];

    private ComboBoxItem GroupedHierarchyItem => (ComboBoxItem)ShapingModeSelector.Items[3];

    private TextBlock CaptionText => (TextBlock)((Panel)Content).Children[^1];

    private string DefaultKey => GroupKeys.Count > 0 && GroupKeys[0] is ComboBoxItem { Tag: string tag } ? tag : string.Empty;

    // Call once after InitializeComponent; until then SelectionChanged from InitializeComponent and
    // GroupKeys insertion is ignored. Person rows get the manager relation unless the page passes its
    // own or sets HierarchyUnavailableReason.
    public ShapingOptions Attach(TableView table, TableViewSource source, Func<object?, string, object>? groupKeyOf = null, ParentRelation? parentRelation = null)
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
            _parentDeclared = false;
        }

        _table = table;
        _source = source;
        if (groupKeyOf is not null)
        {
            _groupKeyOf = groupKeyOf;
        }

        _relation = string.IsNullOrEmpty(HierarchyUnavailableReason)
            ? parentRelation ?? _relation ?? (groupKeyOf is null ? ParentRelation.Manager : null)
            : null;
        UpdateHierarchyItems();

        AppliedKey = DefaultKey;
        return this;
    }

    // Pages that reattach a new source re-apply the current mode with this.
    public bool IsShaped => AppliedMode != "flat";

    // Flat pages only set the gating: no ClearGroupBy on a fresh source.
    internal void ApplyInitialState()
    {
        if (_source is not null && InitialMode != "flat")
        {
            Apply(announce: false);
        }
        else
        {
            UpdateGating();
        }
    }

    public void Apply(bool announce)
    {
        if (_table is null || _source is null || _revertingMode)
        {
            return;
        }

        var mode = SelectedMode;
        var key = SelectedKey;
        var selected = RestoreSelection ? _table.SelectedItem : null;
        ShapingApplying?.Invoke(this, new ShapingApplyingEventArgs(mode, key));

        var source = _source;
        var groupKeyOf = _groupKeyOf;
        var relation = _relation;
        if (Array.IndexOf(Modes, mode) < 0 || (relation is null && IsHierarchicalMode(mode)))
        {
            mode = "flat";
        }

        var hadParentBy = _parentDeclared;
        long elapsedMilliseconds;
        try
        {
            elapsedMilliseconds = Time(() => ApplyShaping(source, mode, row => groupKeyOf(row, key), relation, hadParentBy));
        }
        catch (ArgumentException ex)
        {
            // ParentBy rejects duplicate keys and cycles and keeps the previous projection.
            RevertModeSelection();
            ShapingApplied?.Invoke(this, new ShapingAppliedEventArgs(AppliedMode, AppliedKey, SelectedKeyLabel, announce: true, 0, AppliedText,
                string.Format(CultureInfo.CurrentCulture, "Shaping -> {0} failed: {1}", ShapingText(mode, SelectedKeyLabel, relation?.Label), ex.Message)));
            return;
        }

        AppliedMode = mode;
        AppliedKey = key;
        _parentDeclared = IsHierarchicalMode(mode);
        ArrangeHierarchyColumn(_parentDeclared);
        _groupDeclaredAt = IsGrouped ? ++_declarations : -1;

        // Workaround for AB#64548351: the control drops the selection when a row's group key changes.
        // Workaround: the control re-anchors the selection after a reshape, except when the selected row's
        // group key changed; then re-select it once the projection is rebuilt. A row inside a collapsed
        // subtree has no index to select, so it stays unselected.
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
        var message = "Shaping -> " + AppliedText;
        ShapingApplied?.Invoke(this, new ShapingAppliedEventArgs(mode, key, SelectedKeyLabel, announce, elapsedMilliseconds, AppliedText, message));
    }

    // <snippet Groups Sort CellEditing CellTemplating Density Filter GridLinesVisibility HeadersVisibility TextWrap Virtualization>
    // Reshape the ONE TableViewSource in place. GroupBy/ClearGroupBy is the group axis and
    // ParentBy/ClearParentBy the hierarchy axis; together, GroupBy buckets the roots and each root
    // keeps its subtree.
    private static void ApplyShaping(TableViewSource source, string mode, Func<object?, object> groupKeyOf, ParentRelation? relation, bool hadParentBy)
    {
        if (relation is not null && IsHierarchicalMode(mode))
        {
            // A second ParentBy replaces the relation and resets expansion, so declare it once.
            if (!hadParentBy)
            {
                source.ParentBy(row => relation.KeyOf(row), row => relation.ParentKeyOf(row));
            }
        }
        else if (hadParentBy)
        {
            source.ClearParentBy();
        }

        if (mode is "grouped" or "groupedHierarchy")
        {
            // The key selector receives the ROW; the identity selector receives the KEY.
            source.GroupBy(row => groupKeyOf(row), SampleShaping.GroupIdentity);
        }
        else
        {
            source.ClearGroupBy();
        }
    }
    // </snippet>

    private static bool IsHierarchicalMode(string mode) => mode is "hierarchy" or "groupedHierarchy";

    // TableView draws the chevron and the per-level indent in the first visible column. In the
    // Hierarchy modes the HierarchyColumnHeader column leads (narrow lead columns such as Photo or #
    // move after it) and is widened by the indent it hosts; Flat and Grouped restore both.
    private void ArrangeHierarchyColumn(bool hierarchical)
    {
        if (_table is null)
        {
            return;
        }

        var columns = _table.Columns;
        if (hierarchical)
        {
            if (_treeColumn is not null)
            {
                return;
            }

            TableViewColumn? tree = null;
            foreach (var column in columns)
            {
                if (column.Visibility == Visibility.Visible && Equals(column.Header, HierarchyColumnHeader))
                {
                    tree = column;
                    break;
                }
            }

            if (tree is null)
            {
                return;
            }

            var leading = new List<TableViewColumn>();
            for (var i = columns.IndexOf(tree) - 1; i >= 0; i--)
            {
                leading.Insert(0, columns[i]);
                columns.RemoveAt(i);
            }

            for (var i = 0; i < leading.Count; i++)
            {
                columns.Insert(columns.IndexOf(tree) + 1 + i, leading[i]);
            }

            _treeColumn = tree;
            _movedLeadColumns.Clear();
            _movedLeadColumns.AddRange(leading);
            _treeColumnWidth = tree.Width;
            _treeColumnMinWidth = tree.MinWidth;
            if (tree.Width.IsAbsolute)
            {
                tree.Width = new GridLength(tree.Width.Value + HierarchyIndentReserve);
            }
            else
            {
                tree.MinWidth = Math.Max(tree.MinWidth, 0) + HierarchyIndentReserve;
            }

            return;
        }

        if (_treeColumn is not { } treeColumn)
        {
            return;
        }

        _treeColumn = null;
        foreach (var column in _movedLeadColumns)
        {
            var at = columns.IndexOf(treeColumn);
            var from = columns.IndexOf(column);
            if (at < 0 || from < 0 || from < at)
            {
                continue;
            }

            columns.RemoveAt(from);
            columns.Insert(at, column);
        }

        _movedLeadColumns.Clear();
        treeColumn.Width = _treeColumnWidth;
        treeColumn.MinWidth = _treeColumnMinWidth;
    }

    private static string ShapingText(string mode, string keyLabel, string? relationLabel) => mode switch
    {
        "grouped" => string.Format(CultureInfo.CurrentCulture, "Grouped by {0}", keyLabel),
        "hierarchy" => string.Format(CultureInfo.CurrentCulture, "Hierarchy by {0}", relationLabel),
        "groupedHierarchy" => string.Format(CultureInfo.CurrentCulture, "Grouped by {0}, hierarchy by {1}", keyLabel, relationLabel),
        _ => "Flat",
    };

    // Clears both axes, e.g. before refilling the collection row by row (every Add reshapes a grouped
    // or hierarchical projection); Apply restores the selected mode afterwards.
    public void Detach()
    {
        if (_source is null)
        {
            return;
        }

        if (_parentDeclared)
        {
            _source.ClearParentBy();
            _parentDeclared = false;
        }

        _source.ClearGroupBy();
    }

    // GroupBy does not move a row whose key changed: call after any write to the grouped-on property.
    public bool ReapplyIfGroupedOn(string? propertyName)
    {
        if (IsGrouped && propertyName == AppliedKey)
        {
            Apply(announce: false);
            return true;
        }

        return false;
    }

    // Re-declaring the sort makes it a new declaration after GroupBy, so it would only sort within
    // groups; GroupBy is applied again after it so the groups keep their order.
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

    // Under GroupBy + ParentBy, ExpandAllGroups affects only the group headers and ExpandAllRows only
    // the tree rows, so Grouped hierarchy calls both.
    public void ExpandAll()
    {
        var table = _table;
        var grouped = IsGrouped;
        var hierarchical = IsHierarchical;
        var elapsedMilliseconds = Time(() =>
        {
            if (grouped)
            {
                table?.ExpandAllGroups();
            }

            if (hierarchical)
            {
                table?.ExpandAllRows();
            }
        });
        ActionPerformed?.Invoke(this, new ShapingActionEventArgs(ShapingAction.ExpandedAll, elapsedMilliseconds, "Expanded all " + ExpandTargets(grouped, hierarchical)));
    }

    public void CollapseAll()
    {
        var table = _table;
        var grouped = IsGrouped;
        var hierarchical = IsHierarchical;
        var elapsedMilliseconds = Time(() =>
        {
            if (grouped)
            {
                table?.CollapseAllGroups();
            }

            if (hierarchical)
            {
                table?.CollapseAllRows();
            }
        });
        ActionPerformed?.Invoke(this, new ShapingActionEventArgs(ShapingAction.CollapsedAll, elapsedMilliseconds, "Collapsed all " + ExpandTargets(grouped, hierarchical)));
    }

    private static string ExpandTargets(bool grouped, bool hierarchical) =>
        grouped && hierarchical ? "groups and rows" : hierarchical ? "rows" : "groups";

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
        GroupKeySelector.IsEnabled = IsGrouped;
        var shaped = IsShaped;
        ExpandAllButton.IsEnabled = shaped;
        CollapseAllButton.IsEnabled = shaped;
        var (expand, collapse) = (IsGrouped, IsHierarchical) switch
        {
            (true, true) => ("Calls TableView.ExpandAllGroups() and ExpandAllRows().", "Calls TableView.CollapseAllGroups() and CollapseAllRows()."),
            (false, true) => ("Calls TableView.ExpandAllRows().", "Calls TableView.CollapseAllRows()."),
            _ => ("Calls TableView.ExpandAllGroups().", "Calls TableView.CollapseAllGroups()."),
        };
        ToolTipService.SetToolTip(ExpandAllButton, expand);
        ToolTipService.SetToolTip(CollapseAllButton, collapse);
    }

    private void UpdateHierarchyItems()
    {
        var reason = !string.IsNullOrEmpty(HierarchyUnavailableReason) ? HierarchyUnavailableReason
            : _relation is null ? NoRelationReason
            : null;
        HierarchyItem.IsEnabled = reason is null;
        GroupedHierarchyItem.IsEnabled = reason is null && IsGroupingAvailable;
        var groupedReason = reason ?? (IsGroupingAvailable ? null : "Grouping is not available on this page.");
        AutomationProperties.SetHelpText(HierarchyItem, reason ?? string.Format(CultureInfo.CurrentCulture, "Nests the rows by {0} (TableViewSource.ParentBy).", _relation!.Label));
        AutomationProperties.SetHelpText(GroupedHierarchyItem, groupedReason ?? string.Format(CultureInfo.CurrentCulture, "Groups the root rows by the group key; each root keeps its {0} subtree.", _relation!.Label));
        CaptionText.Text = reason is null
            ? string.Format(CultureInfo.CurrentCulture, "Group key needs a grouped mode; Expand all and Collapse all need any mode but Flat. The Hierarchy modes nest the rows by {0} in the {1} column, shown first; nested rows start collapsed.", _relation!.Label, HierarchyColumnHeader)
            : "Group key needs a grouped mode; Expand all and Collapse all need any mode but Flat. Hierarchy: " + reason;
    }

    private void RevertModeSelection()
    {
        _revertingMode = true;
        try
        {
            ShapingModeSelector.SelectedIndex = Math.Max(0, Array.IndexOf(Modes, AppliedMode));
        }
        finally
        {
            _revertingMode = false;
        }
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
