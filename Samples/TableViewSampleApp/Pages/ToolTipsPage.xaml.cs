// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

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
        panel.Children.Add(new TextBlock { Text = string.Format(CultureInfo.CurrentCulture, "{0}, {1}", person.Role, person.Department), Opacity = 0.75 });
        panel.Children.Add(new TextBlock { Text = person.Email, Opacity = 0.6, FontSize = 12 });

        return panel;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

/// <summary>
/// Demonstrates the two opt-in tooltip surfaces, both per column:
/// <list type="bullet">
///   <item><description><c>TableViewColumn.HeaderToolTip</c>: a plain object. A header is not
///     bound against a row, so there is nothing to defer and no binding involved. A string is
///     reported as help text alongside the sort state; non-string content is mouse-only.</description></item>
///   <item><description><c>TableViewColumn.CellToolTipBinding</c>: a Binding evaluated against
///     each row's data item. A CLR property rather than a DP, so XAML passes the Binding through
///     unevaluated instead of resolving it once.</description></item>
/// </list>
/// The control never invents a tooltip, and never touches one a cell's own template already set.
/// </summary>
public sealed partial class ToolTipsPage : Page
{
    private const string LongBio =
        "Runs the quarterly planning review, mentors two new hires, and is the person everyone asks about the customer onboarding pipeline, from contract hand-off to the first support call.";

    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";

    public ToolTipsPage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();

        // The control sorts the TableViewSource itself when a header is clicked; no app code is
        // needed for the Name, Department and Salary columns.
        AttachCellToolTips();
        ApplyHeaderToolTips("original");
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource? Source => _source;

    // ---- Cell tooltips ----------------------------------------------------------------------

    private void AttachCellToolTips()
    {
        // A tooltip on an image cell: the person's name.
        PhotoColumn.CellToolTipBinding = new Binding { Path = new PropertyPath(nameof(Person.FullName)) };

        // No Path: the binding produces the row item, and the converter turns it into content.
        NameColumn.CellToolTipBinding = new Binding { Converter = new RowCardConverter() };

        // Same text as the cell itself: the usual case for a column that truncates.
        BioColumn.CellToolTipBinding = new Binding { Path = new PropertyPath(nameof(Person.Bio)) };

        // A different property, and an observable one: Person raises PropertyChanged for Role, so
        // the tooltip follows the data with no invalidation call from the app.
        DepartmentColumn.CellToolTipBinding = new Binding { Path = new PropertyPath(nameof(Person.Role)) };

        // Salary deliberately gets nothing, so it is easy to confirm no tooltip appears.
    }

    private void DetachCellToolTips()
    {
        // Clearing the binding retracts the tooltip; there is no separate remove call.
        PhotoColumn.CellToolTipBinding = null;
        NameColumn.CellToolTipBinding = null;
        BioColumn.CellToolTipBinding = null;
        DepartmentColumn.CellToolTipBinding = null;
    }

    private void OnCellToolTipsToggled(object sender, RoutedEventArgs e)
    {
        // Fires during InitializeComponent (IsOn="True"); the constructor attaches the bindings.
        if (!IsLoaded)
        {
            return;
        }

        if (CellToolTipsToggle.IsOn)
        {
            AttachCellToolTips();
            SetLastAction("Cell tooltips -> attached (CellToolTipBinding set on 4 columns)");
        }
        else
        {
            DetachCellToolTips();
            SetLastAction("Cell tooltips -> detached (CellToolTipBinding = null)");
        }
    }

    // ---- Header tooltips --------------------------------------------------------------------

    private void OnHeaderToolTipsChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        ApplyHeaderToolTips(SampleShaping.SelectedTag(HeaderToolTipsSelector, "original"));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Header tooltips -> {0}", SampleShaping.Label(HeaderToolTipsSelector)));
    }

    private void ApplyHeaderToolTips(string mode)
    {
        switch (mode)
        {
            case "changed":
                // Changed in place on one column, retracted on another: both update paths.
                NameColumn.HeaderToolTip = "Full name: given name, then family name";
                DepartmentColumn.HeaderToolTip = null;
                break;
            case "none":
                foreach (var column in PeopleTable.Columns)
                {
                    column.HeaderToolTip = null;
                }

                break;
            default:
                NameColumn.HeaderToolTip = "Full name, as entered";
                DepartmentColumn.HeaderToolTip = "Team the person reports into";

                // HeaderToolTip takes any object. Non-string content is mouse-only: there is no
                // sensible way to announce a visual tree, so it is not reported as help text.
                var card = new StackPanel { Spacing = 4 };
                card.Children.Add(new TextBlock { Text = "Bio", FontWeight = FontWeights.SemiBold });
                card.Children.Add(new TextBlock { Text = "A short free-form summary", Opacity = 0.75 });
                BioColumn.HeaderToolTip = card;

                // Repeats the header's own text, so the peer drops it rather than announcing it twice.
                SalaryColumn.HeaderToolTip = "Salary";
                break;
        }
    }

    // ---- Actions ------------------------------------------------------------------------

    private void OnNextRoleClick(object sender, RoutedEventArgs e)
    {
        var changed = People.Take(10).ToList();
        foreach (var person in changed)
        {
            person.Role = SampleShaping.Next(PersonData.Roles, person.Role);
        }

        ReapplyIfGroupedOn(nameof(Person.Role));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Gave {0:N0} people their next role; {1} is now {2}",
            changed.Count, changed[0].FullName, changed[0].Role));
    }

    private void OnLongBioClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        person.Bio = LongBio;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Gave {0} a long bio; hover the Bio cell to read all of it", person.FullName));
    }

    private void OnChangeDepartmentClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = person.Department;
        person.Department = SampleShaping.Next(PersonData.Departments, from);

        // GroupBy takes a delegate, not a property path, so the control cannot re-bucket the row
        // on PropertyChanged; re-apply the grouping when the grouped-on value changed.
        ReapplyIfGroupedOn(nameof(Person.Department));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, person.Department));
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || CellToolTipsText is null || PerColumnText is null || PeopleTable is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        CellToolTipsText.Text = NameColumn.CellToolTipBinding is null ? "Detached" : "Attached";

        // Read back from the columns, so the readout reports what the control actually holds.
        var parts = new List<string>();
        foreach (var column in PeopleTable.Columns)
        {
            var what = new List<string>();
            if (column.HeaderToolTip is not null)
            {
                what.Add(column.HeaderToolTip is string ? "header" : "header (card)");
            }

            if (column.CellToolTipBinding is not null)
            {
                what.Add("cell");
            }

            if (column == EmailColumn)
            {
                what.Add("template-owned");
            }

            parts.Add(string.Format(CultureInfo.CurrentCulture, "{0}: {1}", column.Header, what.Count == 0 ? "none" : string.Join(", ", what)));
        }

        PerColumnText.Text = string.Join("; ", parts);
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard every element this path touches.
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
                // The columns, and so every tooltip, are untouched by a reshape.
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

    // Call after ANY write to the grouped-on property.
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