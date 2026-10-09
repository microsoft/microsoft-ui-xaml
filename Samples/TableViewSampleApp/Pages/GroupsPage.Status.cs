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
            .GroupBy(p => SampleShaping.KeyOf(p, key))
            .Select(g => string.Format(CultureInfo.CurrentCulture, "{0} {1:N0}", g.Key, g.Count()))
            .ToList();
        GroupsText.Text = groups.Count == 0
            ? "0 (empty source)"
            : string.Format(CultureInfo.CurrentCulture, "{0:N0}: {1}", groups.Count, string.Join(", ", groups));
        ExpansionText.Text = ExpansionSummary();
    }

    private IEnumerable<Person> InViewOrder() =>
        SampleShaping.InViewOrder(PeopleTable, People, SortKey, IsGrouped ? p => SampleShaping.KeyOf(p, AppliedGroupKey) : null, SortOrdersGroups);

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
        return People.Select(p => SampleShaping.GroupIdentity(SampleShaping.KeyOf(p, key))).ToHashSet();
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

    // <snippet Groups>
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
    // </snippet>
}
