// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;

namespace TableViewSampleApp.Pages;

/// <summary>
/// The gallery's single accessibility page, merged from the former
/// KeyboardNav and AccessibilityRegression pages.
///
/// Top half — the user-facing lesson: keyboard navigation over a read-only
/// TableView, with focus sentinels either side so tab order in and out can be
/// verified, plus row/column counts read from the bound source and Columns
/// collection.
///
/// Bottom half — a manual UIA / Narrator assessment fixture. Four scenarios
/// (text columns, template cells with intrinsic names, template cells with
/// explicit <c>AutomationProperties.Name</c>, and a deliberately unlabeled
/// table) run over the same 24 deterministic records so an assessor can compare
/// what the provider reports in each shape. The fixture's determinism is the
/// point: stable IDs and source order, in-place mutation that changes a group
/// key and raises PropertyChanged, remove/restore that preserves object
/// identity and original order, and a reset that restores edited values and
/// membership without duplicating rows. Nothing here measures accessibility;
/// the on-page readout reports app/model state only.
/// </summary>
public sealed partial class KeyboardNavPage : Page
{
    private readonly AccessibilityFixtureData _data = new();
    private TableViewSource? _source;
    private bool _updating = true;
    private bool _listening;
    private string _lastAction = "Reset";

    public KeyboardNavPage()
    {
        InitializeComponent();

        // Person has no Id/Index property, so project the shared dataset into
        // a lightweight row that carries a stable 1-based "#" for the leading
        // column (same idiom as RTLPlaygroundPage). The ordinal also makes
        // Home/End/PageUp/PageDown movement obvious.
        int number = 1;
        foreach (var p in PersonData.All)
        {
            People.Add(new KeyboardNavRow
            {
                Number = number++,
                FirstName = p.FirstName,
                LastName = p.LastName,
                Email = p.Email,
                Department = p.Department,
                Role = p.Role,
            });
        }

        PeopleTable.ItemsSource = People;
        PeopleTable.SizeChanged += (_, _) => UpdateReadout();

        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;

        ResetFixture();
    }

    public ObservableCollection<KeyboardNavRow> People { get; } = new();

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        UpdateReadout();

        if (_listening)
        {
            return;
        }

        foreach (var row in _data.OriginalRows)
        {
            row.PropertyChanged += OnRowPropertyChanged;
        }

        _listening = true;
        UpdateFixtureReadout();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        foreach (var row in _data.OriginalRows)
        {
            row.PropertyChanged -= OnRowPropertyChanged;
        }

        _listening = false;
    }

    // ---- keyboard navigation half -------------------------------------------------

    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        SelectedIndexText.Text = PeopleTable.SelectedIndex.ToString();
    }

    private void UpdateReadout()
    {
        RowCountText.Text = People.Count.ToString();
        ColumnCountText.Text = PeopleTable.Columns.Count.ToString();
        MajorText.Text = "RowMajor";
    }

    // ---- assessment fixture half --------------------------------------------------

    private string Scenario => (ScenarioSelector.SelectedItem as ComboBoxItem)?.Tag as string ?? "Text";

    private void ResetFixture()
    {
        if (!CloseEditForSetup()) return;

        _updating = true;
        try
        {
            AssessmentTable.DeselectAll();
            AssessmentTable.ItemsSource = null;
            _data.Reset();
            ScenarioSelector.SelectedIndex = 0;
            GroupingToggle.IsOn = false;
            EditingToggle.IsOn = false;
            EnabledToggle.IsOn = true;
            SelectionModeSelector.SelectedIndex = 0;
            AssessmentTable.IsEnabled = true;
            AssessmentTable.IsReadOnly = true;
            AssessmentTable.SelectionMode = TableViewSelectionMode.Single;
            ConfigureColumns();
            _source = TableViewSource.From(_data.Rows);
            AssessmentTable.ItemsSource = _source;
        }
        finally
        {
            _updating = false;
        }

        SetAction("Reset to baseline; no rows selected by the app.");
    }

    private void ConfigureColumns()
    {
        AssessmentTable.ClearSort();
        AssessmentTable.Columns.Clear();

        if (Scenario is "Intrinsic" or "Explicit")
        {
            AssessmentTable.Columns.Add(new TableViewTemplateColumn
            {
                Header = "Record",
                Width = new GridLength(240),
                CellTemplate = (DataTemplate)Resources[Scenario == "Explicit"
                    ? "ExplicitRecordTemplate" : "IntrinsicRecordTemplate"],
                CanSort = false,
            });
            AssessmentTable.Columns.Add(new TableViewTemplateColumn
            {
                Header = "Department",
                Width = new GridLength(220),
                CellTemplate = (DataTemplate)Resources["DepartmentTemplate"],
                CanSort = false,
            });
        }
        else
        {
            AssessmentTable.Columns.Add(new TableViewTextColumn
            {
                Header = "Record",
                Width = new GridLength(240),
                Binding = new Binding { Path = new PropertyPath(nameof(AccessibilityRow.DisplayText)) },
                SortMemberPath = nameof(AccessibilityRow.DisplayText),
            });
            AssessmentTable.Columns.Add(new TableViewTextColumn
            {
                Header = "Department",
                Width = new GridLength(220),
                Binding = new Binding { Path = new PropertyPath(nameof(AccessibilityRow.Department)) },
                SortMemberPath = nameof(AccessibilityRow.Department),
            });
        }

        // One deliberate default-name baseline; no row/cell blanket names or provider overrides.
        if (Scenario == "Unlabeled")
        {
            AssessmentTable.ClearValue(AutomationProperties.NameProperty);
        }
        else
        {
            AutomationProperties.SetName(AssessmentTable, "Accessibility regression records");
        }

        ExpectedObservationsText.Text = Scenario switch
        {
            "Intrinsic" =>
                "Two template columns only. With baseline values, inspect row names containing Record 01 and Design, " +
                "and the Record cell name Record, Record 01. The inner buttons derive their names from displayed content. " +
                "Activate one to log an app Click event. Template cells do not offer the cell Value pattern in this preview, " +
                "even when Allow text editing is on; inspect the buttons' Invoke pattern separately.",
            "Explicit" =>
                "With baseline values, the Record button displays Record 01 but has the explicit app name " +
                "Explicit label for record 01. The Record cell name should include that label. Also check whether " +
                "the row name includes Department (Design) before and after inspecting the child buttons. " +
                "Source caveat: the cheap row-name pass may omit the Department button until its peer exists. " +
                "Department retains its intrinsic name. This tests Name precedence, NOT Value: template cells do not offer " +
                "the cell Value pattern. Compare with Template-only: intrinsic without resetting the objects.",
            "Unlabeled" =>
                "No app AutomationProperties.Name or LabeledBy is supplied on this table; A11yTable remains its stable ID. " +
                "Record the provider's default/empty Name without treating AutomationId as a spoken label. " +
                "Rows and cells should still describe their contents. Compare with Text columns for the app-named case.",
            _ =>
                "Baseline: 24 source records and two text columns. Inspect the app-provided table Name " +
                "Accessibility regression records and ID A11yTable. For the first flat row, compare row text " +
                "Record 01, Design with cell Name Record, Record 01. With Allow text editing on, plain text cells " +
                "can offer Value with the displayed text (Record 01), not the composed cell Name. Turning it off removes " +
                "that editable Value pattern; verify externally rather than assuming a read-only Value provider exists.",
        };
    }

    private bool CloseEditForSetup()
    {
        if (!AssessmentTable.IsEditing || AssessmentTable.CancelEdit()) return true;
        SetAction("Setup not applied: CancelEdit did not close the editor. Finish or cancel the edit in the table.");
        return false;
    }

    private void OnScenarioChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_updating || !CloseEditForSetup()) return;
        ConfigureColumns();
        SetAction($"Scenario changed to {Scenario}; source objects retained.");
    }

    private void OnResetClick(object sender, RoutedEventArgs e) => ResetFixture();

    private void OnGroupingToggled(object sender, RoutedEventArgs e)
    {
        if (_updating || _source is null || !CloseEditForSetup()) return;

        if (GroupingToggle.IsOn)
        {
            // Department is a reference-type (string) key, so the current API requires the
            // two-selector overload: the built-in value-type identity does not cover it and
            // the single-selector form would fail fast at projection time.
            _source.GroupBy(GroupKeySelector, GroupIdentity);
        }
        else
        {
            _source.ClearGroupBy();
        }

        SetAction("Grouping setup changed on the same TableViewSource.");
    }

    private static object GroupKeySelector(object item) => ((AccessibilityRow)item).Department;

    private static string GroupIdentity(object key)
    {
        var identity = key?.ToString();
        return string.IsNullOrWhiteSpace(identity) ? "(none)" : identity;
    }

    private void OnEditingToggled(object sender, RoutedEventArgs e)
    {
        if (_updating || !CloseEditForSetup()) return;
        AssessmentTable.IsReadOnly = !EditingToggle.IsOn;
        SetAction("Table IsReadOnly changed; template scenarios still have no cell editor.");
    }

    private void OnEnabledToggled(object sender, RoutedEventArgs e)
    {
        if (_updating || !CloseEditForSetup()) return;
        AssessmentTable.IsEnabled = EnabledToggle.IsOn;
        SetAction("Table IsEnabled changed; setup controls remain enabled.");
    }

    private void OnSelectionModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_updating) return;
        AssessmentTable.SelectionMode = SelectionModeSelector.SelectedIndex == 1
            ? TableViewSelectionMode.None : TableViewSelectionMode.Single;
        SetAction("Selection mode setup changed (only None and Single are supported).");
    }

    private void OnMutateSelectedClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup()) return;
        var row = AssessmentTable.SelectedItem as AccessibilityRow;
        SetAction(_data.Mutate(row)
            ? $"Mutated selected record {row!.Id} in place."
            : "No present data record is selected; select one in the table first.");
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup()) return;
        var row = AssessmentTable.SelectedItem as AccessibilityRow;
        SetAction(_data.Remove(row)
            ? $"Removed selected record {row!.Id}; the control owns selection reconciliation."
            : "No present data record is selected; select one in the table first.");
    }

    private void OnMutateLastClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup()) return;
        SetAction(_data.Mutate(_data.LastRow)
            ? "Mutated record 24 in place; its actual offscreen/realization state was not queried."
            : "Record 24 is removed. Restore it first.");
    }

    private void OnRemoveLastClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup()) return;
        SetAction(_data.Remove(_data.LastRow) ? "Removed record 24." : "Record 24 is already removed.");
    }

    private void OnRestoreClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup()) return;
        SetAction($"Restored {_data.RestoreRemoved()} original objects in source order; their edited values were retained.");
    }

    private void OnCommitEditClick(object sender, RoutedEventArgs e) =>
        SetAction($"CommitEdit returned {AssessmentTable.CommitEdit()}. Focus loss may already have committed.");

    private void OnCancelEditClick(object sender, RoutedEventArgs e) =>
        SetAction($"CancelEdit returned {AssessmentTable.CancelEdit()}. Use Esc inside the editor to assess rollback.");

    private void OnTemplateContentClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: AccessibilityRow row })
        {
            SetAction($"Template button Click event for record {row.Id}; input origin was not measured.");
        }
    }

    private void OnTableSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        if (!_updating) UpdateFixtureReadout();
    }

    private void OnBeginningEdit(TableView sender, TableViewBeginningEditEventArgs args) =>
        SetAction("BeginningEdit event observed; this is not an edit-completion signal.");

    private void OnCellEditEnding(TableView sender, TableViewCellEditEndingEventArgs args)
    {
        SetAction($"CellEditEnding: {args.EditAction} (pre-close event).");
        DispatcherQueue.TryEnqueue(() =>
        {
            if (_listening) UpdateFixtureReadout();
        });
    }

    private void OnRowPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!_updating) UpdateFixtureReadout();
    }

    private void SetAction(string action)
    {
        _lastAction = action;
        if (!_updating) UpdateFixtureReadout();
    }

    private void UpdateFixtureReadout()
    {
        var selected = AssessmentTable.SelectedItem as AccessibilityRow;
        ModelStatusText.Text =
            $"Source objects: {_data.Rows.Count}; removed: {_data.RemovedCount}; declared columns: {AssessmentTable.Columns.Count}.\n" +
            $"SelectedItem record ID: {selected?.Id.ToString() ?? "(none)"}; control SelectedIndex: {AssessmentTable.SelectedIndex}.\n" +
            $"IsEnabled: {AssessmentTable.IsEnabled}; IsReadOnly: {AssessmentTable.IsReadOnly}; IsEditing: {AssessmentTable.IsEditing}.\n" +
            $"Last app action: {_lastAction}\nNo focus, UIA, or screen-reader result is inferred.";
    }

    /// <summary>
    /// Lightweight row for the keyboard-navigation table: carries a stable
    /// 1-based ordinal so Home/End/PageUp/PageDown movement is obvious.
    /// </summary>
    public sealed class KeyboardNavRow
    {
        public int Number { get; set; }
        public string FirstName { get; set; } = string.Empty;
        public string LastName { get; set; } = string.Empty;
        public string Email { get; set; } = string.Empty;
        public string Department { get; set; } = string.Empty;
        public string Role { get; set; } = string.Empty;
    }
}
