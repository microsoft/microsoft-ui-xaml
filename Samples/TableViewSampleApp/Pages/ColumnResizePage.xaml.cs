// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
using Microsoft.UI.Xaml.Controls.Primitives;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
// Tabular-namespace type aliases: disambiguate from the stale-mock base Microsoft.UI.Xaml.Controls TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableViewColumn's width contract:
///   * Width — the user-configured target.
///   * MinWidth / MaxWidth — clamp bounds.
///   * ActualWidth (read-only) — the rendered, clamped value.
///   * CanResize — opt out of the per-header resize gripper.
///
/// Drag the right edge of any header to resize live, or use the sliders to
/// drive Width / MinWidth / MaxWidth programmatically. Both paths feed the same
/// ActualWidth pipeline. The "Role" column opts out of the gripper.
/// </summary>
public sealed partial class ColumnResizePage : Page
{
    private bool _suppressSliderHandlers;
    private TableViewTextColumn? _activeColumn;

    public ColumnResizePage()
    {
        InitializeComponent();
        People = PersonData.Take(40);
        PeopleTable.ItemsSource = People;

        Loaded += (_, _) =>
        {
            // Populate the column-picker once columns are realized so we can
            // map the strings 1:1 to the column references in the visual tree.
            PopulateColumnCombo();
            SetActiveColumn(FirstNameColumn);
        };
    }

    public ObservableCollection<Person> People { get; }

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
            "First name"    => FirstNameColumn,
            "Last name"     => LastNameColumn,
            "Email"         => EmailColumn,
            "Department"    => DepartmentColumn,
            "Role (locked)" => RoleColumn,
            _               => null,
        };
        SetActiveColumn(next);
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

        // Push the column's current values into the sliders without echoing
        // back into the column. The handlers always re-read sender.Value so
        // there's no risk of capturing a stale slider state.
        _suppressSliderHandlers = true;
        try
        {
            // MaxWidth defaults to +inf, which we can't represent on a finite
            // slider — pin to slider max in that case.
            var maxForSlider = double.IsInfinity(column.MaxWidth)
                ? MaxWidthSlider.Maximum
                : System.Math.Min(column.MaxWidth, MaxWidthSlider.Maximum);

            MinWidthSlider.Value = System.Math.Min(column.MinWidth, MinWidthSlider.Maximum);
            WidthSlider.Value    = System.Math.Min(column.Width.Value, WidthSlider.Maximum);
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
        if (_suppressSliderHandlers || _activeColumn is null) return;
        _activeColumn.MinWidth = e.NewValue;
        UpdateLabelsAndReadout();
    }

    private void OnWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null) return;
        _activeColumn.Width = new GridLength(e.NewValue);
        UpdateLabelsAndReadout();
    }

    private void OnMaxWidthSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_suppressSliderHandlers || _activeColumn is null) return;
        _activeColumn.MaxWidth = e.NewValue;
        UpdateLabelsAndReadout();
    }

    private void UpdateLabelsAndReadout()
    {
        var inv = CultureInfo.InvariantCulture;
        MinWidthValue.Text = MinWidthSlider.Value.ToString("0", inv);
        WidthValue.Text    = WidthSlider.Value.ToString("0", inv);
        MaxWidthValue.Text = MaxWidthSlider.Value.ToString("0", inv);

        if (_activeColumn is not null)
        {
            // Format identical to the API-test-friendly "W / A" so behaviour
            // is easy to copy into automation later.
            WidthReadoutText.Text =
                $"{_activeColumn.Width.Value.ToString("0", inv)} / {_activeColumn.ActualWidth.ToString("0", inv)}";
        }
    }
}
