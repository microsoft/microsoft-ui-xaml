// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using TableViewSelectionMode = Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using TableViewSource = Microsoft.UI.Xaml.Controls.Tabular.TableViewSource;
using TableViewKeySelector = Microsoft.UI.Xaml.Controls.Tabular.TableViewKeySelector;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSampleApp.Data;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Accessibility regression fixture for TableView.
///
/// The page is deliberately state-rich: every axis a Narrator / UI Automation
/// assessment wants to exercise — column shape, table name, grouping,
/// read-only versus editable cells, enablement, selection mode, the cell edit
/// lifecycle, and live data mutation — is driven from the Options rail so a
/// tester can walk the whole matrix without rebuilding the app.
///
/// Behaviour the page documents (verified against the live control, so the copy
/// must not drift from it):
///   * Focus is CELL-scoped. Cells are keyboard focusable and name themselves
///     "{column}, {value}".
///   * Left / Right step between cells in a row; Up / Down move between rows and
///     keep the focused column; Home / End jump to the first and last cell in the
///     row; Ctrl+Home / Ctrl+End jump to the first and last cell in the table.
///   * The table body is a single tab stop, so Tab enters it once and then leaves.
///   * Clicking a cell focuses that cell.
///   * Selection stays row-scoped: a vertical arrow moves focus and selection
///     together in Single selection mode, while Left / Right raise no selection
///     change.
///   * Column headers are focusable and Enter toggles sort, which raises a UIA
///     notification.
/// </summary>
public sealed partial class KeyboardNavPage : Page
{
    private const string TableAutomationName = "People grid";

    // Guards the re-entrant handler storm a programmatic reset would otherwise
    // cause: assigning SelectedIndex on the option ComboBoxes raises
    // SelectionChanged, which would each re-apply state and overwrite the
    // "Reset fixture" status message.
    private bool _isApplyingState;

    private int _mutationGeneration;

    public KeyboardNavPage()
    {
        // Person carries no ordinal, so project the shared 1,000-row dataset into a
        // row type that owns a stable 1-based "#". The ordinal makes row-by-row
        // movement (and Home / End / PageUp / PageDown) obvious at a glance, and
        // keeping all 1,000 rows preserves the virtualization / recycling surface
        // that the stale-value check depends on.
        int number = 1;
        foreach (var p in PersonData.All)
        {
            Rows.Add(new FixtureRow
            {
                Number = number++,
                FirstName = p.FirstName,
                LastName = p.LastName,
                Email = p.Email,
                Department = p.Department,
                Role = p.Role,
                IsActive = p.IsActive,
            });
        }

        InitializeComponent();

        ApplyBaseline(announce: false);

        Loaded += (_, _) => UpdateReadout();
        PeopleTable.SizeChanged += (_, _) => UpdateReadout();
    }

    public ObservableCollection<FixtureRow> Rows { get; } = new();

    // ----- Scenario (column shape + table name) -----

    private void OnScenarioChanged(object sender, TableViewSelectionChangedEventArgs e)
    {
        if (_isApplyingState || PeopleTable is null)
        {
            return;
        }

        ApplyScenario(SelectedTag(ScenarioSelector, "Text"));
        SetLastAction($"Scenario set to {SelectedContent(ScenarioSelector, "Text columns")}.");
    }

    private void ApplyScenario(string scenario)
    {
        PeopleTable.Columns.Clear();

        switch (scenario)
        {
            case "Template":
                AddTemplateColumn("Person", "PersonCellTemplate", "FullName", 200);
                AddTemplateColumn("Department", "DepartmentBadgeTemplate", "Department", 140);
                AddTemplateColumn("Active", "ActiveCellTemplate", "IsActive", 90);
                AddTemplateColumn("Role", "RoleCellTemplate", "Role", 200);
                break;

            case "Mixed":
                AddTextColumn("#", "Number", 80);
                AddTemplateColumn("Person", "PersonCellTemplate", "FullName", 200);
                AddTextColumn("Email", "Email", 220);
                AddTemplateColumn("Department", "DepartmentBadgeTemplate", "Department", 140);
                AddTemplateColumn("Active", "ActiveCellTemplate", "IsActive", 90);
                AddTextColumn("Role", "Role", 200);
                break;

            default:
                // "Text" and "Unlabeled" share the column shape; only the table's
                // AutomationProperties.Name differs, which is the whole point of
                // the Unlabeled scenario.
                AddTextColumn("#", "Number", 80);
                AddTextColumn("First name", "FirstName", 160);
                AddTextColumn("Last name", "LastName", 160);
                AddTextColumn("Email", "Email", 220);
                AddTextColumn("Department", "Department", 140);
                AddTextColumn("Role", "Role", 200);
                break;
        }

        if (scenario == "Unlabeled")
        {
            // Intentionally unnamed so the tester can observe what Narrator and
            // Inspect fall back to for a table with no AutomationProperties.Name.
            PeopleTable.ClearValue(AutomationProperties.NameProperty);
        }
        else
        {
            AutomationProperties.SetName(PeopleTable, TableAutomationName);
        }

        UpdateReadout();
    }

    private void AddTextColumn(string header, string path, double width)
    {
        PeopleTable.Columns.Add(new TableViewTextColumn
        {
            Header = header,
            Width = new GridLength(width),
            Binding = new Binding { Path = new PropertyPath(path) },
            SortMemberPath = path,
        });
    }

    private void AddTemplateColumn(string header, string templateKey, string sortMemberPath, double width)
    {
        // A template column has no Binding for the framework to fall back on, so
        // SortMemberPath is mandatory for its header to sort.
        PeopleTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = header,
            Width = new GridLength(width),
            SortMemberPath = sortMemberPath,
            CellTemplate = (DataTemplate)Resources[templateKey],
        });
    }

    // ----- Grouping -----

    private void OnGroupByDepartmentToggled(object sender, RoutedEventArgs e)
    {
        if (_isApplyingState || PeopleTable is null)
        {
            return;
        }

        ApplySource();
        SetLastAction(GroupByDepartmentToggle.IsOn
            ? "Grouping on: ItemsSource is TableViewSource.From(rows).GroupBy(Department)."
            : "Grouping off: ItemsSource is a flat TableViewSource.");
    }

    private void ApplySource()
    {
        var source = TableViewSource.From(Rows);
        if (GroupByDepartmentToggle.IsOn)
        {
            source = source.GroupBy(new TableViewKeySelector(item => ((FixtureRow)item).Department));
        }

        PeopleTable.ItemsSource = source;
        UpdateReadout();
    }

    // ----- Editing, enablement, selection mode -----

    private void OnAllowEditingToggled(object sender, RoutedEventArgs e)
    {
        if (_isApplyingState || PeopleTable is null)
        {
            return;
        }

        PeopleTable.IsReadOnly = !AllowEditingToggle.IsOn;
        UpdateReadout();
        SetLastAction(PeopleTable.IsReadOnly
            ? "IsReadOnly = true: text cells are display-only and expose no ValuePattern."
            : "IsReadOnly = false: double-tap a text cell, or press F2 on the focused row, to open an editor.");
    }

    private void OnEnableTableToggled(object sender, RoutedEventArgs e)
    {
        if (_isApplyingState || PeopleTable is null)
        {
            return;
        }

        PeopleTable.IsEnabled = EnableTableToggle.IsOn;
        UpdateReadout();
        SetLastAction(PeopleTable.IsEnabled
            ? "IsEnabled = true: the table is back in the Tab order."
            : "IsEnabled = false: Tab should now skip from Before table straight to After table.");
    }

    private void OnSelectionModeChanged(object sender, TableViewSelectionChangedEventArgs e)
    {
        if (_isApplyingState || PeopleTable is null)
        {
            return;
        }

        ApplySelectionMode();
        SetLastAction(PeopleTable.SelectionMode == TableViewSelectionMode.None
            ? "SelectionMode = None: arrow keys move focus without raising selection changes."
            : "SelectionMode = Single: focus and selection move together, so one arrow press raises both.");
    }

    private void ApplySelectionMode()
    {
        PeopleTable.SelectionMode = SelectedTag(SelectionModeSelector, "Single") == "None"
            ? TableViewSelectionMode.None
            : TableViewSelectionMode.Single;

        UpdateReadout();
    }

    // ----- Edit lifecycle -----

    private void OnCommitEditClick(object sender, RoutedEventArgs e)
    {
        var closed = PeopleTable.CommitEdit();
        UpdateReadout();
        SetLastAction(closed
            ? "CommitEdit() returned true: the open cell editor committed and closed."
            : "CommitEdit() returned false: no editor was open, or the close was vetoed.");
    }

    private void OnCancelEditClick(object sender, RoutedEventArgs e)
    {
        var closed = PeopleTable.CancelEdit();
        UpdateReadout();
        SetLastAction(closed
            ? "CancelEdit() returned true: the open cell editor was discarded and closed."
            : "CancelEdit() returned false: no editor was open, or the close was vetoed.");
    }

    // ----- Data mutation (stale-value check) -----

    private void OnMutateRowsClick(object sender, RoutedEventArgs e)
    {
        var target = SelectedTag(MutationTargetSelector, "Selected");
        int start;
        int count;

        switch (target)
        {
            case "First25":
                start = 0;
                count = 25;
                break;

            case "Offscreen":
                start = 500;
                count = 25;
                break;

            default:
                start = PeopleTable.SelectedIndex;
                count = 1;
                if (start < 0)
                {
                    SetLastAction("Nothing selected: select a row first, or pick a range target.");
                    return;
                }
                break;
        }

        if (start >= Rows.Count)
        {
            SetLastAction("Mutation target is outside the bound range.");
            return;
        }

        count = Math.Min(count, Rows.Count - start);
        _mutationGeneration++;

        for (int i = start; i < start + count; i++)
        {
            Rows[i].Mutate(_mutationGeneration);
        }

        UpdateReadout();
        SetLastAction(count == 1
            ? $"Mutated row {start + 1} in place (generation {_mutationGeneration}). Scroll it out of view and back: it must not announce its previous values."
            : $"Mutated rows {start + 1} to {start + count} in place (generation {_mutationGeneration}). Scroll them out of view and back to check for stale announcements.");
    }

    // ----- Reset -----

    private void OnResetFixtureClick(object sender, RoutedEventArgs e)
    {
        ApplyBaseline(announce: true);
    }

    /// <summary>
    /// Restores the documented baseline, which the Options rail also states in
    /// plain text: Text columns, table named "People grid", grouping off,
    /// IsReadOnly = true, IsEnabled = true, SelectionMode = Single, nothing
    /// selected, no sort, and all 1,000 rows back at their original values.
    /// </summary>
    private void ApplyBaseline(bool announce)
    {
        _isApplyingState = true;
        try
        {
            ScenarioSelector.SelectedIndex = 0;
            SelectionModeSelector.SelectedIndex = 1;
            GroupByDepartmentToggle.IsOn = false;
            AllowEditingToggle.IsOn = false;
            EnableTableToggle.IsOn = true;
            MutationTargetSelector.SelectedIndex = 0;

            PeopleTable.CancelEdit();
            PeopleTable.IsReadOnly = true;
            PeopleTable.IsEnabled = true;

            foreach (var row in Rows)
            {
                row.Restore();
            }
            _mutationGeneration = 0;

            ApplyScenario("Text");
            ApplySource();
            ApplySelectionMode();
            PeopleTable.ClearSort();
            PeopleTable.DeselectAll();

            _lastSort = "(none)";
        }
        finally
        {
            _isApplyingState = false;
        }

        UpdateReadout();
        SetLastAction(announce
            ? "Fixture reset to baseline: Text columns, named table, grouping off, read-only, enabled, Single selection, nothing selected, no sort, row values restored."
            : "Fixture loaded at baseline.");
    }

    // ----- Events from the table -----

    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        UpdateReadout();
    }

    private void OnSorted(TableView sender, TableViewSortedEventArgs args)
    {
        _lastSort = args.Column is null
            ? "(cleared)"
            : $"{args.Column.Header} {args.Direction}";

        if (LastSortText is not null)
        {
            LastSortText.Text = _lastSort;
        }
    }

    private void OnBoundaryButtonClick(object sender, RoutedEventArgs e)
    {
        var which = ReferenceEquals(sender, BeforeTableButton) ? "Before table" : "After table";
        SetLastAction($"{which} activated. Tab and Shift+Tab from here to test the table's focus boundary.");
    }

    // ----- Readout -----

    private string _lastSort = "(none)";

    private void UpdateReadout()
    {
        if (ScenarioText is null || PeopleTable is null)
        {
            return;
        }

        var name = AutomationProperties.GetName(PeopleTable);

        ScenarioText.Text = SelectedContent(ScenarioSelector, "Text columns");
        TableNameText.Text = string.IsNullOrEmpty(name) ? "(none — unnamed table)" : name;
        GroupingText.Text = GroupByDepartmentToggle.IsOn ? "On (by Department)" : "Off";
        ReadOnlyText.Text = PeopleTable.IsReadOnly ? "True" : "False";
        EnabledText.Text = PeopleTable.IsEnabled ? "True" : "False";
        SelectionModeText.Text = PeopleTable.SelectionMode.ToString();
        SelectedIndexText.Text = PeopleTable.SelectedIndex.ToString(CultureInfo.InvariantCulture);
        RowCountText.Text = Rows.Count.ToString("N0", CultureInfo.InvariantCulture);
        ColumnCountText.Text = PeopleTable.Columns.Count.ToString(CultureInfo.InvariantCulture);
        LastSortText.Text = _lastSort;
    }

    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }
    }

    private static string SelectedTag(ComboBox? box, string fallback) =>
        box?.SelectedItem is ComboBoxItem item && item.Tag is string tag ? tag : fallback;

    private static string SelectedContent(ComboBox? box, string fallback) =>
        box?.SelectedItem is ComboBoxItem item && item.Content is string content ? content : fallback;

    /// <summary>
    /// Row type for the fixture. Implements <see cref="INotifyPropertyChanged"/>
    /// so the "Data mutation" control can rewrite values on a live row object and
    /// the tester can confirm a recycled row never announces stale content.
    /// </summary>
    public sealed class FixtureRow : INotifyPropertyChanged
    {
        private string _firstName = string.Empty;
        private string _lastName = string.Empty;
        private string _email = string.Empty;
        private string _department = string.Empty;
        private string _role = string.Empty;
        private bool _isActive;

        private string? _originalFirstName;
        private string? _originalDepartment;
        private string? _originalRole;
        private bool _originalIsActive;
        private bool _hasOriginal;

        public int Number { get; set; }

        public string FirstName
        {
            get => _firstName;
            set => Set(ref _firstName, value);
        }

        public string LastName
        {
            get => _lastName;
            set => Set(ref _lastName, value);
        }

        public string Email
        {
            get => _email;
            set => Set(ref _email, value);
        }

        public string Department
        {
            get => _department;
            set => Set(ref _department, value);
        }

        public string Role
        {
            get => _role;
            set => Set(ref _role, value);
        }

        public bool IsActive
        {
            get => _isActive;
            set => Set(ref _isActive, value);
        }

        public string FullName => $"{_firstName} {_lastName}";

        /// <summary>Single-character avatar initial used by the template cell.</summary>
        public string Initial => _firstName.Length > 0
            ? _firstName.Substring(0, 1)
            : (_lastName.Length > 0 ? _lastName.Substring(0, 1) : "?");

        /// <summary>
        /// Rewrites the bound values in place. The row object identity is kept, so
        /// the control updates the existing row rather than replacing it — which is
        /// exactly the path a stale-announcement bug would hide in.
        /// </summary>
        public void Mutate(int generation)
        {
            CaptureOriginal();

            FirstName = $"Mutated{generation}";
            Department = $"Dept-{generation}";
            Role = $"Role after mutation {generation}";
            IsActive = !IsActive;
        }

        /// <summary>Restores the pre-mutation values captured on first mutation.</summary>
        public void Restore()
        {
            if (!_hasOriginal)
            {
                return;
            }

            FirstName = _originalFirstName ?? string.Empty;
            Department = _originalDepartment ?? string.Empty;
            Role = _originalRole ?? string.Empty;
            IsActive = _originalIsActive;
            _hasOriginal = false;
        }

        private void CaptureOriginal()
        {
            if (_hasOriginal)
            {
                return;
            }

            _originalFirstName = _firstName;
            _originalDepartment = _department;
            _originalRole = _role;
            _originalIsActive = _isActive;
            _hasOriginal = true;
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        private void Set<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
        {
            if (Equals(field, value))
            {
                return;
            }

            field = value;
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
            if (propertyName is nameof(FirstName) or nameof(LastName))
            {
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(FullName)));
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Initial)));
            }
        }
    }
}
