// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates the control's built-in in-place cell editing. When
/// <c>TableView.IsReadOnly</c> is <c>false</c>, a <c>TableViewTextColumn</c>
/// cell enters edit on double-tap (or F2 on the focused row); Enter / Tab
/// commit the edited value back to the bound model and Esc cancels.
///
/// Unlike <see cref="DensityReadOnlyPage"/> — which demonstrates the density
/// presets — this page is the single home for the read-only story: a table-level
/// gate that locks or unlocks every text column at once. Because editing is a
/// model-first, two-way write, every committed edit lands directly on the
/// <see cref="Person"/> model (INotifyPropertyChanged) and is surfaced live in
/// the edit log. Grouping is enabled here as well so that committing an edit
/// inside a grouped projection — a distinct code path — is exercised too.
/// </summary>
public sealed partial class CellEditingPage : Page, INotifyPropertyChanged
{
    // Property names that map to an editable text column on this page.
    private static readonly string[] s_editableProperties =
    {
        nameof(Person.FirstName),
        nameof(Person.LastName),
        nameof(Person.Role),
        nameof(Person.Department),
        nameof(Person.Email),
    };

    private string _statusText = string.Empty;
    private TableViewSource? _source;
    private string _mode = "flat";
    private string _groupKey = "Department";

    // Set only once GroupBy / ClearGroupBy has actually returned, so the readout
    // cannot claim a mode the source never took.
    private string _appliedMode = "Flat";
    private string _lastAction = "no action yet";

    public CellEditingPage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(20))
        {
            person.PropertyChanged += OnPersonPropertyChanged;
            People.Add(person);
        }

        _source = TableViewSource.From(People);
        DemoTable.ItemsSource = _source;

        // Start editable so double-tap / F2 works without first flipping the toggle.
        DemoTable.IsReadOnly = false;

        UpdateStatus();
    }

    public ObservableCollection<Person> People { get; } = new();

    /// <summary>Rolling log of committed edits, newest first (bound in XAML).</summary>
    public ObservableCollection<string> EditLog { get; } = new();

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

    private void OnReadOnlyToggled(object sender, RoutedEventArgs e)
    {
        if (DemoTable is null || ReadOnlyToggle is null)
        {
            return;
        }

        DemoTable.IsReadOnly = ReadOnlyToggle.IsOn;
        UpdateStatus();
    }

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _mode = tag;
        ApplyGrouping();
    }

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || GroupKeySelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyGrouping();
    }

    private void OnExpandAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode == "Flat")
        {
            return;
        }

        DemoTable.ExpandAllGroups();
    }

    private void OnCollapseAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode == "Flat")
        {
            return;
        }

        DemoTable.CollapseAllGroups();
    }

    /// <summary>
    /// Reshapes the existing <see cref="TableViewSource"/> in place. GroupBy /
    /// ClearGroupBy mutate and return the same instance, so the source is never
    /// rebuilt — rebuilding would drop selection, scroll offset and expansion.
    /// </summary>
    private void ApplyGrouping()
    {
        if (_source is null)
        {
            return;
        }

        var grouped = _mode == "grouped";
        switch (_mode)
        {
            case "grouped":
            {
                var key = _groupKey;
                // The key selector receives the item; the identity selector receives the
                // group KEY produced above, so it only has to stringify it.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");

                _appliedMode = $"Grouped by {key}";
                break;
            }

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release. TableViewSource.idl
            //     exposes only Filter / GroupBy / Sort and their Clear* counterparts, so
            //     there is no hierarchy verb to call here yet and nothing is written rather
            //     than naming an API that does not exist. When the control ships hierarchy
            //     support, apply it to THIS same source instance alongside the GroupBy stage
            //     above so the two axes compose rather than replace one another, then remove
            //     IsEnabled="False" from the two hierarchy items in the Shaping mode selector.
            //     break;

            default:
                _source.ClearGroupBy();
                _appliedMode = "Flat";
                break;
        }

        if (ExpandAllButton is not null)
        {
            GroupKeySelector.IsEnabled = grouped;
            ExpandAllButton.IsEnabled = grouped;
            CollapseAllButton.IsEnabled = grouped;
        }

        if (grouped)
        {
            DispatcherQueue.TryEnqueue(() => DemoTable?.ExpandAllGroups());
        }

        UpdateStatus();
    }

    private static string GroupValue(object item, string key)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Role" => person.Role,
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private void OnPersonPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || e.PropertyName is null)
        {
            return;
        }

        // Only surface edits to the columns this page actually exposes; skip
        // derived-property notifications (FullName / Initial / JoinDateText).
        if (Array.IndexOf(s_editableProperties, e.PropertyName) < 0)
        {
            return;
        }

        var newValue = e.PropertyName switch
        {
            nameof(Person.FirstName) => person.FirstName,
            nameof(Person.LastName) => person.LastName,
            nameof(Person.Role) => person.Role,
            nameof(Person.Department) => person.Department,
            nameof(Person.Email) => person.Email,
            _ => string.Empty,
        };

        // Newest first; cap the log so it stays readable in the fixed-height rail.
        EditLog.Insert(0, $"{person.FullName} · {e.PropertyName} = {newValue}");
        while (EditLog.Count > 50)
        {
            EditLog.RemoveAt(EditLog.Count - 1);
        }

        UpdateStatus();
    }

    private void UpdateStatus()
    {
        var mode = ReadOnlyToggle is not null && ReadOnlyToggle.IsOn ? "read-only" : "editable";
        var grouping = _appliedMode == "Flat" ? "not grouped" : _appliedMode.ToLowerInvariant();
        StatusText = $"Table: {mode} · {grouping} · {People.Count:N0} rows · {EditLog.Count:N0} edits committed · {_lastAction}";
    }

    // ---- actions -------------------------------------------------------------------

    private void OnTableSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (EditGroupedValueButton is not null)
        {
            EditGroupedValueButton.IsEnabled = DemoTable.SelectedItem is Person;
        }
    }

    /// <summary>
    /// Edits the property the table is currently grouped on. The edit lands on
    /// the model, the edit log records it, and the row re-groups — the one
    /// editing/grouping interaction the rest of the gallery never exercises.
    /// </summary>
    private void OnEditGroupedValueClick(object sender, RoutedEventArgs e)
    {
        if (DemoTable.SelectedItem is not Person person)
        {
            return;
        }

        var byRole = _appliedMode.EndsWith("Role", StringComparison.Ordinal);
        var pool = byRole
            ? People.Select(p => p.Role).Distinct(StringComparer.Ordinal).OrderBy(r => r, StringComparer.Ordinal).ToList()
            : PersonData.Departments.ToList();

        if (pool.Count == 0)
        {
            return;
        }

        var current = byRole ? person.Role : person.Department;
        var next = pool[(pool.IndexOf(current) + 1) % pool.Count];

        if (byRole)
        {
            person.Role = next;
        }
        else
        {
            person.Department = next;
        }

        _lastAction = $"edited the grouped-on value to {next}";
        UpdateStatus();
    }

    private void OnCommitEditClick(object sender, RoutedEventArgs e)
    {
        _lastAction = $"CommitEdit returned {DemoTable.CommitEdit()}";
        UpdateStatus();
    }

    private void OnCancelEditClick(object sender, RoutedEventArgs e)
    {
        _lastAction = $"CancelEdit returned {DemoTable.CancelEdit()}";
        UpdateStatus();
    }

    private void OnClearLogClick(object sender, RoutedEventArgs e)
    {
        EditLog.Clear();
        _lastAction = "cleared the edit log";
        UpdateStatus();
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
