// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Consolidates the column-layout validation scenarios into one page: live width
/// tweaking, Auto, Star, Pixel overflow, and Mixed sizing strategies.
/// </summary>
public sealed partial class LayoutPage : Page
{
    private const string PlaygroundMode = "Playground";
    private const string PixelMode = "Pixel";

    private bool _suppressSliderHandlers;
    private TableViewTextColumn? _activeColumn;
    private ScrollViewer? _headerScroller;
    private ScrollViewer? _bodyScroller;

    public LayoutPage()
    {
        People = PersonData.Take(50);
        InitializeComponent();
        WidthsTable.ItemsSource = People;

        Loaded += (_, _) =>
        {
            ApplySelectedMode();
            AttachHorizontalSyncReadout();
        };
    }

    public ObservableCollection<Person> People { get; }

    private static TableViewTextColumn Text(string header, string path, GridLength width) =>
        new()
        {
            Header = header,
            Binding = new Binding { Path = new PropertyPath(path) },
            Width = width,
            CanSort = false,
        };

    private static GridLength Auto() => new(1, GridUnitType.Auto);
    private static GridLength Star(double factor = 1) => new(factor, GridUnitType.Star);
    private static GridLength Px(double px) => new(px, GridUnitType.Pixel);

    private void OnModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (WidthsTable is null || PlaygroundPanel is null)
        {
            return;
        }

        ApplySelectedMode();
    }

    private void ApplySelectedMode()
    {
        if (ModeSelector.SelectedItem is not string mode || ModeDescription is null || PlaygroundPanel is null)
        {
            return;
        }

        WidthsTable.Columns.Clear();
        PlaygroundPanel.Visibility = mode == PlaygroundMode ? Visibility.Visible : Visibility.Collapsed;

        switch (mode)
        {
            case PlaygroundMode:
                BuildPlaygroundColumns();
                PopulateColumnCombo();
                SetActiveColumn(WidthsTable.Columns[0] as TableViewTextColumn);
                ModeDescription.Text = "Playground — pick a column and tweak MinWidth, Width, and MaxWidth live while watching ActualWidth clamp.";
                break;

            case "Auto":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Auto()));
                WidthsTable.Columns.Add(Text("Last name", nameof(Person.LastName), Auto()));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Auto()));
                WidthsTable.Columns.Add(Text("Role", nameof(Person.Role), Auto()));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Auto()));
                ModeDescription.Text = "All Auto columns — every column uses GridUnitType.Auto and sizes to the widest realized header or cell content.";
                break;

            case "Star":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Star(1)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Star(1)));
                WidthsTable.Columns.Add(Text("Role", nameof(Person.Role), Star(2)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Star(3)));
                ModeDescription.Text = "All Star columns — every column shares remaining width by factor, so Email (*3) gets three times First name (*1).";
                break;

            case PixelMode:
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Px(180)));
                WidthsTable.Columns.Add(Text("Last name", nameof(Person.LastName), Px(180)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Px(320)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Px(220)));
                WidthsTable.Columns.Add(Text("Role", nameof(Person.Role), Px(260)));
                WidthsTable.Columns.Add(Text("Join date", nameof(Person.JoinDateText), Px(180)));
                WidthsTable.Columns.Add(Text("Bio", nameof(Person.Bio), Px(520)));
                ModeDescription.Text = "All Pixel columns — every column has a fixed GridUnitType.Pixel width, independent of content. Their combined width exceeds a narrow window, so the body scrolls horizontally and the sticky header stays in sync.";
                break;

            case "Mixed":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Auto()));
                WidthsTable.Columns.Add(Text("Role", nameof(Person.Role), Px(160)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Star(1)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Star(2)));
                WidthsTable.Columns.Add(Text("Bio", nameof(Person.Bio), Star(3)));
                ModeDescription.Text = "Mixed columns — Auto and fixed Pixel columns are sized first, then Star columns split the remaining width.";
                break;
        }

        AttachHorizontalSyncReadout();
        UpdateLabelsAndReadout();
        QueueHorizontalSyncReadout();
    }

    private void BuildPlaygroundColumns()
    {
        WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Px(160)));
        WidthsTable.Columns.Add(Text("Last name", nameof(Person.LastName), Px(160)));
        WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Px(220)));
        WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Px(140)));
        var roleColumn = Text("Role (locked)", nameof(Person.Role), Px(200));
        roleColumn.CanResize = false;
        WidthsTable.Columns.Add(roleColumn);
    }

    private void PopulateColumnCombo()
    {
        ColumnCombo.Items.Clear();
        ColumnCombo.Items.Add("First name");
        ColumnCombo.Items.Add("Last name");
        ColumnCombo.Items.Add("Email");
        ColumnCombo.Items.Add("Department");
        ColumnCombo.Items.Add("Role (locked)");
        ColumnCombo.SelectedIndex = 0;
    }

    private void OnColumnComboChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ColumnCombo.SelectedItem is not string label)
        {
            return;
        }

        TableViewTextColumn? next = label switch
        {
            "First name" => FindColumn("First name"),
            "Last name" => FindColumn("Last name"),
            "Email" => FindColumn("Email"),
            "Department" => FindColumn("Department"),
            "Role (locked)" => FindColumn("Role (locked)"),
            _ => null,
        };
        SetActiveColumn(next);
    }

    private TableViewTextColumn? FindColumn(string header)
    {
        foreach (var column in WidthsTable.Columns)
        {
            if (column is TableViewTextColumn textColumn
                && string.Equals(textColumn.Header?.ToString(), header, StringComparison.Ordinal))
            {
                return textColumn;
            }
        }

        return null;
    }

    private void SetActiveColumn(TableViewTextColumn? column)
    {
        _activeColumn = column;
        if (column is null)
        {
            SelectedColumnText.Text = "(none)";
            WidthReadoutText.Text = "?";
            return;
        }

        _suppressSliderHandlers = true;
        try
        {
            var maxForSlider = double.IsInfinity(column.MaxWidth)
                ? MaxWidthSlider.Maximum
                : Math.Min(column.MaxWidth, MaxWidthSlider.Maximum);

            MinWidthSlider.Value = Math.Min(column.MinWidth, MinWidthSlider.Maximum);
            WidthSlider.Value = Math.Min(column.Width.Value, WidthSlider.Maximum);
            MaxWidthSlider.Value = maxForSlider;
        }
        finally
        {
            _suppressSliderHandlers = false;
        }

        SelectedColumnText.Text = column.Header?.ToString() ?? "(unnamed)";
        UpdateLabelsAndReadout();
    }

    private void OnMinWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null)
        {
            return;
        }

        _activeColumn.MinWidth = e.NewValue;
        UpdateLabelsAndReadout();
    }

    private void OnWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null)
        {
            return;
        }

        _activeColumn.Width = new GridLength(e.NewValue);
        UpdateLabelsAndReadout();
        QueueHorizontalSyncReadout();
    }

    private void OnMaxWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null)
        {
            return;
        }

        _activeColumn.MaxWidth = e.NewValue;
        UpdateLabelsAndReadout();
    }

    private void UpdateLabelsAndReadout()
    {
        if (MinWidthValue is null)
        {
            return;
        }

        var inv = CultureInfo.InvariantCulture;
        MinWidthValue.Text = MinWidthSlider.Value.ToString("0", inv);
        WidthValue.Text = WidthSlider.Value.ToString("0", inv);
        MaxWidthValue.Text = MaxWidthSlider.Value.ToString("0", inv);

        if (_activeColumn is not null)
        {
            WidthReadoutText.Text =
                $"{_activeColumn.Width.Value.ToString("0", inv)} / {_activeColumn.ActualWidth.ToString("0", inv)}";
        }
    }

    private void AttachHorizontalSyncReadout()
    {
        DetachHorizontalSyncReadout();

        _headerScroller = FindDescendantByName<ScrollViewer>(WidthsTable, "PART_HeaderScroller");
        _bodyScroller = FindDescendantByName<ScrollViewer>(WidthsTable, "PART_BodyScroller");

        if (_headerScroller is not null)
        {
            _headerScroller.ViewChanged += OnScrollViewerViewChanged;
        }
        if (_bodyScroller is not null)
        {
            _bodyScroller.ViewChanged += OnScrollViewerViewChanged;
        }
    }

    private void DetachHorizontalSyncReadout()
    {
        if (_headerScroller is not null)
        {
            _headerScroller.ViewChanged -= OnScrollViewerViewChanged;
        }
        if (_bodyScroller is not null)
        {
            _bodyScroller.ViewChanged -= OnScrollViewerViewChanged;
        }

        _headerScroller = null;
        _bodyScroller = null;
    }

    private void OnScrollViewerViewChanged(object? sender, ScrollViewerViewChangedEventArgs e)
    {
        QueueHorizontalSyncReadout();
    }

    private void QueueHorizontalSyncReadout()
    {
        DispatcherQueue.TryEnqueue(UpdateHorizontalSyncReadout);
    }

    private void UpdateHorizontalSyncReadout()
    {
        if (HorizontalSyncText is null)
        {
            return;
        }

        if (ModeSelector.SelectedItem is not string mode || mode != PixelMode)
        {
            HorizontalSyncText.Text = "Select Pixel mode, then scroll horizontally to validate that the sticky header tracks the body offset.";
            return;
        }

        if (_headerScroller is null || _bodyScroller is null)
        {
            AttachHorizontalSyncReadout();
        }

        if (_headerScroller is null || _bodyScroller is null)
        {
            HorizontalSyncText.Text = "Waiting for the TableView header and body scrollers to load.";
            return;
        }

        var headerOffset = _headerScroller.HorizontalOffset;
        var bodyOffset = _bodyScroller.HorizontalOffset;
        var delta = Math.Abs(headerOffset - bodyOffset);
        var totalPixelWidth = TotalPixelWidth();
        var status = delta <= 0.5 ? "Synced" : "Desynced";
        HorizontalSyncText.Text =
            $"{status}: header {headerOffset:0.##} px / body {bodyOffset:0.##} px (delta {delta:0.##} px). Pixel columns total {totalPixelWidth:0} px.";
    }

    private double TotalPixelWidth()
    {
        double total = 0;
        foreach (var column in WidthsTable.Columns)
        {
            if (column.Width.GridUnitType == GridUnitType.Pixel)
            {
                total += column.Width.Value;
            }
        }

        return total;
    }

    private static T? FindDescendantByName<T>(DependencyObject root, string name)
        where T : FrameworkElement
    {
        var count = VisualTreeHelper.GetChildrenCount(root);
        for (var i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is T element && string.Equals(element.Name, name, StringComparison.Ordinal))
            {
                return element;
            }

            var match = FindDescendantByName<T>(child, name);
            if (match is not null)
            {
                return match;
            }
        }

        return null;
    }
}
