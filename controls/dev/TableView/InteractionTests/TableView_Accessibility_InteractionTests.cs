// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView accessibility-route interaction tests.
    // Backlog: docs\design-notes\TabularControls\TableView-interaction-test-plan.md.
    //
    // These own the ASSISTIVE-TECHNOLOGY route, which is a different entry point from both the keyboard and the
    // pointer files. A screen reader does not Tab through a page to reach an element: it calls
    // IUIAutomationElement::SetFocus on the element it wants, then sends keys. That is what these tests do, and
    // it is why they are not merged into the keyboard file - the keyboard file must reach its targets by real
    // tab-stop traversal, or it stops proving anything about keyboard navigation.
    //
    // CRASH CONSTRAINT (product finding #13): asking a TableViewRow peer for its children crashes the app
    // (0xC0000420). Nothing here descends into a row's children; every assertion is made at the row/group-header
    // level or by counting the peers directly under the rows host.
    [TestClass]
    public class TableViewAccessibilityInteractionTests
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
        }

        #region Assistive-technology focus route

        [TestMethod]
        [TestProperty("Description", "Verifies a group header focused through UIA SetFocus - the route a screen reader takes - still holds focus after its own group collapses, so the next key reaches the same header.")]
        public void GroupHeaderKeepsFocusAcrossCollapseWhenFocusedThroughUia()
        {
            // PRODUCT FINDING #15, expected to FAIL until the product is fixed. Left failing deliberately: the
            //   protocol forbids weakening a test to match current behaviour.
            // Scenario: a screen reader moves focus with IUIAutomationElement::SetFocus and then sends an
            //   activation key. Measured three times: the collapse itself is correct, and the header then stops
            //   holding focus, so the next key lands elsewhere and the user cannot re-open the group they just
            //   closed. The keyboard file's Tab-route tests pass and do NOT cover this - Tab and SetFocus place
            //   focus by different paths, so one passing says nothing about the other.
            // Failure means: TableView is operable by a sighted keyboard user but not by a screen-reader user,
            //   which is an accessibility bug, not a cosmetic one.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                int baselineRows = CountRows(rowsHost);
                Verify.IsGreaterThan(baselineRows, 0, "The grouped table should realize rows before any collapse.");

                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                var expandCollapse = new ExpandCollapseImplementation(groupHeader);

                Log.Comment("Place focus the way an assistive technology does: UIA SetFocus on the header itself.");
                groupHeader.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(groupHeader.HasKeyboardFocus, "Precondition: UIA SetFocus must land focus on the group header.");

                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();

                // All three observations are taken and logged BEFORE any of them is asserted. Verify throws in
                // this suite, so the first failing assertion ends the test; without this the focus defect would
                // hide whether the collapse itself was correct, and the two answers mean different bugs.
                int rowsAfterCollapse = CountRows(rowsHost);
                ExpandCollapseState stateAfterCollapse = expandCollapse.ExpandCollapseState;
                bool keptFocus = groupHeader.HasKeyboardFocus;
                Log.Comment("After Enter on a UIA-focused header: realized rows = {0} (baseline {1}), header reports {2}, header kept focus = {3}.",
                    rowsAfterCollapse, baselineRows, stateAfterCollapse, keptFocus);

                Verify.IsLessThan(rowsAfterCollapse, baselineRows, "Enter on the focused group header must collapse its rows.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, stateAfterCollapse, "After Enter the header must report Collapsed.");
                Verify.IsTrue(keptFocus, "The group header must KEEP focus across its own collapse when focus arrived through UIA SetFocus - a screen-reader user must be able to re-open the group they just closed.");

                // Only reached once the defect is fixed: the whole point of keeping focus is that the next key
                // works, so the test proves the consequence rather than just the focus flag.
                Log.Comment("Re-open the same group with a second Enter, which is only possible if focus stayed put.");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.AreEqual(baselineRows, CountRows(rowsHost), "A second Enter must re-expand the same group and restore the baseline row count.");
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "After the second Enter the header must report Expanded.");
            }
        }

        #endregion

        #region Helpers

        // Group headers and rows are siblings under the rows host; the header's class name is
        // "TableViewGroupHeader" (TableViewGroupHeaderAutomationPeer::GetClassNameCore).
        private static UIObject GetFirstGroupHeader(UIObject rowsHost)
        {
            foreach (UIObject child in rowsHost.Children)
            {
                if (child != null && child.ClassName != null && child.ClassName.Contains("GroupHeader"))
                {
                    return child;
                }
            }

            return null;
        }

        // Counts the TableViewRow peers directly under the rows host - row level only, never a row's children.
        private static int CountRows(UIObject rowsHost)
        {
            ElementCache.Clear();

            int count = 0;
            foreach (UIObject child in rowsHost.Children)
            {
                if (child != null && child.ClassName != null && child.ClassName.Contains("TableViewRow"))
                {
                    count++;
                }
            }

            return count;
        }

        // Switches the page Pivot to the Grouped item, then returns GroupedTableView. Uses the page's
        // GoToGroupedButton rather than a name-based Pivot header search: a name search makes the provider
        // compute names for realized TableViewRow peers and trips finding #13 (0xC0000420).
        private static UIObject SelectGroupedPivotAndGetTable()
        {
            var goToGrouped = FindElement.ById<Button>("GoToGroupedButton");
            if (goToGrouped == null)
            {
                Verify.Fail("GoToGroupedButton was not found.");
                return null;
            }

            goToGrouped.InvokeAndWait();
            Wait.ForIdle();

            UIObject tableView = FindElement.ById("GroupedTableView");
            if (tableView == null)
            {
                Verify.Fail("GroupedTableView was not found after selecting the Grouped pivot item.");
                return null;
            }

            return tableView;
        }

        #endregion
    }
}
