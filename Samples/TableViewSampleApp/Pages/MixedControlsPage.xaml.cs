// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
// Tabular-namespace type aliases: disambiguate from the stale-mock base Microsoft.UI.Xaml.Controls TableView projection
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates that <c>TableViewTemplateColumn</c> can host arbitrary
/// XAML controls — not just text. Four picker controls (DatePicker,
/// TimePicker, ComboBox, CheckBox) are placed directly inside read-only cells via
/// <c>CellTemplate</c> DataTemplates and bound OneWay to the underlying
/// <see cref="Person"/> row data.
/// </summary>
public sealed partial class MixedControlsPage : Page
{
    // SamplePageHeader rationale also applies here — host XAML's default
    // Page measure pass mismeasures Frame-hosted pages on first render.
    protected override Windows.Foundation.Size MeasureOverride(Windows.Foundation.Size availableSize)
    {
        if (Content is FrameworkElement child)
        {
            child.Measure(availableSize);
            return child.DesiredSize;
        }
        return new Windows.Foundation.Size(0, 0);
    }

    protected override Windows.Foundation.Size ArrangeOverride(Windows.Foundation.Size finalSize)
    {
        if (Content is FrameworkElement child)
        {
            child.Arrange(new Windows.Foundation.Rect(0, 0, finalSize.Width, finalSize.Height));
        }
        return finalSize;
    }

    public MixedControlsPage()
    {
        InitializeComponent();
        // 18 rows is enough to demonstrate scrolling while keeping the cell
        // controls compact in the viewport.
        People = new ObservableCollection<Person>(PersonData.Take(18));
        PeopleTable.ItemsSource = People;
        // Wire default consumer-owned sort + filter so the header click
        // (or programmatic SortDescriptions) re-shapes People. Template
        // columns carry explicit SortMemberPath in XAML so the helper can
        // resolve each key.
        SampleShape.EnableDefaults(PeopleTable, People);
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
    }

    public ObservableCollection<Person> People { get; }

    private Person? _watchedRow;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        if (People.Count > 0)
        {
            _watchedRow = People[0];
            _watchedRow.PropertyChanged += OnWatchedRowChanged;
            RefreshReadout();
        }
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        if (_watchedRow is not null)
        {
            _watchedRow.PropertyChanged -= OnWatchedRowChanged;
            _watchedRow = null;
        }
    }

    private void OnWatchedRowChanged(object? sender, PropertyChangedEventArgs e) => RefreshReadout();

    private void OnDepartmentComboBoxLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is ComboBox comboBox)
        {
            comboBox.ItemsSource = PersonData.Departments;
        }
    }

    private void RefreshReadout()
    {
        if (_watchedRow is null)
        {
            LiveReadout.Text = "(no rows)";
            return;
        }

        LiveReadout.Text =
            $"Name        : {_watchedRow.FullName}\n" +
            $"JoinDate    : {_watchedRow.JoinDate:yyyy-MM-dd}\n" +
            $"ShiftStart  : {_watchedRow.ShiftStart:hh\\:mm}\n" +
            $"Department  : {_watchedRow.Department}\n" +
            $"IsActive    : {_watchedRow.IsActive}";
    }
}
