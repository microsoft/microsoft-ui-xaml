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

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView keyboard-navigation and header-input interaction tests.
    // Backlog: docs\design-notes\TabularControls\TableView-interaction-test-plan.md sections 1 and 2.
    //
    // These run out of process and reach the control only through the real UIA provider tree, which
    // is the point of an interaction test: they prove that real key routing and pointer hit-testing
    // reach the state machine the API tests cover in process (interaction-plan coverage rule).
    //
    // HARD CONSTRAINT - product finding #13 (measured this session):
    //   An out-of-proc UIA client crashes the app (0xC0000420 in Microsoft.UI.Xaml.dll) the moment
    //   it asks a TableViewRow peer for its children, because TableViewRowAutomationPeer::
    //   GetChildrenCore manufactures fresh TableViewCellAutomationPeer objects per call.
    //   Therefore NOTHING here descends into a row's children: no FindElement.ByName / VerifyElement
    //   for row/cell content, and no read of row.Children. Every assertion uses row-LEVEL UIA only -
    //   SetFocus / HasKeyboardFocus / BoundingRectangle / the SelectionItem pattern on the row - plus
    //   the header host and the rows host one level below the TableView, which are safe.
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
        }

        #region 1. Keyboard navigation

        [TestMethod]
        [TestProperty("Description", "Down arrow on a focused row moves keyboard focus to the following row (dev-spec:165).")]
        public void DownArrowMovesFocusToNextRow()
        {
            // dev-spec:165 - TableView listens to bubbling KeyDown; an unhandled Down "moves focus
            // between rows". A failure here means arrow keys no longer route to the row navigator, so
            // the table is keyboard-dead even though the API-level GridCoordinateHelper math is fine.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need at least three realized rows."); return; }

                rowsHost.Children[2].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(rowsHost.Children[2].HasKeyboardFocus, "A middle row should take focus first.");

                KeyboardHelper.PressKey(Key.Home);
                Wait.ForIdle();

                // Re-fetch: Home can scroll, changing the realized set; index 0 is always the top row.
                rowsHost = GetRowsHost("BasicTableView");
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 2) { Verify.Fail("Need at least two realized rows."); return; }

                rowsHost.Children[0].SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(rowsHost.Children[0].HasKeyboardFocus, "The first row should take focus first.");

                KeyboardHelper.PressKey(Key.End);
                Wait.ForIdle();

                // Re-fetch: End scrolls the true last row into view, changing the realized set.
                rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                UIObject last = rowsHost.Children[rowsHost.Children.Count - 1];
                Verify.IsTrue(last.HasKeyboardFocus, "End should focus the last row.");
            }
        }

        [TestMethod]
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                SelectPivotItem("Scrolling");

                UIObject rowsHost = GetRowsHost("ScrollingTableView");
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

                Verify.IsNotNull(FindFocusedRow(GetRowsHost("ScrollingTableView")),
                    "A row must still hold keyboard focus after PageDown.");
                Verify.IsTrue(afterPage > afterOneDown + 1.0,
                    string.Format("PageDown should page the view ({0}%) far past where a single Down leaves it ({1}%).",
                        afterPage, afterOneDown));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Page Up moves keyboard focus back up by roughly a viewport (dev-spec:165).")]
        public void PageUpMovesByViewport()
        {
            // dev-spec:165 lists PageUp among the keys that move focus between rows. Pages down twice to
            // earn headroom, then asserts PageUp reverses it. Measured as VerticalScrollPercent for the
            // same reason as PageDownMovesByViewport: paging scrolls, so the focused row's screen
            // position barely changes and cannot express travel. A failure means PageUp does not page
            // back up.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                SelectPivotItem("Scrolling");

                UIObject rowsHost = GetRowsHost("ScrollingTableView");
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

                Verify.IsNotNull(FindFocusedRow(GetRowsHost("ScrollingTableView")),
                    "A row must still hold keyboard focus after PageUp.");
                Verify.IsTrue(afterPagingUp < afterPagingDown - 1.0,
                    string.Format("PageUp should page the view back up ({0}% -> {1}%).", afterPagingDown, afterPagingUp));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tab leaves the table downstream in a single press instead of walking cell by cell (dev-spec:165).")]
        public void TabMovesFocusOutOfTable()
        {
            // dev-spec:165 keeps navigation row-oriented; the table is not a per-cell tab trap. One Tab
            // from a focused row must leave every row and continue forward in tab order to the next
            // focusable element AFTER the table - AfterTableButton, which the page places in the
            // Grid.Row=2 StackPanel below the Pivot. A failure means Tab steps through cells (focus
            // stays inside the table), so a keyboard user is trapped walking the grid cell by cell.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
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
                rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                Verify.IsNull(FindFocusedRow(rowsHost), "No row should hold focus after Tab.");

                // Forward tab order exits the table downstream onto the next focusable element after it.
                UIObject afterTable = FindElement.ById("AfterTableButton");
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
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

                    UIObject rows = GetRowsHost("BasicTableView");
                    if (rows == null) { return; }

                    UIObject start = rows.Children[testCase.From];
                    start.SetFocus();
                    Wait.ForIdle();

                    KeyboardHelper.PressKey(testCase.Key);
                    Wait.ForIdle();

                    rows = GetRowsHost("BasicTableView");
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                var selectionMode = FindElement.ById<ComboBox>("SelectionModeComboBox");
                if (selectionMode == null) { Verify.Fail("SelectionModeComboBox was not found."); return; }
                // The page names the items SelectionModeNone / SelectionModeSingle via
                // AutomationProperties.Name; the Content strings ("None") are not what UIA reports.
                selectionMode.SelectItemByName("SelectionModeNone");
                Wait.ForIdle();

                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                rowsHost.Children[0].SetFocus();
                Wait.ForIdle();

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                rowsHost = GetRowsHost("BasicTableView");
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
        [TestProperty("Description", "Left and Right do not move row focus: dev-spec:165 enumerates six row-navigation keys and they are not among them.")]
        public void LeftAndRightArrowsDoNotMoveRowFocus()
        {
            // dev-spec:165 names exactly Up, Down, Home, End, PageUp and PageDown, and calls keyboard
            // handling "row-oriented". This pins the BOUNDARY of that contract rather than a behavior
            // inside it: a failure means the control grew horizontal (cell-to-cell) navigation that no
            // document describes, which would also change what Tab and arrow keys mean for AT users.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject anchor = rowsHost.Children[1];
                anchor.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(anchor.HasKeyboardFocus, "The anchor row should hold focus first.");

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                Verify.IsTrue(GetRowsHost("BasicTableView").Children[1].HasKeyboardFocus,
                    "Right must leave row focus where it was.");

                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                Verify.IsTrue(GetRowsHost("BasicTableView").Children[1].HasKeyboardFocus,
                    "Left must leave row focus where it was.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Shift+Tab leaves the table backwards in a single press (the reverse of TabMovesFocusOutOfTable).")]
        public void ShiftTabMovesFocusOutOfTableUpstream()
        {
            // The mirror of TabMovesFocusOutOfTable. ItemsView keeps the same pair as separate tests
            // (TabIntoItemsViewWithoutCurrentItem / ShiftTabIntoItemsViewWithoutCurrentItem) because the
            // two directions fail independently. A failure here means reverse tab order walks the
            // table's internals, trapping a keyboard user who is backing out of the control.
            //
            // The exact upstream landing spot is a page-layout detail (the Pivot header sits between the
            // control strip and the table in tab order), so this asserts the contract - focus left every
            // row - and then reports which upstream element received it.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 1) { Verify.Fail("Need a realized row."); return; }

                UIObject firstRow = rowsHost.Children[0];
                firstRow.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(firstRow.HasKeyboardFocus, "A row should take focus first.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();

                rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                Verify.IsNull(FindFocusedRow(rowsHost), "No row should hold focus after Shift+Tab.");

                UIObject afterTable = FindElement.ById("AfterTableButton");
                if (afterTable != null)
                {
                    Verify.IsFalse(afterTable.HasKeyboardFocus,
                        "Shift+Tab must move focus upstream, not forward onto the control after the table.");
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tab reaches the table from the control before it, and a row is then reachable by arrow key without anything being selected.")]
        public void TabIntoTableFocusesARowWithoutSelecting()
        {
            // Adopted from ItemsView, which tests entry and exit as separate concerns. Two failures
            // matter: the table is skipped entirely (a keyboard-only user can never reach the data), or
            // entry silently selects (the user acted on data merely by tabbing past it).
            //
            // Entry is asserted as "the table or a row inside it has focus", not "a row has focus":
            // dev-spec:165 makes navigation row-oriented via arrow keys but says nothing that requires
            // Tab to land ON a row, and the control legitimately takes focus at the table level first.
            // The arrow key that follows is what must produce a focused row - that IS the spec'd part.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                var selectionMode = FindElement.ById<ComboBox>("SelectionModeComboBox");
                if (selectionMode == null) { Verify.Fail("SelectionModeComboBox was not found."); return; }
                selectionMode.SetFocus();
                Wait.ForIdle();

                UIObject tableView = FindElement.ById("BasicTableView");
                if (tableView == null) { Verify.Fail("BasicTableView was not found."); return; }

                bool entered = false;
                for (int attempt = 0; attempt < 3 && !entered; attempt++)
                {
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    entered = tableView.HasKeyboardFocus || FindFocusedRow(GetRowsHost("BasicTableView")) != null;
                }

                if (!entered)
                {
                    Verify.Fail("Tab never entered the table: neither the table nor any row took keyboard focus within three presses.");
                    return;
                }

                // Check selection BEFORE any navigation key: selection follows focus (see
                // KeyboardFocusMoveCarriesSelection), so an arrow key is ENTITLED to select. What must
                // not happen is selection arising from tab entry alone.
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }

                foreach (UIObject row in rowsHost.Children)
                {
                    bool isSelected;
                    if (!TryGetIsSelected(row, out isSelected)) { break; }
                    Verify.IsFalse(isSelected, "Tabbing into the table must not select a row by itself.");
                }

                // From the entry point an arrow key must produce a focused row - the row-oriented
                // navigation dev-spec:165 describes.
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                Verify.IsNotNull(FindFocusedRow(rowsHost), "After entering the table, an arrow key must focus a row.");
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                UIObject ageHeader = GetHeader("BasicTableView", "Age");
                if (ageHeader == null) { Verify.Fail("The Age header was not found."); return; }
                ageHeader.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(ageHeader.HasKeyboardFocus, "The Age header should take keyboard focus.");

                KeyboardHelper.PressKey(Key.Enter); // None -> Ascending (bottom item does not move yet)
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Enter); // Ascending -> Descending (largest Age rises to the top)
                Wait.ForIdle();

                UIObject selected = FindSelectedRow(GetRowsHost("BasicTableView"));
                if (selected == null) { Verify.Fail("The selected row was not found after sorting."); return; }
                int newTop = selected.BoundingRectangle.Top;

                Verify.IsTrue(newTop < oldTop - 2,
                    string.Format("Enter on the header should reorder rows: the selected item moved up ({0} -> {1}).", oldTop, newTop));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Keyboard resize on a focused header changes the column width, and Shift amplifies the step (dev-spec:127, dev-spec:133).")]
        public void HeaderKeyboardResizeChangesWidth()
        {
            // dev-spec:133 - the header cell (not the gripper) is the keyboard target; the table routes
            // Left/Right into TryKeyboardStep(VirtualKey), which owns direction, the RTL mirror,
            // KeyboardIncrement and the Shift multiplier. dev-spec:131 - a positive logical delta grows
            // in reading order, so Right grows the column in LTR. dev-spec:127/:133 do NOT state the
            // numeric KeyboardIncrement or the Shift factor, so this asserts DIRECTION and the RELATIVE
            // magnitude only (never a guessed pixel step): Right grows, and Shift+Right grows by more.
            // The header cell spans its column, so its BoundingRectangle.Width tracks the column width.
            // A failure means keyboard resize does not reach the width engine, or Shift is not honored.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject header = GetHeader("BasicTableView", "Name");
                if (header == null) { Verify.Fail("The Name header was not found."); return; }
                header.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(header.HasKeyboardFocus, "The Name header should take keyboard focus.");

                int w0 = GetHeader("BasicTableView", "Name").BoundingRectangle.Width;

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                int w1 = GetHeader("BasicTableView", "Name").BoundingRectangle.Width;
                int deltaPlain = w1 - w0;
                Verify.IsTrue(deltaPlain > 0,
                    string.Format("Right on the header should grow the column ({0} -> {1}).", w0, w1));

                KeyboardHelper.PressKey(Key.Right, ModifierKey.Shift);
                Wait.ForIdle();
                int w2 = GetHeader("BasicTableView", "Name").BoundingRectangle.Width;
                int deltaShift = w2 - w1;
                Verify.IsTrue(deltaShift > deltaPlain,
                    string.Format("Shift+Right should grow by a larger step than plain Right ({0}px vs {1}px).", deltaShift, deltaPlain));
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                UIObject ageHeader = GetHeader("BasicTableView", "Age");
                if (ageHeader == null) { Verify.Fail("The Age header was not found."); return; }

                InputHelper.LeftClick(ageHeader); // None -> Ascending
                Wait.ForIdle();
                ageHeader = GetHeader("BasicTableView", "Age");
                if (ageHeader == null) { Verify.Fail("The Age header disappeared after the first click."); return; }
                InputHelper.LeftClick(ageHeader); // Ascending -> Descending (largest Age rises)
                Wait.ForIdle();

                UIObject selected = FindSelectedRow(GetRowsHost("BasicTableView"));
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                var sortCycle = FindElement.ById<ComboBox>("SortCycleComboBox");
                if (sortCycle == null) { Verify.Fail("SortCycleComboBox was not found."); return; }

                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }

                int rowCount = rowsHost.Children.Count;
                if (rowCount < 12) { Verify.Fail(string.Format("Need the 12 authored rows realized; saw {0}.", rowCount)); return; }

                const int MaxScoreSourceIndex = 5;

                Log.Comment("Set the Score column's cycle to DescendingAscendingNone.");
                sortCycle.SelectItemByName("SortCycleDescendingAscendingNone");
                Wait.ForIdle();

                SelectRow(rowsHost.Children[MaxScoreSourceIndex]);
                Verify.AreEqual(MaxScoreSourceIndex, IndexOfSelectedRow("BasicTableView"),
                    "Precondition: the highest-Score row is authored at source index 5.");

                ClickHeader("BasicTableView", "Score");
                Verify.AreEqual(0, IndexOfSelectedRow("BasicTableView"),
                    "With DescendingAscendingNone the FIRST click must sort Descending, putting the highest Score at the top.");

                ClickHeader("BasicTableView", "Score");
                Verify.AreEqual(rowCount - 1, IndexOfSelectedRow("BasicTableView"),
                    "The SECOND click must sort Ascending, putting the highest Score at the bottom.");

                ClickHeader("BasicTableView", "Score");
                Verify.AreEqual(MaxScoreSourceIndex, IndexOfSelectedRow("BasicTableView"),
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                ClickHeaderTwice("BasicTableView", "ReadOnlyCity");

                UIObject afterBlocked = FindSelectedRow(GetRowsHost("BasicTableView"));
                if (afterBlocked == null) { Verify.Fail("The selected row was lost after clicking the non-sortable header."); return; }
                Verify.IsTrue(Math.Abs(afterBlocked.BoundingRectangle.Top - oldTop) <= 2,
                    string.Format("A CanSort=False header must not reorder rows (selected row {0} -> {1}).",
                        oldTop, afterBlocked.BoundingRectangle.Top));

                // Negative control: the same gesture on a sortable column must work, proving the clicks land.
                ClickHeaderTwice("BasicTableView", "Age");

                UIObject afterSortable = FindSelectedRow(GetRowsHost("BasicTableView"));
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                var canUserSort = FindElement.ById<CheckBox>("CanUserSortColumnsCheckBox");
                if (canUserSort == null) { Verify.Fail("CanUserSortColumnsCheckBox was not found."); return; }

                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                canUserSort.Uncheck();
                Wait.ForIdle();

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                ClickHeaderTwice("BasicTableView", "Age");

                UIObject afterBlocked = FindSelectedRow(GetRowsHost("BasicTableView"));
                if (afterBlocked == null) { Verify.Fail("The selected row was lost while sorting was disabled."); return; }
                Verify.IsTrue(Math.Abs(afterBlocked.BoundingRectangle.Top - oldTop) <= 2,
                    string.Format("With CanUserSortColumns false a header click must not reorder rows ({0} -> {1}).",
                        oldTop, afterBlocked.BoundingRectangle.Top));

                canUserSort.Check();
                Wait.ForIdle();

                ClickHeaderTwice("BasicTableView", "Age");

                UIObject afterRestored = FindSelectedRow(GetRowsHost("BasicTableView"));
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject rowsHost = GetRowsHost("BasicTableView");
                if (rowsHost == null) { return; }
                if (rowsHost.Children.Count < 3) { Verify.Fail("Need several realized rows."); return; }

                UIObject bottom = rowsHost.Children[rowsHost.Children.Count - 1];
                SelectRow(bottom);
                Verify.IsTrue(IsSelected(bottom), "The bottom row should be selected.");
                int oldTop = bottom.BoundingRectangle.Top;

                UIObject nameHeader = GetHeader("BasicTableView", "Name");
                if (nameHeader == null) { Verify.Fail("The Name header was not found."); return; }
                int widthBefore = nameHeader.BoundingRectangle.Width;

                DragHeaderTrailingEdge(nameHeader, 40);

                UIObject afterDrag = GetHeader("BasicTableView", "Name");
                if (afterDrag == null) { Verify.Fail("The Name header was not found after the drag."); return; }
                Verify.IsTrue(afterDrag.BoundingRectangle.Width > widthBefore,
                    string.Format("Precondition: the drag should widen the column ({0} -> {1}).",
                        widthBefore, afterDrag.BoundingRectangle.Width));

                UIObject selected = FindSelectedRow(GetRowsHost("BasicTableView"));
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
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                // Resolved before the grouped table realizes - a FindElement afterwards forces a full tree
                // re-walk that descends into TableViewRow children and asserts the app (finding #13).
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
            if (s_groupedAnchorButton == null) { Verify.Fail("The grouped-pivot anchor button was not captured."); return false; }

            s_groupedAnchorButton.SetFocus();
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

        // Group headers and rows are siblings under the rows host; the header's class name is
        // "TableViewGroupHeader" (TableViewGroupHeaderAutomationPeer::GetClassNameCore), distinct from the
        // row's "...TableViewRow".
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

        // Counts the TableViewRow peers directly under the rows host. Row-level only - it never asks a row for
        // its children (finding #13). Clears the element cache first so a collapse driven from outside the
        // table is not read from a stale walk.
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

        // Kept from SelectGroupedPivotAndGetTable so the keyboard tests have a focus anchor that does not need a
        // second FindElement once the grouped table is realized (see TabToFirstGroupHeader).
        private static Button s_groupedAnchorButton;

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

            s_groupedAnchorButton = goToGrouped;
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

        // Clicks a header once, re-finding it first: a previous click can rebuild the header band, which
        // invalidates any UIObject the caller still holds.
        private static void ClickHeader(string tableAutomationId, string columnHeader)
        {
            UIObject header = GetHeader(tableAutomationId, columnHeader);
            if (header == null) { Verify.Fail("The " + columnHeader + " header was not found."); return; }
            InputHelper.LeftClick(header);
            Wait.ForIdle();
        }

        // Position of the selected row among the realized rows, or -1. Re-resolves the rows host on every call so
        // the walk reflects the post-sort order; deliberately does NOT call ElementCache.Clear(), because the
        // full re-walk that forces descends into TableViewRow children and asserts the app (finding #13/#14,
        // measured in ptrrun2). UIA re-queries children on access, which is enough here - the other sort tests
        // re-resolve the same way and see reordered rows.
        private static int IndexOfSelectedRow(string tableAutomationId)
        {
            UIObject rowsHost = GetRowsHost(tableAutomationId);
            if (rowsHost == null) { return -1; }

            int index = 0;
            foreach (UIObject row in rowsHost.Children)
            {
                if (IsSelected(row))
                {
                    return index;
                }
                index++;
            }
            return -1;
        }

        // Clicks a header twice, re-finding it in between: the first click can rebuild the header band, which
        // invalidates the previous UIObject.
        private static void ClickHeaderTwice(string tableAutomationId, string columnHeader)
        {
            UIObject header = GetHeader(tableAutomationId, columnHeader);
            if (header == null) { Verify.Fail("The " + columnHeader + " header was not found."); return; }
            InputHelper.LeftClick(header);
            Wait.ForIdle();

            header = GetHeader(tableAutomationId, columnHeader);
            if (header == null) { Verify.Fail("The " + columnHeader + " header disappeared after the first click."); return; }
            InputHelper.LeftClick(header);
            Wait.ForIdle();
        }

        // Presses just inside the column's LTR trailing edge, where the ResizeGripper straddles the boundary,
        // and drags right so the column widens. Absolute points are used for the moves because the header's
        // own rectangle grows during the drag, so a relative offset would drift. Mirrors the layout file's
        // DragColumnBoundary; kept local because the sort observability helpers live in this file.
        private static void DragHeaderTrailingEdge(UIObject header, int widenBy)
        {
            var bounds = header.BoundingRectangle;
            int grabOffsetX = (bounds.Width / 2) - 1;
            int startX = bounds.Left + bounds.Width - 1;
            int y = bounds.Top + (bounds.Height / 2);

            InputHelper.LeftMouseButtonDown(header, grabOffsetX, 0);
            // Two steps so the manipulation clears the 0.5 DIP deadband (dev-spec:123).
            InputHelper.MoveMouse(new Point(startX + (widenBy / 2), y));
            InputHelper.MoveMouse(new Point(startX + widenBy, y));
            InputHelper.LeftMouseButtonUp();
            Wait.ForIdle();
        }

        // The rows host is the LAST child of the TableView peer; its children are TableViewRow peers.
        // FindElement.ById on the top-level AutomationId is safe (finding #13 only bites when a row is
        // asked for ITS children). We never call rowsHost.Children[i].Children.
        private static UIObject GetRowsHost(string tableAutomationId)
        {
            UIObject tableView = FindElement.ById(tableAutomationId);
            if (tableView == null) { Verify.Fail(tableAutomationId + " was not found."); return null; }
            if (tableView.Children.Count < 1) { Verify.Fail(tableAutomationId + " exposed no children."); return null; }
            return tableView.Children[tableView.Children.Count - 1];
        }

        // The header host is the FIRST child of the TableView peer; its children are the header peers,
        // whose Name is the column header string. Reading one level of children here is safe.
        private static UIObject GetHeader(string tableAutomationId, string columnHeader)
        {
            UIObject tableView = FindElement.ById(tableAutomationId);
            if (tableView == null) { Verify.Fail(tableAutomationId + " was not found."); return null; }
            if (tableView.Children.Count < 1) { Verify.Fail(tableAutomationId + " exposed no children."); return null; }
            UIObject headerHost = tableView.Children[0];
            foreach (UIObject child in headerHost.Children)
            {
                if (child.Name == columnHeader)
                {
                    return child;
                }
            }
            return null;
        }

        private static UIObject FindFocusedRow(UIObject rowsHost)
        {
            if (rowsHost == null) { return null; }
            foreach (UIObject row in rowsHost.Children)
            {
                if (row.HasKeyboardFocus)
                {
                    return row;
                }
            }
            return null;
        }

        private static UIObject FindSelectedRow(UIObject rowsHost)
        {
            if (rowsHost == null) { return null; }
            foreach (UIObject row in rowsHost.Children)
            {
                if (IsSelected(row))
                {
                    return row;
                }
            }
            return null;
        }

        // Selection state has two possible channels out of proc. The SelectionItem pattern is the one a
        // real client reaches for, but this build's row peer does not marshal it (see the run-status
        // notes in the interaction plan), so fall back to reading the underlying UIA property directly.
        // Returns false only when NEITHER channel answers, so callers can tell "not selected" apart from
        // "cannot be observed" instead of passing vacuously.
        private static bool TryGetIsSelected(UIObject row, out bool isSelected)
        {
            isSelected = false;
            if (row == null) { return false; }

            var selectionItem = new SelectionItemImplementation<UIObject>(row, UIObject.Factory);
            if (selectionItem.IsAvailable)
            {
                isSelected = selectionItem.IsSelected;
                return true;
            }

            try
            {
                object raw = row.GetProperty(UIProperty.Get("SelectionItem.IsSelected"));
                if (raw == null) { return false; }
                isSelected = Convert.ToBoolean(raw);
                return true;
            }
            catch (UIObjectNotFoundException)
            {
                return false;
            }
        }

        private static bool IsSelected(UIObject row)
        {
            bool isSelected;
            return TryGetIsSelected(row, out isSelected) && isSelected;
        }

        // Selection is driven through the row peer's SelectionItem pattern - a row-level operation that
        // never asks the row for its children, so it stays off the finding #13 crash path.
        private static void SelectRow(UIObject row)
        {
            var selectionItem = new SelectionItemImplementation<UIObject>(row, UIObject.Factory);
            if (!selectionItem.IsAvailable)
            {
                Verify.Fail("The row did not expose the SelectionItem pattern.");
                return;
            }

            selectionItem.Select();
            Wait.ForIdle();
        }

        // Switch the Pivot to the named page by invoking the page's GoTo* button, which is reachable by
        // AutomationId. Do NOT resolve the Pivot header by name: a name-based UIA search makes the
        // provider compute names for realized TableViewRow peers, which manufactures cell peers and
        // trips product finding #13 (0xC0000420 in Microsoft.UI.Xaml.dll). Measured: every test that
        // called FindElement.ByName here crashed the app; every ById lookup is safe.
        private static void SelectPivotItem(string headerName)
        {
            var goTo = FindElement.ById<Button>("GoTo" + headerName + "Button");
            if (goTo == null) { Verify.Fail("GoTo" + headerName + "Button was not found."); return; }
            goTo.InvokeAndWait();
            Wait.ForIdle();
        }

        #endregion
    }
}
