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
using Point = System.Drawing.Point;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView pointer / group-header interaction tests.
    // Scope: interaction test plan (docs\design-notes\TabularControls\TableView-interaction-test-plan.md)
    // §3 "Pointer selection and focus" and §4 "Group header input".
    //
    // These run out of process and reach the control only through the real UIA provider tree. They own the
    // GESTURE ROUTE: they prove real pointer/keyboard input reaches the same selection and expand/collapse
    // state machine that TableView_Selection_APITests.cs and TableView_Grouping_APITests.cs cover
    // programmatically. They deliberately do not re-assert the state machine's permutations — only that a
    // gesture arrives at it.
    //
    // CRASH CONSTRAINT (product finding #13, measured this session): asking a TableViewRow peer for its
    // children crashes the app (0xC0000420 in Microsoft.UI.Xaml.dll) because
    // TableViewRowAutomationPeer::GetChildrenCore manufactures fresh cell peers per call. Therefore NOTHING
    // here descends into a row's children: no FindElement by cell text, no row.Children enumeration. All
    // assertions are made at the row level (SelectionItem pattern, keyboard focus, BoundingRectangle) or by
    // counting the row/group-header peers that are direct children of the rows host. Cells are exercised, when
    // needed, by clicking the row at a coordinate offset — never by resolving a cell peer.
    [TestClass]
    public class TableViewPointerInteractionTests
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

        #region 3. Pointer selection and focus

        [TestMethod]
        [TestProperty("Description", "Verifies a pointer click on a row selects it (SelectionItem pattern) and moves keyboard focus to it, taking focus away from an unrelated control.")]
        public void PointerClickSelectsAndFocusesRow()
        {
            // Interaction plan §3 PointerClickSelectsRow.
            // Spec: dev-spec Gesture layer (TableView-dev-spec.md:284) - PointerPressed establishes the row's
            //   participation in selection and the current cell; :296-301 - pointer focus lands on the ROW.
            // API twin: TableView_Selection_APITests.cs drives Select() on the UI thread. This test proves the
            //   pointer route reaches the same selection, which no API test can observe.
            // Failure means: hit-testing or the row PointerPressed handler never reaches selection/focus, so the
            //   control is unusable by a mouse even though the programmatic path passes.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = FindElement.ById("BasicTableView");
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                Button dummyButton = FindElement.ById<Button>("DummyButton");
                if (dummyButton == null)
                {
                    Verify.Fail("DummyButton was not found on the test page.");
                    return;
                }

                UIObject row = GetRow(tableView, 0);
                if (row == null)
                {
                    Verify.Fail("The first realized TableViewRow peer was not found under the rows host.");
                    return;
                }

                // Park focus on an unrelated control so the focus-move assertion cannot pass vacuously.
                dummyButton.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(dummyButton.HasKeyboardFocus, "Precondition: DummyButton should hold keyboard focus before the click.");

                ClickRow(row);
                Wait.ForIdle();

                Verify.IsTrue(row.HasKeyboardFocus, "A pointer click must move keyboard focus to the clicked row (dev-spec:296-301).");
                Verify.IsFalse(dummyButton.HasKeyboardFocus, "Focus must leave the previously focused unrelated control.");
                var selectionItem = new SelectionItemImplementation<UIObject>(row, UIObject.Factory);
                Verify.IsTrue(selectionItem.IsAvailable, "Under SelectionMode.Single the row peer must advertise SelectionItem.");
                Verify.IsTrue(selectionItem.IsSelected, "A pointer click must select the row (dev-spec:284); the gesture must reach the same state Select() does.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clicking a second row moves single selection off the first row onto the second, matching SelectionMode.Single.")]
        public void PointerClickOnSecondRowMovesSelection()
        {
            // Interaction plan §3 PointerClickSelectsRow (the "clicking a second row moves selection" leg).
            // Spec: dev-spec:284 - the press selects; SelectionMode.Single (TableView.idl:536) permits one row.
            // Failure means: the pointer route writes selection additively or fails to clear the prior row, so a
            //   mouse user can hold two selected rows in a single-selection control.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = FindElement.ById("BasicTableView");
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                UIObject firstRow = GetRow(tableView, 0);
                UIObject secondRow = GetRow(tableView, 1);
                if (firstRow == null || secondRow == null)
                {
                    Verify.Fail("Two realized TableViewRow peers are required for this test.");
                    return;
                }

                ClickRow(firstRow);
                Wait.ForIdle();

                var firstSelection = new SelectionItemImplementation<UIObject>(firstRow, UIObject.Factory);
                Verify.IsTrue(firstSelection.IsSelected, "Precondition: the first row must be selected after the first click.");

                ClickRow(secondRow);
                Wait.ForIdle();

                var secondSelection = new SelectionItemImplementation<UIObject>(secondRow, UIObject.Factory);
                Verify.IsTrue(secondSelection.IsSelected, "Clicking the second row must select it.");
                Verify.IsTrue(secondRow.HasKeyboardFocus, "Clicking the second row must move keyboard focus to it.");

                // Re-query the first row's pattern after the reshape of selection state.
                var firstSelectionAfter = new SelectionItemImplementation<UIObject>(firstRow, UIObject.Factory);
                Verify.IsFalse(firstSelectionAfter.IsSelected, "SelectionMode.Single must clear the first row when the second is clicked.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that with SelectionMode.None a pointer click selects nothing (SelectionItem withheld) yet keyboard focus still moves to the clicked row.")]
        public void PointerClickInSelectionModeNoneSelectsNothingButMovesFocus()
        {
            // Interaction plan §3 PointerClickInSelectionModeNoneSelectsNothing.
            // Spec: TableView.idl:536 - None = "display-only"; the AutomationPeer contract
            //   (TableView_AutomationPeer_APITests.cs VerifyRowPeerSelectionItemPatternTracksSelectionMode)
            //   withholds SelectionItem under None. The interaction plan states focus STILL moves to the row.
            // Failure means: the None gate lives only inside Select() and the pointer handler writes selection
            //   behind it; or the row refuses focus in None, breaking keyboard reachability of a display-only table.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                ComboBox selectionModeComboBox = new ComboBox(FindElement.ById("SelectionModeComboBox"));
                if (selectionModeComboBox == null)
                {
                    Verify.Fail("SelectionModeComboBox was not found on the test page.");
                    return;
                }

                selectionModeComboBox.SelectItemByName("SelectionModeNone");
                Wait.ForIdle();

                UIObject tableView = FindElement.ById("BasicTableView");
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                Button dummyButton = FindElement.ById<Button>("DummyButton");
                UIObject row = GetRow(tableView, 0);
                if (dummyButton == null || row == null)
                {
                    Verify.Fail("DummyButton and a realized TableViewRow peer are required for this test.");
                    return;
                }

                dummyButton.SetFocus();
                Wait.ForIdle();

                ClickRow(row);
                Wait.ForIdle();

                // Selection must not happen: the row peer withholds the SelectionItem pattern under None. If the
                // product regressed and still advertised it, prove at least that nothing is selected.
                var selectionItem = new SelectionItemImplementation<UIObject>(row, UIObject.Factory);
                if (selectionItem.IsAvailable)
                {
                    Verify.IsFalse(selectionItem.IsSelected, "SelectionMode.None must not select a row on a pointer click, even if the pattern is (wrongly) advertised.");
                }
                else
                {
                    Log.Comment("SelectionItem is correctly withheld from the row peer under SelectionMode.None.");
                }

                // Focus must still move to the row: a display-only table remains keyboard-reachable.
                Verify.IsTrue(row.HasKeyboardFocus, "Focus must still move to the clicked row under SelectionMode.None (interaction plan §3).");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a pointer press establishes the current cell so a following F2 opens the edit on the clicked column, both on a fresh row and on one that already has focus.")]
        public void PointerPressEstablishesCurrentCellForKeyboardEditing()
        {
            // Interaction plan §3 PointerPressEstablishesCurrentCellForKeyboardEditing.
            // Spec: dev-spec:284 - the pointer handler is the ONLY place a pointer establishes the current
            //   cell, and without it keyboard editing "silently fails"; dev-spec:286 - once the row holds
            //   focus the press arrives with the row as OriginalSource and resolution falls back to
            //   hit-testing the pointer position. The second leg is therefore a different code path, not a
            //   repeat of the first.
            // Why Age is the load-bearing leg: a keyboard-only fallback resolves to the first editable
            //   column, which is Name. A Name-only test would pass on a control that ignores the pointer.
            // Observability: read through the page's BeginningEdit report (TableViewBeginningEditEventArgs
            //   .Column is public IDL). Locating the editor itself would mean descending into a row's
            //   children, which fail-fasts the app (finding #13).
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = FindElement.ById("BasicTableView");
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                Edit report = FindElement.ById<Edit>("EditColumnReportTextBlock");
                if (report == null)
                {
                    Verify.Fail("EditColumnReportTextBlock was not found on the test page.");
                    return;
                }

                UIObject row = GetRow(tableView, 0);
                if (row == null)
                {
                    Verify.Fail("The first realized TableViewRow peer was not found under the rows host.");
                    return;
                }

                Verify.AreEqual(string.Empty, ReadEditReport(), "Precondition: no edit should have been reported before the test acts.");

                // Authored column widths on BasicTableView: Name 160, Age 100, ReadOnlyCity 160, Score 100, Template 200.
                // Click x is expressed row-relative and converted to the center-relative offset InputHelper wants.
                ClickRowAtColumnOffset(row, 210);   // inside Age
                KeyboardHelper.PressKey(Key.F2);
                Wait.ForIdle();

                Verify.AreEqual("Age;", ReadEditReport(), "F2 after pressing inside the Age column must open the edit on Age, not on the first editable column (dev-spec:284).");

                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                // Second leg: the row now holds focus, so the press arrives with the row as source and the
                // control must fall back to hit-testing the pointer position (dev-spec:286).
                ClickRowAtColumnOffset(row, 80);    // inside Name
                KeyboardHelper.PressKey(Key.F2);
                Wait.ForIdle();

                Verify.AreEqual("Age;Name;", ReadEditReport(), "A press on an already-focused row must re-establish the current cell by hit-test, so F2 opens on Name (dev-spec:286).");

                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a row enters its PointerOver common state while the mouse is over it and returns to Normal when the mouse leaves.")]
        public void RowPointerOverEntersHoverState()
        {
            // Interaction plan §3 RowPointerOverEntersHoverState.
            // Spec: dev-spec:95 names the row's CommonStates - Normal, PointerOver, Pressed, Disabled.
            // Moved from API §5.5: m_isPointerOver is set only from the row's own pointer handlers, so no
            //   programmatic route enters the state.
            // Read through the page's transition LOG rather than a live query: any interaction that read the
            //   state would itself move the pointer off the row and destroy what it was measuring.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = FindElement.ById("BasicTableView");
                Button hookButton = FindElement.ById<Button>("HookRowStatesButton");
                Button dummyButton = FindElement.ById<Button>("DummyButton");
                if (tableView == null || hookButton == null || dummyButton == null)
                {
                    Verify.Fail("BasicTableView, HookRowStatesButton and DummyButton are all required for this test.");
                    return;
                }

                UIObject row = GetRow(tableView, 0);
                if (row == null)
                {
                    Verify.Fail("The first realized TableViewRow peer was not found under the rows host.");
                    return;
                }

                Edit stateLog = FindElement.ById<Edit>("RowStateLogTextBlock");
                if (stateLog == null)
                {
                    Verify.Fail("RowStateLogTextBlock was not found on the test page.");
                    return;
                }

                hookButton.InvokeAndWait();
                Wait.ForIdle();
                Verify.AreEqual("hooked;", stateLog.Value, "Precondition: the first row's CommonStates group must have been hooked.");

                HoverRow(row);
                Wait.ForIdle();

                Verify.IsTrue(stateLog.Value.Contains("PointerOver;"), $"Hovering a row must take it into the PointerOver common state (dev-spec:95). Log was '{stateLog.Value}'.");

                // Move well away from the table so the row sees a PointerExited.
                InputHelper.MoveMouse(dummyButton, 0, 0);
                Wait.ForIdle();

                string log = stateLog.Value;
                int hoverIndex = log.IndexOf("PointerOver;", System.StringComparison.Ordinal);
                Verify.IsTrue(log.IndexOf("Normal;", hoverIndex, System.StringComparison.Ordinal) > hoverIndex, $"Moving the pointer off the row must return it to Normal (dev-spec:95). Log was '{log}'.");
            }
        }

        #endregion

        #region 4. Group header input

        [TestMethod]
        [TestProperty("Description", "Verifies clicking a group header band collapses the group's rows and expands them again, with the header's ExpandCollapse peer agreeing at each step.")]
        public void GroupHeaderPointerClickTogglesRowVisibility()
        {
            // Interaction plan §4 GroupHeaderActivationExpandsAndCollapsesRows (pointer leg).
            // Spec: TableViewGroupHeader::RequestToggle is raised only from OnPointerReleased / OnKeyDown
            //   (TableViewGroupHeader.cpp), a different entry point than the peer's RequestExpansion that API §9
            //   drives - so this gesture route is not covered by the API test.
            // Expected: GroupedTableView has 9 items over 3 equal city groups (page code-behind), so all-expanded
            //   realizes 9 TableViewRow peers; collapsing one group drops it to 6; re-expanding returns to 9.
            // Failure means: the band click never reaches TableView::ToggleGroupExpansion, so grouping is not
            //   operable by pointer.
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
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "Baseline: the group header should report Expanded.");

                Log.Comment("Collapse the first group by clicking its header band.");
                InputHelper.LeftClick(groupHeader);
                Wait.ForIdle();

                int collapsedRows = CountRows(rowsHost);
                Verify.IsLessThan(collapsedRows, baselineRows, "Clicking the group header band must drop the count of realized rows (its group collapsed).");
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "After the click the header's ExpandCollapse peer must report Collapsed.");

                Log.Comment("Expand the first group again by clicking its header band.");
                InputHelper.LeftClick(groupHeader);
                Wait.ForIdle();

                int reexpandedRows = CountRows(rowsHost);
                Verify.AreEqual(baselineRows, reexpandedRows, "Re-clicking the header band must restore the original realized row count.");
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "After the second click the header must report Expanded again.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the group header's CommonStates follow the pointer: Normal to PointerOver on enter, Pressed while the button is down, back to PointerOver on release, and Normal when the pointer leaves.")]
        public void GroupHeaderPointerAndPressedVisualStates()
        {
            // Interaction plan §4 GroupHeaderPointerAndPressedVisualStates.
            // Spec: TableView.idl documents the group header's states as "CommonStates
            //   Normal|PointerOver|Pressed|Disabled" - a contract on the control, so the names are spec-derived,
            //   not read out of the implementation.
            // Route: these states are set only from the header's own pointer handlers, so there is no
            //   programmatic route in - the same reason the API tier records
            //   VerifyRowPointerOverVisualState as not implementable there (TableView_Rows_APITests.cs:664).
            //   API §9 covers ExpansionStates, which is a different group reached by a different mechanism.
            // Failure means: the group header gives no hover or press feedback, so a pointer user cannot tell
            //   the band is interactive before clicking it.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                // Everything on the page toolbar is resolved BEFORE the grouped table realizes: a FindElement
                // afterwards forces ElementCache.Refresh() to re-walk the whole tree, which descends into
                // TableViewRow children and asserts the app (finding #13, measured).
                Button hookButton = FindElement.ById<Button>("HookGroupHeadersButton");
                Edit stateLog = FindElement.ById<Edit>("GroupHeaderStateLogTextBlock");
                UIObject awayTarget = FindElement.ById("DummyButton");
                if (hookButton == null || stateLog == null || awayTarget == null)
                {
                    Verify.Fail("The page's group-header instrumentation was not found.");
                    return;
                }

                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                UIObject groupHeader = GetGroupHeaderAt(rowsHost, 0);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                hookButton.InvokeAndWait();
                Verify.AreEqual("hooked;", stateLog.Value, "Precondition: the page must have hooked the first group header's CommonStates group.");

                var bounds = groupHeader.BoundingRectangle;
                var headerPoint = new Point(bounds.Left + (bounds.Width / 2), bounds.Top + (bounds.Height / 2));
                var awayBounds = awayTarget.BoundingRectangle;
                var awayPoint = new Point(awayBounds.Left + (awayBounds.Width / 2), awayBounds.Top + (awayBounds.Height / 2));

                Log.Comment("Move the pointer onto the header band.");
                PointerInput.Move(headerPoint);
                Wait.ForIdle();

                Log.Comment("Press and hold on the band, then release it.");
                PointerInput.Press(PointerButtons.Primary);
                Wait.ForIdle();
                PointerInput.Release(PointerButtons.Primary);
                Wait.ForIdle();

                Log.Comment("Move the pointer off the table entirely.");
                PointerInput.Move(awayPoint);
                Wait.ForIdle();

                // The whole gesture is driven first and the log read once at the end: reading between steps
                // would be safe, but asserting between them would not - Verify throws in this suite, so an
                // early failure would leave the later transitions unmeasured and undiagnosable.
                string log = stateLog.Value;
                Log.Comment("Group header CommonStates log: {0}", log);
                Verify.AreEqual("hooked;PointerOver;Pressed;PointerOver;Normal;", log,
                    "The header must enter PointerOver on hover, Pressed while held, return to PointerOver on release, and Normal when the pointer leaves.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a pointer activation of a group header band raises ToggleRequested exactly once, carrying that group's own key, and that the key survives a handler that mutates the header re-entrantly.")]
        public void GroupHeaderPointerToggleRaisesToggleRequestedWithGroupKey()
        {
            // Interaction plan §4 GroupHeaderToggleRequestedCarriesTheGroupKey (pointer leg).
            // Spec: TableView.idl:350-355 - "Carried on the args so a handler that re-enters and mutates the
            //   header still sees the key that was actually activated." The page handler therefore reads the
            //   key, flips sender.IsExpanded, and reads the key again; the report records "<key>|<key>;" per
            //   raise, so the entry count is the number of raises.
            // Route: RequestToggle is raised only from OnKeyDown and OnPointerReleased, never from the
            //   automation peer's Expand/Collapse - which is why this moved out of the API plan
            //   (TableView_Grouping_APITests.cs:575).
            // Fixture: the page builds the grouped source from Cities = { Redmond, Seattle, Bellevue } in
            //   source order, so the first two groups are Redmond and Seattle. Driving TWO headers is the
            //   point: a constant key would satisfy a single-header test.
            // Failure means: a handler cannot tell which group the user activated, so per-group behaviour
            //   built on the event is impossible.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                Button hookButton = FindElement.ById<Button>("HookGroupHeadersButton");
                Edit toggleReport = FindElement.ById<Edit>("GroupToggleReportTextBlock");
                if (hookButton == null || toggleReport == null)
                {
                    Verify.Fail("The page's group-header instrumentation was not found.");
                    return;
                }

                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                UIObject firstHeader = GetGroupHeaderAt(rowsHost, 0);
                UIObject secondHeader = GetGroupHeaderAt(rowsHost, 1);
                if (firstHeader == null || secondHeader == null)
                {
                    Verify.Fail("The grouped table should realize at least two group headers.");
                    return;
                }

                hookButton.InvokeAndWait();
                Verify.AreEqual("hooked;", toggleReport.Value, "Precondition: the page must have hooked ToggleRequested on the realized group headers.");

                Log.Comment("Activate the FIRST group's header band.");
                ClickPoint(CentreOf(firstHeader));
                Verify.AreEqual("hooked;Redmond|Redmond;", toggleReport.Value,
                    "One band click must raise ToggleRequested exactly once, carrying the first group's key both before and after the handler mutates the header.");

                Log.Comment("Activate the SECOND group's header band.");
                ClickPoint(CentreOf(secondHeader));
                Verify.AreEqual("hooked;Redmond|Redmond;Seattle|Seattle;", toggleReport.Value,
                    "The second header must raise the event once more and carry ITS OWN key, not the first group's.");
            }
        }


        public void ExpandAllGroupsReconcilesGestureCollapsedGroup()
        {
            // Interaction plan §4 "expand-all / collapse-all driven from the page controls reconcile with header state".
            // This owns the RECONCILIATION between the gesture route (TableViewGroupHeader::RequestToggle) and the
            //   programmatic route (TableView::ExpandAllGroups, driven here by the page button). API §9 already covers
            //   ExpandAllGroups on its own; the interesting claim here is that a group collapsed by a real gesture is
            //   healed by the bulk API - i.e. the header's visual/peer state and the source's expansion state are one
            //   store, not two.
            // Failure means: the two paths keep separate state and drift apart - the gesture-collapsed header stays
            //   Collapsed (or its rows stay unrealized) even though ExpandAllGroups ran. Invisible to a purely
            //   programmatic test, because that test never collapsed the group by gesture in the first place.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                Button expandAllButton = FindElement.ById<Button>("ExpandAllGroupsButton");
                if (expandAllButton == null)
                {
                    Verify.Fail("ExpandAllGroupsButton was not found on the test page.");
                    return;
                }

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                int baselineRows = CountRows(rowsHost);
                Verify.IsGreaterThan(baselineRows, 0, "All groups start expanded, so the baseline realized row count should be non-zero.");

                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                var expandCollapse = new ExpandCollapseImplementation(groupHeader);
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "Baseline: the first group header should report Expanded.");

                Log.Comment("Collapse the first group by gesture (click its header band).");
                InputHelper.LeftClick(groupHeader);
                Wait.ForIdle();
                Verify.IsLessThan(CountRows(rowsHost), baselineRows, "Precondition: the gesture must collapse the first group and drop the realized row count.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "Precondition: the gesture-collapsed header must report Collapsed.");

                Log.Comment("Expand all groups through the page button (GroupedTableView.ExpandAllGroups).");
                InputHelper.LeftClick(expandAllButton);
                Wait.ForIdle();

                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState,
                    "ExpandAllGroups must re-expand the group that a gesture collapsed; the header peer must report Expanded again.");
                Verify.AreEqual(baselineRows, CountRows(rowsHost),
                    "ExpandAllGroups must restore the full realized row count, reconciling with the gesture-collapsed group.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clicking CollapseAllGroupsButton re-collapses a group that was expanded by gesture: the header peer reports Collapsed and the realized row count returns to the all-collapsed baseline.")]
        public void CollapseAllGroupsReconcilesGestureExpandedGroup()
        {
            // Interaction plan §4 "expand-all / collapse-all driven from the page controls reconcile with header state"
            //   (mirror of the expand-all case).
            // Failure means: a group expanded by gesture is left Expanded (its rows still realized) after
            //   CollapseAllGroups runs, i.e. the gesture wrote expansion state the bulk API cannot see.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                Button collapseAllButton = FindElement.ById<Button>("CollapseAllGroupsButton");
                if (collapseAllButton == null)
                {
                    Verify.Fail("CollapseAllGroupsButton was not found on the test page.");
                    return;
                }

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                int expandedBaselineRows = CountRows(rowsHost);
                Verify.IsGreaterThan(expandedBaselineRows, 0, "All groups start expanded, so the realized row count should be non-zero.");

                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                var expandCollapse = new ExpandCollapseImplementation(groupHeader);

                // Establish an all-collapsed baseline through the page button, then gesture-EXPAND the first group so
                // the two routes disagree before the reconcile.
                Log.Comment("Collapse all groups through the page button (GroupedTableView.CollapseAllGroups).");
                InputHelper.LeftClick(collapseAllButton);
                Wait.ForIdle();

                // Both observations are taken and logged BEFORE either is asserted. TAEF's Verify unwinds the
                // test on the first failure here, so asserting the row count first would hide the group state -
                // and those two answers distinguish "CollapseAllGroups did nothing" from "the group state
                // changed but the rows were never de-realized", which are different product bugs.
                int collapsedBaselineRows = CountRows(rowsHost);
                ExpandCollapseState stateAfterCollapseAll = expandCollapse.ExpandCollapseState;
                Log.Comment("After CollapseAllGroups: realized rows = {0} (expanded baseline {1}), first group header reports {2}.",
                    collapsedBaselineRows, expandedBaselineRows, stateAfterCollapseAll);

                Verify.AreEqual(ExpandCollapseState.Collapsed, stateAfterCollapseAll, "Precondition: the first group header must report Collapsed after CollapseAllGroups.");
                Verify.IsLessThan(collapsedBaselineRows, expandedBaselineRows, "Precondition: collapsing all groups must drop the realized row count below the expanded baseline.");

                Log.Comment("Expand the first group by gesture (click its header band).");
                InputHelper.LeftClick(groupHeader);
                Wait.ForIdle();
                Verify.IsGreaterThan(CountRows(rowsHost), collapsedBaselineRows, "Precondition: the gesture must expand the first group and raise the realized row count.");
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "Precondition: the gesture-expanded header must report Expanded.");

                Log.Comment("Collapse all groups again through the page button.");
                InputHelper.LeftClick(collapseAllButton);
                Wait.ForIdle();

                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState,
                    "CollapseAllGroups must re-collapse the group that a gesture expanded; the header peer must report Collapsed again.");
                Verify.AreEqual(collapsedBaselineRows, CountRows(rowsHost),
                    "CollapseAllGroups must restore the all-collapsed realized row count, reconciling with the gesture-expanded group.");
            }
        }

        #endregion

        #region Helpers

        // Row-relative x of a plain text cell (inside the Age column: Name is 0-160, Age is 160-260).
        // Any cell would do for a selection click; a text cell is used so nothing depends on template content.
        private const int TextCellRelativeX = 210;

        // Reads the page's BeginningEdit report. Resolved fresh each call but WITHOUT ElementCache.Clear():
        // clearing the cache forces the next FindElement to re-walk the whole visual tree, and that walk
        // descends into TableViewRow children - the peer-manufacturing path of finding #13 - which asserts
        // the app (0xC0000420 in Microsoft.UI.Xaml.dll, measured in ptrrun2). UIA property reads are live, so
        // a plain Value read already sees the current text.
        private static string ReadEditReport()
        {
            Edit report = FindElement.ById<Edit>("EditColumnReportTextBlock");
            return report == null ? null : report.Value;
        }

        // ALL pointer input below is expressed as ABSOLUTE SCREEN POINTS derived from BoundingRectangle, and
        // never as a UIObject + offset. Two reasons, both measured:
        //   1. PRODUCT FINDING #14 - every InputHelper offset overload (LeftClick, MoveMouse, LeftMouseButtonDown)
        //      resolves its anchor with IUIAutomationElement::GetClickablePoint, and that call access-violates
        //      the app (0xC0000005) on a TableViewRow peer, because UIA computes the point by walking into the
        //      row's children and trips the same peer bug as finding #13.
        //   2. The offset overloads that do NOT crash are anchored inconsistently - some at the clickable point
        //      (centre), some at the upper-left corner - so a row-relative column x cannot be expressed
        //      reliably as an offset. ptrrun2 proved this the hard way: clicks at a centre-relative offset
        //      landed off the table entirely and every selection and focus assertion failed.
        // The WebView2 interaction tests use the same absolute-point workaround for the same GetClickablePoint
        // fault (see their Task 30555367 comments).
        private static void ClickPoint(Point point)
        {
            Log.Comment("Click at absolute point ({0}, {1}).", point.X, point.Y);
            PointerInput.Move(point);
            PointerInput.Press(PointerButtons.Primary);
            PointerInput.Release(PointerButtons.Primary);
            Wait.ForIdle();
        }

        private static void ClickRow(UIObject row)
        {
            ClickRowAtColumnOffset(row, TextCellRelativeX);
        }

        private static void HoverRow(UIObject row)
        {
            var bounds = row.BoundingRectangle;
            InputHelper.MoveMouse(new Point(bounds.Left + TextCellRelativeX, bounds.Top + (bounds.Height / 2)));
        }

        // Clicks the row at a row-relative x, so a caller can name a column by its authored width.
        private static void ClickRowAtColumnOffset(UIObject row, int rowRelativeX)
        {
            var bounds = row.BoundingRectangle;
            ClickPoint(new Point(bounds.Left + rowRelativeX, bounds.Top + (bounds.Height / 2)));
        }

        // Returns the rows host (last child of the TableView) and the row peer at index, or null. Never descends
        // into the row's own children - the crash constraint forbids that.
        private static UIObject GetRow(UIObject tableView, int index)
        {
            UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
            if (rowsHost == null || index >= rowsHost.Children.Count)
            {
                return null;
            }

            UIObject candidate = rowsHost.Children[index];
            if (candidate == null || candidate.ClassName == null || !candidate.ClassName.Contains("TableViewRow"))
            {
                return null;
            }

            return candidate;
        }

        // Counts the TableViewRow peers directly under the rows host. This is row-level (one hop from the rows
        // host to the row peers) and therefore safe; it never asks a row for its children.
        // Clears the element cache first: a collapse driven from OUTSIDE the table (the page's
        // CollapseAllGroups button) changes the rows host's children without any interaction inside the tree
        // to invalidate MITA's cache, so a cached walk can report the pre-collapse count.
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

        // Group headers and rows are siblings under the rows host; the header's class name is "TableViewGroupHeader"
        // (TableViewGroupHeaderAutomationPeer::GetClassNameCore), distinct from the row's "...TableViewRow".
        private static UIObject GetFirstGroupHeader(UIObject rowsHost)
        {
            return GetGroupHeaderAt(rowsHost, 0);
        }

        // The index-th group header among the rows host's own children. Sibling-level only; never asks a row
        // or a header for its children.
        private static UIObject GetGroupHeaderAt(UIObject rowsHost, int index)
        {
            int seen = 0;
            foreach (UIObject child in rowsHost.Children)
            {
                if (child != null && child.ClassName != null && child.ClassName.Contains("GroupHeader"))
                {
                    if (seen == index)
                    {
                        return child;
                    }
                    seen++;
                }
            }

            return null;
        }

        private static Point CentreOf(UIObject element)
        {
            var bounds = element.BoundingRectangle;
            return new Point(bounds.Left + (bounds.Width / 2), bounds.Top + (bounds.Height / 2));
        }

        // Switches the page Pivot to the Grouped item, then returns GroupedTableView.
        // Uses the page's GoToGroupedButton (found by AutomationId) rather than clicking the Pivot
        // header by name: a name-based UIA search makes the provider compute names for realized
        // TableViewRow peers, which manufactures cell peers and trips finding #13 (0xC0000420).
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
