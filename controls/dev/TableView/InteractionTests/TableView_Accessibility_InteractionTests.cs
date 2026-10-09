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
    // TableView accessibility-route interaction tests.
    //
    // These own the ASSISTIVE-TECHNOLOGY route, which is a different entry point from both the keyboard and the
    // pointer files. A screen reader does not Tab through a page to reach an element: it calls
    // IUIAutomationElement::SetFocus on the element it wants, then sends keys. That is what these tests do, and
    // it is why they are not merged into the keyboard file - the keyboard file must reach its targets by real
    // tab-stop traversal, or it stops proving anything about keyboard navigation.
    //
    // Product finding #13 (a client asking a row peer for its children fail-fasted the app) shaped the older tests in
    // this file: they observe rows at row level, point at cells by coordinates, and read editors and visual states
    // through in-process page readouts. #11820 fixed the peers, and cell peers are now read safely; see the history
    // note in TableViewInteractionTestHelpers. The older tests are kept as written - their techniques still work.
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
            RestartAppIfLongRunning();
        }

        #region Assistive-technology focus route

        [TestMethod]
        [TestProperty("Description", "Verifies a group header focused through UIA SetFocus - the route a screen reader takes - still holds focus after its own group collapses, so the next key reaches the same header.")]
        public void GroupHeaderKeepsFocusAcrossCollapseWhenFocusedThroughUia()
        {
            // PRODUCT FINDING #15. Was [Ignore]d while it failed. It passes on builds with the group-header focus
            //   restore changes (six consecutive VM runs) and failed on the build before them.
            // Scenario: a screen reader moves focus with IUIAutomationElement::SetFocus and then sends an
            //   activation key. The collapse recycles the header, and the header must hold focus afterwards so the
            //   next key re-opens the group just closed. The keyboard file's Tab-route tests do NOT cover this - Tab
            //   and SetFocus place focus by different paths, so one passing says nothing about the other.
            // Failure means: TableView is operable by a sighted keyboard user but not by a screen-reader user,
            //   which is an accessibility bug, not a cosmetic one.
            using (var setup = new TestSetupHelper(PageName))
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

        #region Automation peer geometry

        [TestMethod]
        [TestProperty("Description", "Verifies the TableView's UIA BoundingRectangle ends at its own layout box, not at the realized cache rows below the body viewport.")]
        public void TableViewBoundingRectangleExcludesRowsBelowViewport()
        {
            // ScrollingTableView is Height=300 over 200 rows, so the body repeater realizes cache rows well below the
            // viewport. ScrollOffsetTextBlock is the next element in the same StackPanel, so its top is a scale-free
            // bound on where the table really ends. FrameworkElementAutomationPeer's default bounds union every
            // realized descendant; without TableViewAutomationPeer::GetBoundingRectangleCore the table reports
            // roughly twice its rendered height and overlaps the text block.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem))
                {
                    Verify.Fail("Could not select the 'Scrolling' pivot item.");
                    return;
                }

                UIObject tableView = GetTable(ScrollingTable);
                UIObject nextElement = FindElement.ById(ScrollOffsets);
                if (tableView == null || nextElement == null)
                {
                    Verify.Fail("ScrollingTableView or ScrollOffsetTextBlock was not found on the test page.");
                    return;
                }

                var tableBounds = tableView.BoundingRectangle;
                var nextBounds = nextElement.BoundingRectangle;
                var headerHostBounds = GetHeaderHost(tableView).BoundingRectangle;
                Log.Comment("Table bounds {0}; header host bounds {1}; next element bounds {2}.", tableBounds, headerHostBounds, nextBounds);

                Verify.IsGreaterThan(tableBounds.Height, 0, "The on-screen table must report non-empty bounds.");
                Verify.IsLessThanOrEqual(tableBounds.Bottom, nextBounds.Top + 1,
                    "The table's bounding rectangle must end at its own box, above the element laid out after it.");

                // Top-left: the header strip starts where the table does, at any scale (a wrong DIP->physical
                // conversion shows up here at 150%).
                Verify.IsLessThanOrEqual(Math.Abs(tableBounds.Left - headerHostBounds.Left), 1,
                    "The table's left edge must be its own layout box's left edge.");
                Verify.IsLessThanOrEqual(Math.Abs(tableBounds.Top - headerHostBounds.Top), 1,
                    "The table's top edge must be its own layout box's top edge.");

                // Horizontal: ScrollingTableView is Width=520 Height=300 while its columns sum to 840 DIP, so the
                // unclipped union is both wider and taller. The aspect ratio is scale-free.
                double aspect = (double)tableBounds.Width / tableBounds.Height;
                Log.Comment("Aspect ratio {0:F3}, layout box 520x300 = {1:F3}.", aspect, 520.0 / 300.0);
                Verify.IsLessThan(Math.Abs(aspect - (520.0 / 300.0)), 0.03,
                    "The table's bounding rectangle must have its layout box's proportions (Width=520, Height=300).");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the TableView's UIA BoundingRectangle stays on its layout box when the body is scrolled to the middle, with cache rows realized ABOVE the viewport too.")]
        public void TableViewBoundingRectangleStaysOnLayoutBoxWhenScrolled()
        {
            // At offset 0 nothing is realized above the viewport, so the unscrolled test cannot see top inflation.
            // After a scroll to the middle the repeater keeps cache rows on both sides.
            // Failure means the table's rectangle grows upward (or moves) as the user scrolls.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem))
                {
                    Verify.Fail("Could not select the 'Scrolling' pivot item.");
                    return;
                }

                UIObject tableView = GetTable(ScrollingTable);
                UIObject nextElement = FindElement.ById(ScrollOffsets);
                if (tableView == null || nextElement == null)
                {
                    Verify.Fail("ScrollingTableView or ScrollOffsetTextBlock was not found on the test page.");
                    return;
                }

                var before = tableView.BoundingRectangle;
                string offsetsBefore = ReadScrollOffsets();

                // 200 rows in a 300px table: ten notches land well inside the list, far from both ends.
                WheelAtPoint(CentreOf(tableView), -10 * 120);
                Wait.ForIdle();

                string offsetsAfter = ReadScrollOffsets();
                var after = tableView.BoundingRectangle;
                var nextBounds = nextElement.BoundingRectangle;
                Log.Comment("Offsets '{0}' -> '{1}'. Table bounds {2} -> {3}; next element bounds {4}.",
                    offsetsBefore, offsetsAfter, before, after, nextBounds);

                Verify.AreNotEqual(offsetsBefore, offsetsAfter, "Precondition: the wheel must scroll the body.");
                Verify.IsLessThanOrEqual(Math.Abs(after.Top - before.Top), 1, "Scrolling must not move the table's top edge.");
                Verify.IsLessThanOrEqual(Math.Abs(after.Left - before.Left), 1, "Scrolling must not move the table's left edge.");
                Verify.IsLessThanOrEqual(Math.Abs(after.Height - before.Height), 1, "Scrolling must not change the table's height.");
                Verify.IsLessThanOrEqual(after.Bottom, nextBounds.Top + 1,
                    "The scrolled table's bounding rectangle must still end above the element laid out after it.");
            }
        }

        #endregion
    }
}
