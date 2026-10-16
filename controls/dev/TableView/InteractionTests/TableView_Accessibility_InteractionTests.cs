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
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
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
            // Scenario: a screen reader moves focus with IUIAutomationElement::SetFocus and then sends an
            //   activation key. Tab-route tests do NOT cover this: Tab and SetFocus place focus by different
            //   paths (the group-header peer's SetFocusCore must focus with FocusState::Keyboard).
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

        [TestMethod]
        [TestProperty("Description", "Verifies a UIA Collapse on an already-collapsed group header raises no StructureChanged, while the first Collapse does.")]
        public void GroupHeaderRepeatedCollapseRaisesNoStructureChanged()
        {
            // Events are only observable from an out-of-proc client (see TableViewTests.VerifyStructureChangedEventsReachAUiaClient).
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    Verify.Fail("The grouped TableView was not found.");
                    return;
                }

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                var expandCollapse = new ExpandCollapseImplementation(groupHeader);
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "Precondition: group 0 starts expanded.");

                // Group StructureChanged is raised on the TableView's own peer, so element scope on the table.
                using (var waiter = new StructureChangedEventWaiter(tableView, Scope.Element))
                {
                    expandCollapse.Collapse();
                    Wait.ForIdle();
                    Verify.IsTrue(waiter.TryWait(TimeSpan.FromSeconds(5)), "Positive control: the first Collapse must raise StructureChanged.");
                    Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "The first Collapse must collapse the group.");

                    // Drain any trailing events from the first Collapse so they cannot be blamed on the second.
                    waiter.Reset();
                    Wait.ForIdle();
                    waiter.TryWait(TimeSpan.FromMilliseconds(500));
                    waiter.Reset();

                    expandCollapse.Collapse();
                    Wait.ForIdle();
                    Verify.IsFalse(waiter.TryWait(TimeSpan.FromSeconds(2)), "Collapsing an already-collapsed group changes nothing and must not raise StructureChanged.");
                }
            }
        }

        #endregion

        #region Automation peer geometry

        [TestMethod]
        [TestProperty("Description", "Verifies the TableView's UIA BoundingRectangle ends at its own layout box, not at the realized cache rows below the body viewport.")]
        public void TableViewBoundingRectangleExcludesRowsBelowViewport()
        {
            // ScrollOffsetTextBlock follows the table in the same StackPanel, so its top bounds the table's end.
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

                // A wrong DIP->physical conversion shows up here at 150%.
                Verify.IsLessThanOrEqual(Math.Abs(tableBounds.Left - headerHostBounds.Left), 1,
                    "The table's left edge must be its own layout box's left edge.");
                Verify.IsLessThanOrEqual(Math.Abs(tableBounds.Top - headerHostBounds.Top), 1,
                    "The table's top edge must be its own layout box's top edge.");

                // Columns sum to 840 DIP, so the unclipped union is wider too. The aspect ratio is scale-free.
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
            // At offset 0 nothing is realized above the viewport, so only a scrolled table can show top inflation.
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

        #region Group expansion structure events

        [TestMethod]
        [TestProperty("Description", "Verifies collapsing and re-expanding a group in a flat grouped table each raise StructureChanged to a UIA client, and a no-op expand does not.")]
        public void GroupExpansionRaisesStructureChangedForEachRealChange()
        {
            // Contract: RaiseGroupStructureChanged fires when, and only when, a group's expansion actually
            //   changed. RowMetadataProvider::SetGroupExpandedCore reports "changed", not the resulting state.
            //   Before hierarchy it returned the resulting state, so a collapse (state false) never raised and
            //   a client kept the collapsed rows in its cached tree.
            // Failure means: a screen reader keeps reading rows a collapse removed, or re-reads a tree that a
            //   no-op expand never changed.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                var expandCollapse = new ExpandCollapseImplementation(groupHeader);
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "Precondition: the first group starts expanded.");

                using (var waiter = new StructureChangedEventWaiter(tableView, Scope.Element))
                {
                    expandCollapse.Collapse();
                    Wait.ForIdle();
                    bool collapseRaised = waiter.TryWait(TimeSpan.FromSeconds(5));

                    waiter.Reset();
                    expandCollapse.Expand();
                    Wait.ForIdle();
                    bool expandRaised = waiter.TryWait(TimeSpan.FromSeconds(5));

                    waiter.Reset();
                    expandCollapse.Expand();
                    Wait.ForIdle();
                    bool noOpRaised = waiter.TryWait(TimeSpan.FromSeconds(2));

                    Log.Comment("StructureChanged observed: collapse={0} expand={1} no-op expand={2}.",
                        collapseRaised, expandRaised, noOpRaised);

                    Verify.IsTrue(collapseRaised, "Collapsing a group must raise StructureChanged.");
                    Verify.IsTrue(expandRaised, "Re-expanding a group must raise StructureChanged.");
                    Verify.IsFalse(noOpRaised, "Expanding an already-expanded group changes nothing and must not raise StructureChanged.");
                }
            }
        }

        #endregion
    }
}
