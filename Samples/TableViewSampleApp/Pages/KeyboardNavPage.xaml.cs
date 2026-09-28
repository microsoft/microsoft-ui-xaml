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
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates keyboard navigation over a read-only TableView and shows
/// row/column counts using the bound source and Columns collection.
/// </summary>
public sealed partial class KeyboardNavPage : Page
{
    public KeyboardNavPage()
    {
        InitializeComponent();

        // Person has no Id/Index property, so project the shared dataset into
        // a lightweight row that carries a stable 1-based "#" for the leading
        // column (same idiom as RTLPlaygroundPage). The
        // ordinal also makes Home/End/PageUp/PageDown movement obvious.
        int number = 1;
        foreach (var p in PersonData.All)
        {
            People.Add(new KeyboardNavRow
            {
                Number = number++,
                FirstName = p.FirstName,
                LastName = p.LastName,
                Email = p.Email,
                Department = p.Department,
                Role = p.Role,
            });
        }

        PeopleTable.ItemsSource = People;
        Loaded += (_, _) => UpdateReadout();
        PeopleTable.SizeChanged += (_, _) => UpdateReadout();
    }

    public ObservableCollection<KeyboardNavRow> People { get; } = new();

    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        SelectedIndexText.Text = PeopleTable.SelectedIndex.ToString();
    }

    private void UpdateReadout()
    {
        RowCountText.Text = People.Count.ToString();
        ColumnCountText.Text = PeopleTable.Columns.Count.ToString();
        MajorText.Text = "RowMajor";
    }

    /// <summary>
/// Demonstrates keyboard navigation over a read-only TableView and shows
/// row/column counts using the bound source and Columns collection.
/// </summary>
    public sealed class KeyboardNavRow
    {
        public int Number { get; set; }
        public string FirstName { get; set; } = string.Empty;
        public string LastName { get; set; } = string.Empty;
        public string Email { get; set; } = string.Empty;
        public string Department { get; set; } = string.Empty;
        public string Role { get; set; } = string.Empty;
    }
}
