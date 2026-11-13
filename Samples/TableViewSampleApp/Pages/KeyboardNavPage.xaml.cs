// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
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
    // Reset restores values AND membership without new objects, so a UIA inspector stays valid across it.
    private readonly AccessibilityFixtureData _data = new();
    private CollectionViewSource? _fixtureSource;
    private bool _updating = true;
    private bool _listening;

    public KeyboardNavPage()
    {
        // <snippet>
        Source = new CollectionViewSource { Source = People };
        InitializeComponent();
        PeopleTable.ItemsSource = Source.View;
        ResetFixture(announce: false); // snippet:skip
        // </snippet>
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackLifetime(
            () => PeopleTable.SelectionChanged += OnPeopleSelectionChanged,
            () => PeopleTable.SelectionChanged -= OnPeopleSelectionChanged);
        TrackItems(People, OnPersonChanged);
        TrackLifetime(StartListeningToFixture, StopListeningToFixture);
    }

    // <snippet>
    public ObservableCollection<Person> People { get; } = PersonData.Take(40);
    // </snippet>

    // <snippet>
    public CollectionViewSource Source { get; }
    // </snippet>

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
        RefreshReadouts();
    }

    // In-cell editors write the model; re-bucket when the grouped-on value changed, because
    // Grouping does not follow PropertyChanged; the view regroups on Refresh().
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

    // ---- Assessment fixture -----------------------------------------------------------------

    private string Scenario => SampleShaping.SelectedTag(ScenarioSelector, "Text");

    private void ResetFixture(bool announce)
    {
        _updating = true;
        var previousObservations = ExpectedObservationsText.Text;
        try
        {
            AssessmentTable.DeselectAll();
            AssessmentTable.ItemsSource = null;
            _data.Reset();
            ScenarioSelector.SelectedIndex = 0;
            GroupingToggle.IsOn = false;
            EnabledToggle.IsOn = true;
            FixtureSelectionModeSelector.SelectedIndex = 0;
            AssessmentTable.IsEnabled = true;
            AssessmentTable.SelectionMode = TableViewSelectionMode.Single;
            ConfigureColumns();
            _fixtureSource = new CollectionViewSource { Source = _data.Rows };
            AssessmentTable.ItemsSource = _fixtureSource.View;
        }
        finally
        {
            _updating = false;
        }

        if (announce)
        {
            if (ExpectedObservationsText.Text != previousObservations)
            {
                AnnounceExpectedObservations();
            }

            SetLastAction("Reset the fixture to its baseline; no rows selected by the app.");
        }
    }

    // TextBlock does not raise LiveRegionChanged on a text change; raise it for the user's change only.
    private void AnnounceExpectedObservations() =>
        (FrameworkElementAutomationPeer.FromElement(ExpectedObservationsText)
            ?? FrameworkElementAutomationPeer.CreatePeerForElement(ExpectedObservationsText))
            ?.RaiseAutomationEvent(AutomationEvents.LiveRegionChanged);

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
                CanUserSort = false,
            });
            AssessmentTable.Columns.Add(new TableViewTemplateColumn
            {
                Header = "Department",
                Width = new GridLength(1, GridUnitType.Star),
                MinWidth = 160,
                CellTemplate = (DataTemplate)Resources["DepartmentTemplate"],
                CanUserSort = false,
            });
        }
        else
        {
            // Text template columns sort by their SortMemberPath, set to the bound property.
            var record = SampleColumns.Text("Record", nameof(AccessibilityRow.DisplayText), new GridLength(2, GridUnitType.Star));
            record.MinWidth = 240;
            AssessmentTable.Columns.Add(record);
            var department = SampleColumns.Text("Department", nameof(AccessibilityRow.Department), new GridLength(1, GridUnitType.Star));
            department.MinWidth = 160;
            AssessmentTable.Columns.Add(department);
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

        ExpectedObservationsText.Text = ExpectedObservations(Scenario);
    }

    private static string ExpectedObservations(string scenario) =>
        scenario switch
        {
            "Intrinsic" =>
                "Two template columns only. With baseline values, check whether the row names contain Record 01 and Design, " +
                "and whether the Record cell name reads Record 01, with Record reported once as its column header item. The inner buttons take their names from the displayed text. " +
                "Activate one to log an app Click event in Last action. Template cells do not offer the cell Value pattern; " +
                "inspect the buttons' Invoke pattern separately.",
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
                "name reads Record 01, Design and the cell name Record 01, with Record reported once as its column header item. Cells do not offer " +
                "the Value pattern; verify it with an inspector.",
        };

    private void OnScenarioChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_updating || AssessmentTable is null)
        {
            return;
        }

        ConfigureColumns();
        AnnounceExpectedObservations();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Fixture scenario -> {0}; source objects kept.", SampleShaping.Label(ScenarioSelector)));
    }

    private void OnResetClick(object sender, RoutedEventArgs e) => ResetFixture(announce: true);

    private void OnGroupingToggled(object sender, RoutedEventArgs e)
    {
        if (_updating || _fixtureSource is null)
        {
            return;
        }

        if (GroupingToggle.IsOn)
        {
            _fixtureSource.GroupDescriptions.Add(new PropertyGroupDescription(nameof(AccessibilityRow.Department)));
        }
        else
        {
            _fixtureSource.GroupDescriptions.Clear();
        }

        // Turning grouping on or off gives the CollectionViewSource a new View.
        AssessmentTable.ItemsSource = _fixtureSource.View;
        SetLastAction(GroupingToggle.IsOn
            ? "Fixture grouped by Department on the same CollectionViewSource."
            : "Fixture grouping cleared on the same CollectionViewSource.");
    }

    private void OnEnabledToggled(object sender, RoutedEventArgs e)
    {
        if (_updating)
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
        var row = AssessmentTable.SelectedItem as AccessibilityRow;
        SetLastAction(_data.Mutate(row)
            ? string.Format(CultureInfo.CurrentCulture, "Mutated selected record {0} in place.", row!.Id)
            : "No row selected.");
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        var row = AssessmentTable.SelectedItem as AccessibilityRow;
        SetLastAction(_data.Remove(row)
            ? string.Format(CultureInfo.CurrentCulture, "Removed selected record {0}; the control owns what happens to the selection.", row!.Id)
            : "No row selected.");
    }

    private void OnMutateLastClick(object sender, RoutedEventArgs e)
    {
        SetLastAction(_data.Mutate(_data.LastRow)
            ? "Mutated record 24 in place; whether it was realized was not checked."
            : "Record 24 is removed. Restore it first.");
    }

    private void OnRemoveLastClick(object sender, RoutedEventArgs e)
    {
        SetLastAction(_data.Remove(_data.LastRow) ? "Removed record 24." : "Record 24 is already removed.");
    }

    private void OnRestoreClick(object sender, RoutedEventArgs e)
    {
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored {0} original objects in source order; their edited values were kept.", _data.RestoreRemoved()));
    }

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

    private void OnFixtureRowChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!_updating)
        {
            RefreshReadouts();
        }
    }
}
