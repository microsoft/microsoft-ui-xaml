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
        internal const string HierarchyTable = "HierarchyTable";
        internal const string HierarchyRtlTable = "HierarchyRtlTable";

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
        internal const string GroupByDeptButton = "GroupByDept";
        internal const string FilterDanButton = "FilterDan";
        internal const string ClearShapingButton = "ClearShaping";
        internal const string ResetHierarchyButton = "ResetHierarchy";

        // Pivot navigation buttons are "GoTo<Item>Button".
        internal const string BasicPivotItem = "Basic";
        internal const string RtlPivotItem = "Rtl";
        internal const string GroupedPivotItem = "Grouped";
        internal const string ScrollingPivotItem = "Scrolling";
        internal const string HierarchyPivotItem = "Hierarchy";

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

        // Read-only TextBox describing HierarchyTable, as "key=value" fields joined by ';':
        //   tree=<projection label>   rows in order as Name + Level + mark ('+' collapsed, '-' expanded,
        //                             nothing for a leaf); group headers as [key]
        //   rtl=<projection label>    the same for HierarchyRtlTable
        //   selected=<name|none>      HierarchyTable.SelectedItem
        //   focus=<name>:<row|cell>   the HierarchyTable row holding focus, itself or through a cell; or none
        //   beginning=<n>             BeginningEdit count since the last reset
        //   editing=<True|False>      HierarchyTable.IsEditing
        internal const string HierarchyReadout = "HierarchyReadout";

        internal const string TreeField = "tree";
        internal const string RtlTreeField = "rtl";
        internal const string SelectedField = "selected";
        internal const string FocusField = "focus";
        internal const string BeginningEditField = "beginning";
        internal const string EditingField = "editing";
        internal const string NoneValue = "none";

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

        // ---- Hierarchy fixture (ParentBy(Id, ManagerId)) ----
        //
        //   Ada (Eng)          Eve (Ops)       Gus (Ops, ManagerId 99: orphan, so a root leaf)
        //   +- Ben             +- Fay
        //   |  +- Dan
        //   +- Cy
        //
        // Roots start collapsed. Level is 1-based for tree rows (roots are 1).

        internal static readonly int[] HierarchyIds = { 1, 2, 3, 4, 5, 6, 7 };
        internal static readonly int?[] HierarchyManagerIds = { null, 1, 1, 2, null, 5, 99 };
        internal static readonly string[] HierarchyNames = { "Ada", "Ben", "Cy", "Dan", "Eve", "Fay", "Gus" };
        internal static readonly string[] HierarchyDepts = { "Eng", "Eng", "Eng", "Eng", "Ops", "Ops", "Ops" };
        internal static readonly int[] HierarchyScores = { 90, 75, 60, 85, 70, 95, 50 };

        internal static readonly string[] HierarchyColumns = { "Name", "Dept", "Score" };

        internal const string HierarchyAllCollapsed = "Ada1+ Eve1+ Gus1";
        internal const string HierarchyAdaExpanded = "Ada1- Ben2+ Cy2 Eve1+ Gus1";
        internal const string HierarchyAdaSubtreeExpanded = "Ada1- Ben2- Dan3 Cy2 Eve1+ Gus1";

        // Row-relative x of the centre of an UNGROUPED ROOT row's chevron. The gutter is
        // TableViewRowExpanderSize (24) wide and a root gets no indent, so it spans [0, 24) from the row's
        // leading edge. Like BasicColumnWidths this mirrors a XAML value by hand.
        internal const int HierarchyRootChevronCentreX = 12;

        // HierarchyTable column widths (Name, Dept, Score), authored in XAML.
        internal static readonly int[] HierarchyColumnWidths = { 160, 100, 100 };

        // Row-relative x of the centre of the Dept cell, well clear of the chevron.
        internal static int HierarchyDeptCellCentreX => HierarchyColumnWidths[0] + (HierarchyColumnWidths[1] / 2);
    }
}
