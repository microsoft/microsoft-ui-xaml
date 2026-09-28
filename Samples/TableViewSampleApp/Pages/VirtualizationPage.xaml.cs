// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
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
    private int _nextRowId = 1;
    private int _mutationRevision;
    private bool _rowsInitialized;

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
        if (!_rowsInitialized)
        {
            var count = SizeSelector?.SelectedItem is ComboBoxItem item
                && int.TryParse(item.Tag?.ToString(), out int selectedCount)
                ? selectedCount : 10_000;
            ApplyRowCount(count);
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

    private void OnUpdateRowClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not NumberedPerson row || !People.Contains(row))
        {
            MutationStatusText.Text = "Select a row first.";
            return;
        }

        row.Role = $"Updated role {++_mutationRevision}";
        MutationStatusText.Text = $"Row {row.Id}: {row.Role}. Same row object and ID.";
    }

    private void OnInsertRowClick(object sender, RoutedEventArgs e)
    {
        var index = PeopleTable.SelectedItem is NumberedPerson selected ? People.IndexOf(selected) : 0;
        var id = _nextRowId++;
        People.Insert(index < 0 ? 0 : index, new NumberedPerson
        {
            Id = id,
            FirstName = "Inserted",
            LastName = $"Row {id}",
            Email = $"inserted.{id}@example.invalid",
            Department = "Engineering",
            Role = "Inserted role",
        });
        MutationStatusText.Text = $"Inserted row {id}. Existing row IDs unchanged.";
        UpdateReadout();
    }

    private void OnRemoveRowClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not NumberedPerson row)
        {
            MutationStatusText.Text = "Select a row first.";
            return;
        }

        if (People.Remove(row))
        {
            MutationStatusText.Text = $"Removed row {row.Id}. Existing row IDs unchanged.";
        }
        else
        {
            MutationStatusText.Text = "The selected row is no longer in the dataset.";
        }
        UpdateReadout();
    }

    private void OnResetRowsClick(object sender, RoutedEventArgs e)
    {
        if (SizeSelector.SelectedItem is ComboBoxItem item &&
            int.TryParse(item.Tag?.ToString(), out int count))
        {
            ApplyRowCount(count);
        }
        else
        {
            MutationStatusText.Text = "Choose a dataset size first.";
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
        _rowsInitialized = true;
        _nextRowId = count + 1;
        _mutationRevision = 0;
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
        if (MutationStatusText is not null)
        {
            MutationStatusText.Text = $"Reset to {count:N0} rows with IDs 1 through {count:N0}.";
        }
        UpdateReadout();
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
        var hasSelectedRow = table?.SelectedItem is NumberedPerson selected && People.Contains(selected);
        if (UpdateRowButton is not null) UpdateRowButton.IsEnabled = hasSelectedRow;
        if (RemoveRowButton is not null) RemoveRowButton.IsEnabled = hasSelectedRow;
        if (SelectedRowsText != null) SelectedRowsText.Text = ((table?.SelectedItem is null ? 0 : 1)).ToString("N0");
        if (ColumnCountText != null) ColumnCountText.Text = (table?.Columns.Count ?? 0).ToString();
    }
}

public sealed class NumberedPerson : INotifyPropertyChanged
{
    private string _role = string.Empty;

    public int Id { get; set; }
    public string FirstName { get; set; } = string.Empty;
    public string LastName { get; set; } = string.Empty;
    public string Email { get; set; } = string.Empty;
    public string Department { get; set; } = string.Empty;
    public string Role
    {
        get => _role;
        set
        {
            if (_role != value)
            {
                _role = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Role)));
            }
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;
}
