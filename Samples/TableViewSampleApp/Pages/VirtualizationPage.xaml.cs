// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSampleApp.Data;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates row virtualization over TableView.
///
/// The page consumes only the aligned public surface: ItemsSource for the bound
/// dataset and SelectedItems / Columns for the status readout. The control
/// exposes no realized-row count or scroll offset, so the readout reports the
/// data-side counts the control does surface.
/// </summary>
public sealed partial class VirtualizationPage : Page
{
    public VirtualizationPage()
    {
        InitializeComponent();
        ApplyInMemoryAsActive();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
    }

    public ObservableCollection<NumberedPerson> People { get; } = new();

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        PeopleTable.SelectionChanged -= OnTableSelectionChanged;
        PeopleTable.SelectionChanged += OnTableSelectionChanged;
        if (People.Count == 0)
        {
            ApplyRowCount(10_000);
        }
        UpdateReadout();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        PeopleTable.SelectionChanged -= OnTableSelectionChanged;
    }

    private void OnTableSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
        => UpdateReadout();

    private void OnSizeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (SizeSelector?.SelectedItem is ComboBoxItem item
            && int.TryParse(item.Tag?.ToString(), out int n))
        {
            ApplyRowCount(n);
            UpdateReadout();
        }
    }

    private void OnAutosizeClick(object sender, RoutedEventArgs e)
    {
        // There is no AutoSizeAllColumns in this release. Auto-sizing is a width INTENT, so
        // setting each column's Width to Auto is the equivalent.
        if (PeopleTable is not null)
        {
            foreach (var column in PeopleTable.Columns)
            {
                column.Width = new GridLength(1, GridUnitType.Auto);
            }
        }
        UpdateReadout();
    }

    private void OnRefreshClick(object sender, RoutedEventArgs e) => UpdateReadout();

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
        People.Clear();
        var seed = PersonData.All;
        for (int i = 0; i < count; i++)
        {
            var p = seed[i % seed.Count];
            People.Add(new NumberedPerson
            {
                Id = i + 1,
                FirstName = p.FirstName,
                LastName = p.LastName,
                Email = p.Email,
                Department = p.Department,
                Role = p.Role,
            });
        }
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
