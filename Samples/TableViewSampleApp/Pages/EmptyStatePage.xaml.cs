// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableView.EmptyTemplate — the DataTemplate the control renders,
/// centered over the row area, whenever ItemsSource is null or resolves to zero
/// rows. Separate null, empty-collection, and filter-to-empty cases avoid
/// treating those different source configurations as the same test.
/// </summary>
public sealed partial class EmptyStatePage : Page, INotifyPropertyChanged
{
    private string _statusText = string.Empty;
    private readonly ObservableCollection<Person> _emptyRows = new();
    private TableViewSource? _source;

    public EmptyStatePage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(20))
        {
            Rows.Add(person);
        }

        _source = TableViewSource.From(Rows);
        ApplySource("Populated");
    }

    public ObservableCollection<Person> Rows { get; } = new();

    public string StatusText
    {
        get => _statusText;
        private set
        {
            if (_statusText != value)
            {
                _statusText = value;
                OnPropertyChanged();
            }
        }
    }

    private void OnDataSourceChecked(object sender, RoutedEventArgs e)
    {
        if (_source is null || sender is not FrameworkElement { Tag: string tag })
        {
            return;
        }

        ApplySource(tag);
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        DemoTable.DeselectAll();
        if (PopulatedRadio.IsChecked == true) ApplySource("Populated");
        else PopulatedRadio.IsChecked = true;
    }

    private void ApplySource(string mode)
    {
        if (_source is null) return;

        _source.ClearFilter();
        switch (mode)
        {
            case "Null":
                DemoTable.ItemsSource = null;
                StatusText = $"Setup: ItemsSource = null; {Rows.Count} stored objects retained. Expected: empty template.";
                break;
            case "Collection":
                DemoTable.ItemsSource = _emptyRows;
                StatusText = $"Setup: non-null collection with {_emptyRows.Count} objects. Expected: empty template.";
                break;
            case "Filtered":
                _source.Filter(_ => false);
                DemoTable.ItemsSource = _source;
                StatusText = $"Setup: {Rows.Count} source objects; reject-all filter (expected matches: 0). Expected: empty template.";
                break;
            default:
                DemoTable.ItemsSource = _source;
                StatusText = $"Setup: {Rows.Count} source objects; no filter. Expected: data rows, no empty template.";
                break;
        }
        StatusText += " Rendering and UIA not measured.";
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
