// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
// Tabular-namespace aliases: disambiguate from the stale-mock base TableView projection.
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
using TableViewSource = Microsoft.UI.Xaml.Controls.Tabular.TableViewSource;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates the aligned TableView filtering surface: a live
/// <see cref="TableViewSource"/> whose <c>Filter(predicate)</c> narrows the rows the
/// control renders (and <c>ClearFilter()</c> restores them) without mutating the source
/// collection. The predicate combines a free-text query with an "active only" toggle.
/// </summary>
public sealed partial class FilterPage : Page
{
    private readonly ObservableCollection<Person> _people;

    public FilterPage()
    {
        _people = new ObservableCollection<Person>(PersonData.Take(60));
        InitializeComponent();
        BuildColumns();
        ApplyFilter();
    }

    private void BuildColumns()
    {
        FilterTable.Columns.Add(Col("First name", nameof(Person.FirstName), new GridLength(1, GridUnitType.Auto)));
        FilterTable.Columns.Add(Col("Last name", nameof(Person.LastName), new GridLength(1, GridUnitType.Auto)));
        FilterTable.Columns.Add(Col("Department", nameof(Person.Department), new GridLength(1, GridUnitType.Auto)));
        FilterTable.Columns.Add(Col("Role", nameof(Person.Role), new GridLength(1, GridUnitType.Star)));
        FilterTable.Columns.Add(Col("Email", nameof(Person.Email), new GridLength(2, GridUnitType.Star)));
    }

    private static TableViewTextColumn Col(string header, string path, GridLength width) =>
        new()
        {
            Header = header,
            Binding = new Binding { Path = new PropertyPath(path) },
            Width = width,
        };

    private void OnSearchChanged(object sender, TextChangedEventArgs e)
    {
        if (FilterTable is null) return;
        ApplyFilter();
    }

    private void OnActiveToggled(object sender, RoutedEventArgs e)
    {
        if (FilterTable is null) return;
        ApplyFilter();
    }

    private void ApplyFilter()
    {
        string query = SearchBox?.Text?.Trim() ?? string.Empty;
        bool activeOnly = ActiveOnlyToggle?.IsOn == true;

        var source = TableViewSource.From(_people);
        bool filtered = query.Length > 0 || activeOnly;

        // Filter(predicate) narrows the projection; with no criteria we leave the source
        // unfiltered (equivalent to ClearFilter) so all rows show.
        FilterTable.ItemsSource = filtered
            ? source.Filter(item => Match((Person)item, query, activeOnly))
            : source;

        int matched = _people.Count(p => Match(p, query, activeOnly));
        ReadoutText.Text = filtered
            ? string.Format(CultureInfo.InvariantCulture, "Showing {0} of {1} rows (filtered)", matched, _people.Count)
            : string.Format(CultureInfo.InvariantCulture, "Showing all {0} rows", _people.Count);
    }

    private static bool Match(Person person, string query, bool activeOnly)
    {
        if (activeOnly && !person.IsActive)
        {
            return false;
        }

        if (query.Length == 0)
        {
            return true;
        }

        return Contains(person.FirstName, query)
            || Contains(person.LastName, query)
            || Contains(person.Department, query)
            || Contains(person.Role, query)
            || Contains(person.Email, query);
    }

    private static bool Contains(string? value, string query) =>
        value is not null && value.Contains(query, StringComparison.OrdinalIgnoreCase);

    private static string PersonIdentity(object item) => item is Person person ? person.Email : string.Empty;
}
