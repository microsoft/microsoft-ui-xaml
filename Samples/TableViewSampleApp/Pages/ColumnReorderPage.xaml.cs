// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewDensity = Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Column reordering: CanUserReorderColumns, ColumnReordered, frozen leading columns, RTL and density.
/// </summary>
public sealed partial class ColumnReorderPage : SamplePageBase
{
    private const int LogLength = 10;
    private readonly List<TableViewColumn> _defaultOrder;

    public ColumnReorderPage()
    {
        InitializeComponent();
        _defaultOrder = PeopleTable.Columns.ToList();
        InitializeSample(Status);
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public ObservableCollection<string> Log { get; } = new();

    // <snippet>
    private void OnColumnReordered(TableView sender, TableViewColumnReorderedEventArgs args)
    {
        // Raised after a user move; OldIndex and NewIndex are positions in Columns.
        Log.Insert(0, string.Format(CultureInfo.CurrentCulture, "{0}: {1} -> {2}", args.Column.Header, args.OldIndex, args.NewIndex));
        while (Log.Count > LogLength) // snippet:skip
        { // snippet:skip
            Log.RemoveAt(Log.Count - 1); // snippet:skip
        } // snippet:skip
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} to column {1}", args.Column.Header, args.NewIndex + 1));
    }
    // </snippet>

    // The rail's initial values raise these handlers while the page is still being parsed.
    private void OnReorderToggled(object sender, RoutedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        PeopleTable.CanUserReorderColumns = ReorderToggle.IsOn;
        SetLastAction("CanUserReorderColumns -> " + ReorderToggle.IsOn);
    }

    private void OnFrozenChanged(object sender, SelectionChangedEventArgs e)
    {
        if (IsLoaded)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Frozen leading columns -> {0}", ApplyFrozenColumns()));
        }
    }

    // Freezes the first N columns in their current order; only a leading prefix can be frozen.
    private int ApplyFrozenColumns()
    {
        int count = int.Parse((string)((ComboBoxItem)FrozenSelector.SelectedItem).Tag, CultureInfo.InvariantCulture);
        for (int i = 0; i < PeopleTable.Columns.Count; i++)
        {
            PeopleTable.Columns[i].FrozenEdge = i < count ? TableViewFrozenEdge.Leading : TableViewFrozenEdge.None;
        }
        return count;
    }

    private void OnRtlToggled(object sender, RoutedEventArgs e)
    {
        PeopleTable.FlowDirection = RtlToggle.IsOn ? FlowDirection.RightToLeft : FlowDirection.LeftToRight;
        SetLastAction("FlowDirection -> " + PeopleTable.FlowDirection);
    }

    private void OnDensityChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        PeopleTable.Density = Enum.Parse<TableViewDensity>((string)((ComboBoxItem)DensitySelector.SelectedItem).Tag);
        SetLastAction("Density -> " + PeopleTable.Density);
    }

    // A move in code raises no ColumnReordered, so Reset leaves the log alone.
    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        for (int target = 0; target < _defaultOrder.Count; target++)
        {
            int current = PeopleTable.Columns.IndexOf(_defaultOrder[target]);
            if (current != target)
            {
                PeopleTable.Columns.RemoveAt(current);
                PeopleTable.Columns.Insert(target, _defaultOrder[target]);
            }
        }
        ApplyFrozenColumns();
        SetLastAction("Column order -> reset");
    }
}
