// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableView.EmptyTemplate — the DataTemplate the control renders,
/// centered over the row area, whenever ItemsSource is null or resolves to zero
/// rows. A radio toggle swaps ItemsSource between the populated collection and
/// null so the empty state can be shown on demand.
/// </summary>
public sealed partial class EmptyStatePage : Page, INotifyPropertyChanged
{
    private string _statusText = string.Empty;

    public EmptyStatePage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(20))
        {
            Rows.Add(person);
        }

        DemoTable.ItemsSource = Rows;
        UpdateStatus(populated: true);
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
        if (DemoTable is null || sender is not FrameworkElement { Tag: string tag })
        {
            return;
        }

        var populated = tag == "Populated";
        // Setting ItemsSource to null resolves to zero rows, so the control swaps
        // in EmptyTemplate; restoring the collection brings the rows back.
        DemoTable.ItemsSource = populated ? Rows : null;
        UpdateStatus(populated);
    }

    private void UpdateStatus(bool populated)
    {
        StatusText = populated
            ? $"Populated · {Rows.Count:N0} rows · EmptyTemplate hidden"
            : "Empty · ItemsSource is null · EmptyTemplate shown";
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
