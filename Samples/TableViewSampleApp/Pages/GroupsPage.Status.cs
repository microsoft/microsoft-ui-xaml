// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Controls;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using Windows.System;

namespace TableViewSampleApp.Pages;

public sealed partial class GroupsPage
{
    private const string AllExpanded = "All expanded";
    private const string AllCollapsed = "All collapsed";

    // Group identities (SampleShaping.GroupIdentity) the page has collapsed. The control has no
    // public per-group state query, so the page records every expansion change it makes or sees.
    private readonly HashSet<string> _collapsedGroups = new();

    // The rows ItemsRepeater, which hosts the group headers; found on first use.
    private FrameworkElement? _rowsHost;
    private bool _reconcileQueued;

    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        EmptyToggleButton.Content = People.Count > 0 ? "Remove all rows" : "Restore all rows";

        if (!IsGrouped)
        {
            GroupsText.Text = "(grouping off)";
            ExpansionText.Text = "(grouping off)";
            return;
        }

        var key = AppliedGroupKey;
        var groups = InViewOrder()
            .GroupBy(GroupInView(key))
            .Select(g => string.Format(CultureInfo.CurrentCulture, "{0} {1:N0}", g.Key, g.Count()))
            .ToList();
        GroupsText.Text = groups.Count == 0
            ? "0 (empty source)"
            : string.Format(CultureInfo.CurrentCulture, "{0:N0}: {1}", groups.Count, string.Join(", ", groups));
        ExpansionText.Text = ExpansionSummary();
    }

    // Under Grouped hierarchy GroupBy buckets only the roots: a nested row is in its root's group.
    private Func<Person, object> GroupInView(string key)
    {
        var rowOf = GroupingRowOf(People);
        return person => SampleShaping.KeyOf(rowOf(person), key);
    }

    private IEnumerable<Person> InViewOrder() =>
        SampleShaping.InViewOrder(PeopleTable, People, SortKey, IsGrouped ? GroupInView(AppliedGroupKey) : null, SortOrdersGroups);

    private static IComparable? SortKey(Person person, string path) => path switch
    {
        nameof(Person.FullName) => person.FullName,
        nameof(Person.Department) => person.Department,
        nameof(Person.IsActive) => person.IsActive,
        nameof(Person.JoinDate) => person.JoinDate,
        nameof(Person.Salary) => person.Salary,
        _ => null,
    };

    private HashSet<string> CurrentGroupIdentities()
    {
        var key = AppliedGroupKey;
        var groupOf = GroupInView(key);
        return People.Select(p => SampleShaping.GroupIdentity(groupOf(p))).ToHashSet();
    }

    private string ExpansionSummary()
    {
        var groups = CurrentGroupIdentities();
        if (groups.Count == 0)
        {
            return "(no groups)";
        }

        var collapsed = groups.Count(_collapsedGroups.Contains);
        return collapsed == 0 ? AllExpanded
            : collapsed == groups.Count ? AllCollapsed
            : string.Format(CultureInfo.CurrentCulture, "Mixed: {0:N0} of {1:N0} collapsed", collapsed, groups.Count);
    }

    protected override void OnShapingAction(ShapingActionEventArgs e)
    {
        _collapsedGroups.Clear();
        if (e.Action == ShapingAction.CollapsedAll)
        {
            _collapsedGroups.UnionWith(CurrentGroupIdentities());
        }
    }

    // Header click or Enter/Space toggles the group; Right and Left expand and collapse it (reversed in RTL).
    private void OnTableTapped(object sender, TappedRoutedEventArgs e) => NoteHeaderExpansion(e.OriginalSource as DependencyObject, desired: null);

    // The second click of a double-click toggles the group again but raises DoubleTapped, not Tapped.
    private void OnTableDoubleTapped(object sender, DoubleTappedRoutedEventArgs e) => NoteHeaderExpansion(e.OriginalSource as DependencyObject, desired: null);

    private void OnTableKeyDown(object sender, KeyRoutedEventArgs e)
    {
        var rtl = PeopleTable.FlowDirection == FlowDirection.RightToLeft;
        bool? desired = e.Key switch
        {
            VirtualKey.Right => !rtl,
            VirtualKey.Left => rtl,
            _ => null,
        };

        if (desired is not null || e.Key is VirtualKey.Enter or VirtualKey.Space)
        {
            NoteHeaderExpansion(e.OriginalSource as DependencyObject, desired);
        }
    }

    private void NoteHeaderExpansion(DependencyObject? source, bool? desired)
    {
        while (source is not null && source is not TableViewGroupHeader)
        {
            source = VisualTreeHelper.GetParent(source);
        }

        if (!IsGrouped || source is not TableViewGroupHeader { IsExpandable: true } header
            || header.Content is not TableViewGroupInfo group)
        {
            return;
        }

        // A header click toggles its group on a LATER dispatcher turn, so header.IsExpanded still holds
        // the old state here. Compute the new state instead of reading it back.
        var identity = SampleShaping.GroupIdentity(group.Key);
        var expanded = desired ?? _collapsedGroups.Contains(identity);
        if (expanded)
        {
            _collapsedGroups.Remove(identity);
        }
        else
        {
            _collapsedGroups.Add(identity);
        }

        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            "{0} the {1} group from its header",
            expanded ? "Expanded" : "Collapsed",
            group.KeyText));
    }

    // UI Automation ExpandCollapse raises no input event, so group headers report IsExpanded changes.
    public static readonly DependencyProperty ReportsExpansionProperty = DependencyProperty.RegisterAttached(
        "ReportsExpansion", typeof(bool), typeof(GroupsPage), new PropertyMetadata(false, OnReportsExpansionChanged));

    public static bool GetReportsExpansion(DependencyObject element) => (bool)element.GetValue(ReportsExpansionProperty);

    public static void SetReportsExpansion(DependencyObject element, bool value) => element.SetValue(ReportsExpansionProperty, value);

    private static void OnReportsExpansionChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is TableViewGroupHeader header && e.NewValue is true)
        {
            header.RegisterPropertyChangedCallback(TableViewGroupHeader.IsExpandedProperty, OnHeaderIsExpandedChanged);
        }
    }

    private static void OnHeaderIsExpandedChanged(DependencyObject sender, DependencyProperty property)
    {
        DependencyObject? node = sender;
        while (node is not null and not GroupsPage)
        {
            node = VisualTreeHelper.GetParent(node);
        }

        (node as GroupsPage)?.RecordHeaderExpansion((TableViewGroupHeader)sender);
    }

    // A recycled header reports its new group's real state, so recording it is still correct.
    private void RecordHeaderExpansion(TableViewGroupHeader header)
    {
        if (TryRecordHeaderExpansion(header))
        {
            RefreshReadouts();
        }
    }

    // A UIA toggle can land on a recycled header whose IsExpanded already holds the new value, which
    // raises no change, so re-read the realized headers after each layout pass (LayoutUpdated fires for
    // every pass on the thread, so keep this cheap). An applied expansion change always re-lays out
    // the rows, so the readout catches up even if this runs before the control's deferred work.
    private void OnTableLayoutUpdated(object? sender, object e)
    {
        if (!IsGrouped || _reconcileQueued)
        {
            return;
        }

        _reconcileQueued = DispatcherQueue.TryEnqueue(ReconcileRealizedHeaders);
    }

    private void ReconcileRealizedHeaders()
    {
        _reconcileQueued = false;
        if (!IsLoaded || !IsGrouped)
        {
            return;
        }

        if (_rowsHost is not { IsLoaded: true })
        {
            _rowsHost = FindRowsHost(PeopleTable);
        }

        if (_rowsHost is not { } host)
        {
            return;
        }

        var changed = false;
        for (int i = VisualTreeHelper.GetChildrenCount(host) - 1; i >= 0; i--)
        {
            if (VisualTreeHelper.GetChild(host, i) is TableViewGroupHeader header && !IsParkedByRepeater(header))
            {
                changed |= TryRecordHeaderExpansion(header);
            }
        }

        if (changed)
        {
            RefreshReadouts();
        }
    }

    private static FrameworkElement? FindRowsHost(DependencyObject root)
    {
        var pending = new Stack<DependencyObject>();
        pending.Push(root);
        while (pending.Count > 0)
        {
            var node = pending.Pop();
            // The template's only ItemsRepeater is the rows host; it exists even with no rows realized.
            if (node is Microsoft.UI.Xaml.Controls.ItemsRepeater repeater)
            {
                return repeater;
            }

            for (int i = VisualTreeHelper.GetChildrenCount(node) - 1; i >= 0; i--)
            {
                pending.Push(VisualTreeHelper.GetChild(node, i));
            }
        }

        return null;
    }

    // ItemsRepeater parks recycled elements at (-10000, -10000) without updating their Content or IsExpanded.
    private static bool IsParkedByRepeater(UIElement element) => element.ActualOffset.X <= -9999 && element.ActualOffset.Y <= -9999;

    private bool TryRecordHeaderExpansion(TableViewGroupHeader header)
    {
        if (!IsGrouped || !header.IsExpandable || header.Content is not TableViewGroupInfo group)
        {
            return false;
        }

        var identity = SampleShaping.GroupIdentity(group.Key);
        return header.IsExpanded ? _collapsedGroups.Remove(identity) : _collapsedGroups.Add(identity);
    }
}
