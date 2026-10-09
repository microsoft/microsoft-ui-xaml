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

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView pointer interaction tests: pointer selection and focus, and group header input.
    //
    // These run out of process and reach the control only through the real UIA provider tree. They own the
    // GESTURE ROUTE: they prove real pointer/keyboard input reaches the same selection and expand/collapse
    // state machine that TableView_Selection_APITests.cs and TableView_Grouping_APITests.cs cover
    // programmatically. They deliberately do not re-assert the state machine's permutations — only that a
    // gesture arrives at it.
    //
    // Product finding #13 (a client asking a row peer for its children fail-fasted the app) shaped the older tests in
    // this file: they observe rows at row level, point at cells by coordinates, and read editors and visual states
    // through in-process page readouts. #11820 fixed the peers, and cell peers are now read safely; see the history
    // note in TableViewInteractionTestHelpers. The older tests are kept as written - their techniques still work.
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
            RestartAppIfLongRunning();
        }

        #region 3. Pointer selection and focus

        [TestMethod]
        [TestProperty("Description", "Verifies a pointer click on a cell selects its row (SelectionItem pattern) and moves keyboard focus to that cell, taking focus away from an unrelated control.")]
        public void PointerClickSelectsAndFocusesCell()
        {
            // Previously named PointerClickSelectsAndFocusesRow.
            // Spec: dev-spec:284 - PointerPressed establishes selection participation and the current cell. Focus
            //   on the pressed CELL is the #11820 model (spec debt: dev-spec:427 still says the row).
            // Clicks Age (column 1), not Name (column 0): column 0 is where keyboard drill-in lands, so a Name click
            //   could not tell "focused the clicked cell" from "focused the first cell".
            // Failure means: the click never reaches selection, or focus lands somewhere other than the cell under
            //   the pointer, so a screen reader names a different cell from the one clicked.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                Button dummyButton = FindElement.ById<Button>(DummyButton);
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

                int focusedCell = IndexOfFocusedCell(row);
                Log.Comment("After the click: focused cell index={0}, row focused={1}.", focusedCell, row.HasKeyboardFocus);

                Verify.AreEqual(AgeColumn, focusedCell, "A pointer click must move keyboard focus to the clicked cell (Age, visible column 1).");
                Verify.IsFalse(dummyButton.HasKeyboardFocus, "Focus must leave the previously focused unrelated control.");
                var selectionItem = new SelectionItemImplementation<UIObject>(row, UIObject.Factory);
                Verify.IsTrue(selectionItem.IsAvailable, "Under SelectionMode.Single the row peer must advertise SelectionItem.");
                Verify.IsTrue(selectionItem.IsSelected, "A pointer click must select the row (dev-spec:284); the gesture must reach the same state Select() does.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clicking a second row moves single selection off the first row onto the second, and focus onto the clicked cell of the second row.")]
        public void PointerClickOnSecondRowMovesSelection()
        {
            // Spec: dev-spec:284 - the press selects; SelectionMode.Single (TableView.idl:536) permits one row. The
            //   focus target is the clicked cell (#11820 model; spec debt dev-spec:427).
            // Failure means: the pointer route writes selection additively or fails to clear the prior row, so a
            //   mouse user can hold two selected rows in a single-selection control.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
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
                Verify.AreEqual(AgeColumn, IndexOfFocusedCell(secondRow), "Clicking the second row must move keyboard focus to its clicked cell (Age).");

                // Re-query the first row's pattern after the reshape of selection state.
                var firstSelectionAfter = new SelectionItemImplementation<UIObject>(firstRow, UIObject.Factory);
                Verify.IsFalse(firstSelectionAfter.IsSelected, "SelectionMode.Single must clear the first row when the second is clicked.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that with SelectionMode.None a pointer click selects nothing (SelectionItem withheld) yet keyboard focus still moves to the clicked cell.")]
        public void PointerClickInSelectionModeNoneSelectsNothingButMovesFocus()
        {
            // Spec: TableView.idl:536 - None = "display-only"; display-only restricts selection, not reachability.
            //   The focus target is the clicked cell (#11820 model; spec debt dev-spec:427).
            // Failure means: the None gate lives only inside Select() and the pointer handler writes selection
            //   behind it; or the cell refuses focus in None, breaking keyboard/AT reachability of a display-only table.
            using (var setup = new TestSetupHelper(PageName))
            {
                ComboBox selectionModeComboBox = new ComboBox(FindElement.ById(SelectionModeComboBox));
                if (selectionModeComboBox == null)
                {
                    Verify.Fail("SelectionModeComboBox was not found on the test page.");
                    return;
                }

                selectionModeComboBox.SelectItemByName("SelectionModeNone");
                Wait.ForIdle();

                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                Button dummyButton = FindElement.ById<Button>(DummyButton);
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

                // Focus must still move to the clicked cell: a display-only table remains keyboard-reachable.
                Verify.AreEqual(AgeColumn, IndexOfFocusedCell(row), "Focus must still move to the clicked cell under SelectionMode.None.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a pointer press establishes the current cell so a following F2 opens the edit on the clicked column, both on a fresh row and on one that already has focus.")]
        public void PointerPressEstablishesCurrentCellForKeyboardEditing()
        {
            // Spec: dev-spec:284 - the pointer handler is the ONLY place a pointer establishes the current
            //   cell, and without it keyboard editing "silently fails". Since #11820 the first press focuses the
            //   pressed CELL, so the second leg is "a cell is already focused and a different cell is pressed" -
            //   cell-to-cell transfer of the current cell - rather than dev-spec:286's row-source hit-test fallback.
            // Why Age is the load-bearing leg: a keyboard-only fallback resolves to the first editable
            //   column, which is Name. A Name-only test would pass on a control that ignores the pointer.
            // Observability: read through the page's BeginningEdit report (TableViewBeginningEditEventArgs
            //   .Column is public IDL), an independent data-side channel.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                Edit report = FindElement.ById<Edit>(EditColumnReport);
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
                ClickRowAtColumnOffset(row, BasicColumnCentreX(AgeColumn));
                KeyboardHelper.PressKey(Key.F2);
                Wait.ForIdle();

                Verify.AreEqual("Age;", ReadEditReport(), "F2 after pressing inside the Age column must open the edit on Age, not on the first editable column (dev-spec:284).");

                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                // Second leg: the row now holds focus, so the press arrives with the row as source and the
                // control must fall back to hit-testing the pointer position (dev-spec:286).
                ClickRowAtColumnOffset(row, BasicColumnCentreX(NameColumn));
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
            // Spec: dev-spec:95 names the row's CommonStates - Normal, PointerOver, Pressed, Disabled.
            // Moved from API §5.5: m_isPointerOver is set only from the row's own pointer handlers, so no
            //   programmatic route enters the state.
            // Read through the page's transition LOG rather than a live query: any interaction that read the
            //   state would itself move the pointer off the row and destroy what it was measuring.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                Button hookButton = FindElement.ById<Button>(HookRowStatesButton);
                Button dummyButton = FindElement.ById<Button>(DummyButton);
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

                Edit stateLog = FindElement.ById<Edit>(RowStateLog);
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
            // Pointer leg.
            // Spec: TableViewGroupHeader::RequestToggle is raised only from OnPointerReleased / OnKeyDown
            //   (TableViewGroupHeader.cpp), a different entry point than the peer's RequestExpansion that API §9
            //   drives - so this gesture route is not covered by the API test.
            // Expected: GroupedTableView has 9 items over 3 equal city groups (page code-behind), so all-expanded
            //   realizes 9 TableViewRow peers; collapsing one group drops it to 6; re-expanding returns to 9.
            // Failure means: the band click never reaches TableView::ToggleGroupExpansion, so grouping is not
            //   operable by pointer.
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
        [TestProperty("Description", "Verifies a pointer click on a group header band moves keyboard focus to that header, taking it away from an unrelated control, and that the next key acts on that header.")]
        public void GroupHeaderPointerClickMovesKeyboardFocus()
        {
            // Spec: dev-spec "Pointer" - a click on a row or cell makes it the keyboard focus target; the group
            //   header band is the same kind of body container (ListViewItem/TreeViewItem do the same).
            // The click also collapses the group, and the reshape recycles the header container, so the header is
            //   re-found after the reshape and the test proves the CONSEQUENCE (Enter re-opens the same group)
            //   rather than trusting a HasKeyboardFocus read on a UIObject resolved before the click.
            // Failure means: the click toggles the group but keyboard focus stays on whatever had it before, so
            //   the next Left/Right/Enter acts somewhere other than the band the user just clicked.
            using (var setup = new TestSetupHelper(PageName))
            {
                // Resolve toolbar controls before the grouped table realizes (see GroupHeaderPointerAndPressedVisualStates).
                Button dummyButton = FindElement.ById<Button>(DummyButton);
                if (dummyButton == null)
                {
                    Verify.Fail("DummyButton was not found on the test page.");
                    return;
                }

                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    Verify.Fail("The grouped table was not found.");
                    return;
                }

                UIObject rowsHost = GetRowsHost(tableView);
                int baselineRows = CountRows(rowsHost);
                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }
                string headerName = groupHeader.Name;
                Verify.IsNull(FindSelectedRow(rowsHost), "Precondition: no row is selected before the click.");

                dummyButton.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(dummyButton.HasKeyboardFocus, "Precondition: DummyButton should hold keyboard focus before the click.");

                InputHelper.LeftClick(groupHeader);
                Wait.ForIdle();

                // Observe everything before asserting anything (Verify throws): the collapse, the fresh header and
                // where focus went are separate bugs.
                rowsHost = GetRowsHost(tableView);
                int rowsAfterClick = CountRows(rowsHost);
                UIObject freshHeader = GetFirstGroupHeader(rowsHost);
                ExpandCollapseState stateAfterClick = freshHeader == null
                    ? ExpandCollapseState.LeafNode
                    : new ExpandCollapseImplementation(freshHeader).ExpandCollapseState;
                bool freshHasFocus = freshHeader != null && freshHeader.HasKeyboardFocus;
                Log.Comment("After the click: rows={0} (baseline {1}), header '{2}' state={3}, fresh header focused={4}, focused element {5}.",
                    rowsAfterClick, baselineRows, freshHeader == null ? "<none>" : freshHeader.Name, stateAfterClick, freshHasFocus, DescribeFocused());

                if (freshHeader == null) { Verify.Fail("The first group header must still be realized after it collapses."); return; }
                Verify.AreEqual(headerName, freshHeader.Name, "The first group header must still name the same group.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, stateAfterClick, "Precondition: the click must collapse the group.");
                Verify.IsLessThan(rowsAfterClick, baselineRows, "Precondition: the collapse must remove the group's rows.");
                Verify.IsTrue(freshHasFocus, "A pointer click on a group header band must move keyboard focus to that header.");
                Verify.IsFalse(dummyButton.HasKeyboardFocus, "Focus must leave the previously focused unrelated control.");

                // The consequence: the next key acts on the header that was clicked.
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                rowsHost = GetRowsHost(tableView);
                Verify.AreEqual(baselineRows, CountRows(rowsHost), "Enter after the click must re-open the clicked group.");
                Verify.IsNull(FindSelectedRow(rowsHost), "Clicking a group header must not select a row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a press on a group header band that is dragged off before release neither toggles the group nor moves keyboard focus.")]
        public void GroupHeaderPointerPressDraggedOffDoesNothing()
        {
            // Click semantics: activation happens on release INSIDE the band. The header takes focus on that release,
            //   not on press, so that moving focus off an open editor (which commits it on a later turn and can
            //   reshape the table) never happens in the middle of a gesture that may still be cancelled.
            // Failure means: a cancelled press still toggles the group or steals keyboard focus.
            using (var setup = new TestSetupHelper(PageName))
            {
                Button dummyButton = FindElement.ById<Button>(DummyButton);
                if (dummyButton == null)
                {
                    Verify.Fail("DummyButton was not found on the test page.");
                    return;
                }

                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    Verify.Fail("The grouped table was not found.");
                    return;
                }

                UIObject rowsHost = GetRowsHost(tableView);
                int baselineRows = CountRows(rowsHost);
                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                dummyButton.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(dummyButton.HasKeyboardFocus, "Precondition: DummyButton should hold keyboard focus before the press.");

                var dummyBounds = dummyButton.BoundingRectangle;
                PointerInput.Move(CentreOf(groupHeader));
                PointerInput.Press(PointerButtons.Primary);
                Wait.ForIdle();
                PointerInput.Move(new Point(dummyBounds.Left + (dummyBounds.Width / 2), dummyBounds.Top + (dummyBounds.Height / 2)));
                Wait.ForIdle();
                PointerInput.Release(PointerButtons.Primary);
                Wait.ForIdle();

                rowsHost = GetRowsHost(tableView);
                UIObject freshHeader = GetFirstGroupHeader(rowsHost);
                ExpandCollapseState state = freshHeader == null
                    ? ExpandCollapseState.LeafNode
                    : new ExpandCollapseImplementation(freshHeader).ExpandCollapseState;
                Log.Comment("After press, drag off and release: rows={0} (baseline {1}), state={2}, focused element {3}.",
                    CountRows(rowsHost), baselineRows, state, DescribeFocused());

                Verify.AreEqual(ExpandCollapseState.Expanded, state, "A press dragged off the band must not toggle the group.");
                Verify.AreEqual(baselineRows, CountRows(rowsHost), "A press dragged off the band must not change the rows.");
                Verify.IsTrue(dummyButton.HasKeyboardFocus, "A cancelled press must not move keyboard focus to the header.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the group header's CommonStates follow the pointer: Normal to PointerOver on enter, Pressed while the button is down, back to PointerOver on release, and Normal when the pointer leaves.")]
        public void GroupHeaderPointerAndPressedVisualStates()
        {
            // Spec: TableView.idl documents the group header's states as "CommonStates
            //   Normal|PointerOver|Pressed|Disabled" - a contract on the control, so the names are spec-derived,
            //   not read out of the implementation.
            // Route: these states are set only from the header's own pointer handlers, so there is no
            //   programmatic route in - the same reason the API tier records
            //   VerifyRowPointerOverVisualState as not implementable there (TableView_Rows_APITests.cs:664).
            //   API §9 covers ExpansionStates, which is a different group reached by a different mechanism.
            // Failure means: the group header gives no hover or press feedback, so a pointer user cannot tell
            //   the band is interactive before clicking it.
            using (var setup = new TestSetupHelper(PageName))
            {
                // Everything on the page toolbar is resolved BEFORE the grouped table realizes: a FindElement
                // afterwards forces ElementCache.Refresh() to re-walk the whole tree, which descends into
                // TableViewRow children and asserts the app (finding #13, measured).
                Button hookButton = FindElement.ById<Button>(HookGroupHeadersButton);
                Edit stateLog = FindElement.ById<Edit>(GroupHeaderStateLog);
                UIObject awayTarget = FindElement.ById(DummyButton);
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
            // Pointer leg.
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
            using (var setup = new TestSetupHelper(PageName))
            {
                Button hookButton = FindElement.ById<Button>(HookGroupHeadersButton);
                Edit toggleReport = FindElement.ById<Edit>(GroupToggleReport);
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

        [TestMethod]
        public void ExpandAllGroupsReconcilesGestureCollapsedGroup()
        {
            // "expand-all / collapse-all driven from the page controls reconcile with header state".
            // This owns the RECONCILIATION between the gesture route (TableViewGroupHeader::RequestToggle) and the
            //   programmatic route (TableView::ExpandAllGroups, driven here by the page button). API §9 already covers
            //   ExpandAllGroups on its own; the interesting claim here is that a group collapsed by a real gesture is
            //   healed by the bulk API - i.e. the header's visual/peer state and the source's expansion state are one
            //   store, not two.
            // Failure means: the two paths keep separate state and drift apart - the gesture-collapsed header stays
            //   Collapsed (or its rows stay unrealized) even though ExpandAllGroups ran. Invisible to a purely
            //   programmatic test, because that test never collapsed the group by gesture in the first place.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                Button expandAllButton = FindElement.ById<Button>(ExpandAllGroupsButton);
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
        [TestProperty("Ignore", "True")] // Product finding #16: CollapseAllGroups() does not collapse in the live control.
        [TestProperty("Description", "Verifies clicking CollapseAllGroupsButton re-collapses a group that was expanded by gesture: the header peer reports Collapsed and the realized row count returns to the all-collapsed baseline.")]
        public void CollapseAllGroupsReconcilesGestureExpandedGroup()
        {
            // "expand-all / collapse-all driven from the page controls reconcile with header state"
            //   (mirror of the expand-all case).
            // Failure means: a group expanded by gesture is left Expanded (its rows still realized) after
            //   CollapseAllGroups runs, i.e. the gesture wrote expansion state the bulk API cannot see.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                Button collapseAllButton = FindElement.ById<Button>(CollapseAllGroupsButton);
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

        #region N.3 Pointer (new tests for the #11820 model)

        [TestMethod]
        [TestProperty("Description", "A click on the row's empty strip, past the last column, focuses the row itself (no cell) and selects it.")]
        public void PointerClickOnRowStripFocusesRow()
        {
            // The complement of the #11820 "a cell press focuses the cell" rule, and the case where dev-spec:427's
            // "pointer focus lands on the row" still applies (spec debt). Failure means the row's empty area is a dead
            // target, or focus goes to a cell the user did not click.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                Button dummyButton = FindElement.ById<Button>(DummyButton);
                if (tableView == null || dummyButton == null) { Verify.Fail("BasicTableView and DummyButton are required."); return; }

                UIObject row = GetRow(tableView, 0);
                if (row == null) { Verify.Fail("The first realized TableViewRow peer was not found."); return; }

                UIObject headerHost = tableView.Children[0];
                UIObject lastHeader = headerHost.Children[headerHost.Children.Count - 1];
                int lastColumnRight = lastHeader.BoundingRectangle.Left + lastHeader.BoundingRectangle.Width;
                var rowBounds = row.BoundingRectangle;
                int rowRight = rowBounds.Left + rowBounds.Width;
                Log.Comment("Last column right edge={0}, row right edge={1}.", lastColumnRight, rowRight);

                if (rowRight - lastColumnRight < 12)
                {
                    Verify.Fail("Harness limitation: the row has no empty strip past its last column at this window size, so the strip click cannot be measured.");
                    return;
                }

                dummyButton.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(dummyButton.HasKeyboardFocus, "Precondition: DummyButton should hold keyboard focus before the click.");

                ClickPoint(new Point((lastColumnRight + rowRight) / 2, rowBounds.Top + (rowBounds.Height / 2)));
                Wait.ForIdle();

                Verify.IsTrue(row.HasKeyboardFocus, "A click on the row's empty strip must focus the row itself.");
                Verify.AreEqual(-1, IndexOfFocusedCell(row), "A click on the empty strip must not focus any cell.");
                var selectionItem = new SelectionItemImplementation<UIObject>(row, UIObject.Factory);
                Verify.IsTrue(selectionItem.IsSelected, "A click on the row's empty strip must select the row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "A right-click on a row does not select it; a following left-click on another row does (positive control).")]
        public void RightClickDoesNotSelect()
        {
            // functional-spec:54 reserves a per-row/cell context menu, so right-click is a menu gesture, not a
            // selection one; ListView agrees (spec debt: no sentence says it). Failure means opening a context menu
            // silently changes selection.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null) { Verify.Fail("BasicTableView was not found on the test page."); return; }

                UIObject row1 = GetRow(tableView, 1);
                UIObject row2 = GetRow(tableView, 2);
                if (row1 == null || row2 == null) { Verify.Fail("Three realized rows are required."); return; }

                var bounds = row1.BoundingRectangle;
                Point point = new Point(bounds.Left + TextCellRelativeX, bounds.Top + (bounds.Height / 2));
                Log.Comment("Right-click at absolute point ({0}, {1}).", point.X, point.Y);
                PointerInput.Move(point);
                PointerInput.Press(PointerButtons.Secondary);
                PointerInput.Release(PointerButtons.Secondary);
                Wait.ForIdle();

                var row1Selection = new SelectionItemImplementation<UIObject>(row1, UIObject.Factory);
                Verify.IsFalse(row1Selection.IsSelected, "A right-click must not select the row.");

                // Dismiss anything the right-click opened before the positive control.
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                ClickRow(row2);
                Wait.ForIdle();

                var row2Selection = new SelectionItemImplementation<UIObject>(row2, UIObject.Factory);
                Verify.IsTrue(row2Selection.IsSelected, "Positive control: a left-click must select the row.");
                Verify.IsFalse(new SelectionItemImplementation<UIObject>(row1, UIObject.Factory).IsSelected,
                    "Row 1 must still be unselected.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "A press on a row that is dragged off the table and released elsewhere selects nothing; a normal click then selects (positive control).")]
        public void PressAndDragOffRowDoesNotSelect()
        {
            // Owner decision; dev-spec gesture layer: "Selection is
            // applied on release, and only when the release lands on the row that was pressed". Failure means a user
            // cannot back out of a selection gesture by dragging away.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                UIObject afterTable = FindElement.ById(AfterTableButton);
                if (tableView == null || afterTable == null) { Verify.Fail("BasicTableView and AfterTableButton are required."); return; }

                UIObject row1 = GetRow(tableView, 1);
                UIObject row2 = GetRow(tableView, 2);
                if (row1 == null || row2 == null) { Verify.Fail("Three realized rows are required."); return; }

                var bounds = row1.BoundingRectangle;
                Point press = new Point(bounds.Left + TextCellRelativeX, bounds.Top + (bounds.Height / 2));
                Point release = CentreOf(afterTable);
                Log.Comment("Press at ({0}, {1}); release at ({2}, {3}).", press.X, press.Y, release.X, release.Y);

                PointerInput.Move(press);
                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Move(new Point((press.X + release.X) / 2, (press.Y + release.Y) / 2));
                PointerInput.Move(release);
                PointerInput.Release(PointerButtons.Primary);
                Wait.ForIdle();

                Verify.IsFalse(new SelectionItemImplementation<UIObject>(row1, UIObject.Factory).IsSelected,
                    "A press dragged off the row and released elsewhere must not select the pressed row.");

                ClickRow(row2);
                Wait.ForIdle();

                Verify.IsTrue(new SelectionItemImplementation<UIObject>(row2, UIObject.Factory).IsSelected,
                    "Positive control: a normal click must select the row.");
                Verify.IsFalse(new SelectionItemImplementation<UIObject>(row1, UIObject.Factory).IsSelected,
                    "Row 1 must still be unselected.");
            }
        }

        #endregion

        #region Helpers

        // Row-relative x of a plain text cell (inside the Age column: Name is 0-160, Age is 160-260).
        // Any cell would do for a selection click; a text cell is used so nothing depends on template content.
        private static readonly int TextCellRelativeX = BasicColumnCentreX(AgeColumn);

        private static void ClickRow(UIObject row)
        {
            ClickRowAtColumnOffset(row, TextCellRelativeX);
        }

        private static void HoverRow(UIObject row)
        {
            var bounds = row.BoundingRectangle;
            InputHelper.MoveMouse(new Point(bounds.Left + TextCellRelativeX, bounds.Top + (bounds.Height / 2)));
        }

        #endregion
    }
}
