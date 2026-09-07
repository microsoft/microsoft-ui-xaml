// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates column ordering and sizing with the shipping Columns vector:
/// moving a TableViewColumn within the vector reorders it, and setting Width
/// to Auto asks the control to size from realized content.
///
/// The page uses index-by-current-position semantics: when you pick "Email"
/// and click Move-left, we look up the column's current index and move the
/// same column instance to currentIndex - 1.
/// </summary>
public sealed partial class ColumnReorderPage : Page
{
    public ColumnReorderPage()
    {
        InitializeComponent();

        foreach (var p in PersonData.Take(60))
        {
            People.Add(p);
        }
        PeopleTable.ItemsSource = People;

        Loaded += (_, _) =>
        {
            CapturePicker();
            UpdateReadout();
        };
    }

    public ObservableCollection<Person> People { get; } = new();

    // ----- Button handlers -----

    private void OnMoveLeftClick(object sender, RoutedEventArgs e)
        => MoveSelected(direction: -1);

    private void OnMoveRightClick(object sender, RoutedEventArgs e)
        => MoveSelected(direction: +1);

    private void OnAutoSizeClick(object sender, RoutedEventArgs e)
    {
        var col = SelectedColumn();
        if (col == null)
        {
            LastActionText.Text = "Autosize skipped — no column selected.";
            return;
        }

        col.Width = new GridLength(1, GridUnitType.Auto);
        LastActionText.Text =
            $"Set \"{col.Header}\" width to Auto -> ActualWidth={col.ActualWidth:0}";
        UpdateReadout();
    }

    private void OnAutoSizeAllClick(object sender, RoutedEventArgs e)
    {
        foreach (var c in PeopleTable.Columns) { c.Width = new GridLength(1, GridUnitType.Auto); }
        LastActionText.Text = "Set all column widths to Auto.";
        UpdateReadout();
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        string[] order = ["First name", "Last name", "Email", "Department", "Role"];
        for (int target = 0; target < order.Length; target++)
        {
            var current = PeopleTable.Columns.Select((c, i) => new { c, i }).FirstOrDefault(x => x.c.Header?.ToString() == order[target])?.i ?? -1;
            if (current >= 0 && current != target)
            {
                // Moving the entry in the observable Columns vector IS the reorder, and the
                // header band and realized rows follow.
                var moving = PeopleTable.Columns[current];
                PeopleTable.Columns.RemoveAt(current);
                PeopleTable.Columns.Insert(target, moving);
            }
        }
        LastActionText.Text = "Reset -> restored original column order.";
        CapturePicker();
        UpdateReadout();
    }
    // ----- Helpers -----

    private void MoveSelected(int direction)
    {
        var col = SelectedColumn();
        if (col == null)
        {
            LastActionText.Text = "Move skipped — no column selected.";
            return;
        }

        int from = PeopleTable.Columns.IndexOf(col);
        int to = from + direction;
        if (from < 0)
        {
            LastActionText.Text = "Move skipped — column not found.";
            return;
        }
        if (to < 0 || to >= PeopleTable.Columns.Count)
        {
            LastActionText.Text =
                $"Move {from} -> {to} skipped — would move past edge.";
            return;
        }

        // Moving the entry in the observable Columns vector IS the reorder.
        var moving = PeopleTable.Columns[from];
        PeopleTable.Columns.RemoveAt(from);
        PeopleTable.Columns.Insert(to, moving);
        bool moved = true;
        LastActionText.Text =
            $"Moved column {from} -> {to} (\"{col.Header}\")";
        if (moved)
        {
            CapturePicker();
        }
        UpdateReadout();
    }

    private TableViewColumn? SelectedColumn()
    {
        if (ColumnPicker.SelectedItem is ComboBoxItem item
            && item.Tag is string header)
        {
            return PeopleTable.Columns
                .OfType<TableViewTextColumn>()
                .FirstOrDefault(c => string.Equals(c.Header?.ToString(), header));
        }
        return null;
    }

    private void CapturePicker()
    {
        string? previous = (ColumnPicker.SelectedItem as ComboBoxItem)?.Tag as string;
        ColumnPicker.Items.Clear();
        for (int i = 0; i < PeopleTable.Columns.Count; i++)
        {
            string header = PeopleTable.Columns[i].Header?.ToString() ?? $"#{i}";
            var item = new ComboBoxItem
            {
                Content = $"{i}. {header}",
                Tag = header,
            };
            ColumnPicker.Items.Add(item);
        }

        // Reselect previous header so user's pick survives a reorder.
        for (int i = 0; i < ColumnPicker.Items.Count; i++)
        {
            if (ColumnPicker.Items[i] is ComboBoxItem item
                && item.Tag is string tag
                && string.Equals(tag, previous))
            {
                ColumnPicker.SelectedIndex = i;
                return;
            }
        }
        if (ColumnPicker.Items.Count > 0)
        {
            ColumnPicker.SelectedIndex = 0;
        }
    }

    private void UpdateReadout()
    {
        var sb = new StringBuilder();
        for (int i = 0; i < PeopleTable.Columns.Count; i++)
        {
            if (i > 0) sb.Append(" -> ");
            sb.Append(PeopleTable.Columns[i].Header?.ToString());
        }
        ColumnOrderText.Text = sb.ToString();
    }
}
