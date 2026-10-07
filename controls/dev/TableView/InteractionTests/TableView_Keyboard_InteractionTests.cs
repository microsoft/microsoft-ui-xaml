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
using Point = System.Drawing.Point;

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView keyboard-navigation and header-input interaction tests.
    // Backlog: docs\design-notes\TabularControls\TableView-interaction-test-plan.md sections 1 and 2.
    //
    // These run out of process and reach the control only through the real UIA provider tree, which
    // is the point of an interaction test: they prove that real key routing and pointer hit-testing
    // reach the state machine the API tests cover in process (interaction-plan coverage rule).
    //
    // Product finding #13 (a client asking a row peer for its children fail-fasted the app) shaped the older tests in
    // this file: they observe rows at row level, point at cells by coordinates, and read editors and visual states
    // through in-process page readouts. #11820 fixed the peers, and cell peers are now read safely; see the history
    // note in TableViewInteractionTestHelpers. The older tests are kept as written - their techniques still work.
    [TestClass]
    public class TableViewKeyboardInteractionTests
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

        #region 1. Keyboard navigation

        [TestMethod]
        [TestProperty("Description", "Down arrow on a focused row moves keyboard focus to the following row (dev-spec:165).")]
        public void DownArrowMovesFocusToNextRow()
        {
            // dev-spec:165 - TableView listens to bubbling KeyDown; an unhandled Down "moves focus
            // between rows". A failure here means arrow keys no longer route to the row navigator, so
            // the table is keyboard-dead even though the API-level GridCoordinateHelper math is fine.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 2) { Verify.Fail("Need at least two realized rows."); return; }

                UIObject first = rowsHost.Children[0];
                UIObject second = rowsHost.Children[1];

                first.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(first.HasKeyboardFocus, "The first row should take keyboard focus.");

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                Verify.IsTrue(second.HasKeyboardFocus, "Down should move focus to the next row.");
                Verify.IsFalse(first.HasKeyboardFocus, "Focus should leave the first row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Up arrow on a focused row moves keyboard focus to the preceding row (dev-spec:165).")]
        public void UpArrowMovesFocusToPreviousRow()
        {
            // dev-spec:165 - unhandled Up moves focus between rows. Symmetric partner of the Down test;
            // a failure means the navigator handles only one direction.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 2) { Verify.Fail("Need at least two realized rows."); return; }

                UIObject first = rowsHost.Children[0];
                UIObject second = rowsHost.Children[1];

                second.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(second.HasKeyboardFocus, "The second row should take keyboard focus.");

                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();

                Verify.IsTrue(first.HasKeyboardFocus, "Up should move focus to the previous row.");
                Verify.IsFalse(second.HasKeyboardFocus, "Focus should leave the second row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Home moves keyboard focus to the first row (dev-spec:165).")]
        public void HomeKeyMovesToFirstRow()
        {
            // dev-spec:165 lists Home among the keys that move focus between rows. A failure means Home
            // is swallowed (e.g. by a scroll viewer) instead of jumping the row navigator to the top.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need at least three realized rows."); return; }

                rowsHost.Children[2].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(rowsHost.Children[2].HasKeyboardFocus, "A middle row should take focus first.");

                KeyboardHelper.PressKey(Key.Home);
                Wait.ForIdle();

                // Re-fetch: Home can scroll, changing the realized set; index 0 is always the top row.
                rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                Verify.IsTrue(rowsHost.Children[0].HasKeyboardFocus, "Home should focus the first row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "End moves keyboard focus to the last row (dev-spec:165).")]
        public void EndKeyMovesToLastRow()
        {
            // dev-spec:165 lists End among the keys that move focus between rows. A failure means End
            // does not reach the last row (e.g. it stops at the last realized row instead of paging).
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 2) { Verify.Fail("Need at least two realized rows."); return; }

                rowsHost.Children[0].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(rowsHost.Children[0].HasKeyboardFocus, "The first row should take focus first.");

                KeyboardHelper.PressKey(Key.End);
                Wait.ForIdle();

                // Re-fetch: End scrolls the true last row into view, changing the realized set.
                rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                UIObject last = rowsHost.Children[rowsHost.Children.Count - 1];
                Verify.IsTrue(last.HasKeyboardFocus, "End should focus the last row.");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Test debt: scroll-percent proxy cannot separate a page from a step; rewrite on the destination row PositionInSet.
        [TestProperty("Description", "Page Down moves keyboard focus by roughly a viewport, farther than a single Down (dev-spec:165).")]
        public void PageDownMovesByViewport()
        {
            // dev-spec:165 lists PageDown among the keys that move focus between rows. Uses the 200-item
            // ScrollingTableView (Height 300) so a viewport is meaningfully larger than one row; the
            // 12-row BasicTableView is too short to distinguish PageDown from End.
            //
            // Travel is measured as VerticalScrollPercent, NOT as the focused row's screen position.
            // Paging scrolls the view, so the newly focused row lands at roughly the SAME screen Y as
            // the old one and a bounding-rectangle delta reads ~0 even when paging works perfectly.
            // The row peer exposes no index, name or PositionInSet, so the scroll offset is the only
            // identity-free measure of how far the view travelled.
            //
            // The discriminator against "PageDown behaves like a single Down": from the top row, one
            // Down keeps the next row already on screen and scrolls nothing, while a page must move the
            // viewport. A failure means PageDown does not page.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem)) { Verify.Fail(GoToButton(ScrollingPivotItem) + " was not found."); return; }

                UIObject rowsHost = GetRowsHost(ScrollingTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 2) { Verify.Fail("Need realized rows to measure movement."); return; }

                var scroll = new ScrollImplementation(rowsHost);
                if (!scroll.IsAvailable) { Verify.Fail("The rows host does not expose the Scroll pattern."); return; }

                UIObject firstRow = rowsHost.Children[0];
                firstRow.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(firstRow.HasKeyboardFocus, "The top row should take focus first.");
                double atTop = scroll.VerticalScrollPercent;

                // Baseline: one Down stays inside the viewport, so it must not scroll.
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                double afterOneDown = scroll.VerticalScrollPercent;
                Log.Comment("VerticalScrollPercent: top={0}, after one Down={1}.", atTop, afterOneDown);

                // Return to the top, then page.
                KeyboardHelper.PressKey(Key.Home);
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.PageDown);
                Wait.ForIdle();
                double afterPage = scroll.VerticalScrollPercent;
                Log.Comment("VerticalScrollPercent after PageDown={0}.", afterPage);

                Verify.IsNotNull(FindFocusedRow(GetRowsHost(ScrollingTable)),
                    "A row must still hold keyboard focus after PageDown.");
                Verify.IsTrue(afterPage > afterOneDown + 1.0,
                    string.Format("PageDown should page the view ({0}%) far past where a single Down leaves it ({1}%).",
                        afterPage, afterOneDown));
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Test debt: scroll-percent proxy cannot separate a page from a step; rewrite on the destination row PositionInSet.
        [TestProperty("Description", "Page Up moves keyboard focus back up by roughly a viewport (dev-spec:165).")]
        public void PageUpMovesByViewport()
        {
            // dev-spec:165 lists PageUp among the keys that move focus between rows. Pages down twice to
            // earn headroom, then asserts PageUp reverses it. Measured as VerticalScrollPercent for the
            // same reason as PageDownMovesByViewport: paging scrolls, so the focused row's screen
            // position barely changes and cannot express travel. A failure means PageUp does not page
            // back up.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem)) { Verify.Fail(GoToButton(ScrollingPivotItem) + " was not found."); return; }

                UIObject rowsHost = GetRowsHost(ScrollingTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 2) { Verify.Fail("Need realized rows to measure movement."); return; }

                var scroll = new ScrollImplementation(rowsHost);
                if (!scroll.IsAvailable) { Verify.Fail("The rows host does not expose the Scroll pattern."); return; }

                rowsHost.Children[0].SetFocus();
                Wait.ForIdle();

                KeyboardHelper.PressKey(Key.PageDown);
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.PageDown);
                Wait.ForIdle();
                double afterPagingDown = scroll.VerticalScrollPercent;
                Verify.IsTrue(afterPagingDown > 1.0,
                    string.Format("Precondition: two PageDowns should leave the view scrolled ({0}%).", afterPagingDown));

                KeyboardHelper.PressKey(Key.PageUp);
                Wait.ForIdle();
                double afterPagingUp = scroll.VerticalScrollPercent;
                Log.Comment("VerticalScrollPercent: after two PageDowns={0}, after PageUp={1}.", afterPagingDown, afterPagingUp);

                Verify.IsNotNull(FindFocusedRow(GetRowsHost(ScrollingTable)),
                    "A row must still hold keyboard focus after PageUp.");
                Verify.IsTrue(afterPagingUp < afterPagingDown - 1.0,
                    string.Format("PageUp should page the view back up ({0}% -> {1}%).", afterPagingDown, afterPagingUp));
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Tab from a row lands inside the table since the Action (button) column was added; product or fixture TBD.
        [TestProperty("Description", "Tab leaves the table downstream in a single press instead of walking cell by cell (dev-spec:165).")]
        public void TabMovesFocusOutOfTable()
        {
            // dev-spec:165 keeps navigation row-oriented; the table is not a per-cell tab trap. One Tab
            // from a focused row must leave every row and continue forward in tab order to the next
            // focusable element AFTER the table - AfterTableButton, which the page places in the
            // Grid.Row=2 StackPanel below the Pivot. A failure means Tab steps through cells (focus
            // stays inside the table), so a keyboard user is trapped walking the grid cell by cell.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 1) { Verify.Fail("Need a realized row."); return; }

                // Focus the FIRST row, not the last: the last realized row can be clipped by the
                // viewport, and a partially offscreen row does not reliably take focus. Which row is
                // focused is irrelevant to this contract - Tab must leave the table from any of them.
                UIObject anchorRow = rowsHost.Children[0];
                anchorRow.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(anchorRow.HasKeyboardFocus, "A row should take focus first.");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                // No row may still hold focus - a single Tab left the row collection entirely.
                rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                Verify.IsNull(FindFocusedRow(rowsHost), "No row should hold focus after Tab.");

                // Forward tab order exits the table downstream onto the next focusable element after it.
                UIObject afterTable = FindElement.ById(AfterTableButton);
                if (afterTable == null) { Verify.Fail("AfterTableButton was not found."); return; }
                Verify.IsTrue(afterTable.HasKeyboardFocus, "Tab out of the table should land on AfterTableButton.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Each row-navigation key moves focus AND selection onto the target row (owner decision; dev-spec:165 is silent on selection).")]
        public void KeyboardFocusMoveCarriesSelection()
        {
            // dev-spec:165 says unhandled Up/Down/Home/End "move focus between rows" and says nothing
            // about selection. The owner's design decision is that selection follows focus, so this
            // asserts the pair together: after the key, the target row is BOTH focused and selected, and
            // the row we came from is not. A failure means focus and selection have split, which is the
            // classic "Enter acts on the wrong row" bug. The dev spec should be amended to state this;
            // until it is, the authority for this assertion is the decision, not the document.
            //
            // No SelectRow() call is needed to set up: if selection follows focus, the navigation key
            // itself establishes it. That also keeps the test off the row peer's Select() path.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 4) { Verify.Fail("Need at least four realized rows."); return; }

                int lastIndex = rowsHost.Children.Count - 1;

                // key, index to start focus on, index the key should land on.
                var cases = new[]
                {
                    new { Key = Key.Down, From = 1, To = 2, Name = "Down" },
                    new { Key = Key.Up, From = 2, To = 1, Name = "Up" },
                    new { Key = Key.Home, From = 2, To = 0, Name = "Home" },
                    new { Key = Key.End, From = 0, To = lastIndex, Name = "End" },
                };

                foreach (var testCase in cases)
                {
                    Log.Comment("Navigation key: {0} (row {1} -> row {2}).", testCase.Name, testCase.From, testCase.To);

                    UIObject rows = GetRowsHost(BasicTable);
                    if (rows == null) { return; }

                    UIObject start = rows.Children[testCase.From];
                    start.SetFocus();
                    Wait.ForIdle();

                    KeyboardHelper.PressKey(testCase.Key);
                    Wait.ForIdle();

                    rows = GetRowsHost(BasicTable);
                    if (rows == null) { return; }
                    UIObject target = rows.Children[testCase.To];

                    Verify.IsTrue(target.HasKeyboardFocus,
                        string.Format("{0} should move focus onto row {1}.", testCase.Name, testCase.To));

                    bool targetSelected;
                    if (!TryGetIsSelected(target, out targetSelected))
                    {
                        Verify.Fail("The row peer exposes no SelectionItem to this client, so selection cannot be observed.");
                        return;
                    }

                    Verify.IsTrue(targetSelected,
                        string.Format("{0} should carry selection onto row {1} along with focus.", testCase.Name, testCase.To));

                    bool startSelected;
                    if (TryGetIsSelected(rows.Children[testCase.From], out startSelected))
                    {
                        Verify.IsFalse(startSelected,
                            string.Format("Under SelectionMode.Single the row {0} came from must no longer be selected.", testCase.Name));
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "With SelectionMode.None the navigation keys still move focus but select nothing.")]
        public void KeyboardNavigationWithSelectionDisabledMovesFocusOnly()
        {
            // The keyboard twin of the pointer gate in section 3. API section 6 proves Select() honors
            // SelectionMode.None programmatically; neither that nor the pointer test touches key routing.
            // A failure means the mode is enforced only inside Select() while the keyboard path writes
            // selection state behind it - the table would then select rows a user only navigated past.
            using (var setup = new TestSetupHelper(PageName))
            {
                var selectionMode = FindElement.ById<ComboBox>(SelectionModeComboBox);
                if (selectionMode == null) { Verify.Fail("SelectionModeComboBox was not found."); return; }
                // The page names the items SelectionModeNone / SelectionModeSingle via
                // AutomationProperties.Name; the Content strings ("None") are not what UIA reports.
                selectionMode.SelectItemByName("SelectionModeNone");
                Wait.ForIdle();

                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                rowsHost.Children[0].SetFocus();
                Wait.ForIdle();

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }

                Verify.IsTrue(rowsHost.Children[1].HasKeyboardFocus,
                    "SelectionMode.None must not stop the navigation key from moving focus.");

                foreach (UIObject row in rowsHost.Children)
                {
                    bool isSelected;
                    if (TryGetIsSelected(row, out isSelected))
                    {
                        Verify.IsFalse(isSelected, "No row may be selected while SelectionMode is None.");
                    }
                    else
                    {
                        // Withholding SelectionItem entirely under None is the stronger, correct answer.
                        Log.Comment("The row peer withholds SelectionItem under SelectionMode.None, as expected.");
                        break;
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "From a focused row Right drills into the first cell and Left on that cell returns to the row; Left on a row does not leave it (dev-spec:201, treegrid model).")]
        public void RightDrillsIntoFirstCellAndLeftReturnsToRow()
        {
            // Interaction plan §1 RightDrillsIntoFirstCellAndLeftReturnsToRow (was LeftAndRightArrowsDoNotMoveRowFocus).
            // dev-spec:201 makes navigation cell-aware: Left/Right move the cell cursor within the focused row.
            // Entering at the first cell from row level and returning on Left is the WAI-ARIA treegrid convention
            // (spec debt: dev-spec:201 does not state the row <-> cell transition). Failure means a keyboard user
            // cannot reach cell content from row level, cannot get back, or Left on a row walks focus out of the table.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject anchor = rowsHost.Children[1];
                anchor.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(anchor.HasKeyboardFocus, "Precondition: the anchor row should hold focus first.");

                // Negative control: Left on a row has nothing further out to go to.
                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                UIObject row = GetRowsHost(BasicTable).Children[1];
                Verify.IsTrue(row.HasKeyboardFocus, "Left on a focused row must leave focus on that row.");

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                row = GetRowsHost(BasicTable).Children[1];
                int focusedCell = IndexOfFocusedCell(row);
                Log.Comment("After Right: row focused={0}, focused cell index={1}.", row.HasKeyboardFocus, focusedCell);
                Verify.AreEqual(0, focusedCell, "Right on a focused row must move focus to the row's FIRST cell.");
                Verify.IsFalse(row.HasKeyboardFocus, "After drilling in, the row container itself must no longer hold focus.");

                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                row = GetRowsHost(BasicTable).Children[1];
                Verify.IsTrue(row.HasKeyboardFocus, "Left on the first cell must return focus to the row.");
                Verify.AreEqual(-1, IndexOfFocusedCell(row), "No cell may still hold focus once the row is focused again.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Shift+Tab from the body lands on the header band, and a second Shift+Tab leaves the table upstream.")]
        public void ShiftTabMovesFocusOutOfTableUpstream()
        {
            // Interaction plan §1 ShiftTabMovesFocusOutOfTableUpstream (rewritten for the two-band tab model).
            // dev-spec:135 rejects one tab stop per column header; the header band as exactly one stop ahead of the
            // body is the #11820 model (spec debt). Failure means Shift+Tab skips the header band, so headers are
            // unreachable backwards, or walks it header by header.
            // The upstream landing control is page layout and is deliberately not named.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 1) { Verify.Fail("Need a realized row."); return; }

                UIObject firstRow = rowsHost.Children[0];
                firstRow.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(firstRow.HasKeyboardFocus, "Precondition: a row should take focus first.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();

                UIObject headerAfterFirst = FindFocusedHeader(BasicTable);
                UIObject rowAfterFirst = FindFocusedRow(GetRowsHost(BasicTable));
                Log.Comment("After Shift+Tab 1: header={0}, row focused={1}.",
                    headerAfterFirst == null ? "<none>" : headerAfterFirst.Name, rowAfterFirst != null);

                Verify.IsNull(rowAfterFirst, "No row should hold focus after the first Shift+Tab.");
                Verify.IsNotNull(headerAfterFirst, "The first Shift+Tab from the body must land on the header band.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();

                UIObject headerAfterSecond = FindFocusedHeader(BasicTable);
                UIObject rowAfterSecond = FindFocusedRow(GetRowsHost(BasicTable));
                Log.Comment("After Shift+Tab 2: header={0}, row focused={1}.",
                    headerAfterSecond == null ? "<none>" : headerAfterSecond.Name, rowAfterSecond != null);

                Verify.IsNull(headerAfterSecond, "The second Shift+Tab must leave the header band: the band is ONE tab stop, not one per column.");
                Verify.IsNull(rowAfterSecond, "The second Shift+Tab must not return to the body.");

                UIObject afterTable = FindElement.ById(AfterTableButton);
                if (afterTable == null) { Verify.Fail("AfterTableButton was not found."); return; }
                Verify.IsFalse(afterTable.HasKeyboardFocus,
                    "Shift+Tab must move focus upstream, not forward onto the control after the table.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tab enters the table at the header band, the next Tab enters the body, and neither selects a row.")]
        public void TabIntoTableFocusesAHeaderThenARowWithoutSelecting()
        {
            // Interaction plan §1 TabIntoTableFocusesAHeaderThenARowWithoutSelecting (was
            // TabIntoTableFocusesARowWithoutSelecting). The header band first and the body second is the #11820
            // two-band model (spec debt, see ShiftTabMovesFocusOutOfTableUpstream). Selection is checked BEFORE any
            // navigation key, because navigation keys are entitled to select (KeyboardFocusMoveCarriesSelection).
            // Failure means the header band is skipped, the body is not tab-reachable, or tabbing in selects.
            using (var setup = new TestSetupHelper(PageName))
            {
                var selectionMode = FindElement.ById<ComboBox>(SelectionModeComboBox);
                if (selectionMode == null) { Verify.Fail("SelectionModeComboBox was not found."); return; }
                selectionMode.SetFocus();
                Wait.ForIdle();

                UIObject header = null;
                for (int attempt = 0; attempt < 3 && header == null; attempt++)
                {
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    if (FindFocusedBodyElement(GetRowsHost(BasicTable)) != null)
                    {
                        Verify.Fail("Tab reached the body before any column header: the header band was skipped.");
                        return;
                    }

                    header = FindFocusedHeader(BasicTable);
                }

                if (header == null)
                {
                    Verify.Fail("Tab never reached the header band of BasicTableView within three presses.");
                    return;
                }
                Log.Comment("Header band entered on '{0}'.", header.Name);

                VerifyNoRowSelected("entering the header band");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                Verify.IsNull(FindFocusedHeader(BasicTable), "The next Tab must leave the header band (one tab stop for the whole band).");
                Verify.IsNotNull(FindFocusedBodyElement(GetRowsHost(BasicTable)),
                    "The next Tab after the header band must enter the body (a row or one of its cells).");

                VerifyNoRowSelected("entering the body");

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                Verify.IsNotNull(FindFocusedBodyElement(GetRowsHost(BasicTable)),
                    "After entering the body, Down must keep focus on a row (or a cell of a row).");
            }
        }

        #endregion

        #region 2. Header input

        [TestMethod]
        [TestProperty("Description", "Enter on a focused sortable header toggles the sort, reordering rows (accessibility baseline; see the spec-gap note).")]
        public void HeaderEnterTogglesSort()
        {
            // FAILING - PRODUCT DEFECT (measured): the header takes keyboard focus and exposes Invoke to
            // automation, but Enter does nothing. TableView.cpp:1642 wires the sort to headerCell.Tapped
            // ONLY, and a plain Grid does not raise Tapped from a key press the way a Button raises
            // Click. Net effect: a sighted keyboard-only user can focus a sortable header and has no way
            // to sort. HeaderPointerClickTogglesSortAndUpdatesIndicator passes on the same column, so
            // the sort itself works - the keyboard route is simply absent.
            //
            // SPEC GAP, recorded deliberately: dev-spec:133 makes the header cell the keyboard target
            // but routes only Left/Right (resize); dev-spec:165 covers rows. Nothing states that Enter
            // sorts. The expectation here is the accessibility baseline - an element that is focusable,
            // reports itself invokable, and acts on a pointer click must be operable from the keyboard -
            // not a documented sentence. The dev spec should state the header's keyboard contract.
            //
            // Observability: sort-state text is localized and, per product finding #5, no TableView
            // localized string currently resolves, so HelpText is empty - useless as a signal. Instead
            // this asserts the observable consequence: rows reorder. IDL:531-546 makes selection a data
            // ITEM (SelectedItem) with SelectedIndex a coherent projection, so a selected item keeps its
            // selection across a sort and its row moves to the item's new position. We select the bottom
            // row (largest Age) and toggle Age to Descending (None -> Ascending -> Descending); Descending
            // lifts the largest-Age item to the top, an upward move that keeps the row realized.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                UIObject ageHeader = GetHeader(BasicTable, "Age");
                if (ageHeader == null) { Verify.Fail("The Age header was not found."); return; }
                ageHeader.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(ageHeader.HasKeyboardFocus, "The Age header should take keyboard focus.");

                KeyboardHelper.PressKey(Key.Enter); // None -> Ascending (bottom item does not move yet)
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Enter); // Ascending -> Descending (largest Age rises to the top)
                Wait.ForIdle();

                UIObject selected = FindSelectedRow(GetRowsHost(BasicTable));
                if (selected == null) { Verify.Fail("The selected row was not found after sorting."); return; }
                int newTop = selected.BoundingRectangle.Top;

                Verify.IsTrue(newTop < oldTop - 2,
                    string.Format("Enter on the header should reorder rows: the selected item moved up ({0} -> {1}).", oldTop, newTop));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Alt+Right on a focused header widens the column, Alt+Shift+Right widens it by a larger step, and focus stays on the header (dev-spec:127, dev-spec:133).")]
        public void HeaderKeyboardResizeChangesWidth()
        {
            // Interaction plan §2 HeaderKeyboardResizeChangesWidth (rewritten for the Alt+Arrow chord).
            // dev-spec:133 - the header cell is the keyboard target and TryKeyboardStep owns direction, the RTL
            // mirror, KeyboardIncrement and the Shift multiplier. The chord is Alt+Arrow (WPF DataGrid binding,
            // adopted by #11820 because bare arrows now navigate the header band); dev-spec:135 still says bare
            // Left/Right and is spec debt. Asserts DIRECTION and RELATIVE magnitude only, never a pixel step.
            // Failure means the resize key route is dead, Shift is not honored, or the chord also moves focus.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject header = GetHeader(BasicTable, "Name");
                if (header == null) { Verify.Fail("The Name header was not found."); return; }
                header.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(header.HasKeyboardFocus, "Precondition: the Name header should take keyboard focus.");

                int w0 = GetHeader(BasicTable, "Name").BoundingRectangle.Width;

                KeyboardHelper.PressKey(Key.Right, ModifierKey.Alt);
                Wait.ForIdle();
                UIObject afterPlain = GetHeader(BasicTable, "Name");
                int w1 = afterPlain.BoundingRectangle.Width;
                bool focusKeptPlain = afterPlain.HasKeyboardFocus;

                KeyboardHelper.PressKey(Key.Right, ModifierKey.Alt | ModifierKey.Shift);
                Wait.ForIdle();
                UIObject afterShift = GetHeader(BasicTable, "Name");
                int w2 = afterShift.BoundingRectangle.Width;
                bool focusKeptShift = afterShift.HasKeyboardFocus;

                int deltaPlain = w1 - w0;
                int deltaShift = w2 - w1;
                Log.Comment("Keyboard resize: {0} -> {1} (Alt+Right, focus kept={2}) -> {3} (Alt+Shift+Right, focus kept={4}).",
                    w0, w1, focusKeptPlain, w2, focusKeptShift);

                Verify.IsTrue(focusKeptPlain, "Alt+Right must resize, not move focus off the Name header.");
                Verify.IsTrue(deltaPlain > 0,
                    string.Format("Alt+Right on the header should grow the column ({0} -> {1}).", w0, w1));
                Verify.IsTrue(focusKeptShift, "Alt+Shift+Right must resize, not move focus off the Name header.");
                Verify.IsTrue(deltaShift > deltaPlain,
                    string.Format("Alt+Shift+Right should grow by a larger step than Alt+Right ({0}px vs {1}px).", deltaShift, deltaPlain));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Pointer click on a sortable header toggles the sort and reorders rows (dev-spec:286, IDL:531).")]
        public void HeaderPointerClickTogglesSortAndUpdatesIndicator()
        {
            // Interaction plan 2 - the pointer route into sort. The API twin (7.5) drives IInvokeProvider
            // and never touches the pointer hit-test path (dev-spec:286), so this proves a real click
            // reaches the sort. Same observability limits as HeaderEnterTogglesSort: the SortIndicator's
            // localized state text is empty (product finding #5), so we assert the reorder consequence,
            // not the indicator text. Selection tracks the data item (IDL:531-546), so the selected
            // bottom row (largest Age) moving up after two clicks toggle Age to Descending is the signal.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                UIObject ageHeader = GetHeader(BasicTable, "Age");
                if (ageHeader == null) { Verify.Fail("The Age header was not found."); return; }

                InputHelper.LeftClick(ageHeader); // None -> Ascending
                Wait.ForIdle();
                ageHeader = GetHeader(BasicTable, "Age");
                if (ageHeader == null) { Verify.Fail("The Age header disappeared after the first click."); return; }
                InputHelper.LeftClick(ageHeader); // Ascending -> Descending (largest Age rises)
                Wait.ForIdle();

                UIObject selected = FindSelectedRow(GetRowsHost(BasicTable));
                if (selected == null) { Verify.Fail("The selected row was not found after sorting."); return; }
                int newTop = selected.BoundingRectangle.Top;

                Verify.IsTrue(newTop < oldTop - 2,
                    string.Format("Clicking the header should reorder rows: the selected item moved up ({0} -> {1}).", oldTop, newTop));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Repeated header clicks walk the column's authored SortCycle, including the trailing None step that returns the rows to source order.")]
        public void HeaderClicksFollowTheColumnSortCycle()
        {
            // Interaction plan 2 - HeaderClicksFollowTheColumnSortCycle.
            // TableView.idl on SortCycle: the cycle "describes how one column responds to being clicked again",
            // is per-column, and is read AT CLICK TIME, so the page combo box can set it before the clicks and
            // no rebuild is needed. Only the click route consumes it - SortByColumn takes an explicit direction -
            // so no API test can cover this.
            //
            // The Score column exists for the None step. Name and Age are authored in ASCENDING source order, so
            // for them "Ascending" and "None" produce the same order and the third click is unobservable. Score is
            // authored non-monotonic (((id * 7) % 12) + 1), putting its maximum at source index 5: neither first
            // nor last, so all three steps land the tracked row on three distinct indices.
            //
            // Observability is the usual one: sort-state text is empty (product finding #5), so the signal is the
            // selected item's row INDEX. Selection tracks the data item (IDL:531-546) and so survives a reorder.
            using (var setup = new TestSetupHelper(PageName))
            {
                var sortCycle = FindElement.ById<ComboBox>(SortCycleComboBox);
                if (sortCycle == null) { Verify.Fail("SortCycleComboBox was not found."); return; }

                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }

                int rowCount = rowsHost.Children.Count;
                if (rowCount < BasicItemCount) { Verify.Fail(string.Format("Need the {0} authored rows realized; saw {1}.", BasicItemCount, rowCount)); return; }


                Log.Comment("Set the Score column's cycle to DescendingAscendingNone.");
                sortCycle.SelectItemByName("SortCycleDescendingAscendingNone");
                Wait.ForIdle();

                SelectRow(rowsHost.Children[MaxScoreSourceIndex]);
                Verify.AreEqual(MaxScoreSourceIndex, IndexOfSelectedRow(BasicTable),
                    "Precondition: the highest-Score row is authored at source index 5.");

                ClickHeader(BasicTable, "Score");
                Verify.AreEqual(0, IndexOfSelectedRow(BasicTable),
                    "With DescendingAscendingNone the FIRST click must sort Descending, putting the highest Score at the top.");

                ClickHeader(BasicTable, "Score");
                Verify.AreEqual(rowCount - 1, IndexOfSelectedRow(BasicTable),
                    "The SECOND click must sort Ascending, putting the highest Score at the bottom.");

                ClickHeader(BasicTable, "Score");
                Verify.AreEqual(MaxScoreSourceIndex, IndexOfSelectedRow(BasicTable),
                    "The THIRD click must reach the cycle's None step and restore source order, returning the tracked row to index 5.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "A header click on a CanSort=False column does not sort, while a sortable sibling column still does (IDL:156-158).")]
        public void HeaderClickDoesNotSortColumnWithCanSortFalse()
        {
            // TableView.idl:156-158 states the gate as a fact about the CLICK: "Per-column opt-out for the
            // click-to-sort UX. Default true. When false the header click handler ignores this column;
            // programmatic SortByColumn still works." No API test can assert that sentence - API 7 can only
            // cover the programmatic half - so the route test is the only place the opt-out is checked.
            //
            // The ReadOnlyCity column of BasicTableView is authored CanSort="False"; Age is CanSort="True".
            // Clicking Age afterwards is a NEGATIVE CONTROL and is the point of the test: without it, a click
            // that lands nowhere, or a table that stopped sorting entirely, would satisfy "nothing moved".
            //
            // Observability is the same as the other sort tests: sort-state text is empty (product finding
            // #5) and the row peer has no index, so the signal is the selected item's row position. Selection
            // tracks the data ITEM (IDL:531-546), so it survives a reorder and its row moves with it.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                ClickHeaderTwice(BasicTable, "ReadOnlyCity");

                UIObject afterBlocked = FindSelectedRow(GetRowsHost(BasicTable));
                if (afterBlocked == null) { Verify.Fail("The selected row was lost after clicking the non-sortable header."); return; }
                Verify.IsTrue(Math.Abs(afterBlocked.BoundingRectangle.Top - oldTop) <= 2,
                    string.Format("A CanSort=False header must not reorder rows (selected row {0} -> {1}).",
                        oldTop, afterBlocked.BoundingRectangle.Top));

                // Negative control: the same gesture on a sortable column must work, proving the clicks land.
                ClickHeaderTwice(BasicTable, "Age");

                UIObject afterSortable = FindSelectedRow(GetRowsHost(BasicTable));
                if (afterSortable == null) { Verify.Fail("The selected row was lost after clicking the sortable header."); return; }
                Verify.IsTrue(afterSortable.BoundingRectangle.Top < oldTop - 2,
                    string.Format("Control: clicking the sortable Age header must still reorder rows ({0} -> {1}).",
                        oldTop, afterSortable.BoundingRectangle.Top));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Header clicks do not sort while CanUserSortColumns is false, and sort again once it is restored (IDL:574-578).")]
        public void HeaderClickDoesNotSortWhenCanUserSortColumnsIsFalse()
        {
            // TableView.idl:574-578: when CanUserSortColumns is false "header clicks do not sort;
            // programmatic sorting still works." Like the per-column gate this is a statement about the click
            // route, so it is untestable from the API tier. Kept separate from
            // HeaderClickDoesNotSortColumnWithCanSortFalse on purpose: the two gates are different properties
            // consulted at different points, and a gate honored in one place and forgotten in the other is
            // exactly the regression worth catching.
            //
            // Re-checking the box is the negative control - it proves the table can still sort, so the first
            // half measured a gate holding rather than a dead click path.
            using (var setup = new TestSetupHelper(PageName))
            {
                var canUserSort = FindElement.ById<CheckBox>(CanUserSortColumnsCheckBox);
                if (canUserSort == null) { Verify.Fail("CanUserSortColumnsCheckBox was not found."); return; }

                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                canUserSort.Uncheck();
                Wait.ForIdle();

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                ClickHeaderTwice(BasicTable, "Age");

                UIObject afterBlocked = FindSelectedRow(GetRowsHost(BasicTable));
                if (afterBlocked == null) { Verify.Fail("The selected row was lost while sorting was disabled."); return; }
                Verify.IsTrue(Math.Abs(afterBlocked.BoundingRectangle.Top - oldTop) <= 2,
                    string.Format("With CanUserSortColumns false a header click must not reorder rows ({0} -> {1}).",
                        oldTop, afterBlocked.BoundingRectangle.Top));

                canUserSort.Check();
                Wait.ForIdle();

                ClickHeaderTwice(BasicTable, "Age");

                UIObject afterRestored = FindSelectedRow(GetRowsHost(BasicTable));
                if (afterRestored == null) { Verify.Fail("The selected row was lost after re-enabling sorting."); return; }
                Verify.IsTrue(afterRestored.BoundingRectangle.Top < oldTop - 2,
                    string.Format("Control: with CanUserSortColumns restored the same clicks must sort ({0} -> {1}).",
                        oldTop, afterRestored.BoundingRectangle.Top));
            }
        }

        [TestMethod]
        [TestProperty("Description", "A resize drag on a sortable header widens the column without also sorting it.")]
        public void HeaderResizeDragDoesNotSortTheColumn()
        {
            // SPEC-SILENT, REASONED: dev-spec:121-129 gives the gripper its own gesture protocol (BeginDrag ->
            // DragDelta -> EndDrag), while TableView.cpp:1642 puts a Tapped handler on the header cell the
            // gripper sits inside. Whether a manipulation that ends over the header also raises Tapped is not
            // settled by either document. Platform convention (WPF/WinForms DataGrid, the Community Toolkit
            // sizer) is that a drag is not a click, and the alternative - every resize silently reordering the
            // table - is not a behavior any document claims.
            //
            // Distinct from 8 PointerResizeDragChangesColumnWidth, which owns "the drag reaches the width
            // engine". Here the width growth is only a PRECONDITION proving the drag happened; the assertion
            // under test is that the rows did not move.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                UIObject nameHeader = GetHeader(BasicTable, "Name");
                if (nameHeader == null) { Verify.Fail("The Name header was not found."); return; }
                int widthBefore = nameHeader.BoundingRectangle.Width;

                DragColumnBoundary(nameHeader, 40);

                UIObject afterDrag = GetHeader(BasicTable, "Name");
                if (afterDrag == null) { Verify.Fail("The Name header was not found after the drag."); return; }
                Verify.IsTrue(afterDrag.BoundingRectangle.Width > widthBefore,
                    string.Format("Precondition: the drag should widen the column ({0} -> {1}).",
                        widthBefore, afterDrag.BoundingRectangle.Width));

                UIObject selected = FindSelectedRow(GetRowsHost(BasicTable));
                if (selected == null) { Verify.Fail("The selected row was lost after the resize drag."); return; }
                Verify.IsTrue(Math.Abs(selected.BoundingRectangle.Top - oldTop) <= 2,
                    string.Format("A resize drag must not also sort the column (selected row {0} -> {1}).",
                        oldTop, selected.BoundingRectangle.Top));
            }
        }

        #endregion

        #region 4. Group header input (keyboard leg)

        [TestMethod]
        [TestProperty("Description", "Verifies Enter and Space on a group header reached by Tab collapse and expand the group's rows, with the ExpandCollapse peer agreeing at each step.")]
        public void GroupHeaderKeyboardActivationTogglesRowVisibility()
        {
            // Interaction plan §4 GroupHeaderActivationExpandsAndCollapsesRows (keyboard leg: Enter and Space).
            // Lives in the keyboard file, beside the other key-routing tests; the pointer leg
            //   (GroupHeaderPointerClickTogglesRowVisibility) stays in the pointer file. Splitting them keeps one
            //   failure mode per test: pointer hit-testing and key routing to the header fail independently.
            // Spec: RequestToggle is raised from TableViewGroupHeader::OnKeyDown (TableViewGroupHeader.cpp) for the
            //   activation keys.
            // FOCUS ROUTE: focus is taken by TABBING in, not by UIA SetFocus. SetFocus is a provider call, not a
            //   keyboard gesture, so it skipped the tab-stop resolution this test exists to prove.
            //   PRODUCT FINDING #15 IS STILL OPEN and is NOT covered here: on the UIA SetFocus route - the one an
            //   assistive technology takes - the header loses focus across its own collapse. That needs its own
            //   test in the accessibility section; do not read this test's pass as closing #15.
            // Failure means: key routing to the group header is broken, so a keyboard user cannot collapse a group.
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

                if (!TabToFirstGroupHeader(tableView, rowsHost, groupHeader))
                {
                    Verify.Fail("Tabbing from the page controls never put keyboard focus on the first group header.");
                    return;
                }

                Log.Comment("Collapse the group with Enter.");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.IsLessThan(CountRows(rowsHost), baselineRows, "Enter on the focused group header must collapse its rows.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "After Enter the header must report Collapsed.");
                Verify.IsTrue(groupHeader.HasKeyboardFocus, "The group header must KEEP keyboard focus across its own collapse - otherwise the next key lands on a different group and the user cannot re-open what they just closed.");

                Log.Comment("Expand the group again with Enter.");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.AreEqual(baselineRows, CountRows(rowsHost), "A second Enter must restore the rows.");
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "After the second Enter the header must report Expanded.");

                Log.Comment("Collapse the group with Space.");
                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();
                Verify.IsLessThan(CountRows(rowsHost), baselineRows, "Space on the focused group header must collapse its rows.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "After Space the header must report Collapsed.");
                Verify.IsTrue(groupHeader.HasKeyboardFocus, "The group header must keep keyboard focus across a Space collapse, for the same reason as Enter.");

                Log.Comment("Expand the group again with Space.");
                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();
                Verify.AreEqual(baselineRows, CountRows(rowsHost), "A second Space must restore the rows.");
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "After the second Space the header must report Expanded.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Right expands and Left collapses a group header reached by Tab, matching the platform convention TreeView implements.")]
        public void GroupHeaderArrowKeysExpandAndCollapse()
        {
            // Interaction plan §1 GroupHeaderArrowKeysExpandAndCollapse.
            // SPEC GAP, recorded deliberately: neither the dev spec nor the functional spec states any keyboard
            //   contract for group headers beyond activation. The expectation is platform convention, as
            //   implemented by TreeView (TreeViewKeyDownLeftToRightTest asserts exactly this for its items).
            // Failure means: group headers respond to Enter/Space only, so a keyboard user navigating with the
            //   arrow keys - the natural gesture for a hierarchy - cannot open or close a group.
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

                if (!TabToFirstGroupHeader(tableView, rowsHost, groupHeader))
                {
                    Verify.Fail("Tabbing from the page controls never put keyboard focus on the first group header.");
                    return;
                }

                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "Precondition: the group starts expanded.");

                Log.Comment("Left collapses the expanded group.");
                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                Verify.IsLessThan(CountRows(rowsHost), baselineRows, "Left on the focused group header must collapse its rows.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "After Left the header must report Collapsed.");
                Verify.IsTrue(groupHeader.HasKeyboardFocus, "The group header must KEEP keyboard focus across its own collapse - otherwise Right lands on a different group and the user cannot re-open what they just closed.");

                Log.Comment("Left again on an already collapsed group is a no-op, not a toggle.");
                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState, "Left must not re-expand an already collapsed group.");

                Log.Comment("Right expands the collapsed group.");
                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                Verify.AreEqual(baselineRows, CountRows(rowsHost), "Right must restore the collapsed rows.");
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "After Right the header must report Expanded.");

                Log.Comment("Right again on an already expanded group is a no-op, not a toggle.");
                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "Right must not collapse an already expanded group.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Enter and Space on a group header each raise ToggleRequested exactly once, carrying that group's key, and that the key survives a handler that mutates the header re-entrantly.")]
        public void GroupHeaderKeyboardToggleRaisesToggleRequestedWithGroupKey()
        {
            // Interaction plan §4 GroupHeaderToggleRequestedCarriesTheGroupKey (keyboard leg).
            // Spec: TableView.idl:350-355 - the key rides on the args "so a handler that re-enters and mutates
            //   the header still sees the key that was actually activated". The page handler reads the key,
            //   flips sender.IsExpanded, and reads it again, recording "<key>|<key>;" per raise.
            // Route: OnKeyDown raises RequestToggle, a different entry point from OnPointerReleased, so the
            //   pointer leg does not substitute for this one. Neither is reachable from the automation peer,
            //   which is why the item is not in the API plan (TableView_Grouping_APITests.cs:575).
            // Failure means: keys reach the header's expansion state but not its public event, so a handler
            //   built on ToggleRequested silently misses keyboard users.
            using (var setup = new TestSetupHelper(PageName))
            {
                // Resolved before the grouped table realizes - a FindElement afterwards forces a full tree
                // re-walk that descends into TableViewRow children and asserts the app (finding #13).
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
                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                hookButton.InvokeAndWait();
                Verify.AreEqual("hooked;", toggleReport.Value, "Precondition: the page must have hooked ToggleRequested on the realized group headers.");

                if (!TabToFirstGroupHeader(tableView, rowsHost, groupHeader))
                {
                    Verify.Fail("Tabbing from the page controls never put keyboard focus on the first group header.");
                    return;
                }

                // The page authors the grouped source from Cities = { Redmond, Seattle, Bellevue } in source
                // order, so the first group's key is Redmond.
                Log.Comment("Activate the focused header with Enter.");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.AreEqual("hooked;Redmond|Redmond;", toggleReport.Value,
                    "Enter must raise ToggleRequested exactly once, carrying the group's key both before and after the handler mutates the header.");

                Log.Comment("Activate the same header with Space.");
                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();
                Verify.AreEqual("hooked;Redmond|Redmond;Redmond|Redmond;", toggleReport.Value,
                    "Space must raise the event exactly once more, for the same group.");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product: focus does not stay at the same position when a sort reorders rows (dev-spec Keyboard re-shape rule).
        [TestProperty("Description", "When a sort reorders the rows while a row is focused, focus stays at the same position (now another record).")]
        public void FocusStaysAtSamePositionWhenSortReordersRows()
        {
            // Interaction plan §3 FocusStaysAtSamePositionWhenSortReordersRows (owner decision; dev-spec Keyboard
            // "Re-shape while focused"). The sort is driven by the header peer's UIA Invoke, which does not move keyboard
            // focus, so focus is inside the body throughout. Failure means a sort drags focus with the old record.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < BasicItemCount) { Verify.Fail("Need every authored Basic row realized."); return; }

                const int Position = 2;
                rowsHost.Children[Position].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(GetRowsHost(BasicTable).Children[Position].HasKeyboardFocus, "Precondition: row 2 should take focus.");
                Verify.IsTrue(GetRowsHost(BasicTable).Children[Position].Name.Contains(BasicName(Position)), "Precondition: row 2 holds Person 2.");

                UIObject age = GetHeader(BasicTable, "Age");
                if (age == null) { Verify.Fail("The Age header was not found."); return; }
                var invoke = new InvokeImplementation(age);
                invoke.Invoke(); // None -> Ascending
                Wait.ForIdle();
                invoke = new InvokeImplementation(GetHeader(BasicTable, "Age"));
                invoke.Invoke(); // Ascending -> Descending
                Wait.ForIdle();

                rowsHost = GetRowsHost(BasicTable);
                UIObject focused = FindFocusedBodyElement(rowsHost);
                Log.Comment("After the sort focus is on: {0}.", focused == null ? "<none>" : focused.Name);
                if (focused == null) { Verify.Fail("Focus must stay inside the body across a sort."); return; }

                Verify.AreEqual(rowsHost.Children[Position].RuntimeId, focused.RuntimeId, "Focus must stay at the same position (row 2) across a sort.");
                Verify.IsTrue(focused.Name.Contains(BasicName(BasicIdAtPositionWhenAgeDescending(Position))), "Negative control: position 2 must now hold Person 9 (Age descending), proving the rows re-sorted.");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product: focus leaves the body when the focused record is filtered out (dev-spec Keyboard re-shape rule).
        [TestProperty("Description", "When a filter removes the focused record, focus stays at the same projected position.")]
        public void FocusStaysAtSamePositionWhenFilterRemovesFocusedRecord()
        {
            // Interaction plan §3 FocusStaysAtSamePositionWhenFilterRemovesFocusedRecord (owner decision; dev-spec Keyboard
            // "Re-shape while focused ... also applies when the focused record was filtered out"). The page filter acts on
            // the grouped source only. Projection before: H-Redmond, G0, G3, G6, H-Seattle, G1, G4, G7, H-Bellevue, G2, G5,
            // G8. After removing Seattle, position 5 is G2. Failure means a filter drops focus out of the table.
            using (var setup = new TestSetupHelper(PageName))
            {
                var filter = FindElement.ById<Button>(FilterSourceButton);
                if (filter == null) { Verify.Fail("FilterSourceButton was not found."); return; }

                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null) { return; }

                const int Position = 5;
                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                if (rowsHost.Children.Count < GroupedItemCount + Cities.Length) { Verify.Fail("Need the full grouped projection (3 headers + 9 rows) realized."); return; }

                UIObject target = rowsHost.Children[Position];
                Verify.IsTrue(target.Name.Contains(GroupedName(1)), "Precondition: projected position 5 holds Grouped 1, the first Seattle row.");
                target.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(target.HasKeyboardFocus, "Precondition: the Grouped 1 row should take focus.");

                filter.InvokeAndWait();
                Wait.ForIdle();

                rowsHost = tableView.Children[tableView.Children.Count - 1];
                UIObject focused = FindFocusedBodyElement(rowsHost);
                Log.Comment("After the filter focus is on: {0}.", focused == null ? "<none>" : focused.Name);
                if (focused == null) { Verify.Fail("Focus must stay inside the body when the focused record is filtered out."); return; }

                Verify.AreEqual(rowsHost.Children[Position].RuntimeId, focused.RuntimeId, "Focus must stay at the same projected position (5).");
                Verify.IsTrue(focused.Name.Contains(GroupedName(2)), "Position 5 must now be Grouped 2, the first Bellevue row.");
            }
        }

        #endregion

        #region N.1 Keyboard - cell level (new tests for the #11820 model)

        [TestMethod]
        [TestProperty("Description", "At cell level Left/Right move one visible column within the focused row and do not wrap across rows or leave the table (dev-spec:201).")]
        public void CellLeftRightMoveWithinRowWithoutWrapping()
        {
            // Interaction plan N.1 CellLeftRightMoveWithinRowWithoutWrapping.
            // dev-spec:201 - Left/Right move the cell cursor WITHIN the focused row. No-wrap is the WPF/ListView
            // convention of the #11820 model (spec debt). Failure means horizontal movement skips cells, wraps into
            // another row, or escapes the table at an edge.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!DrillIntoRow(BasicTable, 1)) { return; }

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 1, 1, "One Right from cell 0 must reach cell 1 of the SAME row.");

                int cellCount = GetRowsHost(BasicTable).Children[1].Children.Count;
                Verify.IsGreaterThan(cellCount, 2, "Precondition: BasicTableView rows expose one cell per visible column.");

                KeyboardHelper.PressKey(Key.Right, numPresses: (uint)(cellCount - 2));
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 1, cellCount - 1, "Right must reach the last visible cell of the row.");

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 1, cellCount - 1, "Right on the last cell must not wrap to the next row or leave the table.");

                KeyboardHelper.PressKey(Key.Left, numPresses: (uint)(cellCount - 1));
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 1, 0, "Left must walk back to cell 0 of the same row.");

                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                UIObject row = GetRowsHost(BasicTable).Children[1];
                Verify.IsTrue(row.HasKeyboardFocus, "Left on cell 0 must return to the row, not wrap to the previous row's last cell.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "At cell level Home/End move to the first/last cell of the current row and Ctrl+Home/Ctrl+End to the first/last cell of the table (dev-spec:201).")]
        public void CellHomeEndStayInRowAndCtrlHomeEndJumpTable()
        {
            // Interaction plan N.1 CellHomeEndStayInRowAndCtrlHomeEndJumpTable.
            // dev-spec:201, stated outright. Failure means at cell level Home/End still jump rows, throwing the user
            // to another record, or the grid-wide jump is missing.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!DrillIntoRow(BasicTable, 2)) { return; }

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 2, 1, "Precondition: Right from cell 0 reaches cell 1.");

                UIObject rowsHost = GetRowsHost(BasicTable);
                int rowCount = rowsHost.Children.Count;
                int lastCell = rowsHost.Children[2].Children.Count - 1;

                KeyboardHelper.PressKey(Key.End);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 2, lastCell, "End at cell level must reach the last cell of the SAME row.");

                KeyboardHelper.PressKey(Key.Home);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 2, 0, "Home at cell level must reach the first cell of the SAME row.");

                KeyboardHelper.PressKey(Key.End, ModifierKey.Control);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, rowCount - 1, lastCell, "Ctrl+End must reach the last cell of the last row.");

                KeyboardHelper.PressKey(Key.Home, ModifierKey.Control);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 0, 0, "Ctrl+Home must reach the first cell of the first row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "At cell level Down/Up move to the same visible column in the next/previous row (dev-spec:201).")]
        public void CellUpDownPreserveColumn()
        {
            // Interaction plan N.1 CellUpDownPreserveColumn.
            // dev-spec:201 - "move to the same visible column in another row". Failure means vertical movement drops
            // to row level or column 0, so walking down one field across records is impossible.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!DrillIntoRow(BasicTable, 1)) { return; }

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 1, 1, "Precondition: Right from cell 0 reaches cell 1 (Age).");

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 2, 1, "Down at cell level must reach the SAME column in the next row.");
                Verify.IsFalse(GetRowsHost(BasicTable).Children[2].HasKeyboardFocus, "Down at cell level must not drop to the row container.");

                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 1, 1, "Up at cell level must return to the SAME column in the previous row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Ctrl+Down moves row focus without selecting; plain Down then moves and selects (positive control).")]
        public void CtrlDownMovesFocusWithoutSelecting()
        {
            // Interaction plan N.1 CtrlDownMovesFocusWithoutSelecting.
            // Selection-follows-focus is KeyboardFocusMoveCarriesSelection's claim; the Ctrl opt-out is the ListView
            // convention (spec debt). Failure means the cursor cannot move without acting on a row.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                rowsHost.Children[0].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(rowsHost.Children[0].HasKeyboardFocus, "Precondition: row 0 should take focus.");
                VerifyNoRowSelected("focusing row 0 programmatically");

                KeyboardHelper.PressKey(Key.Down, ModifierKey.Control);
                Wait.ForIdle();

                rowsHost = GetRowsHost(BasicTable);
                Verify.IsTrue(rowsHost.Children[1].HasKeyboardFocus, "Ctrl+Down must move focus to row 1.");
                VerifyNoRowSelected("moving with Ctrl+Down");

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                rowsHost = GetRowsHost(BasicTable);
                Verify.IsTrue(rowsHost.Children[2].HasKeyboardFocus, "Positive control: plain Down must move focus to row 2.");
                Verify.IsTrue(IsSelected(rowsHost.Children[2]), "Positive control: plain Down must select the row it lands on.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Space selects the focused row both from row level and from cell level, without moving focus.")]
        public void SpaceSelectsFocusedRowFromRowAndCellLevel()
        {
            // Interaction plan N.1 SpaceSelectsFocusedRowFromRowAndCellLevel.
            // Space for row selection is the ListView/WPF DataGrid convention of the #11820 model (spec debt). Each
            // leg moves with Ctrl+Down first so selection-follows-focus cannot already have selected the row.
            // Failure means a keyboard user who moved with Ctrl has no way to select, or Space is swallowed at
            // cell level.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                rowsHost.Children[0].SetFocus();
                Wait.ForIdle();

                KeyboardHelper.PressKey(Key.Down, ModifierKey.Control);
                Wait.ForIdle();
                VerifyNoRowSelected("moving with Ctrl+Down (row level)");

                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();

                rowsHost = GetRowsHost(BasicTable);
                Verify.IsTrue(IsSelected(rowsHost.Children[1]), "Space on a focused row must select it.");
                Verify.IsTrue(rowsHost.Children[1].HasKeyboardFocus, "Space must not move focus off the row.");

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 1, 0, "Precondition: Right drills into row 1's first cell.");

                KeyboardHelper.PressKey(Key.Down, ModifierKey.Control);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 2, 0, "Precondition: Ctrl+Down at cell level reaches row 2's first cell.");

                rowsHost = GetRowsHost(BasicTable);
                Verify.IsFalse(IsSelected(rowsHost.Children[2]), "Precondition: Ctrl+Down must not have selected row 2.");

                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();

                rowsHost = GetRowsHost(BasicTable);
                Verify.IsTrue(IsSelected(rowsHost.Children[2]), "Space on a focused cell must select that cell's row.");
                Verify.IsFalse(IsSelected(rowsHost.Children[1]), "SelectionMode.Single: selecting row 2 must clear row 1.");
                VerifyFocusedCell(BasicTable, 2, 0, "Space must not move focus off the cell.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Enter on a cell hosting a control moves focus into the control, arrow keys stay inside it, and Escape returns to the cell.")]
        public void EnterOnInteractiveCellEntersContentAndEscapeReturnsToCell()
        {
            // Interaction plan N.1 EnterOnInteractiveCellEntersContentAndEscapeReturnsToCell.
            // WAI-ARIA grid interactive-content convention, stated as a design point in #11820 (spec debt). Uses the
            // page's trailing Action column, whose cell hosts a Button. Failure means hosted controls are keyboard-
            // unreachable, arrows yank the user out of a control, or there is no way back to grid navigation.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!DrillIntoRow(BasicTable, 0)) { return; }

                KeyboardHelper.PressKey(Key.End);
                Wait.ForIdle();

                UIObject row = GetRowsHost(BasicTable).Children[0];
                int actionCell = row.Children.Count - 1;
                VerifyFocusedCell(BasicTable, 0, actionCell, "Precondition: End reaches the Action cell.");

                UIObject button = FindDescendantByName(row.Children[actionCell], "RowActionButton");
                if (button == null) { Verify.Fail("The Action cell does not expose its hosted RowActionButton to UIA."); return; }

                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.IsTrue(button.HasKeyboardFocus, "Enter on a cell hosting a control must move focus into that control.");

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                Verify.IsTrue(button.HasKeyboardFocus, "Arrow keys must stay inside the cell's interactive content.");

                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();
                Verify.IsFalse(button.HasKeyboardFocus, "Escape must leave the hosted control.");
                VerifyFocusedCell(BasicTable, 0, actionCell, "Escape must return focus to the Action cell itself.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Under RTL the drill-in and cell arrows follow reading order: Left drills in and moves forward, Right moves back and pops out.")]
        public void RightToLeftDrillInAndCellArrowsMirror()
        {
            // Interaction plan N.1 RightToLeftDrillInAndCellArrowsMirror.
            // dev-spec:131 makes reading order the frame for horizontal movement; TreeView's RTL key tests are the
            // mirroring precedent; drill-in itself is spec debt. Failure means arrows act in screen direction under RTL.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(RtlPivotItem)) { Verify.Fail(GoToButton(RtlPivotItem) + " was not found."); return; }

                UIObject rowsHost = GetRowsHost(RtlTable);
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 2) { Verify.Fail("Need several realized rows in RtlTableView."); return; }

                rowsHost.Children[1].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(rowsHost.Children[1].HasKeyboardFocus, "Precondition: row 1 of RtlTableView should take focus.");

                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                VerifyFocusedCell(RtlTable, 1, 0, "Under RTL, Left (reading-order forward) must drill into the first cell.");

                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                VerifyFocusedCell(RtlTable, 1, 1, "Under RTL, Left at cell level must move forward to cell 1.");

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedCell(RtlTable, 1, 0, "Under RTL, Right at cell level must move back to cell 0.");

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                Verify.IsTrue(GetRowsHost(RtlTable).Children[1].HasKeyboardFocus,
                    "Under RTL, Right on the first cell must return to the row.");
            }
        }

        #endregion

        #region N.2 Keyboard - header band and tab order (new tests for the #11820 model)

        [TestMethod]
        [TestProperty("Description", "In the header band Left/Right move between headers and clamp at the ends; Up leaves focus in the band.")]
        public void HeaderBandArrowsMoveBetweenHeaders()
        {
            // Interaction plan N.2 HeaderBandArrowsMoveBetweenHeaders (was ...AndStayInBand).
            // dev-spec Keyboard: header-band Left/Right move between visible headers without wrapping; Up stays in the
            // band. Down now LEAVES the band (owner decision) and is owned by UpFromFirstRowMovesToHeaderAndDownReturnsToFirstRow.
            // Failure means headers are unreachable, Up walks out of the table, or the band wraps.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject name = GetHeader(BasicTable, "Name");
                if (name == null) { Verify.Fail("The Name header was not found."); return; }
                name.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(name.HasKeyboardFocus, "Precondition: the Name header should take focus.");

                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                VerifyFocusedHeader(BasicTable, "Name", "Left on the first header must not leave it (no wrap, no exit).");

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedHeader(BasicTable, "Age", "Right must move focus to the next header.");

                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                VerifyFocusedHeader(BasicTable, "Age", "Up must leave focus on the header band.");
                Verify.IsNull(FindFocusedBodyElement(GetRowsHost(BasicTable)), "Up from a header must not enter the body.");

                UIObject tableView = FindElement.ById(BasicTable);
                UIObject headerHost = tableView.Children[0];
                int headerCount = headerHost.Children.Count;
                string lastHeaderName = headerHost.Children[headerCount - 1].Name;

                KeyboardHelper.PressKey(Key.Right, numPresses: (uint)headerCount);
                Wait.ForIdle();
                VerifyFocusedHeader(BasicTable, lastHeaderName, "Right past the last header must leave the last header focused.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "A CanSort=False header is still keyboard-focusable and Enter on it does not sort; Enter on Age does (positive control).")]
        public void NonSortableHeaderIsFocusableAndEnterDoesNotSort()
        {
            // Interaction plan N.2 NonSortableHeaderIsFocusableAndEnterDoesNotSort.
            // TableView.idl:156-158 (CanSort gates the click-to-sort UX) + dev-spec:201 (Enter/Space sort) => the
            // gate covers the key route. Failure means non-sortable headers are unreachable, or Enter ignores CanSort.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                int bottomIndex = rowsHost.Children.Count - 1;
                if (bottomIndex < 2) { Verify.Fail("Need several realized rows."); return; }

                SelectRow(rowsHost.Children[bottomIndex]);
                Verify.AreEqual(bottomIndex, IndexOfSelectedRow(BasicTable), "Precondition: the bottom row is selected.");

                UIObject city = GetHeader(BasicTable, "ReadOnlyCity");
                if (city == null) { Verify.Fail("The ReadOnlyCity header was not found."); return; }
                city.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(city.HasKeyboardFocus, "A CanSort=False header must still take keyboard focus.");

                KeyboardHelper.PressKey(Key.Enter, numPresses: 2);
                Wait.ForIdle();
                Verify.AreEqual(bottomIndex, IndexOfSelectedRow(BasicTable), "Enter on a CanSort=False header must not reorder rows.");

                UIObject age = GetHeader(BasicTable, "Age");
                if (age == null) { Verify.Fail("The Age header was not found."); return; }
                age.SetFocus();
                Wait.ForIdle();

                KeyboardHelper.PressKey(Key.Enter, numPresses: 2); // None -> Ascending -> Descending
                Wait.ForIdle();
                Verify.IsLessThan(IndexOfSelectedRow(BasicTable), bottomIndex, "Positive control: Enter twice on Age must sort Descending and lift the bottom row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Each Space press on a focused header advances the column's sort cycle exactly one step (dev-spec:201).")]
        public void HeaderSpaceTogglesSortOncePerPress()
        {
            // Interaction plan N.2 HeaderSpaceTogglesSortOncePerPress.
            // dev-spec:201 makes Space a sort key. The Score column with DescendingAscendingNone lands the tracked row
            // on three distinct indices (0, last, 5), so a press that fires twice is visible. Failure means Space does
            // not sort, or one press-release advances two steps.
            using (var setup = new TestSetupHelper(PageName))
            {
                var sortCycle = FindElement.ById<ComboBox>(SortCycleComboBox);
                if (sortCycle == null) { Verify.Fail("SortCycleComboBox was not found."); return; }

                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }

                int rowCount = rowsHost.Children.Count;
                if (rowCount < BasicItemCount) { Verify.Fail(string.Format("Need the {0} authored rows realized; saw {1}.", BasicItemCount, rowCount)); return; }


                sortCycle.SelectItemByName("SortCycleDescendingAscendingNone");
                Wait.ForIdle();

                SelectRow(rowsHost.Children[MaxScoreSourceIndex]);
                Verify.AreEqual(MaxScoreSourceIndex, IndexOfSelectedRow(BasicTable), "Precondition: the highest-Score row is authored at source index 5.");

                UIObject score = GetHeader(BasicTable, "Score");
                if (score == null) { Verify.Fail("The Score header was not found."); return; }
                score.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(score.HasKeyboardFocus, "Precondition: the Score header should take focus.");

                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();
                Verify.AreEqual(0, IndexOfSelectedRow(BasicTable), "Space 1 must sort Descending (one step), putting the highest Score at the top.");

                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();
                Verify.AreEqual(rowCount - 1, IndexOfSelectedRow(BasicTable), "Space 2 must sort Ascending (one step), putting the highest Score at the bottom.");

                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();
                Verify.AreEqual(MaxScoreSourceIndex, IndexOfSelectedRow(BasicTable), "Space 3 must reach None and restore source order.");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Tab from the header band does not land on a cell since the Action (button) column was added; product or fixture TBD.
        [TestProperty("Description", "Shift+Tab from a cell lands on that column's header, and Tab back lands on a cell in the same column.")]
        public void TabBetweenBandsPreservesColumn()
        {
            // Interaction plan N.2 TabBetweenBandsPreservesColumn.
            // Shared column cursor across bands is the #11820 model (spec debt), consistent with dev-spec:201 making
            // the column the unit of vertical movement. Failure means crossing bands loses the user's column.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!DrillIntoRow(BasicTable, 0)) { return; }

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 0, 1, "Precondition: Right from cell 0 reaches cell 1 (Age).");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();
                VerifyFocusedHeader(BasicTable, "Age", "Shift+Tab from the Age cell must land on the Age header.");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                int rowIndex, cellIndex;
                FindFocusedCell(BasicTable, out rowIndex, out cellIndex);
                Log.Comment("After Tab back: row={0}, cell={1}.", rowIndex, cellIndex);
                Verify.AreEqual(1, cellIndex, "Tab from the header band must return to a cell in the same column (Age).");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product: Up from the first row does not reach the header band, and Down is absorbed in the band (dev-spec Keyboard).
        [TestProperty("Description", "Up from the first row moves to the header band and Down from a header returns to the first row, keeping the column at cell level.")]
        public void UpFromFirstRowMovesToHeaderAndDownReturnsToFirstRow()
        {
            // Interaction plan N.2 UpFromFirstRowMovesToHeaderAndDownReturnsToFirstRow (owner decision; dev-spec Keyboard:
            // "Up from the first row moves to the header band" / header band "Down moves to the first row"; shared column
            // cursor). Failure means the header band and the rows are only joined by Tab. The Down half is expected to
            // fail on main, which absorbs Down in the band.
            using (var setup = new TestSetupHelper(PageName))
            {
                // Row-level leg.
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                rowsHost.Children[0].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(rowsHost.Children[0].HasKeyboardFocus, "Precondition: row 0 should take focus.");

                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                UIObject header = FindFocusedHeader(BasicTable);
                Log.Comment("Row-level Up landed on header: {0}.", header == null ? "<none>" : header.Name);
                Verify.IsNotNull(header, "Up from the first row must move focus to the header band.");
                Verify.IsNull(FindFocusedBodyElement(GetRowsHost(BasicTable)), "No row may keep focus after Up from the first row.");

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                UIObject bodyFocus = FindFocusedBodyElement(GetRowsHost(BasicTable));
                Verify.IsNotNull(bodyFocus, "Down from a header must move focus to the first row.");
                if (bodyFocus != null)
                {
                    Verify.AreEqual(GetRowsHost(BasicTable).Children[0].RuntimeId, bodyFocus.RuntimeId,
                        "Down from a header must land on the FIRST row.");
                }

                // Cell-level leg: the column is kept across the band boundary.
                if (!DrillIntoRow(BasicTable, 0)) { return; }
                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 0, 1, "Precondition: Right from cell 0 reaches cell 1 (Age).");

                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                VerifyFocusedHeader(BasicTable, "Age", "Up from the Age cell of the first row must land on the Age header.");

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                VerifyFocusedCell(BasicTable, 0, 1, "Down from the Age header must return to the first row's Age cell.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "After a sort moves the focused record, Tab back into the body returns to the same record, not the old index.")]
        public void BodyTabReentryReturnsToSameRecordAfterSort()
        {
            // Interaction plan N.2 BodyTabReentryReturnsToSameRecordAfterSort (owner decision; dev-spec Keyboard:
            // "Tabbing back into the body returns to the same record that last had focus, even after a sort or filter").
            // The record is tracked through selection, which follows the item across a re-order (TableView.idl:531-546).
            // Failure means Tab drops the user on whatever record now occupies the old position.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject rowsHost = GetRowsHost(BasicTable);
                if (rowsHost == null) { return; }
                int bottomIndex = rowsHost.Children.Count - 1;
                if (bottomIndex < 2) { Verify.Fail("Need several realized rows."); return; }

                SelectRow(rowsHost.Children[bottomIndex]);
                rowsHost.Children[bottomIndex].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(GetRowsHost(BasicTable).Children[bottomIndex].HasKeyboardFocus, "Precondition: the bottom row should take focus.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();
                if (FindFocusedHeader(BasicTable) == null) { Verify.Fail("Precondition: Shift+Tab from the body should land on the header band."); return; }

                UIObject age = GetHeader(BasicTable, "Age");
                age.SetFocus();
                Wait.ForIdle();
                VerifyFocusedHeader(BasicTable, "Age", "Precondition: the Age header should take focus.");

                KeyboardHelper.PressKey(Key.Enter, numPresses: 2); // None -> Ascending -> Descending: oldest Age to the top
                Wait.ForIdle();

                int recordIndex = IndexOfSelectedRow(BasicTable);
                Log.Comment("After the sort the tracked record is at index {0} (was {1}).", recordIndex, bottomIndex);
                Verify.IsTrue(recordIndex >= 0 && recordIndex != bottomIndex, "Precondition: the sort must have moved the tracked record.");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                rowsHost = GetRowsHost(BasicTable);
                UIObject reentered = FindFocusedBodyElement(rowsHost);
                if (reentered == null) { Verify.Fail("Tab from the header band must re-enter the body."); return; }

                Verify.AreEqual(rowsHost.Children[recordIndex].RuntimeId, reentered.RuntimeId,
                    "Tab re-entry must return to the row holding the same record (now at index " + recordIndex + "), not the old index.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Ctrl+Right on a focused header neither resizes nor navigates; Alt+Right does resize (positive control).")]
        public void HeaderCtrlArrowDoesNotResize()
        {
            // Interaction plan N.2 HeaderCtrlArrowDoesNotResize (owner decision; dev-spec resize Keyboard: "Alt is the only
            // resize modifier: Ctrl+Arrow on a header does nothing"). Failure means Ctrl acts as an undocumented second chord.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject header = GetHeader(BasicTable, "Name");
                if (header == null) { Verify.Fail("The Name header was not found."); return; }
                header.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(header.HasKeyboardFocus, "Precondition: the Name header should take focus.");

                int w0 = GetHeader(BasicTable, "Name").BoundingRectangle.Width;

                KeyboardHelper.PressKey(Key.Right, ModifierKey.Control);
                Wait.ForIdle();
                int w1 = GetHeader(BasicTable, "Name").BoundingRectangle.Width;
                Log.Comment("Ctrl+Right: {0} -> {1}.", w0, w1);
                Verify.AreEqual(w0, w1, "Ctrl+Right must not resize the column; Alt is the only resize modifier.");
                VerifyFocusedHeader(BasicTable, "Name", "Ctrl+Right must not move focus off the Name header.");

                KeyboardHelper.PressKey(Key.Right, ModifierKey.Alt);
                Wait.ForIdle();
                int w2 = GetHeader(BasicTable, "Name").BoundingRectangle.Width;
                Log.Comment("Alt+Right: {0} -> {1}.", w1, w2);
                Verify.IsTrue(w2 > w1, "Positive control: Alt+Right must widen the column.");
            }
        }

        #endregion

        #region Helpers

        // Walks keyboard focus from the page's GoToGroupedButton into the grouped table with Tab, which is the
        // route a real keyboard user takes. UIA SetFocus is deliberately NOT used on the header: it is a provider
        // call, not a gesture, so it would skip the tab-stop resolution these tests exist to prove. It is also
        // the route on which product finding #15 (focus lost across a collapse) still reproduces - open, and
        // owed its own accessibility test.
        // The anchor button is the one SelectGroupedPivotAndGetTable already resolved - it is NOT looked up again
        // here, because any FindElement call once the grouped table is realized forces ElementCache.Refresh() to
        // re-walk the whole tree, which descends into TableViewRow children and asserts the app (finding #13,
        // measured: 0xC0000420 inside this very helper).
        private static bool TabToFirstGroupHeader(UIObject tableView, UIObject rowsHost, UIObject groupHeader)
        {
            if (GroupedAnchorButton == null) { Verify.Fail("The grouped-pivot anchor button was not captured."); return false; }

            GroupedAnchorButton.SetFocus();
            Wait.ForIdle();

            for (int i = 0; i < 40; i++)
            {
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                if (groupHeader.HasKeyboardFocus)
                {
                    Log.Comment("The first group header took keyboard focus after {0} Tab press(es).", i + 1);
                    return true;
                }

                Log.Comment("Tab {0}: focus is on {1}.", i + 1, DescribeFocusInsideTable(tableView, rowsHost));
            }

            return false;
        }

        // Row-level only: reports whether focus has reached the table, and which of the rows host's own children
        // holds it. Never asks a row for its children (finding #13).
        private static string DescribeFocusInsideTable(UIObject tableView, UIObject rowsHost)
        {
            if (tableView.HasKeyboardFocus) { return "the TableView itself"; }

            int index = 0;
            foreach (UIObject child in rowsHost.Children)
            {
                if (child != null && child.HasKeyboardFocus)
                {
                    return string.Format("rows-host child {0} ({1})", index, child.ClassName);
                }
                index++;
            }

            return "something outside the table";
        }

        #endregion
    }
}
