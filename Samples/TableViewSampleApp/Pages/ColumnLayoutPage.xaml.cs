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
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Every column-width lesson on one page:
///   * Width: the intent, as Pixel, Auto, Star, or a Mixed layout.
///   * MinWidth / MaxWidth: clamp bounds driven live from sliders.
///   * ActualWidth (read-only): the rendered, clamped value.
///   * CanResize: one column opts out of the per-header resize gripper.
///   * CanUserResizeColumns: the table-level gate for every gripper at once.
///
/// Dragging a header edge and moving the sliders both change ActualWidth; the readout follows
/// TableViewColumn.ActualWidthProperty, so both paths show up in it after layout.
/// </summary>
public sealed partial class ColumnLayoutPage : SamplePageBase
{
    private const string PlaygroundMode = "Playground";
    private const string LockedRoleHeader = "Role (locked)";

    private bool _suppressSliderHandlers;
    private TableViewColumn? _activeColumn;
    private long _actualWidthToken = -1;

    public ColumnLayoutPage()
    {
        InitializeComponent();
        InitializeSample(Status);
        TrackLifetime(ApplySelectedMode, () => SetActiveColumn(null));
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    // <snippet>
    // ---- Column factories ---------------------------------------------------------------

    private static TableViewTextColumn Text(string header, string path, GridLength width) =>
        new()
        {
            Header = header,
            Binding = new Binding { Path = new PropertyPath(path) },
            Width = width,
            CanSort = false,
        };

    // Template columns use the shared cells from Templates\PersonCellTemplates.xaml.
    private static TableViewTemplateColumn Templated(string header, string templateKey, GridLength width) =>
        new()
        {
            Header = header,
            CellTemplate = (DataTemplate)Application.Current.Resources[templateKey],
            Width = width,
            CanSort = false,
        };

    private static TableViewTextColumn LockedRole(GridLength width)
    {
        var column = Text(LockedRoleHeader, nameof(Person.Role), width);
        // Per-column opt-out of the resize gripper. Setting Width from code still applies.
        column.CanResize = false;
        return column;
    }

    private static GridLength Auto() => new(1, GridUnitType.Auto);
    private static GridLength Star(double factor = 1) => new(factor, GridUnitType.Star);
    private static GridLength Px(double px) => new(px, GridUnitType.Pixel);

    // ---- Width intent -------------------------------------------------------------------

    private void OnWidthModeChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="0"); Loaded builds the first layout.
        if (!IsLoaded)
        {
            return;
        }

        ApplySelectedMode();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Width intent -> {0}", SampleShaping.Label(WidthModeSelector)));
    }

    private void ApplySelectedMode()
    {
        var mode = SampleShaping.SelectedTag(WidthModeSelector, PlaygroundMode);
        var previousHeader = _activeColumn?.Header?.ToString();

        SetActiveColumn(null);
        WidthsTable.Columns.Clear();

        switch (mode)
        {
            case "Auto":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Auto()));
                WidthsTable.Columns.Add(LockedRole(Auto()));
                WidthsTable.Columns.Add(Templated("Department", "DepartmentChipTemplate", Auto()));
                WidthsTable.Columns.Add(Text("Last name", nameof(Person.LastName), Auto()));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Auto()));
                ModeDescription.Text = "Auto: every column sizes to its header and the widest cell on screen, including the Department chip template. Only realized rows are measured, so scrolling can widen a column, and when the columns need less than the table an empty band is left on the right.";
                break;

            case "Star":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Star(1)));
                WidthsTable.Columns.Add(LockedRole(Star(2)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Star(1)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Star(3)));
                ModeDescription.Text = "Star: the columns share the table width by factor, so Email (3*) gets three times First name (1*). There is no empty band and no horizontal scrolling.";
                break;

            case "Pixel":
                WidthsTable.Columns.Add(Templated("Photo", "AvatarTemplate", Px(56)));
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Px(180)));
                WidthsTable.Columns.Add(LockedRole(Px(220)));
                WidthsTable.Columns.Add(Text("Last name", nameof(Person.LastName), Px(180)));
                WidthsTable.Columns.Add(Templated("Join date", "JoinDatePickerTemplate", Px(200)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Px(320)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Px(220)));
                WidthsTable.Columns.Add(Text("Bio", nameof(Person.Bio), Px(520)));
                ModeDescription.Text = "Pixel: every column has a fixed width, whatever its content. The total is wider than the table, so the body scrolls horizontally and the header stays in step with it.";
                break;

            case "Mixed":
                WidthsTable.Columns.Add(Text("First name", nameof(Person.FirstName), Auto()));
                WidthsTable.Columns.Add(LockedRole(Px(160)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Star(1)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Star(2)));
                WidthsTable.Columns.Add(Text("Bio", nameof(Person.Bio), Star(3)));
                ModeDescription.Text = "Mixed: the Auto and Pixel columns are sized first, then the Star columns split what is left by factor.";
                break;

            default:
                WidthsTable.Columns.Add(Templated("Photo", "AvatarTemplate", Px(56)));
                WidthsTable.Columns.Add(Text("Name", nameof(Person.FullName), Px(160)));
                WidthsTable.Columns.Add(LockedRole(Px(160)));
                WidthsTable.Columns.Add(Templated("Join date", "JoinDatePickerTemplate", Px(200)));
                WidthsTable.Columns.Add(Text("Department", nameof(Person.Department), Px(140)));
                WidthsTable.Columns.Add(Text("Email", nameof(Person.Email), Px(220)));
                ModeDescription.Text = "Playground: fixed Pixel widths you change with the sliders below. The Join date column holds a full date picker in 200 px, so it clips until Width passes about 310.";
                break;
        }

        PopulateColumnSelector(previousHeader);
        ApplyCanUserResizeColumns();
        RefreshReadouts();
    }
    // </snippet>

    private void PopulateColumnSelector(string? preferredHeader)
    {
        ColumnSelector.Items.Clear();
        foreach (var column in WidthsTable.Columns)
        {
            ColumnSelector.Items.Add(column.Header?.ToString() ?? "(unnamed)");
        }

        // Keep the reader's pick across a mode rebuild when the same header still exists.
        var index = preferredHeader is null ? -1 : ColumnSelector.Items.IndexOf(preferredHeader);
        ColumnSelector.SelectedIndex = index >= 0 ? index : (ColumnSelector.Items.Count > 1 ? 1 : 0);
    }

    // ---- Column sizing ------------------------------------------------------------------

    private void OnColumnSelectorChanged(object sender, SelectionChangedEventArgs e)
    {
        if (WidthsTable is null || ColumnSelector.SelectedItem is not string header)
        {
            return;
        }

        SetActiveColumn(FindColumn(header));
    }

    private TableViewColumn? FindColumn(string header)
    {
        foreach (var column in WidthsTable.Columns)
        {
            if (string.Equals(column.Header?.ToString(), header, StringComparison.Ordinal))
            {
                return column;
            }
        }

        return null;
    }

    // <snippet>
    private void SetActiveColumn(TableViewColumn? column)
    {
        if (_activeColumn is not null && _actualWidthToken >= 0)
        {
            _activeColumn.UnregisterPropertyChangedCallback(TableViewColumn.ActualWidthProperty, _actualWidthToken);
            _actualWidthToken = -1;
        }

        _activeColumn = column;
        if (column is null)
        {
            RefreshReadouts();
            return;
        }

        // ActualWidth is resolved during layout, after this call returns, and changes again while
        // the user drags the header edge. Follow it instead of reading it once.
        _actualWidthToken = column.RegisterPropertyChangedCallback(TableViewColumn.ActualWidthProperty, OnActualWidthChanged);
        SeedSliders();
        RefreshReadouts();
    }

    private void OnActualWidthChanged(DependencyObject sender, DependencyProperty dp)
    {
        // Covers drag-resize too: the gripper writes Width, then layout resolves ActualWidth.
        SeedSliders();
        RefreshReadouts();
    }

    // Push the column's current values into the sliders without echoing back into the column.
    private void SeedSliders()
    {
        if (_activeColumn is not { } column)
        {
            return;
        }

        _suppressSliderHandlers = true;
        try
        {
            // MaxWidth defaults to +inf, which a finite slider cannot show: pin to the slider max.
            MinWidthSlider.Value = Math.Min(column.MinWidth, MinWidthSlider.Maximum);
            MaxWidthSlider.Value = double.IsInfinity(column.MaxWidth) ? MaxWidthSlider.Maximum : Math.Min(column.MaxWidth, MaxWidthSlider.Maximum);

            // For Auto and Star, Width.Value is a factor (1, 2, 3), not pixels; show ActualWidth
            // and disable the slider so moving it cannot silently turn the column into Pixel.
            WidthSlider.IsEnabled = column.Width.IsAbsolute;
            WidthSlider.Value = Math.Min(column.Width.IsAbsolute ? column.Width.Value : column.ActualWidth, WidthSlider.Maximum);
        }
        finally
        {
            _suppressSliderHandlers = false;
        }
    }
    // </snippet>

    private void OnMinWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null)
        {
            return;
        }

        _activeColumn.MinWidth = e.NewValue;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "MinWidth of {0} -> {1:N0}", _activeColumn.Header, e.NewValue));
    }

    private void OnWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null)
        {
            return;
        }

        _activeColumn.Width = Px(e.NewValue);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Width of {0} -> {1:N0} px", _activeColumn.Header, e.NewValue));
    }

    private void OnMaxWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null)
        {
            return;
        }

        _activeColumn.MaxWidth = e.NewValue;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "MaxWidth of {0} -> {1:N0}", _activeColumn.Header, e.NewValue));
    }

    // ---- Resize gates -------------------------------------------------------------------

    private void OnCanUserResizeColumnsToggled(object sender, RoutedEventArgs e)
    {
        // Fires during InitializeComponent (IsOn="True"); Loaded applies the state.
        if (!IsLoaded)
        {
            return;
        }

        ApplyCanUserResizeColumns();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "CanUserResizeColumns -> {0}", WidthsTable.CanUserResizeColumns));
    }

    private void ApplyCanUserResizeColumns() =>
        WidthsTable.CanUserResizeColumns = CanUserResizeColumnsToggle.IsOn;

    // ---- Actions ------------------------------------------------------------------------
    //
    // Autosize is GridUnitType.Auto: the control sizes the column from its realized content.

    private static void Autosize(TableViewColumn column) => column.Width = Auto();

    private void OnAutosizeSelectedClick(object sender, RoutedEventArgs e)
    {
        if (_activeColumn is not { } column)
        {
            SetLastAction("No column selected.");
            return;
        }

        Autosize(column);
        SeedSliders();
        SetLastActionAfterLayout(() => string.Format(CultureInfo.CurrentCulture,
            "Width of {0} -> Auto; ActualWidth {1:N0}", column.Header, column.ActualWidth));
    }

    private void OnAutosizeAllClick(object sender, RoutedEventArgs e)
    {
        foreach (var column in WidthsTable.Columns)
        {
            Autosize(column);
        }

        SeedSliders();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Width of all {0:N0} columns -> Auto", WidthsTable.Columns.Count));
    }

    // <snippet>
    private void OnClampSelectedClick(object sender, RoutedEventArgs e)
    {
        if (_activeColumn is not { } column)
        {
            SetLastAction("No column selected.");
            return;
        }

        // Deliberately small, so a raised MinWidth visibly stops ActualWidth from following.
        column.Width = Px(80);
        SeedSliders();
        SetLastActionAfterLayout(() => string.Format(CultureInfo.CurrentCulture,
            "Width of {0} -> 80 px; MinWidth {1:N0} gives ActualWidth {2:N0}", column.Header, column.MinWidth, column.ActualWidth));
    }
    // </snippet>

    private void OnResetWidthsClick(object sender, RoutedEventArgs e)
    {
        ApplySelectedMode();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Rebuilt the {0} columns with their declared widths", SampleShaping.Label(WidthModeSelector)));
    }

    // ActualWidth is only known after the next layout pass: write the message now, then once more
    // when layout has run, so the reported ActualWidth is never the stale pre-layout value.
    private void SetLastActionAfterLayout(Func<string> message)
    {
        SetLastAction(message());

        EventHandler<object>? handler = null;
        handler = (_, _) =>
        {
            WidthsTable.LayoutUpdated -= handler;
            if (IsLoaded)
            {
                SetLastAction(message());
            }
        };
        WidthsTable.LayoutUpdated += handler;
    }
}
