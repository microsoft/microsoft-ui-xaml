// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
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
public sealed partial class HeadersVisibilityPage : SamplePageBase
{
    private int _nextTicket = TicketData.FirstTicketNumber + 20;

    public HeadersVisibilityPage()
    {
        // <snippet>
        Source = TableViewSource.From(Tickets);    // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>
        InitializeSample(Status, Shaping.Attach(TicketsTable, Source, (row, key) => TicketData.GroupKeyOf(row as SupportTicket, key)));
        TrackItems(Tickets, OnTicketChanged);
        TrackLifetime(() => TicketsTable.Sorted += OnTableSorted, () => TicketsTable.Sorted -= OnTableSorted);
    }

    public ObservableCollection<SupportTicket> Tickets { get; } = TicketData.Queue();

    public TableViewSource Source { get; }

    // <snippet>
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
    // </snippet>

    private void OnClearSortClick(object sender, RoutedEventArgs e)
    {
        TicketsTable.ClearSort();
        SetLastAction("ClearSort(): rows back in filing order");
    }

    // <snippet>
    // Raised for header clicks and for SortByColumn / ClearSort alike.
    private void OnTableSorted(TableView sender, TableViewSortedEventArgs e)
    {
        SortText.Text = e.Column is null || e.Direction == TableViewSortDirection.None
            ? "None"
            : string.Format(CultureInfo.CurrentCulture, "{0} {1}", e.Column.Header, e.Direction);
    }
    // </snippet>

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

    // <snippet>
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
    // </snippet>

    private void OnFileTicketClick(object sender, RoutedEventArgs e)
    {
        var ticket = TicketData.Incoming(_nextTicket++);
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

        Tickets.Remove(ticket);

        if (IsGrouped)
        {
            var group = TicketData.GroupKeyOf(ticket, AppliedGroupKey);
            var remaining = Tickets.Count(t => Equals(TicketData.GroupKeyOf(t, AppliedGroupKey), group));
            SetLastAction(remaining == 0
                ? string.Format(CultureInfo.CurrentCulture, "Closed {0}; it was the last ticket in {1}, so that group is gone", ticket.TicketId, group)
                : string.Format(CultureInfo.CurrentCulture, "Closed {0}; {1} keeps {2:N0} ticket(s)", ticket.TicketId, group, remaining));
        }
        else
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Closed {0}", ticket.TicketId));
        }
    }
}
