// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Data;

/// <summary>
/// Deterministic support-ticket queue for the Headers visibility page: six queues, four
/// priorities and a few resolved tickets, so grouping by queue or priority gives buckets a reader
/// recognises. Due dates are relative to today.
/// </summary>
public static class TicketData
{
    /// <summary>Number of the first seeded ticket (INC-1040).</summary>
    public const int FirstTicketNumber = 1040;

    private static readonly (string Queue, string Summary, string Assignee, string Priority, int DueInDays, bool Resolved)[] s_seed =
    {
        ("Identity",      "SSO sign-in loops after password reset",           "Marta Oyelaran",  "Sev 1", 0,  false),
        ("Identity",      "MFA push notifications arrive twice",              "Devin Ashworth",  "Sev 3", 6,  false),
        ("Identity",      "Service principal secret expired in staging",      "Priya Raghunath", "Sev 2", 2,  true),
        ("Billing",       "Invoice PDF renders blank for EU customers",       "Tomas Lindqvist", "Sev 2", 1,  false),
        ("Billing",       "Proration miscalculated on mid-cycle upgrade",     "Hana Okabe",      "Sev 2", 4,  false),
        ("Billing",       "Refund webhook retries forever on 409",            "Marta Oyelaran",  "Sev 3", 9,  true),
        ("Platform",      "Deploy ring 2 stuck behind a wedged health probe", "Owen Castellano", "Sev 1", 0,  false),
        ("Platform",      "Container image pull throttled in west region",    "Priya Raghunath", "Sev 2", 3,  false),
        ("Platform",      "Log ingestion lagging by eleven minutes",          "Devin Ashworth",  "Sev 3", 7,  false),
        ("Platform",      "Nightly backup job skipped two shards",            "Hana Okabe",      "Sev 2", 2,  true),
        ("Data",          "Daily export drops rows with null tenant id",      "Tomas Lindqvist", "Sev 2", 5,  false),
        ("Data",          "Schema migration lock blocks reporting reads",     "Owen Castellano", "Sev 1", 1,  false),
        ("Data",          "Stale materialized view after timezone change",    "Marta Oyelaran",  "Sev 4", 14, true),
        ("Support tools", "Agent console loses draft on tab switch",          "Hana Okabe",      "Sev 3", 8,  false),
        ("Support tools", "Macro insert strips trailing whitespace",          "Devin Ashworth",  "Sev 4", 12, true),
        ("Support tools", "Queue filter resets when the page refreshes",      "Priya Raghunath", "Sev 3", 6,  false),
        ("Mobile",        "Attachment upload fails over cellular",            "Owen Castellano", "Sev 2", 3,  false),
        ("Mobile",        "Push token not refreshed after reinstall",         "Tomas Lindqvist", "Sev 3", 10, false),
        ("Mobile",        "Dark theme contrast fails on the detail sheet",    "Marta Oyelaran",  "Sev 4", 15, true),
        ("Mobile",        "Offline queue replays in the wrong order",         "Hana Okabe",      "Sev 2", 4,  false),
    };

    // Tickets the "File a new ticket" action adds, in turn.
    private static readonly (string Queue, string Summary, string Assignee)[] s_incoming =
    {
        ("Billing",       "Customer reports intermittent 503 on checkout",    "Tomas Lindqvist"),
        ("Identity",      "Guest users cannot accept the invitation link",    "Devin Ashworth"),
        ("Mobile",        "Widget shows yesterday's balance after midnight",  "Hana Okabe"),
        ("Platform",      "Certificate rotation alert fired for a test host", "Owen Castellano"),
        ("Data",          "Usage report double-counts merged accounts",       "Priya Raghunath"),
        ("Support tools", "Canned replies missing from the French locale",    "Marta Oyelaran"),
    };

    /// <summary>The twenty seeded tickets, INC-1040 to INC-1059, as a fresh collection.</summary>
    public static ObservableCollection<SupportTicket> Queue()
    {
        var today = DateTimeOffset.Now.Date;
        var list = new ObservableCollection<SupportTicket>();
        for (var i = 0; i < s_seed.Length; i++)
        {
            var seed = s_seed[i];
            list.Add(new SupportTicket
            {
                TicketId = TicketId(FirstTicketNumber + i),
                Summary = seed.Summary,
                Queue = seed.Queue,
                Assignee = seed.Assignee,
                Priority = seed.Priority,
                Due = new DateTimeOffset(today.AddDays(seed.DueInDays)),
                IsResolved = seed.Resolved,
            });
        }

        return list;
    }

    /// <summary>
    /// A new Sev 2 ticket numbered <paramref name="number"/>, due in three days. The queue,
    /// summary and assignee cycle through a fixed list.
    /// </summary>
    public static SupportTicket Incoming(int number)
    {
        var incoming = s_incoming[Math.Abs(number) % s_incoming.Length];
        return new SupportTicket
        {
            TicketId = TicketId(number),
            Summary = incoming.Summary,
            Queue = incoming.Queue,
            Assignee = incoming.Assignee,
            Priority = "Sev 2",
            Due = new DateTimeOffset(DateTimeOffset.Now.Date.AddDays(3)),
        };
    }

    // <snippet HeadersVisibility>
    // Group key for a ticket (passed to Shaping.Attach, which hands it to GroupBy as the key
    // selector). Never empty: GroupBy fails fast on an empty group identity.
    public static object GroupKeyOf(SupportTicket? ticket, string key)
    {
        var value = key switch
        {
            nameof(SupportTicket.Priority) => ticket?.Priority,
            nameof(SupportTicket.Assignee) => ticket?.Assignee,
            _ => ticket?.Queue,
        };

        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }
    // </snippet>

    private static string TicketId(int number) => "INC-" + number;
}
