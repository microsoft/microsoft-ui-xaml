// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;

namespace TableViewSampleApp.Controls;

/// <summary>
/// A XAML content collection that tells its owner about every insert and removal as it happens,
/// so the owner can place the item synchronously (StatusPanel.Readouts, ShapingOptions.GroupKeys).
/// </summary>
internal sealed partial class CallbackCollection<T> : Collection<T>
{
    private readonly Action<int, T> _inserted;
    private readonly Action<int> _removed;

    public CallbackCollection(Action<int, T> inserted, Action<int> removed)
    {
        _inserted = inserted;
        _removed = removed;
    }

    protected override void InsertItem(int index, T item)
    {
        base.InsertItem(index, item);
        _inserted(index, item);
    }

    protected override void RemoveItem(int index)
    {
        base.RemoveItem(index);
        _removed(index);
    }

    protected override void SetItem(int index, T item)
    {
        RemoveItem(index);
        InsertItem(index, item);
    }

    protected override void ClearItems()
    {
        while (Count > 0)
        {
            RemoveItem(Count - 1);
        }
    }
}
