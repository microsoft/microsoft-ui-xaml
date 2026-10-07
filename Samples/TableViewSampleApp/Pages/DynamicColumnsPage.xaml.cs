// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
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
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Builds a per-row accessible name for a cell editor, so a column of fifty
/// identical checkboxes does not announce the same string fifty times. The
/// binding has no Path, so the whole row item arrives here and the field label
/// comes in as the converter parameter.
/// </summary>
public sealed partial class RowFieldNameConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        var field = parameter?.ToString() ?? "Value";
        var row = value is Person person ? person.FullName : "row";
        return $"{field} — {row}";
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

/// <summary>
/// The whole lifecycle of a column, driven through the aligned
/// TableView.Columns vector (absorbs the former "Column reorder + autosize"
/// page). Show / hide uses TableViewColumn.Visibility, add / remove grows and
/// shrinks the vector, and reorder moves the same TableViewColumn instance to a
/// new index — which is why a column keeps its width wherever it lands.
///
/// The shaping selector reshapes the ROWS underneath at the same time, so a
/// reviewer can confirm that mutating the column vector does not disturb the
/// group headers, and vice versa.
/// </summary>
public sealed partial class DynamicColumnsPage : Page
{
    private const string JoinDateHeader = "Join date";

    private TableViewColumn[]? _canonical;
    private TableViewSource? _source;

    // Requested shaping mode (what the radio says) versus the mode that is
    // actually applied to the source. GroupBy fails fast, so these can differ;
    // every readout and every enable/disable guard reads _appliedMode.
    private string _mode = "flat";
    private string _appliedMode = "flat";
    private string _groupKey = "Department";
    private bool _allGroupsCollapsed;

    public DynamicColumnsPage()
    {
        People = PersonData.Take(50);
        InitializeComponent();

        _canonical = new TableViewColumn[] { ColFirstName, ColLastName, ColDepartment, ColRole, ColSalary, ColActive };

        _source = TableViewSource.From(People);
        DynamicTable.ItemsSource = _source;
        SampleShape.EnableDefaults(DynamicTable, People);

        Loaded += (_, _) =>
        {
            CapturePicker();
            ApplyShaping();
        };
    }

    public ObservableCollection<Person> People { get; }

    // ----- Shaping -----

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null ||
            sender is not ComboBox { SelectedItem: ComboBoxItem { Tag: string tag } })
        {
            return;
        }

        _mode = tag;
        ApplyShaping();
    }

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || GroupKeyCombo?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        // Re-applying with the same mode proves the grouped state survives a key change.
        ApplyShaping();
    }

    private void ApplyShaping()
    {
        if (_source is null)
        {
            return;
        }

        // Reshape IN PLACE: Filter/GroupBy mutate and return the same instance,
        // so the source is never rebuilt per change.
        if (_mode != "grouped")
        {
            _source.ClearGroupBy();
        }
        else
        {
            var key = _groupKey;
            // The two delegates receive DIFFERENT things despite both parameters
            // being named `item`:
            //   TableViewKeySelector(Object item)      -> receives the ROW ITEM
            //   TableViewIdentitySelector(Object item) -> receives the GROUP KEY
            // An item-typed identity lambda returns empty, which is a fail-fast,
            // so grouping would silently never apply.
            _source.GroupBy(
                item => (object)GroupValue(item, key),
                groupKey => groupKey?.ToString() ?? "(none)");
        }

        // case "hierarchy":
        // case "groupedhierarchy":
        //     Hierarchical rows are not available in this release: neither
        //     TableViewSource.idl nor TableView.idl exposes a hierarchy verb
        //     (the only shaping verbs are Filter / GroupBy / Sort and their
        //     Clear* counterparts). When the control ships hierarchy support,
        //     apply it to this same source here, alongside the GroupBy stage
        //     above so the two compose, then set _appliedMode as below.

        _appliedMode = _mode;
        _allGroupsCollapsed = false;

        if (_appliedMode == "grouped")
        {
            DynamicTable.ExpandAllGroups();
        }

        UpdateShapingGates();
        UpdateStatus($"Shaping applied: {ModeLabel(_appliedMode)}");
    }

    private void UpdateShapingGates()
    {
        if (GroupKeyCombo is null)
        {
            return;
        }

        var grouped = _appliedMode == "grouped";

        GroupKeyCombo.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ReassignRowButton.IsEnabled = grouped;
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        DynamicTable.ExpandAllGroups();
        _allGroupsCollapsed = false;
        UpdateStatus("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        DynamicTable.CollapseAllGroups();
        _allGroupsCollapsed = true;
        UpdateStatus("Collapsed all groups");
    }

    private void OnReassignRowClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped" || People.Count == 0)
        {
            return;
        }

        var person = People[0];
        var before = GroupValue(person, _groupKey);
        var after = NextGroupValue(person, _groupKey);
        ApplyGroupValue(person, _groupKey, after);

        UpdateStatus($"Moved \"{person.FullName}\" from {before} to {after}");
    }

    private void OnRemoveRowClick(object sender, RoutedEventArgs e)
    {
        if (People.Count == 0)
        {
            UpdateStatus("Remove skipped — no rows left");
            return;
        }

        var person = People[0];
        People.RemoveAt(0);
        UpdateStatus($"Removed \"{person.FullName}\" (its group disappears when it was the last member)");
    }

    // ----- Show / hide via TableViewColumn.Visibility -----

    private void OnColumnToggle(object sender, RoutedEventArgs e)
    {
        // CheckBox IsChecked="True" raises Checked during InitializeComponent,
        // before _canonical is assigned. The initial state already matches the
        // XAML, so skipping the init-time raise is a safe no-op.
        if (_canonical is null || DynamicTable is null) return;
        if (sender is CheckBox cb && int.TryParse(cb.Tag?.ToString(), out int index))
        {
            var column = _canonical[index];
            column.Visibility = cb.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
            UpdateStatus($"{(cb.IsChecked == true ? "Showed" : "Hid")} the {column.Header} column");
        }
    }

    // ----- Add / remove a column -----

    private void OnAddJoinDateColumnClick(object sender, RoutedEventArgs e)
    {
        if (DynamicTable.Columns.Any(c => c.Header?.ToString() == JoinDateHeader))
        {
            UpdateStatus($"{JoinDateHeader} column already present");
            return;
        }

        // A templated editor column, added at runtime through the same vector the
        // text columns live in — proving add / remove / reorder is not text-only.
        DynamicTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = JoinDateHeader,
            Width = new GridLength(220),
            CellTemplate = (DataTemplate)Resources["JoinDateCell"],
            HeaderToolTip = "Date the person joined. Editable.",
        });
        CapturePicker();
        UpdateStatus($"Added the {JoinDateHeader} column");
    }

    private void OnRemoveJoinDateColumnClick(object sender, RoutedEventArgs e)
    {
        var joinDate = DynamicTable.Columns.FirstOrDefault(c => c.Header?.ToString() == JoinDateHeader);
        if (joinDate is null)
        {
            UpdateStatus($"{JoinDateHeader} column not present");
            return;
        }

        DynamicTable.Columns.Remove(joinDate);
        CapturePicker();
        UpdateStatus($"Removed the {JoinDateHeader} column");
    }

    // ----- Reorder by moving the instance inside the Columns vector -----

    private void OnMoveLeftClick(object sender, RoutedEventArgs e) => MoveSelected(-1);

    private void OnMoveRightClick(object sender, RoutedEventArgs e) => MoveSelected(+1);

    private void MoveSelected(int direction)
    {
        var column = SelectedColumn();
        if (column is null)
        {
            UpdateStatus("Move skipped — no column selected");
            return;
        }

        int from = DynamicTable.Columns.IndexOf(column);
        if (from < 0)
        {
            UpdateStatus("Move skipped — column not found");
            return;
        }

        int to = from + direction;
        if (to < 0 || to >= DynamicTable.Columns.Count)
        {
            UpdateStatus($"Move {from} -> {to} skipped — would move past the edge");
            return;
        }

        // Moving the entry in the observable Columns vector IS the reorder, and the
        // header band and realized rows follow. The instance is reused, so its width
        // travels with it.
        DynamicTable.Columns.RemoveAt(from);
        DynamicTable.Columns.Insert(to, column);

        CapturePicker();
        UpdateStatus($"Moved \"{column.Header}\" from index {from} to {to}");
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        if (_canonical is null)
        {
            return;
        }

        string[] order = ["First name", "Last name", "Department", "Role", "Salary", "Active"];
        for (int target = 0; target < order.Length; target++)
        {
            int current = IndexOfHeader(order[target]);
            if (current >= 0 && current != target)
            {
                var moving = DynamicTable.Columns[current];
                DynamicTable.Columns.RemoveAt(current);
                DynamicTable.Columns.Insert(target, moving);
            }
        }

        foreach (var column in _canonical)
        {
            column.Visibility = Visibility.Visible;
        }

        VisFirstNameCheckBox.IsChecked = true;
        VisLastNameCheckBox.IsChecked = true;
        VisDepartmentCheckBox.IsChecked = true;
        VisRoleCheckBox.IsChecked = true;
        VisSalaryCheckBox.IsChecked = true;
        VisActiveCheckBox.IsChecked = true;

        CapturePicker();
        UpdateStatus("Reset — restored the canonical order and made every column visible");
    }

    private int IndexOfHeader(string header)
    {
        for (int i = 0; i < DynamicTable.Columns.Count; i++)
        {
            if (string.Equals(DynamicTable.Columns[i].Header?.ToString(), header, StringComparison.Ordinal))
            {
                return i;
            }
        }

        return -1;
    }

    private TableViewColumn? SelectedColumn()
    {
        if (ColumnPicker.SelectedItem is ComboBoxItem { Tag: string header })
        {
            return DynamicTable.Columns
                .FirstOrDefault(c => string.Equals(c.Header?.ToString(), header, StringComparison.Ordinal));
        }

        return null;
    }

    private void CapturePicker()
    {
        string? previous = (ColumnPicker.SelectedItem as ComboBoxItem)?.Tag as string;
        ColumnPicker.Items.Clear();
        for (int i = 0; i < DynamicTable.Columns.Count; i++)
        {
            string header = DynamicTable.Columns[i].Header?.ToString() ?? $"#{i}";
            ColumnPicker.Items.Add(new ComboBoxItem { Content = $"{i}. {header}", Tag = header });
        }

        // Reselect the previous header so the reader's pick survives a reorder.
        for (int i = 0; i < ColumnPicker.Items.Count; i++)
        {
            if (ColumnPicker.Items[i] is ComboBoxItem { Tag: string tag } && string.Equals(tag, previous, StringComparison.Ordinal))
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

    // ----- Group key helpers -----
    //
    // GroupValue never returns the empty string: an empty group identity is a
    // fail-fast in GroupBy, so a blank property has to be coalesced to a real
    // bucket label.

    private static string GroupValue(object item, string key)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Role" => person.Role,
            "Active" => person.IsActive ? "Active" : "Inactive",
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private static string NextGroupValue(Person person, string key) => key switch
    {
        "Role" => $"{person.Role} (reassigned)",
        "Active" => person.IsActive ? "Inactive" : "Active",
        _ => PersonData.Departments
                .FirstOrDefault(d => !string.Equals(d, person.Department, StringComparison.Ordinal))
             ?? person.Department,
    };

    private static void ApplyGroupValue(Person person, string key, string value)
    {
        switch (key)
        {
            case "Role":
                person.Role = value;
                break;
            case "Active":
                person.IsActive = string.Equals(value, "Active", StringComparison.Ordinal);
                break;
            default:
                person.Department = value;
                break;
        }
    }

    private static string ModeLabel(string mode) => mode switch
    {
        "grouped" => "Grouped",
        "hierarchy" => "Hierarchy",
        "groupedhierarchy" => "Grouped hierarchy",
        _ => "Flat",
    };

    private string GroupKeyLabel() => _groupKey switch
    {
        "Role" => "role",
        "Active" => "active status",
        _ => "department",
    };

    private void UpdateStatus(string? message = null)
    {
        if (StatusText is null || DynamicTable is null)
        {
            return;
        }

        var order = new StringBuilder();
        for (int i = 0; i < DynamicTable.Columns.Count; i++)
        {
            if (i > 0) order.Append(" -> ");
            order.Append(DynamicTable.Columns[i].Header?.ToString());
            if (DynamicTable.Columns[i].Visibility != Visibility.Visible)
            {
                order.Append(" (hidden)");
            }
        }

        // Readouts describe the APPLIED mode, never the requested one.
        var shaping = _appliedMode == "grouped"
            ? $"Grouped by {GroupKeyLabel()} · {GroupCount()} groups · {(_allGroupsCollapsed ? "all collapsed" : "all expanded")}"
            : "Flat (no grouping)";

        var prefix = message is null ? string.Empty : $"{message}. ";
        StatusText.Text = $"{prefix}Shaping: {shaping}. Rows={People.Count}. Columns ({DynamicTable.Columns.Count}): {order}.";
    }

    private int GroupCount()
    {
        var key = _groupKey;
        return People.Select(p => GroupValue(p, key)).Distinct(StringComparer.Ordinal).Count();
    }
}
