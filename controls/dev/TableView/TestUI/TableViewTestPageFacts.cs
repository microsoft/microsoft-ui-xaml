// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared
{
    // Single source of truth for what the TableView test page (TableViewPage.xaml / .xaml.cs) authors.
    //
    // Compiled into BOTH sides of the process boundary: the page itself (MUXControlsTestApp, via
    // TableView_TestUI.projitems) builds its data from these values, and the interaction tests (MUXControls.Test, via
    // TableView_InteractionTests.projitems) derive their expectations from the same values. An expectation such as
    // "position 2 holds Person 9 after an Age-descending sort" is therefore computed, never re-typed.
    //
    // Plain C# only - no XAML or MITA types - so it compiles in both assemblies.
    //
    // Column widths are authored in XAML (WinUI has no x:Static to bind them), so BasicColumnWidths must be kept in
    // step with TableViewPage.xaml by hand. Every row-relative click offset derives from it.
    internal static class TableViewTestPageFacts
    {
        // ---- Page and navigation ----

        internal const string PageName = "TableView Tests";
        internal const string AxePageName = "TableView-Axe";

        // ---- Tables (AutomationId) ----

        internal const string BasicTable = "BasicTableView";
        internal const string RtlTable = "RtlTableView";
        internal const string GroupedTable = "GroupedTableView";
        internal const string ScrollingTable = "ScrollingTableView";

        // ---- Page controls (AutomationId) ----

        internal const string DummyButton = "DummyButton";
        internal const string AfterTableButton = "AfterTableButton";
        internal const string AddColumnButton = "AddColumnButton";
        internal const string RemoveColumnButton = "RemoveColumnButton";
        internal const string FilterSourceButton = "FilterSourceButton";
        internal const string ExpandAllGroupsButton = "ExpandAllGroupsButton";
        internal const string CollapseAllGroupsButton = "CollapseAllGroupsButton";
        internal const string DelayedUnloadButton = "DelayedUnloadButton";
        internal const string SelectionModeComboBox = "SelectionModeComboBox";
        internal const string SortCycleComboBox = "SortCycleComboBox";
        internal const string CanUserSortColumnsCheckBox = "CanUserSortColumnsCheckBox";
        internal const string HookRowStatesButton = "HookRowStatesButton";
        internal const string HookGroupHeadersButton = "HookGroupHeadersButton";

        // Pivot navigation buttons are "GoTo<Item>Button".
        internal const string BasicPivotItem = "Basic";
        internal const string RtlPivotItem = "Rtl";
        internal const string GroupedPivotItem = "Grouped";
        internal const string ScrollingPivotItem = "Scrolling";

        internal static string GoToButton(string pivotItem) => "GoTo" + pivotItem + "Button";

        // ---- Page readouts (AutomationId) ----

        internal const string EditColumnReport = "EditColumnReportTextBlock";
        internal const string EditorProbe = "EditorProbeTextBlock";
        internal const string EditEndReport = "EditEndReportTextBlock";
        internal const string FirstItemName = "FirstItemNameTextBlock";
        internal const string RowStateLog = "RowStateLogTextBlock";
        internal const string GroupHeaderStateLog = "GroupHeaderStateLogTextBlock";
        internal const string GroupToggleReport = "GroupToggleReportTextBlock";
        internal const string ScrollOffsets = "ScrollOffsetTextBlock";
        internal const string StatusText = "StatusTextBlock";

        // ---- BasicTableView columns, in visible order ----

        internal static readonly string[] BasicColumns = { "Name", "Age", "ReadOnlyCity", "Score", "Template", "Action" };
        internal static readonly int[] BasicColumnWidths = { 160, 100, 160, 100, 200, 120 };

        internal const int NameColumn = 0;
        internal const int AgeColumn = 1;
        internal const int ReadOnlyCityColumn = 2;
        internal const int ScoreColumn = 3;
        internal const int TemplateColumn = 4;
        internal const int ActionColumn = 5;

        // Row-relative x of the CENTRE of a BasicTableView column: the click target for "a cell in that column".
        internal static int BasicColumnCentreX(int column)
        {
            int left = 0;
            for (int i = 0; i < column; i++)
            {
                left += BasicColumnWidths[i];
            }
            return left + (BasicColumnWidths[column] / 2);
        }

        // ---- Data ----

        internal static readonly string[] Cities = { "Redmond", "Seattle", "Bellevue" };

        internal const int BasicItemCount = 12;
        internal const int ScrollingItemCount = 200;
        internal const int GroupedItemCount = 9;

        internal static string BasicName(int id) => "Person " + id;
        internal static int BasicAge(int id) => 20 + id;

        internal static string ScrollingName(int id) => "Scroll " + id;
        internal static int ScrollingAge(int id) => 20 + (id % 40);

        internal static string GroupedName(int id) => "Grouped " + id;
        internal static int GroupedAge(int id) => 30 + id;

        internal static string CityOf(int id) => Cities[id % Cities.Length];

        // Deliberately NON-MONOTONIC in source order so a sort cycle's trailing None step is observable: the
        // maximum lands at a source index that is neither first nor last.
        internal static int ScoreOf(int id) => ((id * 7) % 12) + 1;

        // Source index of the highest Score among the Basic rows.
        internal static int MaxScoreSourceIndex
        {
            get
            {
                int best = 0;
                for (int i = 1; i < BasicItemCount; i++)
                {
                    if (ScoreOf(i) > ScoreOf(best))
                    {
                        best = i;
                    }
                }
                return best;
            }
        }

        // The Basic item at a given position after sorting by Age descending (ages are 20 + id, strictly increasing).
        internal static int BasicIdAtPositionWhenAgeDescending(int position) => BasicItemCount - 1 - position;
    }
}
