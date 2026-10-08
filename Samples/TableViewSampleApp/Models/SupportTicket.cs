// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace TableViewSampleApp.Models;

/// <summary>
/// One row of the Headers visibility page's support queue. A ticket queue is used there instead
/// of <see cref="Person"/> because its columns (priority, due date, resolved) are hard to read
/// without headers, which is the point that page makes. Every property a cell binds to raises
/// PropertyChanged, so an edit made in a recycled row is written to the model and shown again
/// when the row is reused.
/// </summary>
public sealed partial class SupportTicket : INotifyPropertyChanged
{
    private static readonly string[] s_priorities = ["Sev 1", "Sev 2", "Sev 3", "Sev 4"];

    private string _ticketId = string.Empty;
    private string _summary = string.Empty;
    private string _assignee = string.Empty;
    private string _queue = string.Empty;
    private string _priority = "Sev 3";
    private DateTimeOffset _due;
    private bool _isResolved;

    /// <summary>The priority vocabulary, most urgent first.</summary>
    public static IReadOnlyList<string> Priorities => s_priorities;

    public string TicketId
    {
        get => _ticketId;
        set
        {
            if (Set(ref _ticketId, value))
            {
                Raise(nameof(DueAutomationName));
                Raise(nameof(ResolvedAutomationName));
            }
        }
    }

    public string Summary
    {
        get => _summary;
        set => Set(ref _summary, value);
    }

    public string Assignee
    {
        get => _assignee;
        set
        {
            if (Set(ref _assignee, value))
            {
                Raise(nameof(AssigneeAutomationName));
            }
        }
    }

    /// <summary>Team that owns the ticket; a group key.</summary>
    public string Queue
    {
        get => _queue;
        set => Set(ref _queue, value);
    }

    /// <summary>"Sev 1" (most urgent) to "Sev 4"; a group key. Sorts correctly as text.</summary>
    public string Priority
    {
        get => _priority;
        set => Set(ref _priority, value);
    }

    public DateTimeOffset Due
    {
        get => _due;
        set
        {
            if (Set(ref _due, value))
            {
                Raise(nameof(DueOrNull));
                Raise(nameof(DueText));
            }
        }
    }

    /// <summary>
    /// <see cref="Due"/> as a nullable value for <c>CalendarDatePicker.Date</c> two-way binding.
    /// Clearing the picker (null) is ignored, so every ticket keeps a due date.
    /// </summary>
    public DateTimeOffset? DueOrNull
    {
        get => _due;
        set
        {
            if (value is { } due)
            {
                Due = due;
            }
        }
    }

    /// <summary>Due date as short date text in the user's culture.</summary>
    public string DueText => _due.ToString("d", CultureInfo.CurrentCulture);

    public bool IsResolved
    {
        get => _isResolved;
        set => Set(ref _isResolved, value);
    }

    // Per-row accessible names. With the column headers hidden, these names are the only column
    // context a screen reader user gets, so each one says which column and which ticket it is.
    public string AssigneeAutomationName => string.Format(CultureInfo.CurrentCulture, "Assigned to {0}", _assignee);

    public string DueAutomationName => string.Format(CultureInfo.CurrentCulture, "Due date for {0}", _ticketId);

    public string ResolvedAutomationName => string.Format(CultureInfo.CurrentCulture, "Resolved for {0}", _ticketId);

    /// <summary>
    /// One step more urgent ("Sev 3" to "Sev 2"). Sev 1 is the ceiling and is returned unchanged.
    /// </summary>
    public static string Escalate(string priority)
    {
        var index = Array.IndexOf(s_priorities, priority);
        return index <= 0 ? s_priorities[0] : s_priorities[index - 1];
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private bool Set<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (Equals(field, value))
        {
            return false;
        }

        field = value;
        Raise(propertyName);
        return true;
    }

    private void Raise(string? propertyName) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
}
