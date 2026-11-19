// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;

namespace TableViewSampleApp.Controls;

[ContentProperty(Name = nameof(GroupKeys))]
public sealed partial class ShapingOptions : UserControl
{
    private const string NoRelationReason = "These rows do not have one.";

    // ShapingModeSelector item order.
    private static readonly string[] Modes = { "flat", "grouped", "hierarchy", "groupedHierarchy" };

    private TableView? _table;
    private CollectionViewSource? _source;

    private Func<string, string> _groupPathOf = SampleShaping.GroupPathOf;
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

    // Header of the column that hosts the chevron and indent in the Hierarchy modes.
    public string HierarchyColumnHeader { get; set; } = "Name";

    // False when the page orders its own columns (a First column option): the Hierarchy modes then
    // leave the column order and widths alone.
    public bool ArrangesHierarchyColumn { get; set; } = true;

    public Func<Action, long>? TimeCall { get; set; }

    public TableView? Table => _table;

    public CollectionViewSource? Source => _source;

    public string AppliedMode { get; private set; } = "flat";

    public string AppliedKey { get; private set; } = string.Empty;

    public bool IsGrouped => AppliedMode is "grouped" or "groupedHierarchy";

    public bool IsHierarchical => AppliedMode is "hierarchy" or "groupedHierarchy";

    public ParentRelation? Relation => _relation;

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
    // GroupKeys insertion is ignored. groupPathOf maps a group-key Tag to the property path the
    // CollectionViewSource groups by. Person rows get the manager relation unless the page passes its
    // own or sets HierarchyUnavailableReason.
    public ShapingOptions Attach(TableView table, CollectionViewSource source, Func<string, string>? groupPathOf = null, ParentRelation? parentRelation = null)
    {
        if (!ReferenceEquals(table, _table))
        {
            if (_table is not null)
            {
                _table.Sorted -= OnTableSorted;
            }

            table.Sorted += OnTableSorted;
        }

        _table = table;
        _source = source;
        if (groupPathOf is not null)
        {
            _groupPathOf = groupPathOf;
        }

        _relation = string.IsNullOrEmpty(HierarchyUnavailableReason)
            ? parentRelation ?? _relation ?? (groupPathOf is null ? ParentRelation.Manager : null)
            : null;
        UpdateHierarchyItems();

        AppliedKey = DefaultKey;
        return this;
    }

    // Switches the parent relation. Under a Hierarchy mode the key paths are set again, which replaces
    // the relation and resets row expansion.
    public void SetRelation(ParentRelation relation)
    {
        _relation = relation;
        UpdateHierarchyItems();
        if (_parentDeclared)
        {
            _parentDeclared = false;
            Apply(announce: false);
        }
    }

    // Pages that reattach a new source re-apply the current mode with this.
    public bool IsShaped => AppliedMode != "flat";

    // Flat pages only set the gating: nothing to clear on a fresh source.
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
        ShapingApplying?.Invoke(this, new ShapingApplyingEventArgs(mode, key));

        var table = _table;
        var source = _source;
        var groupPath = _groupPathOf(key);
        var relation = _relation;
        if (Array.IndexOf(Modes, mode) < 0 || (relation is null && IsHierarchicalMode(mode)))
        {
            mode = "flat";
        }

        var hadParentBy = _parentDeclared;
        long elapsedMilliseconds;
        try
        {
            elapsedMilliseconds = Time(() => ApplyShaping(table, source, mode, groupPath, relation, hadParentBy));
        }
        catch (ArgumentException ex)
        {
            // The key paths reject duplicate keys and cycles and keep the previous projection.
            if (!hadParentBy)
            {
                table.ParentKeyPath = string.Empty;
            }

            RevertModeSelection();
            ShapingApplied?.Invoke(this, new ShapingAppliedEventArgs(AppliedMode, AppliedKey, SelectedKeyLabel, announce: true, 0, AppliedText,
                string.Format(CultureInfo.CurrentCulture, "Shaping -> {0} failed: {1}", ShapingText(mode, SelectedKeyLabel, relation?.Label), ex.Message)));
            return;
        }

        AppliedMode = mode;
        AppliedKey = key;
        _parentDeclared = IsHierarchicalMode(mode);
        if (ArrangesHierarchyColumn)
        {
            ArrangeHierarchyColumn(_parentDeclared);
        }

        UpdateGating();
        var message = "Shaping -> " + AppliedText;
        ShapingApplied?.Invoke(this, new ShapingAppliedEventArgs(mode, key, SelectedKeyLabel, announce, elapsedMilliseconds, AppliedText, message));
    }

    // ItemKeyPath and ParentKeyPath on the table nest the rows; GroupDescriptions on the
    // CollectionViewSource bucket them. Under both, each root keeps its subtree in its root's group.
    private static void ApplyShaping(TableView table, CollectionViewSource source, string mode, string groupPath, ParentRelation? relation, bool hadParentBy)
    {
        if (relation is not null && IsHierarchicalMode(mode))
        {
            if (!hadParentBy)
            {
                NestRows(table, relation);
            }
        }
        else if (hadParentBy)
        {
            table.ParentKeyPath = string.Empty;
        }

        if (mode is "grouped" or "groupedHierarchy")
        {
            GroupRows(source, groupPath);
        }
        else
        {
            source.GroupDescriptions.Clear();
        }

        ShowView(table, source);
    }

    // <snippet Hierarchy>
    // ItemKeyPath names each row's key and ParentKeyPath its parent's key. Changing either replaces
    // the relation and resets expansion, so set them once.
    private static void NestRows(TableView table, ParentRelation relation)
    {
        table.ItemKeyPath = relation.KeyPath;
        table.ParentKeyPath = relation.ParentKeyPath;
    }
    // </snippet>

    // <snippet Groups>
    // A PropertyGroupDescription names the property to group by. Grouping reads the value when the
    // view refreshes, so call Refresh() after a row's grouped-on value changes.
    private static void GroupRows(CollectionViewSource source, string groupPath)
    {
        var description = new PropertyGroupDescription(groupPath);
        if (source.GroupDescriptions.Count == 1)
        {
            source.GroupDescriptions[0] = description;
        }
        else
        {
            source.GroupDescriptions.Clear();
            source.GroupDescriptions.Add(description);
        }
    }

    // Turning grouping on or off gives the CollectionViewSource a new View, so show the current one.
    private static void ShowView(TableView table, CollectionViewSource source)
    {
        if (!ReferenceEquals(table.ItemsSource, source.View))
        {
            table.ItemsSource = source.View;
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
        if (_source is null || _table is null)
        {
            return;
        }

        if (_parentDeclared)
        {
            _table.ParentKeyPath = string.Empty;
            _parentDeclared = false;
        }

        _source.GroupDescriptions.Clear();
        ShowView(_table, _source);
    }

    // Grouping does not move a row whose key changed: call after any write to the grouped-on property.
    public bool ReapplyIfGroupedOn(string? propertyName)
    {
        if (IsGrouped && _source is not null && (propertyName == AppliedKey || propertyName == _groupPathOf(AppliedKey)))
        {
            _source.Refresh();
            return true;
        }

        return false;
    }

    // <snippet Groups Hierarchy>
    // Under grouping plus key paths, ExpandAllGroups affects only the group headers and ExpandAllRows only
    // the tree rows, so Grouped hierarchy calls both.
    private static void ExpandEvery(TableView table, bool grouped, bool hierarchical)
    {
        if (grouped)
        {
            table.ExpandAllGroups();
        }

        if (hierarchical)
        {
            table.ExpandAllRows();
        }
    }

    private static void CollapseEvery(TableView table, bool grouped, bool hierarchical)
    {
        if (grouped)
        {
            table.CollapseAllGroups();
        }

        if (hierarchical)
        {
            table.CollapseAllRows();
        }
    }
    // </snippet>

    public void ExpandAll()
    {
        var table = _table;
        var grouped = IsGrouped;
        var hierarchical = IsHierarchical;
        var elapsedMilliseconds = Time(() =>
        {
            if (table is not null)
            {
                ExpandEvery(table, grouped, hierarchical);
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
            if (table is not null)
            {
                CollapseEvery(table, grouped, hierarchical);
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
        var unavailable = reason is null ? null : "Hierarchy and Grouped hierarchy need a parent relation. " + reason;
        AutomationProperties.SetHelpText(HierarchyItem, unavailable ?? string.Format(CultureInfo.CurrentCulture, "Nests the rows by {0} (TableView.ItemKeyPath and ParentKeyPath).", _relation!.Label));
        AutomationProperties.SetHelpText(GroupedHierarchyItem, unavailable ?? groupedReason ?? string.Format(CultureInfo.CurrentCulture, "Groups the root rows by the group key; each root keeps its {0} subtree.", _relation!.Label));
        ToolTipService.SetToolTip(HierarchyItem, unavailable);
        ToolTipService.SetToolTip(GroupedHierarchyItem, unavailable ?? groupedReason);
        CaptionText.Text = reason is null
            ? string.Format(CultureInfo.CurrentCulture, "Group key needs a grouped mode; Expand all and Collapse all need any mode but Flat. The Hierarchy modes nest the rows by {0} in the {1} column, shown first; nested rows start collapsed.", _relation!.Label, HierarchyColumnHeader)
            : unavailable + " Group key needs a grouped mode; Expand all and Collapse all need any mode but Flat.";
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
