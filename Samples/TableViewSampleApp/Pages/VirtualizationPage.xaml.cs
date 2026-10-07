// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewRow = Microsoft.UI.Xaml.Controls.Tabular.TableViewRow;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSampleApp.Data;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates row virtualization over TableView.
///
/// The control surfaces no realized-row count, so the readout derives one by walking the
/// visual tree for <see cref="TableViewRow"/> — a public type (TableView.idl:423), not a
/// private template part. That is what makes "Total rows" and "Realized rows" diverge:
/// the whole point of the page.
/// </summary>
public sealed partial class VirtualizationPage : Page
{
    private readonly DispatcherTimer _sampler = new() { Interval = System.TimeSpan.FromMilliseconds(500) };
    private int _peakRealized;

    public VirtualizationPage()
    {
        InitializeComponent();
        ApplyInMemoryAsActive();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        _sampler.Tick += OnSamplerTick;
    }

    public ObservableCollection<NumberedPerson> People { get; private set; } = new();

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        PeopleTable.SelectionChanged -= OnTableSelectionChanged;
        PeopleTable.SelectionChanged += OnTableSelectionChanged;
        if (People.Count == 0)
        {
            ApplyRowCount(10_000);
        }
        UpdateReadout();
        // Realization changes as the user scrolls and the control raises no event for it,
        // so the readout is sampled. Stopped in Unloaded.
        _sampler.Start();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        _sampler.Stop();
        PeopleTable.SelectionChanged -= OnTableSelectionChanged;
    }

    private void OnSamplerTick(object? sender, object e) => UpdateReadout();

    private void OnTableSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
        => UpdateReadout();

    private void OnRowCountChanged(object sender, SelectionChangedEventArgs e)
    {
        if (SizeSelector?.SelectedItem is ComboBoxItem item
            && int.TryParse(item.Tag?.ToString(), out int n))
        {
            ApplyRowCount(n);
            UpdateReadout();
        }
    }

    private void ApplyInMemoryAsActive()
    {
        if (PeopleTable != null)
        {
            PeopleTable.ItemsSource = People;
        }
        if (SizeSelector != null) SizeSelector.IsEnabled = true;
        if (SourceModeText != null) SourceModeText.Text = "In-memory";
        UpdateReadout();
    }

    private void ApplyRowCount(int count)
    {
        // Build off the bind path first. Adding 50,000 items to a bound ObservableCollection
        // one at a time raises 50,000 CollectionChanged notifications on the UI thread, which
        // would stall the very page that exists to show that scrolling stays responsive.
        var seed = PersonData.All;
        var rows = new List<NumberedPerson>(count);
        for (int i = 0; i < count; i++)
        {
            var p = seed[i % seed.Count];
            rows.Add(new NumberedPerson
            {
                Id = i + 1,
                FirstName = p.FirstName,
                LastName = p.LastName,
                Email = p.Email,
                Department = p.Department,
                Role = p.Role,
            });
        }

        People = new ObservableCollection<NumberedPerson>(rows);
        _peakRealized = 0;
        if (PeopleTable != null)
        {
            PeopleTable.ItemsSource = People;
        }
    }

    /// <summary>
    /// Counts realized row containers. TableViewRow is public API, so this stays off the
    /// control's private template parts. Rows never nest, so recursion stops at each hit.
    /// </summary>
    private static int CountRealizedRows(DependencyObject? root, ref double firstRowHeight)
    {
        if (root is null)
        {
            return 0;
        }

        int realized = 0;
        int children = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < children; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is TableViewRow row)
            {
                realized++;
                if (firstRowHeight <= 0 && row.ActualHeight > 0)
                {
                    firstRowHeight = row.ActualHeight;
                }
            }
            else
            {
                realized += CountRealizedRows(child, ref firstRowHeight);
            }
        }

        return realized;
    }

    private void UpdateReadout()
    {
        int total = People.Count;
        if (TotalRowsText != null) TotalRowsText.Text = total.ToString("N0");

        // PeopleTable can be null while UpdateReadout runs synchronously during
        // InitializeComponent: the Options-rail ComboBoxes set SelectedIndex in
        // XAML, which raises SelectionChanged before the later-declared TableView
        // field is assigned. Guard the table-derived readouts; OnPageLoaded
        // re-runs UpdateReadout once the table is wired.
        var table = PeopleTable;
        if (SelectedRowsText != null) SelectedRowsText.Text = ((table?.SelectedItem is null ? 0 : 1)).ToString("N0");
        if (ColumnCountText != null) ColumnCountText.Text = (table?.Columns.Count ?? 0).ToString();

        double rowHeight = 0;
        int realized = CountRealizedRows(table, ref rowHeight);
        if (realized > _peakRealized) _peakRealized = realized;

        if (RealizedRowsText != null) RealizedRowsText.Text = realized.ToString("N0");
        if (PeakRealizedText != null) PeakRealizedText.Text = _peakRealized.ToString("N0");
        if (RowHeightText != null) RowHeightText.Text = rowHeight > 0 ? $"{rowHeight:F0} px" : "—";
        if (RealizedShareText != null)
        {
            RealizedShareText.Text = total > 0
                ? $"{(double)realized / total:P2} of {total:N0}"
                : "—";
        }
    }
}

public sealed class NumberedPerson
{
    public int Id { get; set; }
    public string FirstName { get; set; } = string.Empty;
    public string LastName { get; set; } = string.Empty;
    public string Email { get; set; } = string.Empty;
    public string Department { get; set; } = string.Empty;
    public string Role { get; set; } = string.Empty;
}
