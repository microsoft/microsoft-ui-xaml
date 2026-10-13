// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Cell editing: built-in in-place editing of text columns, a TableViewTemplateColumn editor
/// supplied through CellEditingTemplate, the table-wide TableView.IsReadOnly gate, CommitEdit /
/// CancelEdit, and edits to the grouped-on value re-grouping the row.
/// </summary>
public sealed partial class CellEditingPage : Page
{
    // Properties that an editable column on this page writes.
    private static readonly string[] s_editableProperties =
    {
        nameof(Person.FirstName),
        nameof(Person.LastName),
        nameof(Person.Role),
        nameof(Person.Department),
    };

    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private int _editsCommitted;
    private string _openEdit = "(none)";

    public CellEditingPage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource? Source => _source;

    /// <summary>Values written to the model, newest first (bound in XAML).</summary>
    public ObservableCollection<string> EditLog { get; } = new();

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }
    }

    // ---- Editing ------------------------------------------------------------------------

    private void OnReadOnlyToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTable is null || ReadOnlyToggle is null)
        {
            return;
        }

        PeopleTable.IsReadOnly = ReadOnlyToggle.IsOn;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "IsReadOnly -> {0}", PeopleTable.IsReadOnly));
    }

    private void OnBeginningEdit(TableView sender, TableViewBeginningEditEventArgs args)
    {
        _openEdit = string.Format(CultureInfo.CurrentCulture, "{0} for {1}", args.Column?.Header, (args.Item as Person)?.FullName);
        SetEditButtonsEnabled(true);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Began editing {0}", _openEdit));
    }

    // Raised for every close, with EditAction saying whether it committed or cancelled.
    private void OnCellEditEnding(TableView sender, TableViewCellEditEndingEventArgs args)
    {
        var what = string.Format(CultureInfo.CurrentCulture, "{0} for {1}", args.Column?.Header, (args.Item as Person)?.FullName);
        _openEdit = "(none)";
        SetEditButtonsEnabled(false);
        SetLastAction(args.EditAction == TableViewEditAction.Commit
            ? string.Format(CultureInfo.CurrentCulture, "Committed {0}", what)
            : string.Format(CultureInfo.CurrentCulture, "Cancelled the edit of {0}; the model keeps its value", what));
    }

    private void SetEditButtonsEnabled(bool enabled)
    {
        CommitEditButton.IsEnabled = enabled;
        CancelEditButton.IsEnabled = enabled;
    }

    // The buttons set AllowFocusOnInteraction="False", so clicking one leaves focus, and the
    // open editor, where they are.
    private void OnCommitEditClick(object sender, RoutedEventArgs e)
    {
        if (!PeopleTable.CommitEdit())
        {
            SetLastAction("CommitEdit returned false: no edit was open, or the value was rejected.");
        }
    }

    private void OnCancelEditClick(object sender, RoutedEventArgs e)
    {
        if (!PeopleTable.CancelEdit())
        {
            SetLastAction("CancelEdit returned false: no edit was open.");
        }
    }

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || Array.IndexOf(s_editableProperties, e.PropertyName) < 0)
        {
            return;
        }

        var value = e.PropertyName switch
        {
            nameof(Person.FirstName) => person.FirstName,
            nameof(Person.LastName) => person.LastName,
            nameof(Person.Role) => person.Role,
            _ => person.Department,
        };

        _editsCommitted++;
        EditLog.Insert(0, string.Format(CultureInfo.CurrentCulture, "{0} · {1} = {2}", person.FullName, e.PropertyName, value));
        while (EditLog.Count > 50)
        {
            EditLog.RemoveAt(EditLog.Count - 1);
        }

        // The write arrives from inside the control's commit. Re-group after it returns, so the
        // reshape never runs while the edit is still closing.
        var property = e.PropertyName;
        DispatcherQueue.TryEnqueue(() =>
        {
            if (IsLoaded)
            {
                ReapplyIfGroupedOn(property);
                RefreshReadouts();
            }
        });
        RefreshReadouts();
    }

    // ---- Actions ------------------------------------------------------------------------

    private void OnEditGroupedValueClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = SampleShaping.KeyOf(person, _appliedKey);
        if (_appliedKey == nameof(Person.Role))
        {
            person.Role = SampleShaping.Next(PersonData.Roles, person.Role);
        }
        else
        {
            person.Department = SampleShaping.Next(PersonData.Departments, person.Department);
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Edited {0}'s {1} from {2} to {3}",
            person.FullName, SampleShaping.Label(GroupKeySelector), from, SampleShaping.KeyOf(person, _appliedKey)));
    }

    private void OnClearLogClick(object sender, RoutedEventArgs e)
    {
        EditLog.Clear();
        SetLastAction("Cleared the edit log");
    }

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (!SampleShaping.IsReselecting)
        {
            RefreshReadouts();
        }
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || ReadOnlyText is null || OpenEditText is null || EditsCommittedText is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        ReadOnlyText.Text = PeopleTable.IsReadOnly ? "Yes" : "No";
        OpenEditText.Text = _openEdit;
        EditsCommittedText.Text = _editsCommitted.ToString("N0", CultureInfo.CurrentCulture);
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard every element this path touches.
        if (_source is null || PeopleTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || EditGroupedValueButton is null || ShapingModeText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Department");
        var selected = PeopleTable.SelectedItem;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                _source.GroupBy(item => SampleShaping.KeyOf(item as Person, key), SampleShaping.GroupIdentity);
                break;
            // case "hierarchy":
            // case "groupedHierarchy":
            //     Hierarchical (tree) rows are not available in this release, so the two matching
            //     ComboBoxItems ship disabled. TableViewSource and TableView have no hierarchy
            //     member today. When hierarchy ships, apply it to this same source here, composed
            //     with the GroupBy stage above rather than replacing it, and set _appliedMode only
            //     after the call returns.
            default:
                _source.ClearGroupBy();
                mode = "flat";
                break;
        }

        _appliedMode = mode;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(PeopleTable, selected, People.Count + PersonData.Roles.Count, RefreshReadouts);

        UpdateShapingGating();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
    }

    // Call after ANY write to the grouped-on property: from an action or from an in-cell edit.
    private void ReapplyIfGroupedOn(string? propertyName)
    {
        if (_appliedMode == "grouped" && propertyName == _appliedKey)
        {
            ApplyShaping(announce: false);
        }
    }

    private void UpdateShapingGating()
    {
        var grouped = _appliedMode == "grouped";
        GroupKeySelector.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        EditGroupedValueButton.IsEnabled = grouped;
        ShapingModeText.Text = SampleShaping.ShapingText(grouped, GroupKeySelector);
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.CollapseAllGroups();
        SetLastAction("Collapsed all groups");
    }

    // The only writer of LastActionText.
    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }

        RefreshReadouts();
    }

    #endregion
}
