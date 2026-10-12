// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#if TABLEVIEW_SAMPLE_VERIFY

using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp.Pages;

// Experiment, --verify-row-style-menu=templates: a ContextFlyout declared inside a column's
// CellTemplate (Department) and HeaderTemplate (Name, Salary). Counts flyout instances, checks the
// item after recycling, and records which menu wins once the column's CellContextFlyout /
// HeaderContextFlyout and the table's RowContextFlyout are set as well.
public sealed partial class ContextMenusPage
{
    private const string CellTemplateKind = "CELL TEMPLATE";
    private const string HeaderTemplateKind = "HEADER TEMPLATE";

    private const string CellTemplateXaml =
        "<DataTemplate " + XamlNamespaces + ">" +
        "  <Grid Background='Transparent' HorizontalAlignment='Stretch' VerticalAlignment='Stretch'>" +
        "    <Grid.ContextFlyout>" +
        "      <MenuFlyout><MenuFlyoutItem Text='{Binding FullName}'/></MenuFlyout>" +
        "    </Grid.ContextFlyout>" +
        "    <TextBlock Text='{Binding Department}' VerticalAlignment='Center' Margin='12,0'/>" +
        "  </Grid>" +
        "</DataTemplate>";

    private const string HeaderTemplateXaml =
        "<DataTemplate " + XamlNamespaces + ">" +
        "  <Grid Background='Transparent' HorizontalAlignment='Stretch' VerticalAlignment='Stretch'>" +
        "    <Grid.ContextFlyout>" +
        "      <MenuFlyout><MenuFlyoutItem Text='{Binding}'/></MenuFlyout>" +
        "    </Grid.ContextFlyout>" +
        "    <TextBlock Text='{Binding}' VerticalAlignment='Center'/>" +
        "  </Grid>" +
        "</DataTemplate>";

    private readonly Dictionary<string, List<FlyoutBase>> _templateFlyouts = new()
    {
        [CellTemplateKind] = new(),
        [HeaderTemplateKind] = new(),
    };

    private void ConfigureTemplateMenus()
    {
        DepartmentColumn.CellTemplate = (DataTemplate)XamlReader.Load(CellTemplateXaml);
        NameColumn.HeaderTemplate = (DataTemplate)XamlReader.Load(HeaderTemplateXaml);
        SalaryColumn.HeaderTemplate = (DataTemplate)XamlReader.Load(HeaderTemplateXaml);
    }

    private async Task RunTemplateExperimentAsync()
    {
        // Phase 1: only the template menus (and the table's fallback) in play.
        var departmentApiMenu = MakeLabelledMenu("Department CellContextFlyout");
        PeopleTable.RowContextFlyout = null;
        NameColumn.CellContextFlyout = null;
        foreach (var column in PeopleTable.Columns)
        {
            column.HeaderContextFlyout = null;
        }

        Label(PeopleTable.ContextFlyout, "TABLE");
        Label(_rowMenu!, "ROW (RowContextFlyout)");
        Label(departmentApiMenu, "DEPT CELL (CellContextFlyout)");
        Label(_headerMenus[NameColumn], "NAME HEADER (HeaderContextFlyout)");
        Label(_headerMenus[SalaryColumn], "SALARY HEADER (HeaderContextFlyout)");
        await Task.Delay(300);

        LogTemplateInstances("initial", 0, 7);

        await ExpectExperimentAsync("T1 mouse Department cell row 0", () => RightClick(Cell(0, DepartmentColumn)), People[0], CellTemplateKind);
        await ExpectExperimentAsync("T2 mouse Department cell row 1", () => RightClick(Cell(1, DepartmentColumn)), People[1], CellTemplateKind);
        await ExpectExperimentAsync("T3 mouse Department cell row 0, near right edge", () => RightClick(Cell(0, DepartmentColumn), 0.95), People[0], CellTemplateKind);
        LeftClick(Cell(2, DepartmentColumn));
        await Task.Delay(300);
        await ExpectExperimentAsync("T4 Shift+F10, current cell Department row 2", () => SendKeys(VkShift, VkF10), People[2], CellTemplateKind);
        await ExpectExperimentAsync("T5 mouse Name header", () => RightClick(Header(NameColumn)), null, HeaderTemplateKind);
        await ExpectExperimentAsync("T6 mouse Salary header", () => RightClick(Header(SalaryColumn)), null, HeaderTemplateKind);
        await ExpectExperimentAsync("T7 mouse Name header, near left edge", () => RightClick(Header(NameColumn), 0.03), null, HeaderTemplateKind);

        // Phase 2: the properties for the same targets set as well.
        DepartmentColumn.CellContextFlyout = departmentApiMenu;
        PeopleTable.RowContextFlyout = _rowMenu;
        NameColumn.HeaderContextFlyout = _headerMenus[NameColumn];
        SalaryColumn.HeaderContextFlyout = _headerMenus[SalaryColumn];
        await Task.Delay(300);

        await ExpectExperimentAsync("P1 + CellContextFlyout: mouse Department cell row 1", () => RightClick(Cell(1, DepartmentColumn)), People[1], CellTemplateKind);
        LeftClick(Cell(3, DepartmentColumn));
        await Task.Delay(300);
        await ExpectExperimentAsync("P2 + CellContextFlyout: Shift+F10, current cell Department row 3", () => SendKeys(VkShift, VkF10), People[3], CellTemplateKind);
        await ExpectExperimentAsync("P3 + HeaderContextFlyout: mouse Name header", () => RightClick(Header(NameColumn)), null, HeaderTemplateKind);
        await ExpectExperimentAsync("P4 + HeaderContextFlyout: mouse Salary header", () => RightClick(Header(SalaryColumn)), null, HeaderTemplateKind);

        // Phase 3: recycling, template menus only again.
        DepartmentColumn.CellContextFlyout = null;
        PeopleTable.RowContextFlyout = null;
        await Task.Delay(200);

        var scroller = Descendants(PeopleTable).OfType<ScrollViewer>().First(s => s.Name == "PART_BodyScroller");
        double rowHeight = Row(0).ActualHeight;
        const int target = 24;
        scroller.ChangeView(null, rowHeight * (target - 1), null, true);
        await Task.Delay(700);
        LogTemplateInstances("after scrolling down", target - 2, target + 6);
        await ExpectExperimentAsync($"S1 after scroll: mouse Department cell row {target}", () => RightClick(Cell(target, DepartmentColumn)), People[target], CellTemplateKind);
        await ExpectExperimentAsync($"S2 after scroll: mouse Department cell row {target + 2}", () => RightClick(Cell(target + 2, DepartmentColumn)), People[target + 2], CellTemplateKind);

        scroller.ChangeView(null, 0, null, true);
        await Task.Delay(700);
        LogTemplateInstances("after scrolling back", 0, 7);
        await ExpectExperimentAsync("S3 back at top: mouse Department cell row 0", () => RightClick(Cell(0, DepartmentColumn)), People[0], CellTemplateKind);
        await ExpectExperimentAsync("S4 back at top: mouse Department cell row 3", () => RightClick(Cell(3, DepartmentColumn)), People[3], CellTemplateKind);
    }

    private static MenuFlyout MakeLabelledMenu(string text)
    {
        var menu = new MenuFlyout();
        menu.Items.Add(new MenuFlyoutItem { Text = text });
        return menu;
    }

    private void Label(FlyoutBase flyout, string label)
    {
        _menuLabels[flyout] = label;
        flyout.Opened += OnExperimentFlyoutOpened;
    }

    private void LogTemplateInstances(string when, int first, int last)
    {
        var cells = new List<string>();
        var repeater = Descendants(PeopleTable).OfType<ItemsRepeater>().First(r => r.Name == "PART_RowsRepeater");
        for (int i = first; i <= last; ++i)
        {
            if (repeater.TryGetElement(i) is TableViewRow)
            {
                cells.Add($"{i}:{TemplateMenuId(Cell(i, DepartmentColumn), CellTemplateKind)}");
            }
        }

        var headers = new TableViewColumn[] { NameColumn, SalaryColumn, DepartmentColumn }
            .Select(column => $"{column.Header}:{TemplateMenuId(Header(column), HeaderTemplateKind)}");

        _verificationLines.Add(
            $"INFO {when}: Department cells {string.Join(" ", cells)}; " +
            $"cell template instances total {_templateFlyouts[CellTemplateKind].Count}");
        _verificationLines.Add(
            $"INFO {when}: headers {string.Join(" ", headers)}; " +
            $"header template instances total {_templateFlyouts[HeaderTemplateKind].Count}");
    }

    private string TemplateMenuId(FrameworkElement host, string kind)
    {
        var flyout = Descendants(host).OfType<UIElement>().Select(element => element.ContextFlyout).FirstOrDefault(f => f is not null);
        if (flyout is null)
        {
            return "none";
        }

        var known = _templateFlyouts[kind];
        int index = known.FindIndex(existing => ReferenceEquals(existing, flyout));
        if (index < 0)
        {
            known.Add(flyout);
            index = known.Count - 1;
            Label(flyout, $"{kind} #{known.Count}");
        }

        return $"#{index + 1}";
    }
}

#endif
