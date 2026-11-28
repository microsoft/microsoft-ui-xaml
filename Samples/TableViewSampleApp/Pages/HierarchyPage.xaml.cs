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
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// A Task Manager style process tree. The CollectionViewSource builds the tree from a flat list by process id and
/// parent process id (CollectionViewSource.HierarchyDescription); TableView presents the view's nodes.
/// </summary>
public sealed partial class HierarchyPage : SamplePageBase
{
    private readonly Random _rng = new(12654);
    private readonly DispatcherTimer _timer = new() { Interval = TimeSpan.FromSeconds(1) };
    private int _nextPid = 30_000;

    public HierarchyPage()
    {
        Reload();

        // <snippet>
        // The CollectionViewSource and its HierarchyDescription and SortDescriptions are declared in the page's markup.
        InitializeComponent();
        Processes.Source = ProcessList;
        ProcessTable.ItemsSource = Processes.View;
        // A ListView over the same view shows the tree's nodes depth first.
        ViewList.ItemsSource = Processes.View;
        // </snippet>
        ProcessTable.SelectionChanged += (_, _) => RefreshReadouts();
        _timer.Tick += (_, _) => Tick();
        TrackTimer(_timer, () => LiveToggle.IsOn);
        InitializeSample(Status);
    }

    // <snippet>
    // One flat list: each child process records its parent's id in ParentPid.
    public ObservableCollection<ProcessItem> ProcessList { get; } = new();
    // </snippet>

    private ProcessItem? Selected => ProcessTable?.SelectedItem as ProcessItem;

    private string Query => FilterBox?.Text?.Trim() ?? string.Empty;

    private void Reload()
    {
        ProcessList.Clear();
        foreach (var process in ProcessData.All())
        {
            ProcessList.Add(process);
            foreach (var child in process.Children)
            {
                ProcessList.Add(child);
            }
        }

        foreach (var process in ProcessList)
        {
            UpdateValues(process);
        }
    }

    // <snippet>
    // A match keeps its ancestors in the view as context, so a matching child process shows under its app.
    private void OnProcessFilter(object sender, FilterEventArgs e) =>
        e.Accepted = Query.Length == 0 || ((ProcessItem)e.Item).Name.Contains(Query, StringComparison.CurrentCultureIgnoreCase);

    private void OnFilterChanged(object sender, TextChangedEventArgs e)
    {
        if (!IsLoaded) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        Processes.Refresh();
        SetLastAction(Query.Length == 0 ? "Filter cleared" : "Filter: " + Query);
    }
    // </snippet>

    // <snippet>
    // The SortDescriptions order every sibling list, the roots included.
    private void OnSortChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!IsLoaded) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        using (Processes.DeferRefresh())
        {
            Processes.SortDescriptions.Clear();
            switch (SortCombo.SelectedIndex)
            {
                case 0: Processes.SortDescriptions.Add(new SortDescription(nameof(ProcessItem.CpuPercent), CollectionViewSortDirection.Descending)); break;
                case 1: Processes.SortDescriptions.Add(new SortDescription(nameof(ProcessItem.MemoryMB), CollectionViewSortDirection.Descending)); break;
                case 2: Processes.SortDescriptions.Add(new SortDescription(nameof(ProcessItem.Name), CollectionViewSortDirection.Ascending)); break;
            }
        }
        SetLastAction("Sort: " + ((ComboBoxItem)SortCombo.SelectedItem).Content);
    }
    // </snippet>

    private void OnLiveToggled(object sender, RoutedEventArgs e)
    {
        if (LiveToggle.IsOn)
        {
            _timer.Start();
        }
        else
        {
            _timer.Stop();
        }
        SetLastAction(LiveToggle.IsOn ? "Live updates on" : "Live updates off");
    }

    // <snippet>
    // IsLiveSortingRequested: a CPU change moves the process among its siblings at the next dispatcher turn.
    private void Tick()
    {
        foreach (var process in ProcessList.Where(_ => _rng.Next(3) == 0).ToList())
        {
            UpdateValues(process);
        }
    }
    // </snippet>

    private void UpdateValues(ProcessItem process)
    {
        process.CpuPercent = Math.Round(_rng.NextDouble() * (process.ParentPid is null ? 12 : 6), 1);
        process.MemoryMB = Math.Round(process.MemoryBaseline * (0.9 + _rng.NextDouble() * 0.2), 1);
    }

    // <snippet>
    private void OnExpandAllClick(object sender, RoutedEventArgs e) => ProcessTable.ExpandAllRows();

    private void OnCollapseAllClick(object sender, RoutedEventArgs e) => ProcessTable.CollapseAllRows();

    // ExpandRow also expands the row's ancestors, so the row is shown.
    private void OnRevealClick(object sender, RoutedEventArgs e)
    {
        var busiest = ProcessList.Where(p => p.ParentPid is not null).OrderByDescending(p => p.CpuPercent).FirstOrDefault();
        if (busiest is null)
        {
            return;
        }

        ProcessTable.ExpandRow(busiest);
        var index = ProcessTable.IndexOf(busiest);
        if (index >= 0)
        {
            ProcessTable.Select(index);
            ProcessTable.StartBringItemIntoView(index, new BringIntoViewOptions());
        }
        SetLastAction("Revealed " + busiest.Name);
    }
    // </snippet>

    // <snippet>
    // A new process whose ParentPid is the selected process's Pid lands under it at its sorted place.
    private void OnStartChildClick(object sender, RoutedEventArgs e)
    {
        var parent = Selected;
        var process = new ProcessItem
        {
            Name = parent is null ? "New app" : parent.Name + " helper",
            Category = parent?.Category ?? ProcessData.Categories[0],
            Pid = _nextPid += 4,
            ParentPid = parent?.Pid,
        };
        UpdateValues(process);
        ProcessList.Add(process);
        if (parent is not null)
        {
            ProcessTable.ExpandRow(parent);
        }
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Started {0} (PID {1})", process.Name, process.Pid));
    }

    // Ending a parent makes its child processes roots: their ParentPid no longer matches a process.
    private void OnEndClick(object sender, RoutedEventArgs e)
    {
        if (Selected is { } process)
        {
            ProcessList.Remove(process);
            SetLastAction("Ended " + process.Name);
        }
    }
    // </snippet>

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        Reload();
        SetLastAction("Processes reset");
    }

    protected override void RefreshReadouts()
    {
        if (Status is null || Processes?.View is null)
        {
            return;
        }

        var hierarchy = (ICollectionViewHierarchy)Processes.View;
        Status.Rows = string.Format(CultureInfo.CurrentCulture, "the view holds {0:N0} of {1:N0} processes", Processes.View.Count, ProcessList.Count);
        Status.ShapingText = "Tree by ParentPid";
        RootsText.Text = (hierarchy.CollectionNodes?.Count ?? 0).ToString(CultureInfo.CurrentCulture);
        if (Selected is { } selected)
        {
            var node = hierarchy.NodeFromItem(selected);
            SelectedText.Text = string.Format(CultureInfo.CurrentCulture, "{0} (PID {1}, {2} child processes{3})",
                selected.Name, selected.Pid, node?.Children.Count ?? 0, node?.IsContext == true ? ", context" : "");
        }
        else
        {
            SelectedText.Text = "(none)";
        }
    }
}
