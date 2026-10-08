// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Converters;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Headers visibility: TableView.HeadersVisibility (Column / None) on a support-ticket queue,
/// how the hidden header strip interacts with group headers, and sorting from code
/// (TableView.SortByColumn) while there is no header to click.
/// </summary>
public sealed partial class HeadersVisibilityPage : Page
{
    private static readonly Dictionary<string, (SolidColorBrush Tint, SolidColorBrush Dot)> s_priorityBrushes = new(StringComparer.Ordinal)
    {
        ["Sev 1"] = (new SolidColorBrush(ColorHelper.FromArgb(0x33, 0xDC, 0x26, 0x26)), new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0xDC, 0x26, 0x26))),
        ["Sev 2"] = (new SolidColorBrush(ColorHelper.FromArgb(0x33, 0xEA, 0x58, 0x0C)), new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0xEA, 0x58, 0x0C))),
        ["Sev 3"] = (new SolidColorBrush(ColorHelper.FromArgb(0x33, 0xCA, 0x8A, 0x04)), new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0xCA, 0x8A, 0x04))),
        ["Sev 4"] = (new SolidColorBrush(ColorHelper.FromArgb(0x33, 0x64, 0x74, 0x8B)), new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B))),
    };

    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Queue";
    private int _nextTicket = TicketData.FirstTicketNumber + 20;

    public HeadersVisibilityPage()
    {
        _source = TableViewSource.From(Tickets);
        InitializeComponent();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<SupportTicket> Tickets { get; } = TicketData.Queue();

    public TableViewSource? Source => _source;

    // x:Bind functions for the Priority chip: cached brushes, transparent / theme text under a
    // Contrast theme so the chip never relies on colour alone.
    public static Brush PriorityTint(string priority) =>
        ChipBrushes.IsHighContrast ? ChipBrushes.Transparent
        : s_priorityBrushes.TryGetValue(priority ?? string.Empty, out var brushes) ? brushes.Tint : ChipBrushes.Transparent;

    public static Brush PriorityDot(string priority) =>
        ChipBrushes.IsHighContrast ? ChipBrushes.HighContrastForeground
        : s_priorityBrushes.TryGetValue(priority ?? string.Empty, out var brushes) ? brushes.Dot : ChipBrushes.HighContrastForeground;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        foreach (var ticket in Tickets)
        {
            ticket.PropertyChanged += OnTicketChanged;
        }

        TicketsTable.Sorted += OnTableSorted;
        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        foreach (var ticket in Tickets)
        {
            ticket.PropertyChanged -= OnTicketChanged;
        }

        TicketsTable.Sorted -= OnTableSorted;
    }

    // ---- Headers visibility -------------------------------------------------------------

    private void OnHeadersVisibilityChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="0") before the table exists.
        if (TicketsTable is null || HeadersText is null)
        {
            return;
        }

        TicketsTable.HeadersVisibility = SampleShaping.SelectedTag(HeadersVisibilitySelector, "Column") == "None"
            ? TableViewHeadersVisibility.None
            : TableViewHeadersVisibility.Column;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "HeadersVisibility -> {0}", TicketsTable.HeadersVisibility));
    }

    // ---- Sorting from code --------------------------------------------------------------

    private void OnSortByDueClick(object sender, RoutedEventArgs e)
    {
        // The header click does the same thing; SortByColumn is the only way while it is hidden.
        var direction = DueColumn.SortDirection == TableViewSortDirection.Ascending
            ? TableViewSortDirection.Descending
            : TableViewSortDirection.Ascending;
        TicketsTable.SortByColumn(DueColumn, direction);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "SortByColumn(Due, {0})", direction));
    }

    private void OnClearSortClick(object sender, RoutedEventArgs e)
    {
        TicketsTable.ClearSort();
        SetLastAction("ClearSort(): rows back in filing order");
    }

    // Raised for header clicks and for SortByColumn / ClearSort alike.
    private void OnTableSorted(TableView sender, TableViewSortedEventArgs e)
    {
        SortText.Text = e.Column is null || e.Direction == TableViewSortDirection.None
            ? "None"
            : string.Format(CultureInfo.CurrentCulture, "{0} {1}", e.Column.Header, e.Direction);
    }

    // ---- Edits and actions --------------------------------------------------------------

    private void OnTicketChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not SupportTicket ticket)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(SupportTicket.IsResolved):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} marked {1}", ticket.TicketId, ticket.IsResolved ? "resolved" : "open"));
                break;
            case nameof(SupportTicket.Due):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} due date -> {1}", ticket.TicketId, ticket.DueText));
                break;
        }
    }

    private void OnEscalateClick(object sender, RoutedEventArgs e)
    {
        if (TicketsTable.SelectedItem is not SupportTicket ticket)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = ticket.Priority;
        var to = SupportTicket.Escalate(from);
        if (to == from)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} is already {1}, the most urgent level", ticket.TicketId, from));
            return;
        }

        ticket.Priority = to;

        // GroupBy takes a delegate, not a property path, so the control cannot re-bucket the row
        // on PropertyChanged; re-apply the grouping when the grouped-on value changed.
        ReapplyIfGroupedOn(nameof(SupportTicket.Priority));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Escalated {0} from {1} to {2}", ticket.TicketId, from, to));
    }

    private void OnFileTicketClick(object sender, RoutedEventArgs e)
    {
        var ticket = TicketData.Incoming(_nextTicket++);
        ticket.PropertyChanged += OnTicketChanged;
        Tickets.Add(ticket);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filed {0} in the {1} queue as {2}", ticket.TicketId, ticket.Queue, ticket.Priority));
    }

    private void OnCloseTicketClick(object sender, RoutedEventArgs e)
    {
        if (TicketsTable.SelectedItem is not SupportTicket ticket)
        {
            SetLastAction("No row selected.");
            return;
        }

        ticket.PropertyChanged -= OnTicketChanged;
        Tickets.Remove(ticket);

        if (_appliedMode == "grouped")
        {
            var group = KeyOf(ticket, _appliedKey);
            var remaining = Tickets.Count(t => Equals(KeyOf(t, _appliedKey), group));
            SetLastAction(remaining == 0
                ? string.Format(CultureInfo.CurrentCulture, "Closed {0}; it was the last ticket in {1}, so that group is gone", ticket.TicketId, group)
                : string.Format(CultureInfo.CurrentCulture, "Closed {0}; {1} keeps {2:N0} ticket(s)", ticket.TicketId, group, remaining));
        }
        else
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Closed {0}", ticket.TicketId));
        }
    }

    // Group key for a ticket. Never empty: GroupBy fails fast on an empty group identity.
    private static object KeyOf(SupportTicket? ticket, string key)
    {
        var value = key switch
        {
            nameof(SupportTicket.Priority) => ticket?.Priority,
            nameof(SupportTicket.Assignee) => ticket?.Assignee,
            _ => ticket?.Queue,
        };

        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || ResolvedCountText is null || HeadersText is null || TicketsTable is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(Tickets.Count);
        ResolvedCountText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} of {1:N0}", Tickets.Count(t => t.IsResolved), Tickets.Count);
        HeadersText.Text = TicketsTable.HeadersVisibility.ToString();
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard every element this path touches.
        if (_source is null || TicketsTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Queue");
        var selected = TicketsTable.SelectedItem;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                _source.GroupBy(item => KeyOf(item as SupportTicket, key), SampleShaping.GroupIdentity);
                break;
            // case "hierarchy":
            // case "groupedHierarchy":
            //     Hierarchical (tree) rows are not available in this release, so the two matching
            //     ComboBoxItems ship disabled. TableViewSource and TableView have no hierarchy
            //     member today. When hierarchy ships, apply it to this same source here, composed
            //     with the GroupBy stage above rather than replacing it, and set _appliedMode only
            //     after the call returns.
            default:
                _source.ClearGroupBy();
                mode = "flat";
                break;
        }

        _appliedMode = mode;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(TicketsTable, selected, Tickets.Count * 2, RefreshReadouts);
        UpdateShapingGating();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
    }

    // Call after ANY write to the grouped-on property: from an action or from an in-cell edit.
    private void ReapplyIfGroupedOn(string? propertyName)
    {
        if (_appliedMode == "grouped" && propertyName == _appliedKey)
        {
            ApplyShaping(announce: false);
        }
    }

    private void UpdateShapingGating()
    {
        var grouped = _appliedMode == "grouped";
        GroupKeySelector.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ShapingModeText.Text = SampleShaping.ShapingText(grouped, GroupKeySelector);
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        TicketsTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        TicketsTable.CollapseAllGroups();
        SetLastAction("Collapsed all groups");
    }

    // The only writer of LastActionText.
    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }

        RefreshReadouts();
    }

    #endregion
}
