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
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;

namespace TableViewSampleApp.Controls;

[ContentProperty(Name = nameof(GroupKeys))]
public sealed partial class ShapingOptions : UserControl
{
    // ShapingModeSelector item order.
    private static readonly string[] Modes = { "flat", "grouped" };

    private TableView? _table;
    private CollectionViewSource? _source;

    private Func<string, string> _groupPathOf = SampleShaping.GroupPathOf;

    public ShapingOptions()
    {
        InitializeComponent();
        AutomationProperties.SetName(this, "Shaping");
        GroupKeys = new CallbackCollection<object>(InsertGroupKey, RemoveGroupKey);
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

    public bool IsGroupingAvailable
    {
        get => (bool)GetValue(IsGroupingAvailableProperty);
        set => SetValue(IsGroupingAvailableProperty, value);
    }

    public static readonly DependencyProperty IsGroupingAvailableProperty =
        DependencyProperty.Register(nameof(IsGroupingAvailable), typeof(bool), typeof(ShapingOptions),
            new PropertyMetadata(true, (d, e) => ((ShapingOptions)d).GroupedItem.IsEnabled = (bool)e.NewValue));

    public Func<Action, long>? TimeCall { get; set; }

    public TableView? Table => _table;

    public CollectionViewSource? Source => _source;

    public string AppliedMode { get; private set; } = "flat";

    public string AppliedKey { get; private set; } = string.Empty;

    public bool IsGrouped => AppliedMode is "grouped";

    public string AppliedText => ShapingText(AppliedMode, SelectedKeyLabel);

    public string SelectedMode => SampleShaping.SelectedTag(ShapingModeSelector, "flat");

    public string SelectedKey => SampleShaping.SelectedTag(GroupKeySelector, DefaultKey);

    public string SelectedKeyLabel => SampleShaping.Label(GroupKeySelector);

    // Unnamed in XAML on purpose (see ShapingOptions.xaml).
    private TextBlock DescriptionText => (TextBlock)((Panel)Content).Children[1];

    private ComboBoxItem GroupedItem => (ComboBoxItem)ShapingModeSelector.Items[1];

    private string DefaultKey => GroupKeys.Count > 0 && GroupKeys[0] is ComboBoxItem { Tag: string tag } ? tag : string.Empty;

    // Call once after InitializeComponent; until then SelectionChanged from InitializeComponent and
    // GroupKeys insertion is ignored. groupPathOf maps a group-key Tag to the property path the
    // CollectionViewSource groups by.
    public ShapingOptions Attach(TableView table, CollectionViewSource source, Func<string, string>? groupPathOf = null)
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

        AppliedKey = DefaultKey;
        return this;
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
        if (_table is null || _source is null)
        {
            return;
        }

        var mode = SelectedMode;
        var key = SelectedKey;
        ShapingApplying?.Invoke(this, new ShapingApplyingEventArgs(mode, key));

        var table = _table;
        var source = _source;
        var groupPath = _groupPathOf(key);
        if (Array.IndexOf(Modes, mode) < 0)
        {
            mode = "flat";
        }

        var elapsedMilliseconds = Time(() => ApplyShaping(table, source, mode, groupPath));

        AppliedMode = mode;
        AppliedKey = key;

        UpdateGating();
        var message = "Shaping -> " + AppliedText;
        ShapingApplied?.Invoke(this, new ShapingAppliedEventArgs(mode, key, SelectedKeyLabel, announce, elapsedMilliseconds, AppliedText, message));
    }

    // GroupDescriptions on the CollectionViewSource bucket the rows.
    private static void ApplyShaping(TableView table, CollectionViewSource source, string mode, string groupPath)
    {
        if (mode is "grouped")
        {
            GroupRows(source, groupPath);
        }
        else
        {
            source.GroupDescriptions.Clear();
        }

        ShowView(table, source);
    }

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

    private static string ShapingText(string mode, string keyLabel) => mode switch
    {
        "grouped" => string.Format(CultureInfo.CurrentCulture, "Grouped by {0}", keyLabel),
        _ => "Flat",
    };

    // Clears the grouping, e.g. before refilling the collection row by row (every Add reshapes a grouped
    // projection); Apply restores the selected mode afterwards.
    public void Detach()
    {
        if (_source is null || _table is null)
        {
            return;
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

    // <snippet Groups>
    // ExpandAllGroups and CollapseAllGroups set every group, and groups that appear later start that way too.
    private static void ExpandEvery(TableView table) => table.ExpandAllGroups();

    private static void CollapseEvery(TableView table) => table.CollapseAllGroups();
    // </snippet>

    public void ExpandAll()
    {
        var table = _table;
        var elapsedMilliseconds = Time(() =>
        {
            if (table is not null)
            {
                ExpandEvery(table);
            }
        });
        ActionPerformed?.Invoke(this, new ShapingActionEventArgs(ShapingAction.ExpandedAll, elapsedMilliseconds, "Expanded all groups"));
    }

    public void CollapseAll()
    {
        var table = _table;
        var elapsedMilliseconds = Time(() =>
        {
            if (table is not null)
            {
                CollapseEvery(table);
            }
        });
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
        SortStateChanged?.Invoke(this, EventArgs.Empty);
    }

    private void UpdateGating()
    {
        GroupKeySelector.IsEnabled = IsGrouped;
        ExpandAllButton.IsEnabled = IsGrouped;
        CollapseAllButton.IsEnabled = IsGrouped;
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
