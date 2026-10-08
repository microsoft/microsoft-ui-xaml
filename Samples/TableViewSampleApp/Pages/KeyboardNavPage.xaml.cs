// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Keyboard navigation + accessibility.
///
/// Top: keyboard navigation over a table of mixed cells (text, chip, CheckBox, ComboBox, Button)
/// with focus sentinels either side, so the tab order in and out can be checked.
///
/// Bottom: a manual UIA / Narrator assessment fixture. Four scenarios (text columns, template
/// cells with intrinsic names, template cells with an explicit AutomationProperties.Name, and an
/// unlabeled table) run over the same 24 deterministic records. The fixture's determinism is
/// the point: stable IDs and source order, in-place mutation that changes a group key and raises
/// PropertyChanged, remove/restore that keeps object identity and order, and a reset that
/// restores values and membership without duplicating rows. Nothing here measures
/// accessibility; the readouts report app and model state only.
/// </summary>
public sealed partial class KeyboardNavPage : SamplePageBase
{
    // The fixture's value is determinism: the same 24 objects, in the same order, under every
    // scenario. Reset restores values AND membership without creating new objects, so a UIA
    // inspector can stay open across a reset.
    private readonly AccessibilityFixtureData _data = new();
    private TableViewSource? _fixtureSource;
    private bool _updating = true;
    private bool _listening;

    public KeyboardNavPage()
    {
        // <snippet>
        // The keyboard table reads from one TableViewSource, created once and reshaped in place.
        Source = TableViewSource.From(People);
        InitializeComponent();
        PeopleTable.ItemsSource = Source;
        ResetFixture(announce: false);
        // </snippet>
        Shaping.ProbeLimit = () => People.Count + PersonData.Departments.Count + PersonData.Offices.Count;
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackLifetime(
            () => PeopleTable.SelectionChanged += OnPeopleSelectionChanged,
            () => PeopleTable.SelectionChanged -= OnPeopleSelectionChanged);
        TrackItems(People, OnPersonChanged);
        TrackLifetime(StartListeningToFixture, StopListeningToFixture);
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource Source { get; }

    private void StartListeningToFixture()
    {
        if (!_listening)
        {
            foreach (var row in _data.OriginalRows)
            {
                row.PropertyChanged += OnFixtureRowChanged;
            }

            _listening = true;
        }
    }

    private void StopListeningToFixture()
    {
        foreach (var row in _data.OriginalRows)
        {
            row.PropertyChanged -= OnFixtureRowChanged;
        }

        _listening = false;
    }

    // ---- Keyboard navigation table ----------------------------------------------------------

    private void OnPeopleSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (!SampleShaping.IsReselecting)
        {
            RefreshReadouts();
        }
    }

    // <snippet>
    // In-cell editors write the model; re-bucket when the grouped-on value changed, because
    // GroupBy takes a delegate and cannot follow PropertyChanged.
    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.IsActive):
                ReapplyIfGroupedOn(e.PropertyName);
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Active -> {0} for {1}", person.IsActive ? "checked" : "unchecked", person.FullName));
                break;
            case nameof(Person.Office):
                ReapplyIfGroupedOn(e.PropertyName);
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Office -> {0} for {1}", person.Office, person.FullName));
                break;
        }
    }
    // </snippet>

    // ---- Assessment fixture -----------------------------------------------------------------

    private string Scenario => SampleShaping.SelectedTag(ScenarioSelector, "Text");

    private void ResetFixture(bool announce)
    {
        if (!CloseEditForSetup())
        {
            return;
        }

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
            FixtureSelectionModeSelector.SelectedIndex = 0;
            AssessmentTable.IsEnabled = true;
            AssessmentTable.IsReadOnly = true;
            AssessmentTable.SelectionMode = TableViewSelectionMode.Single;
            ConfigureColumns();
            _fixtureSource = TableViewSource.From(_data.Rows);
            AssessmentTable.ItemsSource = _fixtureSource;
        }
        finally
        {
            _updating = false;
        }

        if (announce)
        {
            SetLastAction("Reset the fixture to its baseline; no rows selected by the app.");
        }
    }

    // <snippet>
    private void ConfigureColumns()
    {
        AssessmentTable.ClearSort();
        AssessmentTable.Columns.Clear();

        // Record takes two thirds of the width and Department one third, so the two columns
        // fill the table in every scenario instead of leaving an empty filler band.
        if (Scenario is "Intrinsic" or "Explicit")
        {
            AssessmentTable.Columns.Add(new TableViewTemplateColumn
            {
                Header = "Record",
                Width = new GridLength(2, GridUnitType.Star),
                MinWidth = 240,
                CellTemplate = (DataTemplate)Resources[Scenario == "Explicit" ? "ExplicitRecordTemplate" : "IntrinsicRecordTemplate"],
                CanSort = false,
            });
            AssessmentTable.Columns.Add(new TableViewTemplateColumn
            {
                Header = "Department",
                Width = new GridLength(1, GridUnitType.Star),
                MinWidth = 160,
                CellTemplate = (DataTemplate)Resources["DepartmentTemplate"],
                CanSort = false,
            });
        }
        else
        {
            // Text columns sort by their Binding path; no SortMemberPath needed.
            AssessmentTable.Columns.Add(new TableViewTextColumn
            {
                Header = "Record",
                Width = new GridLength(2, GridUnitType.Star),
                MinWidth = 240,
                Binding = new Binding { Path = new PropertyPath(nameof(AccessibilityRow.DisplayText)) },
            });
            AssessmentTable.Columns.Add(new TableViewTextColumn
            {
                Header = "Department",
                Width = new GridLength(1, GridUnitType.Star),
                MinWidth = 160,
                Binding = new Binding { Path = new PropertyPath(nameof(AccessibilityRow.Department)) },
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

        ExpectedObservationsText.Text = ExpectedObservations(Scenario); // snippet:skip
    }
    // </snippet>

    // What to check in each scenario: shown under the fixture table.
    private static string ExpectedObservations(string scenario) =>
        scenario switch
        {
            "Intrinsic" =>
                "Two template columns only. With baseline values, check whether the row names contain Record 01 and Design, " +
                "and whether the Record cell name reads Record, Record 01. The inner buttons take their names from the displayed text. " +
                "Activate one to log an app Click event in Last action. Template cells do not offer the cell Value pattern in this preview, " +
                "even when text editing is allowed; inspect the buttons' Invoke pattern separately.",
            "Explicit" =>
                "With baseline values, the Record button displays Record 01 but has the explicit app name " +
                "Explicit label for record 01. Check whether the Record cell name includes that label; record what you see. " +
                "Also check whether the row name includes the Department (Design) before and after you inspect the child buttons, " +
                "because the row name may omit the Department button until its peer exists. " +
                "Department keeps its intrinsic name. This tests Name precedence, not Value: template cells do not offer " +
                "the cell Value pattern. Compare with Template-only: intrinsic without resetting the objects.",
            "Unlabeled" =>
                "No app AutomationProperties.Name or LabeledBy is set on this table; its AutomationId stays KeyboardNavFixtureTable. " +
                "Record the provider's default or empty Name without treating the AutomationId as a spoken label. " +
                "Check whether rows and cells still describe their contents. Compare with Text columns for the app-named case.",
            _ =>
                "Baseline: 24 source records and two text columns. Inspect the app-provided table Name " +
                "Accessibility regression records and the AutomationId KeyboardNavFixtureTable. For the first flat row, check whether the row " +
                "name reads Record 01, Design and the cell name Record, Record 01. With text editing allowed, plain text cells " +
                "can offer the Value pattern with the displayed text (Record 01), not the composed cell name. Turning editing off removes " +
                "that editable Value pattern; verify it with an inspector rather than assuming a read-only Value provider exists.",
        };

    private bool CloseEditForSetup()
    {
        if (!AssessmentTable.IsEditing || AssessmentTable.CancelEdit())
        {
            return true;
        }

        SetLastAction("Setup not applied: CancelEdit did not close the editor. Finish or cancel the edit in the table.");
        return false;
    }

    private void OnScenarioChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_updating || AssessmentTable is null || !CloseEditForSetup())
        {
            return;
        }

        ConfigureColumns();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Fixture scenario -> {0}; source objects kept.", SampleShaping.Label(ScenarioSelector)));
    }

    private void OnResetClick(object sender, RoutedEventArgs e) => ResetFixture(announce: true);

    // <snippet>
    // The fixture groups with its own switch (a documented exception to the Shaping list).
    private void OnGroupingToggled(object sender, RoutedEventArgs e)
    {
        if (_updating || _fixtureSource is null || !CloseEditForSetup())
        {
            return;
        }

        if (GroupingToggle.IsOn)
        {
            // The key selector receives the ROW; the identity selector receives the KEY.
            _fixtureSource.GroupBy(item => ((AccessibilityRow)item).Department, SampleShaping.GroupIdentity);
        }
        else
        {
            _fixtureSource.ClearGroupBy();
        }

        SetLastAction(GroupingToggle.IsOn
            ? "Fixture grouped by Department on the same TableViewSource."
            : "Fixture grouping cleared on the same TableViewSource.");
    }
    // </snippet>

    private void OnEditingToggled(object sender, RoutedEventArgs e)
    {
        if (_updating || !CloseEditForSetup())
        {
            return;
        }

        AssessmentTable.IsReadOnly = !EditingToggle.IsOn;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Fixture IsReadOnly -> {0}; template scenarios still have no cell editor.", AssessmentTable.IsReadOnly));
    }

    private void OnEnabledToggled(object sender, RoutedEventArgs e)
    {
        if (_updating || !CloseEditForSetup())
        {
            return;
        }

        AssessmentTable.IsEnabled = EnabledToggle.IsOn;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Fixture IsEnabled -> {0}; the setup controls stay enabled.", AssessmentTable.IsEnabled));
    }

    private void OnFixtureSelectionModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_updating || AssessmentTable is null)
        {
            return;
        }

        var none = SampleShaping.SelectedTag(FixtureSelectionModeSelector, "Single") == "None";
        AssessmentTable.SelectionMode = none ? TableViewSelectionMode.None : TableViewSelectionMode.Single;
        SetLastAction(none ? "Fixture selection mode -> None" : "Fixture selection mode -> Single");
    }

    private void OnMutateSelectedClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup())
        {
            return;
        }

        var row = AssessmentTable.SelectedItem as AccessibilityRow;
        SetLastAction(_data.Mutate(row)
            ? string.Format(CultureInfo.CurrentCulture, "Mutated selected record {0} in place.", row!.Id)
            : "No row selected.");
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup())
        {
            return;
        }

        var row = AssessmentTable.SelectedItem as AccessibilityRow;
        SetLastAction(_data.Remove(row)
            ? string.Format(CultureInfo.CurrentCulture, "Removed selected record {0}; the control owns what happens to the selection.", row!.Id)
            : "No row selected.");
    }

    private void OnMutateLastClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup())
        {
            return;
        }

        SetLastAction(_data.Mutate(_data.LastRow)
            ? "Mutated record 24 in place; whether it was realized was not checked."
            : "Record 24 is removed. Restore it first.");
    }

    private void OnRemoveLastClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup())
        {
            return;
        }

        SetLastAction(_data.Remove(_data.LastRow) ? "Removed record 24." : "Record 24 is already removed.");
    }

    private void OnRestoreClick(object sender, RoutedEventArgs e)
    {
        if (!CloseEditForSetup())
        {
            return;
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored {0} original objects in source order; their edited values were kept.", _data.RestoreRemoved()));
    }

    private void OnCommitEditClick(object sender, RoutedEventArgs e) =>
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "CommitEdit returned {0}. Leaving the editor may already have committed.", AssessmentTable.CommitEdit()));

    private void OnCancelEditClick(object sender, RoutedEventArgs e) =>
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "CancelEdit returned {0}. Use Esc inside the editor to assess rollback.", AssessmentTable.CancelEdit()));

    private void OnTemplateContentClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement { DataContext: AccessibilityRow row })
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Template button Click event for record {0}; the input source was not measured.", row.Id));
        }
    }

    private void OnFixtureSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (!_updating)
        {
            RefreshReadouts();
        }
    }

    private void OnBeginningEdit(TableView sender, TableViewBeginningEditEventArgs args) =>
        SetLastAction("BeginningEdit raised; this does not mean the edit completed.");

    private void OnCellEditEnding(TableView sender, TableViewCellEditEndingEventArgs args)
    {
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "CellEditEnding: {0} (raised before the editor closes).", args.EditAction));
        DispatcherQueue.TryEnqueue(() =>
        {
            if (_listening)
            {
                RefreshReadouts();
            }
        });
    }

    private void OnFixtureRowChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!_updating)
        {
            RefreshReadouts();
        }
    }
}
