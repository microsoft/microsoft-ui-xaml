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

[ContentProperty(Name = nameof(GroupKeys))]
public sealed partial class ShapingOptions : UserControl
{
    private TableView? _table;
    private TableViewSource? _source;

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
            new PropertyMetadata("flat", (d, e) => ((ShapingOptions)d).ShapingModeSelector.SelectedIndex = (string?)e.NewValue == "grouped" ? 1 : 0));

    public bool IsGroupingAvailable
    {
        get => (bool)GetValue(IsGroupingAvailableProperty);
        set => SetValue(IsGroupingAvailableProperty, value);
    }

    public static readonly DependencyProperty IsGroupingAvailableProperty =
        DependencyProperty.Register(nameof(IsGroupingAvailable), typeof(bool), typeof(ShapingOptions),
            new PropertyMetadata(true, (d, e) => ((ShapingOptions)d).GroupedItem.IsEnabled = (bool)e.NewValue));

    public bool RestoreSelection { get; set; } = true;

    public Func<Action, long>? TimeCall { get; set; }

    public TableView? Table => _table;

    public TableViewSource? Source => _source;

    public string AppliedMode { get; private set; } = "flat";

    public string AppliedKey { get; private set; } = string.Empty;

    public bool IsGrouped => AppliedMode == "grouped";

    // TableViewSource applies its verbs in declaration order: a sort declared before GroupBy orders the
    // groups; one declared after it only sorts within each group.
    public bool SortOrdersGroups => _sortDeclaredAt >= 0 && _sortDeclaredAt < _groupDeclaredAt;

    public string AppliedText => SampleShaping.ShapingText(IsGrouped, GroupKeySelector);

    public string SelectedMode => SampleShaping.SelectedTag(ShapingModeSelector, "flat");

    public string SelectedKey => SampleShaping.SelectedTag(GroupKeySelector, DefaultKey);

    public string SelectedKeyLabel => SampleShaping.Label(GroupKeySelector);

    // Unnamed in XAML on purpose (see ShapingOptions.xaml).
    private TextBlock DescriptionText => (TextBlock)((Panel)Content).Children[1];

    private ComboBoxItem GroupedItem => (ComboBoxItem)ShapingModeSelector.Items[1];

    private string DefaultKey => GroupKeys.Count > 0 && GroupKeys[0] is ComboBoxItem { Tag: string tag } ? tag : string.Empty;

    // Call once after InitializeComponent; until then SelectionChanged from InitializeComponent and
    // GroupKeys insertion is ignored.
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

    // Flat pages only set the gating: no ClearGroupBy on a fresh source.
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

        // Workaround: the control re-anchors the selection after a reshape, except when the selected row's
        // group key changed; then re-select it once the projection is rebuilt.
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
    // Reshape the ONE TableViewSource in place: GroupBy to group, ClearGroupBy for flat.
    private static void ApplyShaping(TableViewSource source, string mode, Func<object?, object> groupKeyOf)
    {
        if (mode == "grouped")
        {
            // The key selector receives the ROW; the identity selector receives the KEY.
            source.GroupBy(row => groupKeyOf(row), SampleShaping.GroupIdentity);
        }
        else
        {
            // Hierarchy is not available in this release; when it ships it composes with GroupBy on this source.
            source.ClearGroupBy();
        }
    }
    // </snippet>

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
