// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView Axe scans in states other than the page's default. Axe scans the whole app window, so each test
    // puts the page into one well-defined state and proves that state took effect before scanning.
    [TestClass]
    public class TableViewAxeInteractionTests
    {
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
        [TestProperty("Description", "Verifies an Axe scan reports no issues after a column is sorted through header activation.")]
        public void VerifyAxeScanPassesWhenSorted()
        {
            using (var setup = new TestSetupHelper(AxePageName))
            {
                UIObject tableView = GetTable(BasicTable);
                if (tableView == null) { return; }

                // Sort-state text is empty (product finding #5), so the reorder is the observable sort state.
                string expectedFirst = BasicName(BasicIdAtPositionWhenAgeDescending(0));
                UIObject firstBefore = GetRow(tableView, 0);
                if (firstBefore == null) { Verify.Fail("The first row of BasicTableView was not found."); return; }
                Verify.IsFalse(firstBefore.Name.StartsWith(expectedFirst, StringComparison.Ordinal),
                    "Precondition: before sorting, the first row must not already hold the largest Age.");

                ClickHeaderTwice(BasicTable, "Age");
                Wait.ForIdle();
                ElementCache.Clear();

                UIObject firstAfter = GetRow(GetTable(BasicTable), 0);
                if (firstAfter == null) { Verify.Fail("The first row of BasicTableView was not found after sorting."); return; }
                Log.Comment("First row after sorting Age descending: '{0}' (expected to start with '{1}').", firstAfter.Name, expectedFirst);
                Verify.IsTrue(firstAfter.Name.StartsWith(expectedFirst, StringComparison.Ordinal),
                    "Precondition: two header clicks must sort Age descending, putting the largest Age first.");

                AxeTestHelper.TestForAxeIssues();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an Axe scan reports no issues on the grouped table, both expanded and with one group collapsed.")]
        public void VerifyAxeScanPassesWhenGrouped()
        {
            using (var setup = new TestSetupHelper(AxePageName))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null) { return; }

                UIObject rowsHost = GetRowsHost(tableView);
                int expandedRows = CountRows(rowsHost);
                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null) { Verify.Fail("No TableViewGroupHeader peer was found in GroupedTableView."); return; }
                Verify.IsGreaterThan(expandedRows, 0, "Precondition: the grouped table must realize rows.");

                string headerName = groupHeader.Name;
                var expandCollapse = new ExpandCollapseImplementation(groupHeader);
                if (!expandCollapse.IsAvailable) { Verify.Fail("The group header must expose the ExpandCollapse pattern."); return; }
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "Precondition: the first group starts expanded.");

                Log.Comment("Scan with every group expanded.");
                AxeTestHelper.TestForAxeIssues();

                expandCollapse.Collapse();
                groupHeader = RefetchFirstGroupHeader(tableView, headerName);
                if (groupHeader == null) { return; }

                int collapsedRows = CountRows(GetRowsHost(tableView));
                Log.Comment("Realized rows: {0} expanded, {1} after collapsing the first group.", expandedRows, collapsedRows);
                Verify.AreEqual(ExpandCollapseState.Collapsed, new ExpandCollapseImplementation(groupHeader).ExpandCollapseState, "Precondition: the first group must report Collapsed.");
                Verify.IsLessThan(collapsedRows, expandedRows, "Precondition: collapsing the first group must remove its rows.");

                Log.Comment("Scan with the first group collapsed.");
                AxeTestHelper.TestForAxeIssues();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an Axe scan reports no issues on the right-to-left table.")]
        public void VerifyAxeScanPassesInRightToLeft()
        {
            using (var setup = new TestSetupHelper(AxePageName))
            {
                if (!SelectPivotItem(RtlPivotItem)) { Verify.Fail(GoToButton(RtlPivotItem) + " was not found."); return; }

                UIObject tableView = GetTable(RtlTable);
                if (tableView == null) { return; }

                UIObject nameHeader = FindColumnHeader(tableView, "RtlName");
                UIObject cityHeader = FindColumnHeader(tableView, "RtlCity");
                if (nameHeader == null || cityHeader == null) { Verify.Fail("The RtlName and RtlCity headers were not found."); return; }

                // FlowDirection is not a UIA property; the mirrored layout is its observable consequence.
                Log.Comment("RtlName bounds={0}, RtlCity bounds={1}.", nameHeader.BoundingRectangle, cityHeader.BoundingRectangle);
                Verify.IsGreaterThan(nameHeader.BoundingRectangle.Left, cityHeader.BoundingRectangle.Left,
                    "Precondition: under RTL the reading-order-first column must be laid out to the screen right of the second.");
                Verify.IsGreaterThan(GetRowsHost(tableView).Children.Count, 0, "Precondition: RtlTableView must realize rows.");

                AxeTestHelper.TestForAxeIssues();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an Axe scan reports no issues with a hierarchy root expanded.")]
        public void VerifyAxeScanPassesWithHierarchyExpanded()
        {
            using (var setup = new TestSetupHelper(AxePageName))
            {
                UIObject tableView = TableViewHierarchyInteractionTests.SelectHierarchyPivotAndGetTable();
                if (tableView == null) { return; }

                UIObject rowsHost = GetRowsHost(tableView);
                int collapsedRows = CountRows(rowsHost);

                UIObject ada = FindRowStartingWith(rowsHost, "Ada");
                if (ada == null) { Verify.Fail("Row 'Ada' was not found in HierarchyTable."); return; }

                var expandCollapse = new ExpandCollapseImplementation(ada);
                if (!expandCollapse.IsAvailable) { Verify.Fail("Ada's row must expose the ExpandCollapse pattern."); return; }

                expandCollapse.Expand();
                Wait.ForIdle();

                // The expand reshapes the rows, so Ada's container is re-read rather than reused.
                int expandedRows = CountRows(rowsHost);
                ada = FindRowStartingWith(GetRowsHost(tableView), "Ada");
                if (ada == null) { Verify.Fail("Row 'Ada' must still be realized after expanding it."); return; }
                Log.Comment("HierarchyTable realized rows: {0} collapsed, {1} after expanding Ada.", collapsedRows, expandedRows);
                Verify.AreEqual(ExpandCollapseState.Expanded, new ExpandCollapseImplementation(ada).ExpandCollapseState, "Precondition: Ada must report Expanded.");
                Verify.IsGreaterThan(expandedRows, collapsedRows, "Precondition: expanding Ada must realize its children.");

                AxeTestHelper.TestForAxeIssues();
            }
        }

        // The realized row whose Name (its cell texts) starts with prefix, or null.
        private static UIObject FindRowStartingWith(UIObject rowsHost, string prefix)
        {
            foreach (UIObject child in rowsHost.Children)
            {
                if (IsRow(child) && child.Name != null && child.Name.StartsWith(prefix, StringComparison.Ordinal))
                {
                    return child;
                }
            }

            return null;
        }
    }
}
