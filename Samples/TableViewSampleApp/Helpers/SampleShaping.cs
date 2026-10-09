// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewRow = Microsoft.UI.Xaml.Controls.Tabular.TableViewRow;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;

namespace TableViewSampleApp.Helpers;

/// <summary>
/// Generic helpers for the canonical Shaping section every table page carries. Pages keep only
/// their feature code; key resolution, group identity, labels and selecting an item come from here
/// so the behaviour is identical everywhere.
/// </summary>
public static class SampleShaping
{
    /// <summary>Bucket label used for a null or blank group key.</summary>
    public const string NoneKey = "(none)";

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
    /// Selects <paramref name="item"/> with ONE <c>Select(int)</c> call.
    /// <para>
    /// Workaround for an API gap: TableView selection is index-only in this release
    /// (<c>TableView.Select(int)</c>; <c>SelectedItem</c> is read-only, TableView.idl "Selection")
    /// and the displayed projection, which includes group headers, has no IndexOf. The table's rows
    /// ItemsRepeater is bound to that projection, so this looks the item up in the repeater's
    /// ItemsSourceView (a read-only scan: no selection events) and selects that index once. Returns
    /// false, and leaves the selection as it was, when the item is not displayed (filtered out,
    /// removed) or no row is realized yet.
    /// </para>
    /// <para>
    /// Usually not needed after a reshape: GroupBy, Sort and Filter raise a Reset, and the control
    /// re-anchors the selection by item identity itself.
    /// </para>
    /// </summary>
    public static bool SelectItem(TableView? table, object? item)
    {
        if (table is null || item is null)
        {
            return false;
        }

        if (ReferenceEquals(table.SelectedItem, item))
        {
            return true;
        }

        var index = ProjectionIndexOf(table, item);
        if (index < 0)
        {
            return false;
        }

        table.Select(index);
        return ReferenceEquals(table.SelectedItem, item);
    }

    /// <summary>Selects the item a realized <paramref name="row"/> shows (see <see cref="SelectItem"/>).</summary>
    public static bool SelectRow(TableView table, TableViewRow row)
    {
        var index = ProjectionIndexOf(row);
        if (index < 0)
        {
            return false;
        }

        table.Select(index);
        return ReferenceEquals(table.SelectedItem, row.DataContext);
    }

    // The index the rows ItemsRepeater gave the container: the projection index Select(int) takes.
    private static int ProjectionIndexOf(TableViewRow row) =>
        RowsRepeaterOf(row, out var container) is { } repeater ? repeater.GetElementIndex(container) : -1;

    private static int ProjectionIndexOf(TableView table, object item)
    {
        if (FindFirstRow(table) is not { } row || RowsRepeaterOf(row, out _)?.ItemsSourceView is not { } view)
        {
            return -1;
        }

        for (var i = 0; i < view.Count; i++)
        {
            if (ReferenceEquals(view.GetAt(i), item))
            {
                return i;
            }
        }

        return -1;
    }

    // The ItemsRepeater that hosts the row containers, and the row's element directly under it.
    private static ItemsRepeater? RowsRepeaterOf(TableViewRow row, out UIElement container)
    {
        container = row;
        for (var parent = VisualTreeHelper.GetParent(row); parent is not null; parent = VisualTreeHelper.GetParent(parent))
        {
            if (parent is ItemsRepeater repeater)
            {
                return repeater;
            }

            if (parent is UIElement element)
            {
                container = element;
            }
        }

        return null;
    }

    private static TableViewRow? FindFirstRow(DependencyObject parent)
    {
        var count = VisualTreeHelper.GetChildrenCount(parent);
        for (var i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);
            if (child is TableViewRow row)
            {
                return row;
            }

            if (FindFirstRow(child) is { } found)
            {
                return found;
            }
        }

        return null;
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

    /// <summary>The column the table is sorted on, or null.</summary>
    public static TableViewColumn? ActiveSortColumn(TableView? table) =>
        table?.Columns.FirstOrDefault(c => c.SortDirection != TableViewSortDirection.None);

    /// <summary>The sort key path of a column: SortMemberPath when set; otherwise a text column's Binding path.</summary>
    public static string? SortPathOf(TableViewColumn column) =>
        column.SortMemberPath is { Length: > 0 } path
            ? path
            : (column as TableViewTextColumn)?.Binding?.Path?.Path;

    /// <summary>
    /// <paramref name="rows"/> in (approximately) the order <paramref name="table"/> shows them, for
    /// the page readouts. TableView v1 does not expose its projection, so this approximates the
    /// control's rules; it is not a parity implementation (string order uses .NET culture comparison,
    /// and custom comparers, key-selector sorts and collapsed groups are not modelled): a stable
    /// sort on the active column's key (<paramref name="sortKey"/>(row, path); culture-aware for
    /// text, nulls first) and, when <paramref name="groupKey"/> is given (grouped mode), groups in
    /// the order their first row appears. The verbs apply in the order they were declared: a sort
    /// declared before GroupBy (<paramref name="sortOrdersGroups"/>, see
    /// <see cref="Controls.ShapingOptions.SortOrdersGroups"/>) orders the rows and so the groups;
    /// one declared after it sorts within each group, and the groups keep source order.
    /// </summary>
    public static IEnumerable<T> InViewOrder<T>(
        TableView? table,
        IEnumerable<T> rows,
        Func<T, string, IComparable?> sortKey,
        Func<T, object>? groupKey,
        bool sortOrdersGroups)
    {
        var column = ActiveSortColumn(table);
        Func<IEnumerable<T>, IEnumerable<T>> sort = r => r;
        if (column is not null && SortPathOf(column) is { } path)
        {
            sort = column.SortDirection == TableViewSortDirection.Descending
                ? r => r.OrderByDescending(x => sortKey(x, path), SortKeyComparer.Instance)
                : r => r.OrderBy(x => sortKey(x, path), SortKeyComparer.Instance);
        }

        if (groupKey is null)
        {
            return sort(rows);
        }

        return sortOrdersGroups
            ? sort(rows).GroupBy(groupKey).SelectMany(g => g)
            : rows.GroupBy(groupKey).SelectMany(g => sort(g));
    }

    private sealed class SortKeyComparer : IComparer<IComparable?>
    {
        public static readonly SortKeyComparer Instance = new();

        public int Compare(IComparable? x, IComparable? y) => (x, y) switch
        {
            (string a, string b) => string.Compare(a, b, StringComparison.CurrentCulture),
            (null, null) => 0,
            (null, _) => -1,
            (_, null) => 1,
            _ => x!.CompareTo(y),
        };
    }
}
