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
    // TableView hierarchical-row interaction tests: UI Automation, keyboard and pointer, end to end.
    //
    // Every test runs on the page's Hierarchy pivot, over the ParentBy(Id, ManagerId) fixture described in
    // TableViewTestPageFacts. SelectHierarchyPivotAndGetTable resets the fixture first, so each test starts with
    // every root collapsed, nothing selected and no shaping applied.
    //
    // Tree shape is read from the page's HierarchyReadout (an in-process walk of the realized rows); focus,
    // selection and ExpandCollapse state are read through UIA wherever UIA exposes them.
    [TestClass]
    public class TableViewHierarchyInteractionTests
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

        #region UI Automation

        [TestMethod]
        [TestProperty("Description", "Verifies toggling a tree row raises a UIA ExpandCollapseState property-changed event, so screen readers announce the change.")]
        public void ExpandCollapseStateChangeRaisesUiaEvent()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }

                UIObject ada = FindTreeRow(HierarchyTable, "Ada");
                if (ada == null) { return; }

                var expandCollapse = new ExpandCollapseImplementation(ada);
                if (!expandCollapse.IsAvailable) { Verify.Fail("Ada's row must expose the ExpandCollapse pattern."); return; }
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "Precondition: Ada starts collapsed.");

                using (var waiter = new PropertyChangedEventWaiter(ada, Scope.Element, UIProperty.Get("ExpandCollapse.ExpandCollapseState")))
                {
                    expandCollapse.Expand();
                    Verify.IsTrue(waiter.TryWait(TimeSpan.FromSeconds(2)), "Expand must raise an ExpandCollapseState property-changed event on the row.");
                }
                Wait.ForIdle();
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "After Expand the row must report Expanded.");

                using (var waiter = new PropertyChangedEventWaiter(ada, Scope.Element, UIProperty.Get("ExpandCollapse.ExpandCollapseState")))
                {
                    expandCollapse.Collapse();
                    Verify.IsTrue(waiter.TryWait(TimeSpan.FromSeconds(2)), "Collapse must raise an ExpandCollapseState property-changed event on the row.");
                }
                Wait.ForIdle();
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "After Collapse the row must report Collapsed.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a tree row focused through UIA SetFocus - the route a screen reader takes - keeps focus when it is collapsed and expanded through its ExpandCollapse pattern, so the next key reaches the same row.")]
        public void TreeRowKeepsFocusAcrossCollapseWhenFocusedThroughUia()
        {
            // Tree-row analogue of GroupHeaderKeepsFocusAcrossCollapseWhenFocusedThroughUia.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }

                UIObject ada = FindTreeRow(HierarchyTable, "Ada");
                if (ada == null) { return; }

                var expandCollapse = new ExpandCollapseImplementation(ada);
                if (!expandCollapse.IsAvailable) { Verify.Fail("Ada's row must expose the ExpandCollapse pattern."); return; }

                Log.Comment("Place focus the way an assistive technology does: UIA SetFocus on the row itself.");
                ada.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(ada.HasKeyboardFocus, "Precondition: UIA SetFocus must land focus on Ada's row.");

                expandCollapse.Expand();
                Wait.ForIdle();

                // Every observation is logged before any is asserted, so a focus failure does not hide whether the
                // toggle itself was correct.
                string treeAfterExpand = WaitForHierarchyField(TreeField, HierarchyAdaExpanded);
                ExpandCollapseState stateAfterExpand = expandCollapse.ExpandCollapseState;
                bool focusedAfterExpand = IsTreeRowFocused(HierarchyTable, "Ada");
                Log.Comment("After Expand: tree = {0}, state = {1}, Ada kept focus = {2}.", treeAfterExpand, stateAfterExpand, focusedAfterExpand);

                Verify.AreEqual(HierarchyAdaExpanded, treeAfterExpand, "Expand must project Ada's children.");
                Verify.AreEqual(ExpandCollapseState.Expanded, stateAfterExpand, "After Expand the row must report Expanded.");
                Verify.IsTrue(focusedAfterExpand, "Ada must keep focus across its own expansion through UIA.");

                expandCollapse.Collapse();
                Wait.ForIdle();

                string treeAfterCollapse = WaitForHierarchyField(TreeField, HierarchyAllCollapsed);
                ExpandCollapseState stateAfterCollapse = expandCollapse.ExpandCollapseState;
                bool focusedAfterCollapse = IsTreeRowFocused(HierarchyTable, "Ada");
                Log.Comment("After Collapse: tree = {0}, state = {1}, Ada kept focus = {2}.", treeAfterCollapse, stateAfterCollapse, focusedAfterCollapse);

                Verify.AreEqual(HierarchyAllCollapsed, treeAfterCollapse, "Collapse must remove Ada's children.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, stateAfterCollapse, "After Collapse the row must report Collapsed.");
                Verify.IsTrue(focusedAfterCollapse, "Ada must keep focus across its own collapse through UIA.");

                Log.Comment("Re-open Ada with Right, which only works if focus stayed on the row.");
                KeyboardHelper.PressKey(Key.Right);
                Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(TreeField, HierarchyAdaExpanded), "Right on the still-focused row must expand Ada again.");
            }
        }

        #endregion

        #region Keyboard (row container focused)

        [TestMethod]
        [TestProperty("Description", "Verifies Right on a focused collapsed tree row expands it and keeps focus on the row (treegrid Right).")]
        public void RightExpandsCollapsedTreeRow()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }
                if (FocusTreeRow(HierarchyTable, "Ada") == null) { return; }

                KeyboardHelper.PressKey(Key.Right);

                Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(TreeField, HierarchyAdaExpanded), "Right on collapsed Ada must expand it.");
                VerifyTreeRowFocused(HierarchyTable, "Ada", "Focus must stay on Ada's row after Right expands it.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Right on an expanded tree row drills into its first cell rather than moving to the next row, and Left from that first cell returns to the row without collapsing it.")]
        public void RightOnExpandedTreeRowDrillsToFirstCell()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }
                if (!ExpandWithRight(HierarchyTable, "Ada", TreeField, HierarchyAdaExpanded)) { return; }

                KeyboardHelper.PressKey(Key.Right);

                UIObject ada = FindTreeRow(HierarchyTable, "Ada");
                if (ada == null) { return; }
                Verify.AreEqual(0, IndexOfFocusedCell(ada), "Right on expanded Ada must focus Ada's first cell.");
                Verify.IsFalse(IsTreeRowFocused(HierarchyTable, "Ben"), "Right on an expanded row must not move focus to the next row.");
                Verify.AreEqual(HierarchyAdaExpanded, ReadHierarchyField(TreeField), "Drilling into a cell must not change the tree.");

                KeyboardHelper.PressKey(Key.Left);

                VerifyTreeRowFocused(HierarchyTable, "Ada", "Left from the first cell must return focus to Ada's row.");
                Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(TreeField, HierarchyAdaExpanded), "Left from the first cell must not collapse Ada.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Left on a focused expanded tree row collapses it and keeps focus on the row.")]
        public void LeftCollapsesExpandedTreeRow()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }
                if (!ExpandWithRight(HierarchyTable, "Ada", TreeField, HierarchyAdaExpanded)) { return; }

                KeyboardHelper.PressKey(Key.Left);

                Verify.AreEqual(HierarchyAllCollapsed, WaitForHierarchyField(TreeField, HierarchyAllCollapsed), "Left on expanded Ada must collapse it.");
                VerifyTreeRowFocused(HierarchyTable, "Ada", "Focus must stay on Ada's row after Left collapses it.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Left on a child row - a leaf, a deeper leaf, or a collapsed expandable child - moves focus to its parent and selects the parent.")]
        public void LeftOnChildTreeRowFocusesAndSelectsParent()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }
                if (!ExpandWithRight(HierarchyTable, "Ada", TreeField, HierarchyAdaExpanded)) { return; }

                Log.Comment("Leaf child: Cy -> Ada.");
                if (FocusTreeRow(HierarchyTable, "Cy") == null) { return; }
                KeyboardHelper.PressKey(Key.Left);
                VerifyTreeRowFocused(HierarchyTable, "Ada", "Left on leaf Cy must focus its parent Ada.");
                Verify.AreEqual("Ada", WaitForHierarchyField(SelectedField, "Ada"), "Left on leaf Cy must select its parent Ada.");
                Verify.AreEqual(HierarchyAdaExpanded, ReadHierarchyField(TreeField), "Moving to the parent must not change the tree.");

                Log.Comment("Deeper leaf: Dan -> Ben.");
                if (!ExpandWithUia(HierarchyTable, "Ben", HierarchyAdaSubtreeExpanded)) { return; }
                if (FocusTreeRow(HierarchyTable, "Dan") == null) { return; }
                KeyboardHelper.PressKey(Key.Left);
                VerifyTreeRowFocused(HierarchyTable, "Ben", "Left on leaf Dan must focus its parent Ben.");
                Verify.AreEqual("Ben", WaitForHierarchyField(SelectedField, "Ben"), "Left on leaf Dan must select its parent Ben.");

                Log.Comment("Collapsed expandable child: Ben -> Ada.");
                UIObject ben = FindTreeRow(HierarchyTable, "Ben");
                if (ben == null) { return; }
                new ExpandCollapseImplementation(ben).Collapse();
                Wait.ForIdle();
                if (WaitForHierarchyField(TreeField, HierarchyAdaExpanded) != HierarchyAdaExpanded) { Verify.Fail("Precondition: Ben did not collapse."); return; }

                if (FocusTreeRow(HierarchyTable, "Ben") == null) { return; }
                KeyboardHelper.PressKey(Key.Left);
                VerifyTreeRowFocused(HierarchyTable, "Ada", "Left on collapsed child Ben must focus its parent Ada.");
                Verify.AreEqual("Ada", WaitForHierarchyField(SelectedField, "Ada"), "Left on collapsed child Ben must select its parent Ada.");
                Verify.AreEqual(HierarchyAdaExpanded, ReadHierarchyField(TreeField), "Moving to the parent must not change the tree.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Left on a root tree row that is not expanded is consumed: focus stays on the row and does not leave the table.")]
        public void LeftOnRootTreeRowIsConsumed()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }

                Log.Comment("Collapsed root: Eve.");
                if (FocusTreeRow(HierarchyTable, "Eve") == null) { return; }
                KeyboardHelper.PressKey(Key.Left);
                VerifyTreeRowFocused(HierarchyTable, "Eve", "Left on collapsed root Eve must keep focus on Eve.");
                Verify.AreEqual(HierarchyAllCollapsed, WaitForHierarchyField(TreeField, HierarchyAllCollapsed), "Left on a collapsed root must not change the tree.");

                Log.Comment("Leaf root: Gus (an orphan, so a root).");
                if (FocusTreeRow(HierarchyTable, "Gus") == null) { return; }
                KeyboardHelper.PressKey(Key.Left);
                VerifyTreeRowFocused(HierarchyTable, "Gus", "Left on leaf root Gus must keep focus on Gus.");
                Verify.AreEqual(HierarchyAllCollapsed, ReadHierarchyField(TreeField), "Left on a leaf root must not change the tree.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Down from an expanded tree row reaches its first child, so children are part of linear arrow navigation.")]
        public void DownFromExpandedTreeRowReachesFirstChild()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }
                if (!ExpandWithRight(HierarchyTable, "Ada", TreeField, HierarchyAdaExpanded)) { return; }

                KeyboardHelper.PressKey(Key.Down);

                VerifyTreeRowFocused(HierarchyTable, "Ben", "Down from expanded Ada must focus its first child Ben.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Multiply (*) on a focused tree row expands the whole subtree beneath it and keeps focus on the row.")]
        // Known product bug: ExpandSubtree rebuilds (Reset) instead of splicing; the repeater moves focus
        // off the cleared container and it lands on the first child (Ben), not Ada. The subtree assert passes.
        [TestProperty("Ignore", "True")]
        public void MultiplyExpandsTreeRowSubtree()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }
                if (FocusTreeRow(HierarchyTable, "Ada") == null) { return; }

                // The numeric-keypad Multiply key: the product handles VirtualKey.Multiply, not Shift+8.
                Log.Comment("Send text '{MULTIPLY}'.");
                TextInput.SendText("{MULTIPLY}");
                Wait.ForIdle();

                Verify.AreEqual(HierarchyAdaSubtreeExpanded, WaitForHierarchyField(TreeField, HierarchyAdaSubtreeExpanded),
                    "Multiply on Ada must expand Ada and every expandable descendant, leaving other roots alone.");
                VerifyTreeRowFocused(HierarchyTable, "Ada", "Focus must stay on Ada's row after Multiply.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Left/Right with Ctrl or Alt held fall through without changing the tree, and Shift uses the plain row drill without changing the tree.")]
        public void ModifiedArrowsLeaveTreeStateUnchanged()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }

                Log.Comment("Right with modifiers on collapsed Ada.");
                foreach (ModifierKey modifier in new[] { ModifierKey.Control, ModifierKey.Alt, ModifierKey.Shift })
                {
                    if (FocusTreeRow(HierarchyTable, "Ada") == null) { return; }
                    KeyboardHelper.PressKey(Key.Right, modifier);
                    Verify.AreEqual(HierarchyAllCollapsed, WaitForHierarchyField(TreeField, HierarchyAllCollapsed),
                        modifier + "+Right must not expand Ada.");

                    if (modifier == ModifierKey.Shift)
                    {
                        UIObject ada = FindTreeRow(HierarchyTable, "Ada");
                        if (ada == null) { return; }
                        Verify.AreEqual(0, IndexOfFocusedCell(ada), "Shift+Right must take the plain drill into Ada's first cell.");

                        // Pop back to row level; while the cell cursor is active, focusing a row lands on its cell.
                        KeyboardHelper.PressKey(Key.Left);
                        VerifyTreeRowFocused(HierarchyTable, "Ada", "Left from the first cell must return focus to Ada's row.");
                    }
                }

                UIObject adaRow = FindTreeRow(HierarchyTable, "Ada");
                if (adaRow == null) { return; }
                new ExpandCollapseImplementation(adaRow).Expand();
                Wait.ForIdle();
                if (WaitForHierarchyField(TreeField, HierarchyAdaExpanded) != HierarchyAdaExpanded) { Verify.Fail("Precondition: Ada did not expand."); return; }

                Log.Comment("Left with modifiers on expanded Ada.");
                foreach (ModifierKey modifier in new[] { ModifierKey.Control, ModifierKey.Alt, ModifierKey.Shift })
                {
                    if (FocusTreeRow(HierarchyTable, "Ada") == null) { return; }
                    KeyboardHelper.PressKey(Key.Left, modifier);
                    Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(TreeField, HierarchyAdaExpanded),
                        modifier + "+Left must not collapse Ada.");

                    if (modifier == ModifierKey.Shift)
                    {
                        VerifyTreeRowFocused(HierarchyTable, "Ada", "Shift+Left at row level is consumed by the plain drill: focus stays on Ada's row.");
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies tree keys mirror under RightToLeft: Left expands then drills in, Right pops out of the first cell, collapses, and moves a child to its parent.")]
        public void RightToLeftTreeRowArrowsMirror()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }
                if (FocusTreeRow(HierarchyRtlTable, "Ada") == null) { return; }

                KeyboardHelper.PressKey(Key.Left);
                Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(RtlTreeField, HierarchyAdaExpanded), "Under RTL, Left on collapsed Ada must expand it.");
                VerifyTreeRowFocused(HierarchyRtlTable, "Ada", "Under RTL, focus must stay on Ada after Left expands it.");

                KeyboardHelper.PressKey(Key.Left);
                UIObject ada = FindTreeRow(HierarchyRtlTable, "Ada");
                if (ada == null) { return; }
                Verify.AreEqual(0, IndexOfFocusedCell(ada), "Under RTL, Left on expanded Ada must focus its first cell.");

                KeyboardHelper.PressKey(Key.Right);
                VerifyTreeRowFocused(HierarchyRtlTable, "Ada", "Under RTL, Right from the first cell must return focus to Ada's row.");
                Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(RtlTreeField, HierarchyAdaExpanded), "Under RTL, leaving the first cell must not collapse Ada.");

                KeyboardHelper.PressKey(Key.Right);
                Verify.AreEqual(HierarchyAllCollapsed, WaitForHierarchyField(RtlTreeField, HierarchyAllCollapsed), "Under RTL, Right on expanded Ada must collapse it.");
                VerifyTreeRowFocused(HierarchyRtlTable, "Ada", "Under RTL, focus must stay on Ada after Right collapses it.");

                KeyboardHelper.PressKey(Key.Left);
                if (WaitForHierarchyField(RtlTreeField, HierarchyAdaExpanded) != HierarchyAdaExpanded) { Verify.Fail("Precondition: Ada did not re-expand."); return; }
                if (FocusTreeRow(HierarchyRtlTable, "Ben") == null) { return; }
                KeyboardHelper.PressKey(Key.Right);
                VerifyTreeRowFocused(HierarchyRtlTable, "Ada", "Under RTL, Right on child Ben must focus its parent Ada.");
                Verify.IsTrue(IsSelected(FindTreeRow(HierarchyRtlTable, "Ada")), "Under RTL, moving to the parent must select Ada.");
            }
        }

        #endregion

        #region Pointer

        [TestMethod]
        [TestProperty("Description", "Verifies clicking a tree row's chevron toggles it: one click expands, the next collapses, and the row's ExpandCollapse state follows.")]
        public void ChevronClickTogglesTreeRow()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }

                UIObject ada = FindTreeRow(HierarchyTable, "Ada");
                if (ada == null) { return; }
                ClickRowAtColumnOffset(ada, HierarchyRootChevronCentreX);

                Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(TreeField, HierarchyAdaExpanded), "A chevron click must expand Ada.");
                ada = FindTreeRow(HierarchyTable, "Ada");
                if (ada == null) { return; }
                Verify.AreEqual(ExpandCollapseState.Expanded, new ExpandCollapseImplementation(ada).ExpandCollapseState, "Ada's peer must report Expanded after the click.");

                ClickRowAtColumnOffset(ada, HierarchyRootChevronCentreX);

                Verify.AreEqual(HierarchyAllCollapsed, WaitForHierarchyField(TreeField, HierarchyAllCollapsed), "A second chevron click must collapse Ada.");
                ada = FindTreeRow(HierarchyTable, "Ada");
                if (ada == null) { return; }
                Verify.AreEqual(ExpandCollapseState.Collapsed, new ExpandCollapseImplementation(ada).ExpandCollapseState, "Ada's peer must report Collapsed after the second click.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the chevron is not a cell: clicking or double-clicking it leaves the selection unchanged and begins no edit, while a double-click on a cell of the same table does begin one.")]
        public void ChevronClickDoesNotSelectOrEdit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectHierarchyPivotAndGetTable() == null) { return; }

                UIObject gus = FindTreeRow(HierarchyTable, "Gus");
                if (gus == null) { return; }
                SelectRow(gus);
                if (WaitForHierarchyField(SelectedField, "Gus") != "Gus") { Verify.Fail("Precondition: Gus could not be selected."); return; }

                UIObject ada = FindTreeRow(HierarchyTable, "Ada");
                if (ada == null) { return; }
                ClickRowAtColumnOffset(ada, HierarchyRootChevronCentreX);

                Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(TreeField, HierarchyAdaExpanded), "Precondition: the chevron click must expand Ada.");
                Verify.AreEqual("Gus", ReadHierarchyField(SelectedField), "A chevron click must not change the selection.");
                Verify.AreEqual("0", ReadHierarchyField(BeginningEditField), "A chevron click must not begin an edit.");

                // Two quick presses on the chevron are two toggles, never a cell double-click.
                ada = FindTreeRow(HierarchyTable, "Ada");
                if (ada == null) { return; }
                DoubleClickRowAtColumnOffset(ada, HierarchyRootChevronCentreX);

                Verify.AreEqual(HierarchyAdaExpanded, WaitForHierarchyField(TreeField, HierarchyAdaExpanded), "A chevron double-press must toggle twice, leaving Ada expanded.");
                Verify.AreEqual("Gus", ReadHierarchyField(SelectedField), "A chevron double-press must not change the selection.");
                Verify.AreEqual("0", ReadHierarchyField(BeginningEditField), "A chevron double-press must not begin an edit.");
                Verify.AreEqual("False", ReadHierarchyField(EditingField), "No editor may be open after a chevron double-press.");

                Log.Comment("Control: a double-click on a plain cell of the same table does begin an edit, so the readout can observe one.");
                UIObject eve = FindTreeRow(HierarchyTable, "Eve");
                if (eve == null) { return; }
                DoubleClickRowAtColumnOffset(eve, HierarchyDeptCellCentreX);
                Verify.AreEqual("1", WaitForHierarchyField(BeginningEditField, "1"), "A double-click on Eve's Dept cell must begin an edit.");

                KeyboardHelper.PressKey(Key.Escape);
            }
        }

        #endregion

        #region Helpers

        // Switches to the Hierarchy pivot, resets the fixture and returns HierarchyTable, or null after failing.
        internal static UIObject SelectHierarchyPivotAndGetTable()
        {
            if (!SelectPivotItem(HierarchyPivotItem))
            {
                Verify.Fail(GoToButton(HierarchyPivotItem) + " was not found.");
                return null;
            }

            var reset = FindElement.ById<Button>(ResetHierarchyButton);
            if (reset == null)
            {
                Verify.Fail(ResetHierarchyButton + " was not found.");
                return null;
            }

            reset.InvokeAndWait();
            Wait.ForIdle();
            ElementCache.Clear();

            UIObject tableView = FindElement.ById(HierarchyTable);
            if (tableView == null)
            {
                Verify.Fail(HierarchyTable + " was not found after selecting the Hierarchy pivot item.");
                return null;
            }

            string tree = WaitForHierarchyField(TreeField, HierarchyAllCollapsed);
            string rtlTree = WaitForHierarchyField(RtlTreeField, HierarchyAllCollapsed);
            if (tree != HierarchyAllCollapsed || rtlTree != HierarchyAllCollapsed)
            {
                Verify.Fail(string.Format("Precondition: both tables must start all collapsed (tree = '{0}', rtl = '{1}').", tree, rtlTree));
                return null;
            }

            return tableView;
        }

        // The value of one "key=value" field of HierarchyReadout, or null if the field is absent.
        private static string ReadHierarchyField(string field)
        {
            string readout = ReadPageReadout(HierarchyReadout);
            if (readout == null)
            {
                return null;
            }

            foreach (string part in readout.Split(';'))
            {
                int separator = part.IndexOf('=');
                if (separator > 0 && part.Substring(0, separator) == field)
                {
                    return part.Substring(separator + 1);
                }
            }

            return null;
        }

        // The page refreshes the readout at Low dispatcher priority; a few idle waits let it settle. Returns the last
        // value read, so the caller asserts on what was actually observed.
        private static string WaitForHierarchyField(string field, string expected)
        {
            string value = ReadHierarchyField(field);
            for (int attempt = 0; attempt < 5 && value != expected; attempt++)
            {
                Wait.ForIdle();
                value = ReadHierarchyField(field);
            }

            Log.Comment("Readout {0} = '{1}' (expected '{2}').", field, value, expected);
            return value;
        }

        // A row peer's Name is its cell texts in visible order, so it starts with the Name column.
        private static bool IsTreeRowNamed(UIObject row, string name) =>
            IsRow(row) && row.Name != null && row.Name.StartsWith(name, StringComparison.Ordinal);

        private static UIObject FindTreeRow(string tableAutomationId, string name)
        {
            ElementCache.Clear();
            UIObject rowsHost = GetRowsHost(tableAutomationId);
            if (rowsHost == null) { return null; }

            foreach (UIObject child in rowsHost.Children)
            {
                if (IsTreeRowNamed(child, name))
                {
                    return child;
                }
            }

            Verify.Fail(string.Format("Row '{0}' was not found in {1}.", name, tableAutomationId));
            return null;
        }

        // Focuses the named row through UIA and checks it took focus; null (after failing) otherwise.
        private static UIObject FocusTreeRow(string tableAutomationId, string name)
        {
            UIObject row = FindTreeRow(tableAutomationId, name);
            if (row == null) { return null; }

            row.SetFocus();
            Wait.ForIdle();
            if (!row.HasKeyboardFocus)
            {
                Verify.Fail(string.Format("Precondition: row '{0}' of {1} did not take focus.", name, tableAutomationId));
                return null;
            }

            return row;
        }

        // True when the named row container itself (not one of its cells) holds keyboard focus.
        private static bool IsTreeRowFocused(string tableAutomationId, string name)
        {
            ElementCache.Clear();
            UIObject focused = FindFocusedRow(GetRowsHost(tableAutomationId));
            return focused != null && IsTreeRowNamed(focused, name);
        }

        private static void VerifyTreeRowFocused(string tableAutomationId, string name, string message)
        {
            ElementCache.Clear();
            UIObject focused = FindFocusedBodyElement(GetRowsHost(tableAutomationId));
            Log.Comment("Focused row: {0} (cell {1}); expected row '{2}' itself.",
                focused == null ? "<none>" : focused.Name,
                focused == null ? -1 : IndexOfFocusedCell(focused),
                name);
            Verify.IsTrue(IsTreeRowFocused(tableAutomationId, name), message);
        }

        private static bool ExpandWithRight(string tableAutomationId, string name, string treeField, string expectedTree)
        {
            if (FocusTreeRow(tableAutomationId, name) == null) { return false; }

            KeyboardHelper.PressKey(Key.Right);
            if (WaitForHierarchyField(treeField, expectedTree) != expectedTree)
            {
                Verify.Fail(string.Format("Precondition: Right did not expand '{0}'.", name));
                return false;
            }

            return true;
        }

        private static bool ExpandWithUia(string tableAutomationId, string name, string expectedTree)
        {
            UIObject row = FindTreeRow(tableAutomationId, name);
            if (row == null) { return false; }

            new ExpandCollapseImplementation(row).Expand();
            Wait.ForIdle();
            if (WaitForHierarchyField(TreeField, expectedTree) != expectedTree)
            {
                Verify.Fail(string.Format("Precondition: Expand did not expand '{0}'.", name));
                return false;
            }

            return true;
        }

        #endregion
    }
}
