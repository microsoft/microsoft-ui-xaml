// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using MUXTestInfra.Shared.Infra;
using Point = System.Drawing.Point;

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // Injected touch must reach the same selection, group toggle and scroll-sync paths the mouse tests cover.
    [TestClass]
    public class TableViewTouchInteractionTests
    {
        private const string NavigateToTouchButton = "navigateToTouch";
        private const string TouchBasicTable = "TouchBasicTableView";
        private const string TouchGroupedTable = "TouchGroupedTableView";
        private const string TouchScrollingTable = "TouchScrollingTableView";
        private const string TouchScrollOffsets = "TouchScrollOffsetTextBlock";
        private const string TouchScrollFirstHeader = "TouchScrollName";

        private const int TappedRowIndex = 2;
        private const int PanDistance = 150;

        // Same sync tolerance as TableView_Layout_InteractionTests.HorizontalScrollKeepsHeaderAligned.
        private const double HeaderSyncTolerance = 1.0;
        private const double HeaderStickyTolerance = 4.0;

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
            RestartAppIfLongRunning();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a touch tap on a row selects that row and moves keyboard focus into it.")]
        public void TouchTapSelectsAndFocusesRow()
        {
            using (var setup = new TestSetupHelper(new[] { PageName, NavigateToTouchButton }))
            {
                UIObject tableView = GetTable(TouchBasicTable);
                if (tableView == null)
                {
                    return;
                }

                UIObject row = GetRow(tableView, TappedRowIndex);
                if (row == null)
                {
                    Verify.Fail(string.Format("Realized row {0} was not found on {1}.", TappedRowIndex, TouchBasicTable));
                    return;
                }

                VerifyNoRowSelected("navigating to the page", TouchBasicTable);
                Verify.AreEqual(-1, IndexOfFocusedBodyRow(TouchBasicTable), "Precondition: no row of the table may hold focus before the tap.");

                InputHelper.Tap(row);
                Wait.ForIdle();

                int selectedIndex = IndexOfSelectedRow(TouchBasicTable);
                int focusedIndex = IndexOfFocusedBodyRow(TouchBasicTable);
                Log.Comment("After the tap: selected row={0}, focused row={1}.", selectedIndex, focusedIndex);

                Verify.AreEqual(TappedRowIndex, selectedIndex, "A touch tap must select the tapped row.");
                Verify.AreEqual(TappedRowIndex, focusedIndex, "A touch tap must move keyboard focus to the tapped row (or one of its cells).");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a touch tap on a group header collapses its group and a second tap expands it again.")]
        public void TouchTapOnGroupHeaderTogglesExpansion()
        {
            using (var setup = new TestSetupHelper(new[] { PageName, NavigateToTouchButton }))
            {
                UIObject tableView = GetTable(TouchGroupedTable);
                if (tableView == null)
                {
                    return;
                }

                UIObject rowsHost = GetRowsHost(tableView);
                int baselineRows = CountRows(rowsHost);
                Verify.IsGreaterThan(baselineRows, 0, "The grouped table should realize rows before any collapse.");

                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                string headerName = groupHeader.Name;
                Verify.AreEqual(ExpandCollapseState.Expanded, new ExpandCollapseImplementation(groupHeader).ExpandCollapseState, "Baseline: the group header should report Expanded.");

                InputHelper.Tap(groupHeader);
                groupHeader = RefetchFirstGroupHeader(tableView, headerName);
                if (groupHeader == null) { return; }

                int collapsedRows = CountRows(GetRowsHost(tableView));
                Verify.IsLessThan(collapsedRows, baselineRows, "Tapping the group header must collapse its group and drop the realized row count.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, new ExpandCollapseImplementation(groupHeader).ExpandCollapseState, "After the tap the header must report Collapsed.");

                InputHelper.Tap(groupHeader);
                groupHeader = RefetchFirstGroupHeader(tableView, headerName);
                if (groupHeader == null) { return; }

                Verify.AreEqual(baselineRows, CountRows(GetRowsHost(tableView)), "A second tap must restore the original realized row count.");
                Verify.AreEqual(ExpandCollapseState.Expanded, new ExpandCollapseImplementation(groupHeader).ExpandCollapseState, "After the second tap the header must report Expanded again.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a touch pan scrolls the body vertically with the header pinned, and horizontally with the header offset tracking the body.")]
        public void TouchPanScrollsBodyAndKeepsHeaderAligned()
        {
            using (var setup = new TestSetupHelper(new[] { PageName, NavigateToTouchButton }))
            {
                UIObject tableView = GetTable(TouchScrollingTable);
                if (tableView == null)
                {
                    return;
                }

                UIObject header = FindColumnHeader(tableView, TouchScrollFirstHeader);
                if (header == null)
                {
                    Verify.Fail("The '" + TouchScrollFirstHeader + "' column header peer was not found on the header host.");
                    return;
                }

                string offsetsBefore = ReadSettledTouchScrollOffsets();
                Verify.AreEqual(0.0, ReadScrollOffsetComponent(offsetsBefore, "H"), "Precondition: the body must start at H=0. Raw: " + offsetsBefore);
                Verify.AreEqual(0.0, ReadScrollOffsetComponent(offsetsBefore, "V"), "Precondition: the body must start at V=0. Raw: " + offsetsBefore);
                Verify.AreEqual(0.0, ReadScrollOffsetComponent(offsetsBefore, "HeaderH"), "Precondition: the header must start at HeaderH=0. Raw: " + offsetsBefore);

                double headerTopBefore = header.BoundingRectangle.Top;
                var bounds = tableView.BoundingRectangle;

                Log.Comment("Pan the body up (finger moves North) to scroll vertically.");
                var verticalStart = new Point(bounds.Left + (bounds.Width / 2), bounds.Top + (bounds.Height * 3 / 4));
                InputHelper.Pan(tableView, verticalStart, PanDistance, Direction.North);

                string offsetsAfterVertical = ReadSettledTouchScrollOffsets();
                double headerTopAfter = FindColumnHeader(tableView, TouchScrollFirstHeader).BoundingRectangle.Top;
                Log.Comment("Offsets '{0}' -> '{1}'; header Top {2} -> {3}.", offsetsBefore, offsetsAfterVertical, headerTopBefore, headerTopAfter);

                Verify.IsGreaterThan(
                    ReadScrollOffsetComponent(offsetsAfterVertical, "V"),
                    0.0,
                    "A vertical touch pan must scroll PART_BodyScroller vertically.");
                Verify.IsLessThanOrEqual(
                    Math.Abs(headerTopAfter - headerTopBefore),
                    HeaderStickyTolerance,
                    "The header row must stay pinned at the top during a vertical touch pan.");

                double bodyHBefore = ReadScrollOffsetComponent(offsetsAfterVertical, "H");

                Log.Comment("Pan the body left (finger moves West) to scroll horizontally.");
                var horizontalStart = new Point(bounds.Right - 40, bounds.Top + (bounds.Height / 2));
                InputHelper.Pan(tableView, horizontalStart, PanDistance, Direction.West);

                string offsetsAfterHorizontal = ReadSettledTouchScrollOffsets();
                double bodyHAfter = ReadScrollOffsetComponent(offsetsAfterHorizontal, "H");
                double headerHAfter = ReadScrollOffsetComponent(offsetsAfterHorizontal, "HeaderH");
                Log.Comment("Offsets '{0}' -> '{1}'.", offsetsAfterVertical, offsetsAfterHorizontal);

                Verify.IsGreaterThan(bodyHAfter, bodyHBefore, "A horizontal touch pan must scroll PART_BodyScroller horizontally.");
                Verify.IsLessThan(
                    Math.Abs(headerHAfter - bodyHAfter),
                    HeaderSyncTolerance,
                    string.Format("The header offset ({0}) must track the body offset ({1}) within the sync tolerance.", headerHAfter, bodyHAfter));
            }
        }

        // Index among the rows host's children of the row that holds focus itself or through a cell, or -1.
        private static int IndexOfFocusedBodyRow(string tableAutomationId)
        {
            UIObject rowsHost = GetRowsHost(tableAutomationId);
            if (rowsHost == null) { return -1; }

            int index = 0;
            foreach (UIObject row in rowsHost.Children)
            {
                if (row.HasKeyboardFocus || IndexOfFocusedCell(row) >= 0)
                {
                    return index;
                }
                index++;
            }
            return -1;
        }

        private static string ReadTouchScrollOffsets()
        {
            var readout = FindElement.ById<TextBlock>(TouchScrollOffsets);
            return readout == null ? "<no readout>" : readout.DocumentText;
        }

        // A touch pan leaves inertia running after Wait.ForIdle, so poll until two consecutive reads agree.
        private static string ReadSettledTouchScrollOffsets()
        {
            string previous = ReadTouchScrollOffsets();
            var timer = Stopwatch.StartNew();
            while (timer.ElapsedMilliseconds < 3000)
            {
                Wait.ForMilliseconds(200);
                string current = ReadTouchScrollOffsets();
                if (current == previous)
                {
                    return current;
                }
                previous = current;
            }
            return previous;
        }
    }
}
