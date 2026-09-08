// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Single-page RTL fixture for the advanced TableView's four RTL-sensitive
/// call sites — frozen transforms, resize gripper alignment, marquee
/// adornment margin, and column drag-reorder drop indicator. Companion to
/// TableView_RTL_APITests.cs, which locks the same invariants in code.
///
/// Data is fully synthetic (Row N, Dept N) — AGENTS.md §12 PII discipline.
/// 50 rows so virtualization engages and the marquee can extend below the
/// realized window (W4-9 extrapolation path).
/// </summary>
public sealed partial class RTLPlaygroundPage : Page
{
    public ObservableCollection<RtlRow> Rows { get; } = new();

    public RTLPlaygroundPage()
    {
        InitializeComponent();

        // 50 rows: covers virtualization + W4-9 marquee extrapolation past
        // the realized window. AGENTS.md §12 — synthetic only.
        for (int i = 0; i < 50; i++)
        {
            Rows.Add(new RtlRow
            {
                Index = i + 1,
                Name = $"Row {i + 1}",
                Department = $"Dept {(i % 6) + 1}",
                Role = $"Role {(i % 4) + 1}",
                Region = $"Region {(i % 5) + 1}",
                JoinDate = $"2026-{((i % 12) + 1):D2}-{((i % 28) + 1):D2}",
                Notes = $"Notes for row {i + 1}",
                Salary = 50000 + (i * 750),
                Status = (i % 3 == 0) ? "Active" : "Inactive",
            });
        }

        PlaygroundTable.ItemsSource = Rows;

        // Hook PlaygroundTable selection + reorder events to feed the
        // readout panel. AGENTS.md §12 + tools/check_event_leaks.ps1:
        // symmetric Loaded/Unloaded detach is mandatory in sample pages,
        // even when the handlers and the host share lifetime — the static
        // scanner enforces the pattern, and re-entry through navigation
        // cache could otherwise stack handlers.
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
    }

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        PlaygroundTable.SelectionChanged += OnSelectionChanged;
        UpdateReadout();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        if (PlaygroundTable != null)
        {
            PlaygroundTable.SelectionChanged -= OnSelectionChanged;
        }
    }

    private void OnRtlToggled(object sender, RoutedEventArgs e)
    {
        if (sender is ToggleSwitch toggle && PlaygroundTable != null)
        {
            PlaygroundTable.FlowDirection = toggle.IsOn
                ? FlowDirection.RightToLeft
                : FlowDirection.LeftToRight;
            UpdateReadout();
        }
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        string[] order = ["#", "Name", "Department", "Role", "Region", "Join date", "Notes", "Salary", "Status"];
        for (int target = 0; target < order.Length; target++)
        {
            var current = -1;
            for (int i = 0; i < PlaygroundTable.Columns.Count; i++)
            {
                if (PlaygroundTable.Columns[i].Header?.ToString() == order[target]) { current = i; break; }
            }
            if (current >= 0 && current != target) { var moved = PlaygroundTable.Columns[current]; PlaygroundTable.Columns.RemoveAt(current); PlaygroundTable.Columns.Insert(target, moved); }
        }
        LastReorderText.Text = "Reset column order.";
    }
    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs e)
    {
        UpdateReadout();
    }

    // Column reordering is not part of this release: there is no CanUserReorderColumns and
    // no ColumnReordered event. Moving an entry in the Columns vector is how an app reorders
    // columns itself - see the Dynamic columns page.

    private void UpdateReadout()
    {
        // Reachable during InitializeComponent (the RTL ToggleSwitch sets IsOn in XAML,
        // raising Toggled before the Options-rail readout TextBlocks are created). Guard so
        // the init-time call no-ops; OnPageLoaded re-runs UpdateReadout.
        if (PlaygroundTable is null || FlowDirectionText is null
            || RowsLoadedText is null || SelectedCountText is null)
        {
            return;
        }

        FlowDirectionText.Text = PlaygroundTable.FlowDirection.ToString();
        RowsLoadedText.Text = Rows.Count.ToString();

        try
        {
            SelectedCountText.Text = (PlaygroundTable.SelectedItem is null ? 0 : 1).ToString();
        }
        catch
        {
            SelectedCountText.Text = "0";
        }
    }
}

/// <summary>
/// Local row record for the RTL playground. Independent of Models.Person so
/// the page doesn't drag in unrelated sample columns (Email / IsActive /
/// ShiftStart) and so the synthetic-data discipline is obvious at a glance.
/// </summary>
public sealed class RtlRow
{
    public int Index { get; set; }
    public string Name { get; set; } = string.Empty;
    public string Department { get; set; } = string.Empty;
    public string Role { get; set; } = string.Empty;
    public string Region { get; set; } = string.Empty;
    public string JoinDate { get; set; } = string.Empty;
    public string Notes { get; set; } = string.Empty;
    public double Salary { get; set; }
    public string Status { get; set; } = string.Empty;
}
