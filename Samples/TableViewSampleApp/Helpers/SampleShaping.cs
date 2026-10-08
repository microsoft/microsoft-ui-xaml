// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Helpers;

/// <summary>
/// Generic helpers for the canonical Shaping section every table page carries
/// (FIX-PLAN §1.2/§1.3). Pages keep only their feature code; key resolution, group identity,
/// labels and re-selection come from here so the behaviour is identical everywhere.
/// </summary>
public static class SampleShaping
{
    /// <summary>Bucket label used for a null or blank group key.</summary>
    public const string NoneKey = "(none)";

    /// <summary>
    /// True while <see cref="Reselect"/> is probing indexes. A page's SelectionChanged handler
    /// can return early while this is set, so its readouts are written once, afterwards.
    /// </summary>
    public static bool IsReselecting { get; private set; }

    /// <summary>The <c>Tag</c> of the selected <see cref="ComboBoxItem"/>, or <paramref name="fallback"/>.</summary>
    public static string SelectedTag(ComboBox? comboBox, string fallback) =>
        comboBox?.SelectedItem is ComboBoxItem { Tag: string tag } && tag.Length > 0 ? tag : fallback;

    /// <summary>The visible <c>Content</c> of the selected <see cref="ComboBoxItem"/> (e.g. "Department").</summary>
    public static string Label(ComboBox? comboBox) =>
        comboBox?.SelectedItem is ComboBoxItem item ? item.Content?.ToString() ?? string.Empty : string.Empty;

    /// <summary>
    /// GroupBy identity selector. It receives the GROUP KEY produced by the key selector (never
    /// the row) and must not return an empty string: TableViewSource fails fast on an empty
    /// identity. Pass it as the second GroupBy argument.
    /// </summary>
    public static string GroupIdentity(object? groupKey) =>
        groupKey?.ToString() is { Length: > 0 } s ? s : NoneKey;

    /// <summary>
    /// GroupBy key selector body for <see cref="Person"/>. <paramref name="key"/> is the
    /// group-key ComboBoxItem Tag (a property name): Department, Office, Role or IsActive.
    /// Blank values are coalesced to <see cref="NoneKey"/>; the bool maps to Active/Inactive.
    /// </summary>
    public static object KeyOf(Person? person, string key)
    {
        if (person is null)
        {
            return NoneKey;
        }

        var value = key switch
        {
            nameof(Person.Office) => person.Office,
            nameof(Person.Role) => person.Role,
            nameof(Person.IsActive) => person.IsActive ? "Active" : "Inactive",
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(value) ? NoneKey : value;
    }

    /// <summary>
    /// Restores the selection to <paramref name="item"/> after a reshape. TableView selection is
    /// index-only in this release (<c>TableView.Select(int)</c>; <c>SelectedItem</c> and
    /// <c>SelectedIndex</c> are read-only, TableView.idl "Selection"), and the displayed row
    /// projection (which includes group headers) is not exposed, so the item's index is found
    /// by probing: Select(i) and compare SelectedItem by reference. Select ignores group-header
    /// and out-of-range indexes. Returns true when the item is selected immediately.
    /// <para>
    /// Right after a group-KEY change the new groups' rows become selectable only once the
    /// projection is rebuilt, so when the immediate probe fails this retries once on the next
    /// (low-priority) dispatcher turn, if the table is still loaded and nothing else was selected
    /// meanwhile. <paramref name="onDeferredSelected"/> runs only when that retry selects the
    /// item, so the page can refresh selection readouts that skipped the probe.
    /// </para>
    /// </summary>
    /// <param name="maxIndex">Upper bound of the probe: rows + group headers. Defaults to 5000.</param>
    /// <param name="onDeferredSelected">Optional; called after a successful deferred retry.</param>
    public static bool Reselect(TableView? table, object? item, int maxIndex = 5000, Action? onDeferredSelected = null)
    {
        if (table is null || item is null)
        {
            return false;
        }

        if (Probe(table, item, maxIndex))
        {
            return true;
        }

        table.DispatcherQueue?.TryEnqueue(DispatcherQueuePriority.Low, () =>
        {
            if (table.IsLoaded && table.SelectedItem is null && Probe(table, item, maxIndex))
            {
                onDeferredSelected?.Invoke();
            }
        });

        return false;
    }

    private static bool Probe(TableView table, object item, int maxIndex)
    {
        if (ReferenceEquals(table.SelectedItem, item))
        {
            return true;
        }

        IsReselecting = true;
        try
        {
            for (var i = 0; i <= maxIndex; i++)
            {
                table.Select(i);
                if (ReferenceEquals(table.SelectedItem, item))
                {
                    return true;
                }
            }

            // The item is no longer displayed (filtered out or removed): leave nothing selected
            // rather than whatever row the probe stopped on.
            table.DeselectAll();
            return false;
        }
        finally
        {
            IsReselecting = false;
        }
    }

    /// <summary>
    /// The next value after <paramref name="current"/> in <paramref name="values"/>, wrapping
    /// around; the first value when <paramref name="current"/> is not in the list. Use it for
    /// actions that cycle a row through REAL values (next department, office, role).
    /// </summary>
    public static string Next(IReadOnlyList<string> values, string? current)
    {
        if (values.Count == 0)
        {
            return current ?? string.Empty;
        }

        for (var i = 0; i < values.Count; i++)
        {
            if (string.Equals(values[i], current, StringComparison.Ordinal))
            {
                return values[(i + 1) % values.Count];
            }
        }

        return values[0];
    }

    /// <summary>
    /// Status value for the "Shaping" readout: "Flat", or "Grouped by Department".
    /// </summary>
    public static string ShapingText(bool grouped, ComboBox? groupKeySelector) =>
        grouped ? string.Format(CultureInfo.CurrentCulture, "Grouped by {0}", Label(groupKeySelector)) : "Flat";

    /// <summary>Status value for the "Rows" readout, e.g. "40" (current culture, N0).</summary>
    public static string RowCountText(int count) => count.ToString("N0", CultureInfo.CurrentCulture);
}
