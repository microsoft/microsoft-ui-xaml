// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableView.HeadersVisibility for the column-header strip, and
/// how that gate interacts with the separate group-header surface: hiding the
/// column headers never hides the group header bands.
///
/// The rows are a synthetic support-ticket queue rather than placeholder text,
/// so grouping by queue or priority produces buckets a reader recognises, and
/// two columns carry real cell editors (a ComboBox for the constrained priority
/// vocabulary and a CheckBox for the resolved flag).
/// </summary>
public sealed partial class HeadersVisibilityPage : Page, INotifyPropertyChanged
{
    private static readonly (string Queue, string Summary, string Assignee, string Priority, int DueInDays, bool Resolved)[] s_seed =
    {
        ("Identity",     "SSO sign-in loops after password reset",           "Marta Oyelaran",  "Sev 1", 0,  false),
        ("Identity",     "MFA push notifications arrive twice",              "Devin Ashworth",  "Sev 3", 6,  false),
        ("Identity",     "Service principal secret expired in staging",      "Priya Raghunath", "Sev 2", 2,  true),
        ("Billing",      "Invoice PDF renders blank for EU customers",       "Tomas Lindqvist", "Sev 2", 1,  false),
        ("Billing",      "Proration miscalculated on mid-cycle upgrade",     "Hana Okabe",      "Sev 2", 4,  false),
        ("Billing",      "Refund webhook retries forever on 409",            "Marta Oyelaran",  "Sev 3", 9,  true),
        ("Platform",     "Deploy ring 2 stuck behind a wedged health probe", "Owen Castellano", "Sev 1", 0,  false),
        ("Platform",     "Container image pull throttled in west region",    "Priya Raghunath", "Sev 2", 3,  false),
        ("Platform",     "Log ingestion lagging by eleven minutes",          "Devin Ashworth",  "Sev 3", 7,  false),
        ("Platform",     "Nightly backup job skipped two shards",            "Hana Okabe",      "Sev 2", 2,  true),
        ("Data",         "Daily export drops rows with null tenant id",      "Tomas Lindqvist", "Sev 2", 5,  false),
        ("Data",         "Schema migration lock blocks reporting reads",     "Owen Castellano", "Sev 1", 1,  false),
        ("Data",         "Stale materialized view after timezone change",    "Marta Oyelaran",  "Sev 4", 14, true),
        ("Support tools","Agent console loses draft on tab switch",          "Hana Okabe",      "Sev 3", 8,  false),
        ("Support tools","Macro insert strips trailing whitespace",          "Devin Ashworth",  "Sev 4", 12, true),
        ("Support tools","Queue filter resets when the page refreshes",      "Priya Raghunath", "Sev 3", 6,  false),
        ("Mobile",       "Attachment upload fails over cellular",            "Owen Castellano", "Sev 2", 3,  false),
        ("Mobile",       "Push token not refreshed after reinstall",         "Tomas Lindqvist", "Sev 3", 10, false),
        ("Mobile",       "Dark theme contrast fails on the detail sheet",    "Marta Oyelaran",  "Sev 4", 15, true),
        ("Mobile",       "Offline queue replays in the wrong order",         "Hana Okabe",      "Sev 2", 4,  false),
    };

    private TableViewSource? _source;

    // Requested shaping mode versus the mode actually applied to the source.
    // GroupBy fails fast, so these can differ; every readout and every
    // enable/disable guard reads _appliedMode.
    private string _mode = "flat";
    private string _appliedMode = "flat";
    private string _groupKey = "Queue";
    private bool _allGroupsCollapsed;
    private int _nextTicket = 1040;
    private string _statusText = string.Empty;

    public HeadersVisibilityPage()
    {
        InitializeComponent();

        foreach (var seed in s_seed)
        {
            Rows.Add(new SupportTicket
            {
                TicketId = $"INC-{_nextTicket++}",
                Summary = seed.Summary,
                Queue = seed.Queue,
                Assignee = seed.Assignee,
                Priority = seed.Priority,
                Due = DateTimeOffset.Now.Date.AddDays(seed.DueInDays),
                IsResolved = seed.Resolved,
            });
        }

        DemoTable.HeadersVisibility = TableViewHeadersVisibility.Column;

        _source = TableViewSource.From(Rows);
        DemoTable.ItemsSource = _source;

        ApplyShaping();
    }

    public ObservableCollection<SupportTicket> Rows { get; } = new();

    public string StatusText
    {
        get => _statusText;
        private set
        {
            if (_statusText != value)
            {
                _statusText = value;
                OnPropertyChanged();
            }
        }
    }

    private void OnHeadersVisibilityChecked(object sender, RoutedEventArgs e)
    {
        if (DemoTable is null || sender is not FrameworkElement { Tag: string tag })
        {
            return;
        }

        DemoTable.HeadersVisibility = tag switch
        {
            "None" => TableViewHeadersVisibility.None,
            _ => TableViewHeadersVisibility.Column,
        };

        UpdateStatus($"HeadersVisibility set to {DemoTable.HeadersVisibility}");
    }

    // ----- Shaping -----

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null ||
            sender is not ComboBox { SelectedItem: ComboBoxItem { Tag: string tag } })
        {
            return;
        }

        _mode = tag;
        ApplyShaping();
    }

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || GroupKeyCombo?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        // Re-applying with the same mode proves the grouped state survives a key change.
        ApplyShaping();
    }

    private void ApplyShaping()
    {
        if (_source is null)
        {
            return;
        }

        // Reshape IN PLACE: Filter/GroupBy mutate and return the same instance,
        // so the source is never rebuilt per change.
        if (_mode != "grouped")
        {
            _source.ClearGroupBy();
        }
        else
        {
            var key = _groupKey;
            // The two delegates receive DIFFERENT things despite both parameters
            // being named `item`:
            //   TableViewKeySelector(Object item)      -> receives the ROW ITEM
            //   TableViewIdentitySelector(Object item) -> receives the GROUP KEY
            // An item-typed identity lambda returns empty, which is a fail-fast,
            // so grouping would silently never apply.
            _source.GroupBy(
                item => (object)GroupValue(item, key),
                groupKey => groupKey?.ToString() ?? "(none)");
        }

        // case "hierarchy":
        // case "groupedhierarchy":
        //     Hierarchical rows are not available in this release: neither
        //     TableViewSource.idl nor TableView.idl exposes a hierarchy verb
        //     (the only shaping verbs are Filter / GroupBy / Sort and their
        //     Clear* counterparts). When the control ships hierarchy support,
        //     apply it to this same source here, alongside the GroupBy stage
        //     above so the two compose, then set _appliedMode as below.

        _appliedMode = _mode;
        _allGroupsCollapsed = false;

        if (_appliedMode == "grouped")
        {
            DemoTable.ExpandAllGroups();
        }

        UpdateShapingGates();
        UpdateStatus($"Shaping applied: {ModeLabel(_appliedMode)}");
    }

    private void UpdateShapingGates()
    {
        if (GroupKeyCombo is null)
        {
            return;
        }

        var grouped = _appliedMode == "grouped";

        GroupKeyCombo.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ReassignRowButton.IsEnabled = grouped;
    }

    // ----- Actions -----

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped") return;

        DemoTable.ExpandAllGroups();
        _allGroupsCollapsed = false;
        UpdateStatus("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped") return;

        DemoTable.CollapseAllGroups();
        _allGroupsCollapsed = true;
        UpdateStatus("Collapsed all groups");
    }

    private void OnReassignRowClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped" || Rows.Count == 0) return;

        var ticket = Rows[0];
        var before = GroupValue(ticket, _groupKey);

        if (_groupKey == "Priority")
        {
            ticket.Priority = SupportTicket.Escalate(ticket.Priority);
        }
        else
        {
            ticket.Queue = NextQueue(ticket.Queue);
        }

        UpdateStatus($"{ticket.TicketId} moved from {before} to {GroupValue(ticket, _groupKey)}");
    }

    private void OnAddRowClick(object sender, RoutedEventArgs e)
    {
        var ticket = new SupportTicket
        {
            TicketId = $"INC-{_nextTicket++}",
            Summary = "Customer reports intermittent 503 on checkout",
            Queue = _groupKey == "Queue" ? NextQueue(Rows.Count > 0 ? Rows[0].Queue : "Platform") : "Platform",
            Assignee = "Unassigned",
            Priority = "Sev 2",
            Due = DateTimeOffset.Now.Date.AddDays(3),
        };

        Rows.Add(ticket);
        UpdateStatus($"Filed {ticket.TicketId} into {GroupValue(ticket, _groupKey)}");
    }

    private void OnRemoveRowClick(object sender, RoutedEventArgs e)
    {
        if (Rows.Count == 0)
        {
            UpdateStatus("Nothing to close — the queue is empty");
            return;
        }

        var ticket = Rows[0];
        Rows.RemoveAt(0);
        UpdateStatus($"Closed {ticket.TicketId}; its group disappears when it was the last member of {GroupValue(ticket, _groupKey)}");
    }

    // ----- Helpers -----
    //
    // GroupValue never returns the empty string: an empty group identity is a
    // fail-fast in GroupBy, so a blank property has to be coalesced.

    private static string GroupValue(object item, string key)
    {
        if (item is not SupportTicket ticket)
        {
            return "(none)";
        }

        var value = key == "Priority" ? ticket.Priority : ticket.Queue;
        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private string NextQueue(string current)
    {
        var queues = Rows.Select(r => r.Queue).Distinct(StringComparer.Ordinal).OrderBy(q => q, StringComparer.Ordinal).ToList();
        if (queues.Count == 0)
        {
            return "Platform";
        }

        var index = queues.IndexOf(current);
        return queues[(index < 0 ? 0 : index + 1) % queues.Count];
    }

    private static string ModeLabel(string mode) => mode switch
    {
        "grouped" => "Grouped",
        "hierarchy" => "Hierarchy",
        "groupedhierarchy" => "Grouped hierarchy",
        _ => "Flat",
    };

    private void UpdateStatus(string? message = null)
    {
        if (DemoTable is null)
        {
            return;
        }

        var headers = DemoTable.HeadersVisibility == TableViewHeadersVisibility.None
            ? "column headers hidden"
            : "column headers shown";

        // Readouts describe the APPLIED mode, never the requested one.
        var key = _groupKey;
        var shaping = _appliedMode == "grouped"
            ? $"Grouped by {key} · {Rows.Select(r => GroupValue(r, key)).Distinct(StringComparer.Ordinal).Count()} groups · {(_allGroupsCollapsed ? "all collapsed" : "all expanded")}"
            : "Flat (no grouping)";

        var prefix = message is null ? string.Empty : $"{message}. ";
        StatusText = $"{prefix}{headers} · {shaping} · {Rows.Count} tickets · {Rows.Count(r => r.IsResolved)} resolved.";
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

/// <summary>
/// One row of the synthetic support queue. Every property a cell editor binds
/// to raises PropertyChanged, so an edit made in a recycled row container is
/// written to the model and re-read when the container is reused — the value
/// survives scrolling instead of reverting.
/// </summary>
public sealed class SupportTicket : INotifyPropertyChanged
{
    private static readonly string[] s_priorities = ["Sev 1", "Sev 2", "Sev 3", "Sev 4"];

    private string _ticketId = string.Empty;
    private string _queue = string.Empty;
    private string _priority = "Sev 3";
    private DateTimeOffset _due;
    private bool _isResolved;

    public string TicketId
    {
        get => _ticketId;
        set
        {
            if (_ticketId != value)
            {
                _ticketId = value;
                Raise(nameof(TicketId));
                Raise(nameof(PriorityAutomationName));
                Raise(nameof(ResolvedAutomationName));
            }
        }
    }

    public string Summary { get; set; } = string.Empty;

    public string Assignee { get; set; } = string.Empty;

    /// <summary>Group key candidate; mutating it moves the row between groups.</summary>
    public string Queue
    {
        get => _queue;
        set
        {
            if (_queue != value)
            {
                _queue = value;
                Raise(nameof(Queue));
            }
        }
    }

    /// <summary>
    /// Second group key candidate, and the ComboBox cell editor's bound value.
    /// </summary>
    public string Priority
    {
        get => _priority;
        set
        {
            if (_priority != value)
            {
                _priority = value;
                Raise(nameof(Priority));
                Raise(nameof(PriorityAutomationName));
            }
        }
    }

    public DateTimeOffset Due
    {
        get => _due;
        set
        {
            if (_due != value)
            {
                _due = value;
                Raise(nameof(Due));
                Raise(nameof(DueText));
            }
        }
    }

    public string DueText => _due.ToString("yyyy-MM-dd", CultureInfo.InvariantCulture);

    /// <summary>The CheckBox cell editor's bound value.</summary>
    public bool IsResolved
    {
        get => _isResolved;
        set
        {
            if (_isResolved != value)
            {
                _isResolved = value;
                Raise(nameof(IsResolved));
                Raise(nameof(ResolvedAutomationName));
            }
        }
    }

    /// <summary>Fixed vocabulary behind the ComboBox cell editor.</summary>
    public IReadOnlyList<string> PriorityOptions => s_priorities;

    // Per-row accessible names: without these, twenty checkboxes all announce
    // the same thing and a screen-reader user cannot tell which row they are on.
    public string PriorityAutomationName => $"Priority for {_ticketId}, currently {_priority}";

    public string ResolvedAutomationName => $"Resolved — {_ticketId}";

    public static string Escalate(string priority)
    {
        var index = Array.IndexOf(s_priorities, priority);
        return index <= 0 ? s_priorities[^1] : s_priorities[index - 1];
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Raise(string name) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
