// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates runtime column management through the aligned TableView.Columns
/// collection. Show / hide uses TableViewColumn.Visibility, while add / remove
/// demonstrates growing and shrinking the vector. CanUserReorderColumns gates
/// the drag-reorder gesture.
/// </summary>
public sealed partial class DynamicColumnsPage : Page
{
    private const string TenureHeader = "Tenure";

    private TableViewTextColumn[]? _canonical;

    public DynamicColumnsPage()
    {
        People = PersonData.Take(50);
        InitializeComponent();
        DynamicTable.ItemsSource = People;
        _canonical = new[] { ColFirstName, ColLastName, ColDepartment, ColRole, ColSalary };
        SampleShape.EnableDefaults(DynamicTable, People);
        Loaded += (_, _) => UpdateStatus();
    }

    public ObservableCollection<Person> People { get; }

    // ----- Show / hide via TableViewColumn.Visibility -----

    private void OnColumnToggle(object sender, RoutedEventArgs e)
    {
        // CheckBox IsChecked="True" raises Checked during InitializeComponent,
        // before _canonical is assigned. The initial state already matches the
        // XAML, so skipping the init-time raise is a safe no-op.
        if (_canonical is null || DynamicTable is null) return;
        if (sender is CheckBox cb && int.TryParse(cb.Tag?.ToString(), out int index))
        {
            SetColumnVisible(index, cb.IsChecked == true);
        }
    }

    private void SetColumnVisible(int canonicalIndex, bool visible)
    {
        if (_canonical is null) return;
        var column = _canonical[canonicalIndex];
        column.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;

        UpdateStatus();
    }

    // ----- Add / remove an extra column -----

    private void OnAddTenureClick(object sender, RoutedEventArgs e)
    {
        if (DynamicTable.Columns.Any(c => c.Header?.ToString() == TenureHeader))
        {
            UpdateStatus($"{TenureHeader} column already present");
            return;
        }

        DynamicTable.Columns.Add(new TableViewTextColumn
        {
            Header = TenureHeader,
            Width = new GridLength(120),
            Binding = new Binding { Path = new PropertyPath("JoinDateText") },
            SortMemberPath = "JoinDate",
        });
        UpdateStatus($"Added {TenureHeader} column");
    }

    private void OnRemoveTenureClick(object sender, RoutedEventArgs e)
    {
        var tenure = DynamicTable.Columns.FirstOrDefault(c => c.Header?.ToString() == TenureHeader);
        if (tenure is not null)
        {
            DynamicTable.Columns.Remove(tenure);
            UpdateStatus($"Removed {TenureHeader} column");
        }
        else
        {
            UpdateStatus($"{TenureHeader} column not present");
        }
    }

    private void OnAutosizeClick(object sender, RoutedEventArgs e)
    {
        foreach (var c in DynamicTable.Columns) { c.Width = new GridLength(1, GridUnitType.Auto); }
        UpdateStatus("AutoSizeAllColumns()");
    }

    private void OnReorderGateToggled(object sender, RoutedEventArgs e)
    {
        if (DynamicTable is null) return;
        // Column reordering is not in this release: there is no CanUserReorderColumns gate and no
        // drag-reorder gesture. Moving an entry in the Columns vector is how an app reorders.
        UpdateStatus($"Column reordering is not available in this release");
    }

    private void UpdateStatus(string? message = null)
    {
        if (StatusText is null || DynamicTable is null) return;
        var headers = string.Join(" → ", DynamicTable.Columns
            .Where(c => c.Visibility == Visibility.Visible)
            .Select(c => c.Header?.ToString()));
        var prefix = message is null ? string.Empty : $"{message}. ";
        StatusText.Text = $"{prefix}Visible columns: {headers}. Total columns={DynamicTable.Columns.Count}.";
    }
}
