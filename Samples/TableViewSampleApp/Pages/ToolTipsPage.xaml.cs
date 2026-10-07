// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
// Tabular aliases keep the sample code concise.
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Turns a row item into rich, non-string tooltip content. A converter is how a computed tooltip is
/// authored when the content comes from a binding rather than a callback: the binding has no Path,
/// so the whole row item arrives here.
/// </summary>
public sealed partial class RowCardConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (value is not Person person)
        {
            return null!;
        }

        // A fresh element per evaluation. The returned UIElement is parented by the cell's ToolTip,
        // and one element cannot have two parents, so this must not be cached.
        var panel = new StackPanel { Spacing = 4 };
        panel.Children.Add(new TextBlock { Text = person.FullName, FontWeight = FontWeights.SemiBold });
        panel.Children.Add(new TextBlock { Text = $"{person.Role} — {person.Department}", Opacity = 0.75 });
        panel.Children.Add(new TextBlock { Text = person.Email, Opacity = 0.6, FontSize = 12 });

        return panel;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

/// <summary>
/// Demonstrates the two opt-in tooltip surfaces, both per column:
/// <list type="bullet">
///   <item><description><c>TableViewColumn.HeaderToolTip</c> — a plain object. A header is not
///     bound against a row, so there is nothing to defer and no binding involved. A string is
///     reported as help text alongside the sort state; non-string content is mouse-only.</description></item>
///   <item><description><c>TableViewColumn.CellToolTipBinding</c> — a Binding evaluated against
///     each row's data item. A CLR property rather than a DP, so XAML passes the Binding through
///     unevaluated instead of resolving it once.</description></item>
/// </list>
/// The control never invents a tooltip, and never touches one a cell's own template already set.
/// </summary>
public sealed partial class ToolTipsPage : Page
{
    private readonly ObservableCollection<Person> _people = PersonData.Take(80);

    private TableViewSource? _source;

    // Requested shaping mode versus the mode actually applied to the source.
    // GroupBy fails fast, so these can differ; every readout and every
    // enable/disable guard reads _appliedMode.
    private string _mode = "flat";
    private string _appliedMode = "flat";
    private string _groupKey = "Department";
    private bool _allGroupsCollapsed;

    private bool _attached;
    private int _revision;

    public ToolTipsPage()
    {
        InitializeComponent();

        _source = TableViewSource.From(_people);
        PeopleTable.ItemsSource = _source;

        // The Name, Department, and Salary columns are sortable (they do not opt
        // out with CanSort="False"), so the header gesture has to do something:
        // this is the gallery's shared reflection-based sort over the same items.
        SampleShape.EnableDefaults(PeopleTable, _people);

        AttachCellToolTips();
        AttachRichHeaderToolTip();
        ApplyShaping();
    }

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
        // so the source is never rebuilt per change. The columns — and therefore
        // every HeaderToolTip and CellToolTipBinding — are untouched by this.
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
            PeopleTable.ExpandAllGroups();
        }

        UpdateShapingGates();
        UpdateReadout();
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
        if (_appliedMode != "grouped") return;

        PeopleTable.ExpandAllGroups();
        _allGroupsCollapsed = false;
        UpdateReadout();
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped") return;

        PeopleTable.CollapseAllGroups();
        _allGroupsCollapsed = true;
        UpdateReadout();
    }

    private void OnReassignRowClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped" || _people.Count == 0) return;

        var person = _people[0];

        if (_groupKey == "Active")
        {
            person.IsActive = !person.IsActive;
        }
        else if (_groupKey == "Role")
        {
            person.Role = $"{person.Role.Split(" [")[0]} [regrouped]";
        }
        else
        {
            person.Department = PersonData.Departments
                .FirstOrDefault(d => !string.Equals(d, person.Department, StringComparison.Ordinal))
                ?? person.Department;
        }

        // The row moves to another group; hover it to confirm its tooltips moved with it.
        UpdateReadout();
    }

    // GroupValue never returns the empty string: an empty group identity is a
    // fail-fast in GroupBy, so a blank property has to be coalesced.
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

    private void AttachCellToolTips()
    {
        // Same text as the cell itself — the usual case for a column that truncates.
        BioColumn.CellToolTipBinding = new Binding { Path = new PropertyPath(nameof(Person.Bio)) };

        // A different property, and an observable one: Person raises PropertyChanged for Notes, so
        // the tooltip follows the data with no invalidation call from the app.
        DepartmentColumn.CellToolTipBinding = new Binding { Path = new PropertyPath(nameof(Person.Role)) };

        // No Path: the binding produces the row item, and the converter turns it into content.
        NameColumn.CellToolTipBinding = new Binding { Converter = new RowCardConverter() };

        // Salary deliberately gets nothing, so it is easy to confirm no tooltip appears.
        _attached = true;
        ToggleButton.Content = "Detach cell tooltips";
    }

    private void AttachRichHeaderToolTip()
    {
        // HeaderToolTip takes any object. Non-string content is mouse-only: there is no sensible
        // way to announce a visual tree, so it is not reported as help text.
        var card = new StackPanel { Spacing = 4 };
        card.Children.Add(new TextBlock { Text = "Bio", FontWeight = FontWeights.SemiBold });
        card.Children.Add(new TextBlock { Text = "Free-form summary", Opacity = 0.75 });

        BioColumn.HeaderToolTip = card;

        // Repeats the header's own text, so the peer drops it rather than announcing it twice.
        SalaryColumn.HeaderToolTip = "Salary";
    }

    private void OnToggleClick(object sender, RoutedEventArgs e)
    {
        if (_attached)
        {
            // Clearing the binding retracts the tooltip; there is no separate remove call.
            BioColumn.CellToolTipBinding = null;
            DepartmentColumn.CellToolTipBinding = null;
            NameColumn.CellToolTipBinding = null;

            _attached = false;
            ToggleButton.Content = "Attach cell tooltips";
        }
        else
        {
            AttachCellToolTips();
        }

        UpdateReadout();
    }

    private void OnMutateClick(object sender, RoutedEventArgs e)
    {
        _revision++;

        foreach (var person in _people.Take(40))
        {
            person.Role = $"{person.Role.Split(" [")[0]} [rev {_revision}]";
        }

        UpdateReadout();
    }

    private void OnMutateHeadersClick(object sender, RoutedEventArgs e)
    {
        _revision++;

        NameColumn.HeaderToolTip = $"Changed in place (rev {_revision})";
        DepartmentColumn.HeaderToolTip = null;

        UpdateReadout();
    }

    private void OnInspectClick(object sender, RoutedEventArgs e)
    {
        var sb = new StringBuilder();

        foreach (var column in PeopleTable.Columns)
        {
            var header = column.Header?.ToString();
            var headerTip = column.HeaderToolTip switch
            {
                null => "-",
                string s => $"\"{s}\"",
                var o => o.GetType().Name,
            };

            var cellTip = column.CellToolTipBinding is null ? "-" : "binding";
            sb.Append($"{(string.IsNullOrEmpty(header) ? "?" : header)}: header {headerTip}, cell {cellTip}   ");
        }

        InspectText.Text = sb.ToString().TrimEnd();
    }

    private void UpdateReadout()
    {
        if (AttachedText is null)
        {
            return;
        }

        AttachedText.Text = _attached ? "attached" : "detached";
        RevisionText.Text = _revision.ToString();

        // Readouts describe the APPLIED mode, never the requested one.
        if (ShapingText is not null)
        {
            var key = _groupKey;
            ShapingText.Text = _appliedMode == "grouped"
                ? $"Grouped by {key} · {_people.Select(p => GroupValue(p, key)).Distinct(StringComparer.Ordinal).Count()} groups · {(_allGroupsCollapsed ? "all collapsed" : "all expanded")}"
                : "Flat (no grouping)";
        }
    }
}
