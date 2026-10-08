// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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
public sealed partial class KeyboardNavPage : Page
{
    private readonly AccessibilityFixtureData _data = new();
    private TableViewSource? _source;          // top table: created ONCE; reshaped in place
    private TableViewSource? _fixtureSource;
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private bool _updating = true;
    private bool _listening;

    public KeyboardNavPage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();
        PeopleTable.ItemsSource = _source;
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        ResetFixture(announce: false);
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        PeopleTable.SelectionChanged += OnPeopleSelectionChanged;
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        if (!_listening)
        {
            foreach (var row in _data.OriginalRows)
            {
                row.PropertyChanged += OnFixtureRowChanged;
            }

            _listening = true;
        }

        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        PeopleTable.SelectionChanged -= OnPeopleSelectionChanged;
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

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

    // In-cell editors write the model; re-bucket when the grouped-on value changed.
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

        ExpectedObservationsText.Text = Scenario switch
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
    }

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

    // ---- Readouts ---------------------------------------------------------------------------

    private void RefreshReadouts()
    {
        if (PeopleTable is null || RowsText is null || SelectedIndexText is null || AssessmentTable is null)
        {
            return;
        }

        var index = PeopleTable.SelectedIndex;
        SelectedIndexText.Text = index >= 0
            ? index.ToString(CultureInfo.CurrentCulture)
            : string.Format(CultureInfo.CurrentCulture, "{0} (none)", -1);
        ColumnsText.Text = PeopleTable.Columns.Count.ToString(CultureInfo.CurrentCulture);
        RowsText.Text = SampleShaping.RowCountText(People.Count);

        FixtureRecordsText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} in the source, {1:N0} removed", _data.Rows.Count, _data.RemovedCount);
        FixtureSelectionText.Text = AssessmentTable.SelectedItem is AccessibilityRow selected
            ? string.Format(CultureInfo.CurrentCulture, "Record {0:00}, SelectedIndex {1}", selected.Id, AssessmentTable.SelectedIndex)
            : string.Format(CultureInfo.CurrentCulture, "(none), SelectedIndex {0}", AssessmentTable.SelectedIndex);
        FixtureEditingText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "{0} (IsReadOnly {1}, IsEnabled {2})",
            AssessmentTable.IsEditing,
            AssessmentTable.IsReadOnly,
            AssessmentTable.IsEnabled);
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent, before the later-declared elements exist.
        if (_source is null || PeopleTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null)
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
        SampleShaping.Reselect(PeopleTable, selected, People.Count + PersonData.Departments.Count + PersonData.Offices.Count, RefreshReadouts);
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
