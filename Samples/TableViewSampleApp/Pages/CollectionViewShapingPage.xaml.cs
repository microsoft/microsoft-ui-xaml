// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// CollectionViewSource shaping: the framework's CollectionViewSource sorts, filters and groups a
/// flat process list, and TableView renders its View. No TableViewSource is involved.
/// </summary>
public sealed partial class CollectionViewShapingPage : SamplePageBase
{
    private readonly Random _rng = new(7);

    public CollectionViewShapingPage()
    {
        foreach (var process in Processes)
        {
            UpdateValues(process);
        }

        // <snippet>
        Shaped = new CollectionViewSource { Source = Processes };
        Shaped.GroupDescriptions.Add(new PropertyGroupDescription(nameof(ProcessItem.Category)));
        InitializeComponent();
        // A view the CollectionViewSource shapes implements ICollectionViewShaping, so a header
        // click writes the view's SortDescriptions.
        ProcessTable.ItemsSource = Shaped.View;
        // </snippet>
        ProcessTable.Sorted += (_, _) => RefreshReadouts();
        InitializeSample(Status);
    }

    // <snippet>
    public ObservableCollection<ProcessItem> Processes { get; } = ProcessData.All();
    // </snippet>

    // <snippet>
    public CollectionViewSource Shaped { get; }
    // </snippet>

    // <snippet>
    private string Query => FilterBox?.Text?.Trim() ?? string.Empty;
    // </snippet>

    // <snippet>
    private void OnFilterChanged(object sender, TextChangedEventArgs e)
    {
        if (!IsLoaded) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        var query = Query;
        Shaped.Filter = query.Length == 0
            ? null
            : new CollectionViewFilter(item => ((ProcessItem)item).Name.Contains(query, StringComparison.CurrentCultureIgnoreCase));
        SetLastAction(query.Length == 0 ? "Filter cleared: " + ShownSummary() : "Filter set: " + ShownSummary());
    }
    // </snippet>

    // <snippet>
    private void OnGroupToggled(object sender, RoutedEventArgs e)
    {
        if (!IsLoaded) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        if (GroupToggle.IsOn)
        {
            Shaped.GroupDescriptions.Add(new PropertyGroupDescription(nameof(ProcessItem.Category)));
        }
        else
        {
            Shaped.GroupDescriptions.Clear();
        }

        // Switching between flat and grouped output gives the CollectionViewSource a new View.
        ProcessTable.ItemsSource = Shaped.View;
        SetLastAction(GroupToggle.IsOn ? "Grouped by Category" : "Grouping cleared");
    }
    // </snippet>

    // <snippet>
    private void OnSortByMemoryClick(object sender, RoutedEventArgs e)
    {
        Shaped.SortDescriptions.Clear();
        Shaped.SortDescriptions.Add(new SortDescription(nameof(ProcessItem.MemoryMB), ListSortDirection.Descending));
        SetLastAction("SortDescriptions: MemoryMB descending");
    }
    // </snippet>

    // <snippet>
    private void OnRefreshClick(object sender, RoutedEventArgs e)
    {
        foreach (var process in Processes)
        {
            UpdateValues(process);
        }

        // A property edit is not a collection change: Refresh() sorts, filters and groups again.
        ((ICollectionViewShaping)Shaped.View).Refresh();
        SetLastAction("Updated CPU and called Refresh()");
    }
    // </snippet>

    private void UpdateValues(ProcessItem process)
    {
        process.CpuPercent = Math.Round(_rng.NextDouble() * (process.Children.Count > 0 ? 12 : 4), 1);
        process.MemoryMB = Math.Round(process.MemoryBaseline * (0.9 + _rng.NextDouble() * 0.2), 1);
    }

    protected override void RefreshReadouts()
    {
        if (Status is null || Shaped is null)
        {
            return;
        }

        Status.Rows = ShownSummary();
        Status.ShapingText = Shaped.GroupDescriptions.Count > 0 ? "Grouped by Category" : "Flat";
        SortStateText.Text = Shaped.SortDescriptions.Count == 0
            ? "(none)"
            : string.Join(", ", Shaped.SortDescriptions.Select(d => string.Format(CultureInfo.CurrentCulture, "{0} {1}", d.PropertyName, d.Direction)));
    }

    private string ShownSummary() =>
        string.Format(CultureInfo.CurrentCulture, "showing {0:N0} of {1:N0} processes", Shaped.View.Count, Processes.Count);
}
