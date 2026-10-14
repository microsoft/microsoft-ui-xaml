// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Helpers;

/// <summary>
/// The parent/child relation a page offers to the Hierarchy shaping modes: the two selectors
/// TableViewSource.ParentBy takes. KeyOf returns a row's unique key, ParentKeyOf its parent's key
/// (null for a root; a key no row has makes an orphan root). Label names the relation in readouts.
/// </summary>
public sealed class ParentRelation
{
    public ParentRelation(string label, Func<object?, object?> keyOf, Func<object?, object?> parentKeyOf)
    {
        Label = label;
        KeyOf = keyOf;
        ParentKeyOf = parentKeyOf;
    }

    /// <summary>Person rows: EmployeeId under ManagerId (the org chart).</summary>
    public static ParentRelation Manager { get; } = new(
        "Manager",
        row => (row as Person)?.EmployeeId,
        row => (row as Person)?.ManagerId);

    public string Label { get; }

    public Func<object?, object?> KeyOf { get; }

    public Func<object?, object?> ParentKeyOf { get; }

    // Readout helpers below: TableView does not expose its projection, so pages compute what it shows.

    /// <summary>Maps each row to the root of its subtree among <paramref name="rows"/> (orphans are roots).</summary>
    public Func<T, T> RootMap<T>(IEnumerable<T> rows)
        where T : class
    {
        var byKey = ByKey(rows);
        return row =>
        {
            var current = row;
            for (var hops = 0; hops <= byKey.Count && ParentKeyOf(current) is { } parentKey && byKey.TryGetValue(parentKey, out var parent); hops++)
            {
                current = parent;
            }

            return current;
        };
    }

    /// <summary>
    /// Rows a filter keeps under ParentBy: every match plus its ancestors. The ancestors are shown as
    /// ordinary rows (selectable, nothing marks them as context), so they count as shown.
    /// </summary>
    public int CountWithAncestors<T>(IEnumerable<T> rows, Func<T, bool> matches)
        where T : class
    {
        var list = rows as IReadOnlyCollection<T> ?? rows.ToList();
        var byKey = ByKey(list);
        var kept = new HashSet<T>(ReferenceEqualityComparer.Instance);
        foreach (var row in list.Where(matches))
        {
            var current = row;
            while (kept.Add(current) && ParentKeyOf(current) is { } parentKey && byKey.TryGetValue(parentKey, out var parent))
            {
                current = parent;
            }
        }

        return kept.Count;
    }

    /// <summary>
    /// The rows in tree order with every row expanded: each root (in source order) followed by its
    /// subtree, siblings in source order. That is the display order of an unsorted ParentBy source.
    /// </summary>
    public List<T> TreeOrder<T>(IReadOnlyList<T> rows)
        where T : class
    {
        var byKey = ByKey(rows);
        var children = new Dictionary<T, List<T>>(ReferenceEqualityComparer.Instance);
        var roots = new List<T>();
        foreach (var row in rows)
        {
            if (ParentKeyOf(row) is { } parentKey && byKey.TryGetValue(parentKey, out var parent) && !ReferenceEquals(parent, row))
            {
                if (!children.TryGetValue(parent, out var list))
                {
                    children[parent] = list = new List<T>();
                }

                list.Add(row);
            }
            else
            {
                roots.Add(row);
            }
        }

        var ordered = new List<T>(rows.Count);
        var stack = new Stack<T>(Enumerable.Reverse(roots));
        while (stack.Count > 0)
        {
            var row = stack.Pop();
            ordered.Add(row);
            if (children.TryGetValue(row, out var list))
            {
                for (var i = list.Count - 1; i >= 0; i--)
                {
                    stack.Push(list[i]);
                }
            }
        }

        return ordered;
    }

    private Dictionary<object, T> ByKey<T>(IEnumerable<T> rows)
        where T : class
    {
        var byKey = new Dictionary<object, T>();
        foreach (var row in rows)
        {
            if (KeyOf(row) is { } key)
            {
                byKey.TryAdd(key, row);
            }
        }

        return byKey;
    }
}
