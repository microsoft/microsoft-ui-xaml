// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Converters;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

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
public sealed partial class ToolTipsPage : SamplePageBase
{
    private const string LongBio =
        "Runs the quarterly planning review, mentors two new hires, and is the person everyone asks about the customer onboarding pipeline, from contract hand-off to the first support call.";

    public ToolTipsPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();

        // The control sorts the TableViewSource itself when a header is clicked; no app code is
        // needed for the Name, Department and Salary columns.
        AttachCellToolTips();
        ApplyHeaderToolTips("original");
        // </snippet>
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource Source { get; }

    // <snippet>
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
    // </snippet>

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
}
