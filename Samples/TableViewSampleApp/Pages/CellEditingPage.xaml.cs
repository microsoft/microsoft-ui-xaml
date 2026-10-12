// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Controls;
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
public sealed partial class CellEditingPage : SamplePageBase
{
    // Properties that an editable column on this page writes.
    private static readonly string[] s_editableProperties =
    {
        nameof(Person.FirstName),
        nameof(Person.LastName),
        nameof(Person.Role),
        nameof(Person.Department),
    };

    public CellEditingPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackItems(People, OnPersonChanged);
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource Source { get; }

    /// <summary>Values written to the model, newest first (bound in XAML).</summary>
    public ObservableCollection<string> EditLog { get; } = new();

    // <snippet>
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
        _openEdit = string.Format(CultureInfo.CurrentCulture, "{0} for {1}", args.Column?.Header, (args.Item as Person)?.FullName); // snippet:skip
        SetEditButtonsEnabled(true);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Began editing {0}", _openEdit));
    }

    // Raised for every close, with EditAction saying whether it committed or cancelled.
    private void OnCellEditEnding(TableView sender, TableViewCellEditEndingEventArgs args)
    {
        var what = string.Format(CultureInfo.CurrentCulture, "{0} for {1}", args.Column?.Header, (args.Item as Person)?.FullName); // snippet:skip
        _openEdit = "(none)"; // snippet:skip
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

        LogEdit(person, e.PropertyName!);

        // The write arrives from inside the control's commit. Re-group after it returns, so the
        // reshape never runs while the edit is still closing.
        var property = e.PropertyName;
        EnqueueIfLoaded(() =>
        {
            var hadFocus = IsFocusWithinTable();
            var regrouped = IsGrouped && property == AppliedGroupKey;
            ReapplyIfGroupedOn(property);
            if (regrouped && hadFocus)
            {
                // The reshape recycles the containers; put focus back on the edited row.
                RestoreRowFocusAfterLayout(person);
            }

            RefreshReadouts();
        });
        RefreshReadouts();
    }
    // </snippet>

    private void LogEdit(Person person, string propertyName)
    {
        var value = propertyName switch
        {
            nameof(Person.FirstName) => person.FirstName,
            nameof(Person.LastName) => person.LastName,
            nameof(Person.Role) => person.Role,
            _ => person.Department,
        };

        _editsCommitted++;
        EditLog.Insert(0, string.Format(CultureInfo.CurrentCulture, "{0} · {1} = {2}", person.FullName, propertyName, value));
        while (EditLog.Count > 50)
        {
            EditLog.RemoveAt(EditLog.Count - 1);
        }
    }

    private bool IsFocusWithinTable()
    {
        for (var node = XamlRoot is null ? null : FocusManager.GetFocusedElement(XamlRoot) as DependencyObject;
             node is not null;
             node = VisualTreeHelper.GetParent(node))
        {
            if (node == PeopleTable)
            {
                return true;
            }
        }

        return false;
    }

    // Workaround, not a pattern to copy: TableView v1 has no focus-by-item API, and re-applying GroupBy
    // keeps focus at the same projected position, often a group header.
    private void RestoreRowFocusAfterLayout(Person person)
    {
        void OnLayoutUpdated(object? sender, object e)
        {
            PeopleTable.LayoutUpdated -= OnLayoutUpdated;
            FindRealizedRow(PeopleTable, person)?.Focus(FocusState.Keyboard);
        }

        PeopleTable.LayoutUpdated += OnLayoutUpdated;
    }

    private static TableViewRow? FindRealizedRow(DependencyObject parent, object item)
    {
        var count = VisualTreeHelper.GetChildrenCount(parent);
        for (var i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);
            if (child is TableViewRow row && ReferenceEquals(row.DataContext, item))
            {
                return row;
            }

            if (child is not TableViewRow && FindRealizedRow(child, item) is { } nested)
            {
                return nested;
            }
        }

        return null;
    }

    protected override void OnShapingApplied(ShapingAppliedEventArgs e) =>
        EditGroupedValueButton.IsEnabled = e.IsGrouped;

    private void OnEditGroupedValueClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = SampleShaping.KeyOf(person, AppliedGroupKey);
        if (AppliedGroupKey == nameof(Person.Role))
        {
            person.Role = SampleShaping.Next(PersonData.Roles, person.Role);
        }
        else
        {
            person.Department = SampleShaping.Next(PersonData.Departments, person.Department);
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Edited {0}'s {1} from {2} to {3}",
            person.FullName, Shaping.SelectedKeyLabel, from, SampleShaping.KeyOf(person, AppliedGroupKey)));
    }

    private void OnClearLogClick(object sender, RoutedEventArgs e)
    {
        EditLog.Clear();
        SetLastAction("Cleared the edit log");
    }

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        RefreshReadouts();
    }
}
