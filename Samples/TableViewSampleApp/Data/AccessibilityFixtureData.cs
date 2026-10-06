// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#nullable enable
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Data;

/// <summary>
/// BCL-only fixture data. TableViewSource owns shaping; this collection stays in source order.
/// Keeping the original objects is essential: TableViewSource uses object identity, not our ID.
/// </summary>
public sealed class AccessibilityFixtureData
{
    private readonly AccessibilityRow[] _originals =
        Enumerable.Range(1, 24).Select(id => new AccessibilityRow(id)).ToArray();

    public AccessibilityFixtureData() => Reset();

    public ObservableCollection<AccessibilityRow> Rows { get; } = new();
    public IReadOnlyList<AccessibilityRow> OriginalRows => _originals;
    public AccessibilityRow LastRow => _originals[_originals.Length - 1];
    public int RemovedCount => _originals.Length - Rows.Count;

    public bool Mutate(AccessibilityRow? row)
    {
        if (row is null || !Rows.Contains(row)) return false;

        var change = row.DisplayText == OriginalText(row);
        row.DisplayText = OriginalText(row) + (change ? " (changed)" : string.Empty);
        var originalDepartment = OriginalDepartment(row);
        row.Department = change
            ? (originalDepartment == "Design" ? "Engineering" : "Design")
            : originalDepartment;
        return true;
    }

    public bool Remove(AccessibilityRow? row) => row is not null && Rows.Remove(row);

    public int RestoreRemoved()
    {
        var restored = 0;
        for (var i = 0; i < _originals.Length; i++)
        {
            if (!Rows.Contains(_originals[i]))
            {
                Rows.Insert(i, _originals[i]);
                restored++;
            }
        }
        return restored;
    }

    public void Reset()
    {
        Rows.Clear();
        foreach (var row in _originals)
        {
            row.DisplayText = OriginalText(row);
            row.Department = OriginalDepartment(row);
            Rows.Add(row);
        }
    }

    private static string OriginalText(AccessibilityRow row) =>
        $"Record {row.Id.ToString("00", CultureInfo.InvariantCulture)}";

    private static string OriginalDepartment(AccessibilityRow row) =>
        row.Id % 2 == 1 ? "Design" : "Engineering";
}
