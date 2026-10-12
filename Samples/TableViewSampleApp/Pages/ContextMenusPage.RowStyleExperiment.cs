// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#if TABLEVIEW_SAMPLE_VERIFY

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

// Experiment: can an app give rows a context menu without RowContextFlyout, through an implicit
// TableViewRow style? Run with --page=ContextMenus and one of
//   --verify-row-style-menu=setter    Style Setter Property="ContextFlyout" with a MenuFlyout value
//   --verify-row-style-menu=template  Style Setter Property="Template" whose root has a ContextFlyout
// Logs how many flyout instances the realized rows hold, and for each request which instance
// opened and which row item its items inherited. Writes verification-row-style-menu-<mode>.txt.
public sealed partial class ContextMenusPage
{
    private const string RowStyleArgumentPrefix = "--verify-row-style-menu=";

    private const string XamlNamespaces =
        "xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
        "xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' " +
        "xmlns:tv='using:Microsoft.UI.Xaml.Controls.Tabular'";

    private const string SetterStyleXaml =
        "<Style " + XamlNamespaces + " TargetType='tv:TableViewRow'>" +
        "  <Setter Property='ContextFlyout'>" +
        "    <Setter.Value>" +
        "      <MenuFlyout><MenuFlyoutItem Text='{Binding FullName}'/></MenuFlyout>" +
        "    </Setter.Value>" +
        "  </Setter>" +
        "</Style>";

    // The default row template's parts without its visual states.
    private const string TemplateStyleXaml =
        "<Style " + XamlNamespaces + " TargetType='tv:TableViewRow'>" +
        "  <Setter Property='Template'>" +
        "    <Setter.Value>" +
        "      <ControlTemplate TargetType='tv:TableViewRow'>" +
        "        <Grid x:Name='ExperimentRoot'>" +
        "          <Grid.ContextFlyout>" +
        "            <MenuFlyout><MenuFlyoutItem Text='{Binding FullName}'/></MenuFlyout>" +
        "          </Grid.ContextFlyout>" +
        "          <Border x:Name='PART_RootBorder' Background='{TemplateBinding Background}'" +
        "                  BorderThickness='{TemplateBinding BorderThickness}' Padding='{TemplateBinding Padding}'>" +
        "            <Grid>" +
        "              <ContentPresenter x:Name='PART_CellForegroundPresenter' Foreground='{TemplateBinding Foreground}'" +
        "                                HorizontalContentAlignment='Stretch' VerticalContentAlignment='Stretch'>" +
        "                <Grid>" +
        "                  <tv:TableViewCellsPanel x:Name='PART_CellsHost' HorizontalAlignment='Left'/>" +
        "                  <Border x:Name='PART_RowExpanderGutter' Width='16' Background='Transparent'" +
        "                          HorizontalAlignment='Left' Visibility='Collapsed'>" +
        "                    <FontIcon x:Name='PART_RowExpanderIcon' FontSize='12' Glyph='&#xE76C;'/>" +
        "                  </Border>" +
        "                </Grid>" +
        "              </ContentPresenter>" +
        "              <Rectangle x:Name='PART_SelectionIndicator' Width='3' Height='16' Fill='Blue'" +
        "                         HorizontalAlignment='Left' IsHitTestVisible='False' Opacity='0'/>" +
        "            </Grid>" +
        "          </Border>" +
        "          <Border x:Name='PART_GridLineBorder' BorderBrush='{TemplateBinding BorderBrush}'" +
        "                  BorderThickness='{TemplateBinding BorderThickness}' IsHitTestVisible='False'/>" +
        "        </Grid>" +
        "      </ControlTemplate>" +
        "    </Setter.Value>" +
        "  </Setter>" +
        "</Style>";

    private string? _rowStyleMode;
    private readonly List<FlyoutBase> _rowStyleFlyouts = new();

    private bool AttachRowStyleExperiment()
    {
        var argument = Environment.GetCommandLineArgs()
            .FirstOrDefault(a => a.StartsWith(RowStyleArgumentPrefix, StringComparison.OrdinalIgnoreCase));
        if (argument is null)
        {
            return false;
        }

        _rowStyleMode = argument[RowStyleArgumentPrefix.Length..].ToLowerInvariant();
        _verificationTitle = $"ROW STYLE MENU EXPERIMENT mode={_rowStyleMode}";
        _logPath = Path.Combine(AppContext.BaseDirectory, $"verification-row-style-menu-{_rowStyleMode}.txt");

        // Added before the rows realize, so every row picks up the implicit style.
        if (_rowStyleMode == "templates")
        {
            ConfigureTemplateMenus();
            return true;
        }

        var xaml = _rowStyleMode == "template" ? TemplateStyleXaml : SetterStyleXaml;
        PeopleTable.Resources[typeof(TableViewRow)] = (Style)XamlReader.Load(xaml);
        return true;
    }

    private async Task RunRowStyleExperimentAsync()
    {
        if (_rowStyleMode == "templates")
        {
            await RunTemplateExperimentAsync();
            return;
        }

        // Leave only the experiment's menu and the table's own fallback in play.
        PeopleTable.RowContextFlyout = null;
        NameColumn.CellContextFlyout = null;
        _menuLabels[PeopleTable.ContextFlyout] = "TABLE";
        PeopleTable.ContextFlyout.Opened += OnExperimentFlyoutOpened;
        await Task.Delay(300);

        const int rowCount = 8;
        var realizedRows = new List<string>();
        for (int i = 0; i < rowCount; ++i)
        {
            var row = Row(i);
            var root = VisualTreeHelper.GetChildrenCount(row) > 0 ? VisualTreeHelper.GetChild(row, 0) as FrameworkElement : null;
            var flyout = row.ContextFlyout ?? root?.ContextFlyout;
            string id = flyout is null ? "none" : $"#{RegisterExperimentFlyout(flyout)}";
            realizedRows.Add($"row {i}: root={root?.Name ?? "?"} row.ContextFlyout={(row.ContextFlyout is null ? "null" : "set")} menu={id}");
        }

        _verificationLines.Add($"INFO {_rowStyleFlyouts.Count} distinct flyout instance(s) across {rowCount} realized rows");
        _verificationLines.AddRange(realizedRows.Select(line => $"INFO {line}"));

        var rows = Enumerable.Range(0, 4).Select(i => People[i]).ToArray();
        await ExpectExperimentAsync("R1 mouse Office cell row 0", () => RightClick(Cell(0, OfficeColumn)), rows[0]);
        await ExpectExperimentAsync("R2 mouse Office cell row 1", () => RightClick(Cell(1, OfficeColumn)), rows[1]);
        await ExpectExperimentAsync("R3 mouse Name cell row 2", () => RightClick(Cell(2, NameColumn)), rows[2]);
        await ExpectExperimentAsync("R4 mouse Office cell row 3", () => RightClick(Cell(3, OfficeColumn)), rows[3]);
        await ExpectExperimentAsync("R5 mouse Office cell row 0 again", () => RightClick(Cell(0, OfficeColumn)), rows[0]);
        LeftClick(Cell(1, OfficeColumn));
        await Task.Delay(300);
        await ExpectExperimentAsync("R6 Shift+F10, current cell Office row 1", () => SendKeys(VkShift, VkF10), rows[1]);

        // Precedence: the column's CellContextFlyout and the table's RowContextFlyout back on.
        _menuLabels[_nameCellMenu!] = "NAME CELL";
        _nameCellMenu!.Opened += OnExperimentFlyoutOpened;
        _menuLabels[_rowMenu!] = "ROW (RowContextFlyout)";
        _rowMenu!.Opened += OnExperimentFlyoutOpened;

        NameColumn.CellContextFlyout = _nameCellMenu;
        await Task.Delay(200);
        await ExpectExperimentAsync("P1 cell menu on: mouse Name cell row 0", () => RightClick(Cell(0, NameColumn)), rows[0]);
        LeftClick(Cell(2, NameColumn));
        await Task.Delay(300);
        await ExpectExperimentAsync("P2 cell menu on: Shift+F10, current cell Name row 2", () => SendKeys(VkShift, VkF10), rows[2]);
        await ExpectExperimentAsync("P3 cell menu on: mouse Office cell row 3", () => RightClick(Cell(3, OfficeColumn)), rows[3]);

        PeopleTable.RowContextFlyout = _rowMenu;
        await Task.Delay(200);
        await ExpectExperimentAsync("P4 cell+row menus on: mouse Name cell row 1", () => RightClick(Cell(1, NameColumn)), rows[1]);
        await ExpectExperimentAsync("P5 cell+row menus on: mouse Office cell row 1", () => RightClick(Cell(1, OfficeColumn)), rows[1]);
        LeftClick(Cell(3, OfficeColumn));
        await Task.Delay(300);
        await ExpectExperimentAsync("P6 cell+row menus on: Shift+F10, current cell Office row 3", () => SendKeys(VkShift, VkF10), rows[3]);

        // Recycling: scroll far enough that containers are reused for other items, then back.
        PeopleTable.RowContextFlyout = null;
        NameColumn.CellContextFlyout = null;
        await RunRecyclingCasesAsync();
    }

    private async Task RunRecyclingCasesAsync()
    {
        var scroller = Descendants(PeopleTable).OfType<ScrollViewer>().First(s => s.Name == "PART_BodyScroller");
        double rowHeight = Row(0).ActualHeight;
        const int target = 24;

        scroller.ChangeView(null, rowHeight * (target - 1), null, true);
        await Task.Delay(700);
        LogRealizedRows("after scrolling down", target - 2, target + 6);
        await ExpectExperimentAsync($"S1 after scroll: mouse Office cell row {target}", () => RightClick(Cell(target, OfficeColumn)), People[target]);
        await ExpectExperimentAsync($"S2 after scroll: mouse Name cell row {target + 2}", () => RightClick(Cell(target + 2, NameColumn)), People[target + 2]);
        LeftClick(Cell(target + 1, OfficeColumn));
        await Task.Delay(300);
        await ExpectExperimentAsync($"S3 after scroll: Shift+F10, current cell Office row {target + 1}", () => SendKeys(VkShift, VkF10), People[target + 1]);

        scroller.ChangeView(null, 0, null, true);
        await Task.Delay(700);
        LogRealizedRows("after scrolling back", 0, 6);
        await ExpectExperimentAsync("S4 back at top: mouse Office cell row 0", () => RightClick(Cell(0, OfficeColumn)), People[0]);
        await ExpectExperimentAsync("S5 back at top: mouse Office cell row 2", () => RightClick(Cell(2, OfficeColumn)), People[2]);
    }

    private void LogRealizedRows(string when, int first, int last)
    {
        int before = _rowStyleFlyouts.Count;
        var ids = new List<string>();
        for (int i = first; i <= last; ++i)
        {
            if (TryRowMenu(i, out var flyout))
            {
                ids.Add($"{i}:{(flyout is null ? "none" : $"#{RegisterExperimentFlyout(flyout)}")}");
            }
        }

        _verificationLines.Add(
            $"INFO {when}: rows {string.Join(" ", ids)}; new instances {_rowStyleFlyouts.Count - before}, total {_rowStyleFlyouts.Count}");
    }

    private bool TryRowMenu(int index, out FlyoutBase? flyout)
    {
        flyout = null;
        var repeater = Descendants(PeopleTable).OfType<ItemsRepeater>().First(r => r.Name == "PART_RowsRepeater");
        if (repeater.TryGetElement(index) is not TableViewRow row)
        {
            return false;
        }

        var root = VisualTreeHelper.GetChildrenCount(row) > 0 ? VisualTreeHelper.GetChild(row, 0) as FrameworkElement : null;
        flyout = row.ContextFlyout ?? root?.ContextFlyout;
        return true;
    }

    private int RegisterExperimentFlyout(FlyoutBase flyout)
    {
        int index = _rowStyleFlyouts.FindIndex(known => ReferenceEquals(known, flyout));
        if (index >= 0)
        {
            return index + 1;
        }

        _rowStyleFlyouts.Add(flyout);
        _menuLabels[flyout] = $"ROW STYLE #{_rowStyleFlyouts.Count}";
        flyout.Opened += OnExperimentFlyoutOpened;
        return _rowStyleFlyouts.Count;
    }

    private void OnExperimentFlyoutOpened(object? sender, object e)
    {
        var flyout = (FlyoutBase)sender!;
        _openedLabel = _menuLabels[flyout];
        _openedItem = (flyout as MenuFlyout)?.Items.FirstOrDefault()?.DataContext;
    }

    private const string RowStyleLabel = "ROW STYLE";

    // expectedLabel null: no expected winner, the outcome is only recorded (OBSERVE).
    private async Task ExpectExperimentAsync(string name, Action request, Person? expectedItem, string? expectedLabel = RowStyleLabel)
    {
        _openedLabel = null;
        _openedItem = null;
        string observed;
        try
        {
            request();
            for (int waited = 0; _openedLabel is null && waited < OpenTimeoutMs; waited += 50)
            {
                await Task.Delay(50);
            }

            observed = _openedLabel ?? "(no menu)";
        }
        catch (Exception ex)
        {
            observed = $"threw {ex.GetType().Name}: {ex.Message}";
        }

        var item = _openedItem as Person;
        bool itemMatches = expectedItem is null || ReferenceEquals(item, expectedItem);
        bool pass = itemMatches && (expectedLabel is null || observed.StartsWith(expectedLabel, StringComparison.Ordinal));
        string verdict = expectedLabel is null && itemMatches ? "OBSERVE" : pass ? "PASS" : "FAIL";
        _verificationLines.Add(
            $"{verdict} {name}: expected {expectedLabel ?? "any menu"}" +
            (expectedItem is null ? "" : $" [{expectedItem.FullName}]") +
            $", got {observed} [{item?.FullName ?? _openedItem?.ToString() ?? "(none)"}]");
        if (pass)
        {
            ++_passed;
        }
        else
        {
            ++_failed;
        }

        File.WriteAllLines(_logPath, _verificationLines);
        await CloseOpenMenusAsync();
    }
}

#endif
