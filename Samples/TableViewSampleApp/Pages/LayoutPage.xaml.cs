// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Consolidates every column-width lesson into one page (absorbs the former
/// "Column resize" page):
///   * Width — the user-configured target, as Pixel, Auto, Star, or a Mixed layout.
///   * MinWidth / MaxWidth — clamp bounds driven live from sliders.
///   * ActualWidth (read-only) — the rendered, clamped value.
///   * CanResize — one column opts out of the per-header resize gripper.
///   * CanUserResizeColumns — the table-level gate for every gripper at once.
///
/// Dragging a header edge and moving the sliders feed the same ActualWidth
/// pipeline, so both paths are visible in one readout.
/// </summary>
public sealed partial class LayoutPage : Page
{
    private const string PlaygroundMode = "Playground";
    private const string LockedRoleHeader = "Role (locked)";

    private bool _suppressSliderHandlers;
    private TableViewTextColumn? _activeColumn;

    public LayoutPage()
    {
        People = PersonData.Take(50);
        InitializeComponent();
        WidthsTable.ItemsSource = People;

        Loaded += (_, _) =>
        {
            ApplySelectedMode();
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

    private static TableViewTextColumn LockedRole(GridLength width)
    {
        var column = Text(LockedRoleHeader, nameof(Person.Role), width);
        // Per-column opt-out of the resize gripper. Programmatic Width still applies.
        column.CanResize = false;
        return column;
    }

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

        var previousHeader = (_activeColumn?.Header?.ToString()) ?? (ColumnCombo.SelectedItem as string);

        WidthsTable.Columns.Clear();
        PlaygroundPanel.Visibility = mode == PlaygroundMode ? Visibility.Visible : Visibility.Collapsed;

        switch (mode)
        {
            case PlaygroundMode:
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Px(160)));
                WidthsTable.Columns.Add(Text("Last name", nameof(Person.LastName), Px(160)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Px(220)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Px(140)));
                WidthsTable.Columns.Add(LockedRole(Px(200)));
                ModeDescription.Text = "Playground — pick a column and tweak MinWidth, Width, and MaxWidth live while watching ActualWidth clamp.";
                break;

            case "Auto":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Auto()));
                WidthsTable.Columns.Add(Text("Last name", nameof(Person.LastName), Auto()));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Auto()));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Auto()));
                WidthsTable.Columns.Add(LockedRole(Auto()));
                ModeDescription.Text = "All Auto columns — every column uses GridUnitType.Auto and sizes to the widest realized header or cell content.";
                break;

            case "Star":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Star(1)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Star(1)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Star(3)));
                WidthsTable.Columns.Add(LockedRole(Star(2)));
                ModeDescription.Text = "All Star columns — every column shares remaining width by factor, so Email (*3) gets three times First name (*1).";
                break;

            case "Pixel":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Px(180)));
                WidthsTable.Columns.Add(Text("Last name", nameof(Person.LastName), Px(180)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Px(320)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Px(220)));
                WidthsTable.Columns.Add(LockedRole(Px(260)));
                WidthsTable.Columns.Add(Text("Join date", nameof(Person.JoinDateText), Px(180)));
                WidthsTable.Columns.Add(Text("Bio", nameof(Person.Bio), Px(520)));
                ModeDescription.Text = "All Pixel columns — every column has a fixed GridUnitType.Pixel width, independent of content. Their combined width exceeds a narrow window, so the body scrolls horizontally and the sticky header stays in sync.";
                break;

            case "Mixed":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Auto()));
                WidthsTable.Columns.Add(LockedRole(Px(160)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Star(1)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Star(2)));
                WidthsTable.Columns.Add(Text("Bio", nameof(Person.Bio), Star(3)));
                ModeDescription.Text = "Mixed columns — Auto and fixed Pixel columns are sized first, then Star columns split the remaining width.";
                break;
        }

        PopulateColumnCombo(previousHeader);
        ApplyCanUserResizeColumns();
        UpdateLabelsAndReadout();
    }

    private void PopulateColumnCombo(string? preferredHeader)
    {
        ColumnCombo.Items.Clear();
        foreach (var column in WidthsTable.Columns)
        {
            ColumnCombo.Items.Add(column.Header?.ToString() ?? "(unnamed)");
        }

        // Keep the reader's pick across a mode rebuild when the same header still exists.
        var index = preferredHeader is null ? -1 : ColumnCombo.Items.IndexOf(preferredHeader);
        ColumnCombo.SelectedIndex = index >= 0 ? index : (ColumnCombo.Items.Count > 0 ? 0 : -1);
    }

    private void OnColumnComboChanged(object sender, SelectionChangedEventArgs e)
    {
        if (WidthsTable is null || ColumnCombo.SelectedItem is not string label)
        {
            return;
        }

        SetActiveColumn(FindColumn(label));
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
            UpdateResizeStateText();
            return;
        }

        // Push the column's current values into the sliders without echoing back
        // into the column. MaxWidth defaults to +inf, which a finite slider cannot
        // represent — pin to slider max in that case.
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
        SetLastAction($"MinWidth of \"{_activeColumn.Header}\" set to {e.NewValue:0}.");
        UpdateLabelsAndReadout();
    }

    private void OnWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null)
        {
            return;
        }

        _activeColumn.Width = new GridLength(e.NewValue);
        SetLastAction($"Width of \"{_activeColumn.Header}\" set to {e.NewValue:0}.");
        UpdateLabelsAndReadout();
    }

    private void OnMaxWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null)
        {
            return;
        }

        _activeColumn.MaxWidth = e.NewValue;
        SetLastAction($"MaxWidth of \"{_activeColumn.Header}\" set to {e.NewValue:0}.");
        UpdateLabelsAndReadout();
    }

    private void OnCanUserResizeColumnsChanged(object sender, RoutedEventArgs e)
    {
        // CheckBox IsChecked="True" raises Checked during InitializeComponent, before the
        // rest of the Options rail exists. WidthsTable lives in the Example slot and is
        // already created, but LastActionText is declared below this checkbox and is still
        // null — writing to it here threw, and an exception out of a XAML-raised handler
        // surfaces as XamlParseException 0x802B000A. The constructor re-applies the state.
        if (WidthsTable is null || LastActionText is null)
        {
            return;
        }

        ApplyCanUserResizeColumns();
        SetLastAction($"CanUserResizeColumns set to {WidthsTable.CanUserResizeColumns}.");
        UpdateLabelsAndReadout();
    }

    private void ApplyCanUserResizeColumns()
    {
        if (WidthsTable is null || CanUserResizeColumnsCheckBox is null)
        {
            return;
        }

        WidthsTable.CanUserResizeColumns = CanUserResizeColumnsCheckBox.IsChecked == true;
    }

    // ----- Actions -----
    //
    // The single autosize implementation for the gallery: setting Width to
    // GridUnitType.Auto asks the control to size from realized content.

    private static void Autosize(TableViewColumn column) => column.Width = new GridLength(1, GridUnitType.Auto);

    private void OnAutosizeSelectedClick(object sender, RoutedEventArgs e)
    {
        if (_activeColumn is null)
        {
            SetLastAction("Autosize skipped — no column selected.");
            return;
        }

        Autosize(_activeColumn);
        SetLastAction($"Set \"{_activeColumn.Header}\" width to Auto -> ActualWidth={_activeColumn.ActualWidth:0}.");
        UpdateLabelsAndReadout();
    }

    private void OnAutosizeAllClick(object sender, RoutedEventArgs e)
    {
        foreach (var column in WidthsTable.Columns)
        {
            Autosize(column);
        }

        SetLastAction($"Set all {WidthsTable.Columns.Count} column widths to Auto.");
        UpdateLabelsAndReadout();
    }

    private void OnClampSelectedClick(object sender, RoutedEventArgs e)
    {
        if (_activeColumn is null)
        {
            SetLastAction("Clamp skipped — no column selected.");
            return;
        }

        // Deliberately below a typical MinWidth so ActualWidth visibly refuses to follow Width.
        _activeColumn.Width = new GridLength(80);
        _suppressSliderHandlers = true;
        try
        {
            WidthSlider.Value = 80;
        }
        finally
        {
            _suppressSliderHandlers = false;
        }

        SetLastAction(
            $"Width of \"{_activeColumn.Header}\" forced to 80; MinWidth={_activeColumn.MinWidth:0} clamps ActualWidth to {_activeColumn.ActualWidth:0}.");
        UpdateLabelsAndReadout();
    }

    private void OnResetWidthsClick(object sender, RoutedEventArgs e)
    {
        ApplySelectedMode();
        SetLastAction($"Rebuilt the columns for {ModeSelector.SelectedItem} with their declared widths.");
    }

    // Null-safe because several handlers can be raised from XAML during
    // InitializeComponent, before this TextBlock exists.
    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }
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
            // Format identical to the API-test-friendly "W / A" so behaviour is
            // easy to copy into automation later.
            WidthReadoutText.Text =
                $"{_activeColumn.Width.Value.ToString("0", inv)} ({_activeColumn.Width.GridUnitType}) / {_activeColumn.ActualWidth.ToString("0", inv)}";
        }

        UpdateResizeStateText();
    }

    private void UpdateResizeStateText()
    {
        if (ResizeStateText is null || WidthsTable is null)
        {
            return;
        }

        var tableGate = WidthsTable.CanUserResizeColumns
            ? "CanUserResizeColumns=True"
            : "CanUserResizeColumns=False (all grippers off)";
        var columnGate = _activeColumn is null
            ? "no column selected"
            : $"CanResize={_activeColumn.CanResize}";

        ResizeStateText.Text = $"{tableGate} · {columnGate}";
    }
}
