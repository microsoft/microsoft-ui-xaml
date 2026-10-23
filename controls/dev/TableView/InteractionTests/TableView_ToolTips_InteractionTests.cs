// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
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
    // TableView tooltip hover interaction tests.
    //
    // These own the SHOWING of a tooltip, which is the part a user experiences and the part no API test can
    // reach: TableView_ToolTips_APITests.cs asserts the ToolTip object that was attached and the HelpText the
    // peer reports, but nothing programmatic makes ToolTipService open a popup - it shows on pointer dwell.
    //
    // Product finding #13 (a client asking a row peer for its children fail-fasted the app) shaped the older tests in
    // this file: they observe rows at row level, point at cells by coordinates, and read editors and visual states
    // through in-process page readouts. #11820 fixed the peers, and cell peers are now read safely; see the history
    // note in TableViewInteractionTestHelpers. The older tests are kept as written - their techniques still work.
    [TestClass]
    public class TableViewToolTipsInteractionTests
    {
        // ToolTipService's initial show delay plus slack. A dwell shorter than the delay reads exactly like a
        // tooltip that never opens, so this is deliberately generous rather than tight.
        private const uint c_toolTipDwellMs = 2000;

        // Enough to bridge the popup layer and its content, and far too shallow to reach a row even if the
        // TableView skip below were to fail.
        private const int c_maxPopupSearchDepth = 8;

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

        #region 9. Tooltip hover

        [TestMethod]
        [TestProperty("Description", "Verifies hovering a column header that sets HeaderToolTip opens a tooltip carrying that text, and that it dismisses when the pointer leaves.")]
        public void HeaderToolTipAppearsOnHover()
        {
            // Contract: TableView.idl:116-119 - HeaderToolTip is opt-in content, "null or empty means no
            //   tooltip", and deliberately NOT a Binding ("a header is not bound against a row, so there is
            //   nothing to defer"). The page authors it on the Name column (TableViewPage.xaml:102) and
            //   authors none on Age (:103-107).
            // API twin: VerifyStringHeaderToolTipProducesToolTipWithThatText and its siblings
            //   (TableView_ToolTips_APITests.cs:585/:618/:645) own ATTACHMENT. This test must not re-assert
            //   that; it owns only the gesture route - dwell, hit-testing and the ToolTipService wiring.
            // Failure means: the tooltip is attached but never shown, so the affordance does not exist for a
            //   user even though every API assertion about it passes.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                UIObject nameHeader = FindColumnHeader(tableView, "Name");
                UIObject ageHeader = FindColumnHeader(tableView, "Age");
                if (nameHeader == null || ageHeader == null)
                {
                    Verify.Fail("The 'Name' and/or 'Age' column header peers were not found on the header host.");
                    return;
                }

                // NEGATIVE CONTROL FIRST, and it is not decoration. 'Age' authors no HeaderToolTip, so nothing
                // may open over it. Running it before the positive case means a dwell that never fires cannot
                // be mistaken for a correctly absent tooltip later: if the positive case then also finds
                // nothing, the two results together say "the dwell is broken", not "the product is broken".
                // Same discipline as the HeaderH= readout in §7.
                HoverOver(ageHeader, "Age");
                UIObject strayToolTip = FindOpenToolTip("after hovering Age");

                // POSITIVE CASE.
                HoverOver(nameHeader, "Name");
                UIObject toolTip = FindOpenToolTip("after hovering Name");

                if (toolTip == null)
                {
                    Verify.Fail(
                        "Hovering the 'Name' header must open a tooltip. Nothing was found on the popup layer " +
                        "or among the window's non-TableView children - see the logged tree above for where " +
                        "the search looked.");
                    return;
                }

                string toolTipText = ReadToolTipText(toolTip);
                Log.Comment("Tooltip found: class='{0}' text='{1}'.", toolTip.ClassName, toolTipText);

                Verify.IsTrue(
                    toolTipText.Contains("The person's name"),
                    string.Format(
                        "The opened tooltip must carry the header's HeaderToolTip content 'The person's name'; got '{0}'.",
                        toolTipText));

                // DISMISSAL. Park the pointer well clear of the header band and let the tooltip close.
                DismissToolTip();
                UIObject afterExit = FindOpenToolTip("after moving off the header");

                Verify.IsNull(
                    afterExit,
                    "The tooltip must dismiss once the pointer leaves the header; one that survives the exit " +
                    "sticks over the content a user is trying to read.");

                // Asserted last so a failure here does not mask the subject above: by this point the positive
                // case has proved the dwell works, which is what makes this check meaningful rather than
                // vacuous.
                Verify.IsNull(
                    strayToolTip,
                    "The 'Age' column sets no HeaderToolTip (TableViewPage.xaml:103-107), so hovering it must " +
                    "open nothing. A tooltip here means the header tooltip is attached to the wrong cell or to " +
                    "the whole header band.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies hovering a cell in a column that sets CellToolTipBinding opens a tooltip carrying that row's bound value.")]
        public void CellToolTipAppearsOnHover()
        {
            // Contract: CellToolTipBinding is evaluated against the ROW ITEM, so the tooltip over row 0 must
            //   carry row 0's value - not the column's header text, and not a constant. The page authors it on
            //   the Name column bound to {Binding Name} (TableViewPage.xaml:101) and authors none on Age
            //   (:103-107). Row 0's item is "Person 0" (TableViewPage.xaml.cs:114).
            // API twin: the API tests own ATTACHMENT and the HelpText the cell peer reports. This test owns the
            //   gesture route, and additionally the one thing no API test can see: that the popup a user
            //   actually gets carries the value of the row they are pointing at.
            // Failure means: cells advertise per-row detail that never reaches a user, or - worse - reaches
            //   them carrying the wrong row's value.
            //
            // HOW THE CELL IS ADDRESSED, and why this is not blocked by finding #13. A cell peer is never
            // resolved. The hover point is composed from two peers that are each independently safe to read:
            // the COLUMN's x from its header (header-host children), and the ROW's y from the row peer itself
            // (rows-host children, one hop, never asking the row for children). Headers and cells share
            // TableViewCellsPanel's column geometry, so the header's horizontal centre is over that column's
            // cell in every row. The crash constraint forbids descending INTO a row; it does not forbid
            // pointing at one.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                UIObject nameHeader = FindColumnHeader(tableView, "Name");
                UIObject ageHeader = FindColumnHeader(tableView, "Age");
                UIObject firstRow = GetRow(tableView, 0);
                if (nameHeader == null || ageHeader == null || firstRow == null)
                {
                    Verify.Fail("The 'Name'/'Age' header peers and/or the first TableViewRow peer were not found.");
                    return;
                }

                Point nameCell = CellPoint(nameHeader, firstRow);
                Point ageCell = CellPoint(ageHeader, firstRow);

                // NEGATIVE CONTROL FIRST, for the same reason as the header test: 'Age' sets no
                // CellToolTipBinding, so a dwell over it must open nothing. Run before the positive case, a
                // broken dwell shows up as BOTH finding nothing, which is distinguishable from a real absence.
                HoverPoint(ageCell, "the Age cell of row 0");
                UIObject strayToolTip = FindOpenToolTip("after hovering the Age cell");

                // POSITIVE CASE.
                HoverPoint(nameCell, "the Name cell of row 0");
                UIObject toolTip = FindOpenToolTip("after hovering the Name cell");

                if (toolTip == null)
                {
                    Verify.Fail(
                        "Hovering the 'Name' cell of row 0 must open a tooltip. Nothing was found on either " +
                        "popup layer - see the logged candidates above.");
                    return;
                }

                string toolTipText = ReadToolTipText(toolTip);
                Log.Comment("Tooltip found: class='{0}' text='{1}'.", toolTip.ClassName, toolTipText);

                Verify.IsTrue(
                    toolTipText.Contains(BasicName(0)),
                    string.Format(
                        "The tooltip over row 0's Name cell must carry that ROW's bound value 'Person 0'; got '{0}'.",
                        toolTipText));

                // The Name column sets BOTH a HeaderToolTip and a CellToolTipBinding, which makes this a real
                // discriminator rather than a restatement of the assertion above: if the hover had landed on
                // the header band, or if the header's tooltip were attached to the whole column, the text here
                // would be the header's.
                Verify.IsFalse(
                    toolTipText.Contains("The person's name"),
                    string.Format(
                        "The cell tooltip must be the row's value, not the column's HeaderToolTip; got '{0}'.",
                        toolTipText));

                DismissToolTip();
                UIObject afterExit = FindOpenToolTip("after moving off the cell");

                Verify.IsNull(
                    afterExit,
                    "The tooltip must dismiss once the pointer leaves the cell.");

                Verify.IsNull(
                    strayToolTip,
                    "The 'Age' column sets no CellToolTipBinding (TableViewPage.xaml:103-107), so hovering its " +
                    "cell must open nothing. A tooltip here means cell tooltips are attached per-row or " +
                    "per-table rather than per-column.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a row container that has been recycled onto a different item shows that item's tooltip, not the one it previously held.")]
        public void RecycledRowShowsCurrentToolTipOnHover()
        {
            // Contract: cell tooltips are attached per COLUMN but evaluated per ITEM, so a container that gets
            //   recycled onto a new item must re-evaluate. A tooltip that is wired once and never rebound is
            //   invisible to every non-scrolling test and to every API test - it only shows up as a user
            //   hovering row N and being told about the row that used to be there.
            // Why this table: ScrollingTableView's FrozenName column displays {Binding Name} but tooltips
            //   {Binding City} (TableViewPage.xaml:177-181). The tooltip is therefore NOT the text in the cell,
            //   so a stale tooltip cannot be mistaken for a correct one that merely matches what is on screen.
            // Data: 200 items, Name "Scroll i", City = Cities[i % 3] over {Redmond, Seattle, Bellevue}
            //   (TableViewPage.xaml.cs:93/:119). Item 0 -> Redmond. Item 199 -> 199 % 3 == 1 -> Seattle.
            // Failure means: the binding is evaluated once at container creation, so tooltips go stale the
            //   moment a user scrolls.
            //
            // WHY THE ENDS OF THE LIST. Sampling a row mid-scroll would require knowing which item landed in
            // it, which out-of-process needs either a cell peer (finding #13) or post-scroll geometry
            // arithmetic (finding #20). Both ends of the list are identified by the scroll extreme instead:
            // scrolled fully to the bottom, the bottom-most realized row is the last item, whatever the
            // realization order happens to be.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem))
                {
                    Verify.Fail("GoToScrollingButton was not found on the test page.");
                    return;
                }

                UIObject tableView = FindElement.ById(ScrollingTable);
                if (tableView == null)
                {
                    Verify.Fail("ScrollingTableView was not found on the test page.");
                    return;
                }

                UIObject frozenNameHeader = FindColumnHeader(tableView, "FrozenName");
                UIObject firstRow = GetRow(tableView, 0);
                if (frozenNameHeader == null || firstRow == null)
                {
                    Verify.Fail("The 'FrozenName' header peer and/or the first TableViewRow peer were not found.");
                    return;
                }

                HoverPoint(CellPoint(frozenNameHeader, firstRow), "the FrozenName cell of the top row");
                UIObject beforeToolTip = FindOpenToolTip("before scrolling");
                if (beforeToolTip == null)
                {
                    Verify.Fail(
                        "Precondition: hovering the top row's FrozenName cell must open a tooltip before any " +
                        "scrolling. Without it there is no 'previous' value for the recycled container to be " +
                        "stale against, and the rest of this test would be vacuous.");
                    return;
                }

                string beforeText = ReadToolTipText(beforeToolTip);
                Log.Comment("Tooltip over the top row before scrolling: '{0}'.", beforeText);
                Verify.IsTrue(
                    beforeText.Contains(CityOf(0)),
                    string.Format(
                        "Precondition: the top row is item 0, whose City is 'Redmond'; got '{0}'.", beforeText));

                DismissToolTip();

                string beforeOffsets = ReadScrollOffsets();
                if (!DragVerticalScrollBarToBottom(tableView))
                {
                    Verify.Fail("The body scroller's vertical ScrollBar was not found, so the list could not be scrolled.");
                    return;
                }

                string afterOffsets = ReadScrollOffsets();
                Log.Comment("Scroll offsets: before='{0}' after='{1}'.", beforeOffsets, afterOffsets);
                Verify.AreNotEqual(
                    beforeOffsets, afterOffsets,
                    "Precondition: the body must actually have scrolled. If it did not, 'the tooltip did not " +
                    "change' would hold for reasons that have nothing to do with recycling.");

                UIObject bottomRow = BottomMostRow(tableView);
                if (bottomRow == null)
                {
                    Verify.Fail(
                        "No realized TableViewRow peer was found inside the table's bounds after scrolling. " +
                        "Either nothing is realized, or the row peers' BoundingRectangles did not follow the " +
                        "scroll - the latter is a harness limitation (finding #20), not a product result.");
                    return;
                }

                HoverPoint(CellPoint(frozenNameHeader, bottomRow), "the FrozenName cell of the bottom row");
                UIObject afterToolTip = FindOpenToolTip("after scrolling to the bottom");

                if (afterToolTip == null)
                {
                    Verify.Fail(
                        "Hovering a recycled row's FrozenName cell must open a tooltip. Getting none here, " +
                        "after the identical hover succeeded before the scroll, means recycling drops the " +
                        "tooltip entirely.");
                    return;
                }

                string afterText = ReadToolTipText(afterToolTip);
                Log.Comment("Tooltip over the bottom row after scrolling: '{0}'.", afterText);

                Verify.IsTrue(
                    afterText.Contains(CityOf(ScrollingItemCount - 1)),
                    string.Format(
                        "The bottom row at maximum scroll is item 199, whose City is 'Seattle'; got '{0}'. A " +
                        "value of 'Redmond' specifically means the container is still showing the tooltip of " +
                        "the item it held before it was recycled.",
                        afterText));
            }
        }

        #endregion

        #region 10. Row tooltip

        [TestMethod]
        [TestProperty("Description", "Verifies RowToolTipBinding opens the focused row's tooltip on keyboard focus, follows Up/Down, and closes on Escape without moving focus.")]
        public void RowToolTipFollowsRowKeyboardFocus()
        {
            // Contract: TableView.RowToolTipBinding attaches the tooltip to the ROW, where keyboard focus lands, so
            //   ToolTipService opens it on keyboard focus as well as hover (WCAG 2.1.1). Escape dismisses it
            //   without moving focus (WCAG 1.4.13). The page binds it to City (TableViewPage.xaml.cs).
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!EnableRowToolTips()) { return; }
                DismissToolTip();

                if (FocusRowAt(BasicTable, 0) == null) { return; }

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                UIObject rowsHost = GetRowsHost(BasicTable);
                Verify.IsTrue(rowsHost.Children[1].HasKeyboardFocus, "Precondition: Down moved focus to row 1.");
                VerifyOpenToolTipText(CityOf(1), "after Down onto row 1");

                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                Verify.IsTrue(rowsHost.Children[0].HasKeyboardFocus, "Precondition: Up moved focus back to row 0.");
                VerifyOpenToolTipText(CityOf(0), "after Up onto row 0");

                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                Verify.IsTrue(WaitForToolTipClosed("after Escape"), "Escape must close the row tooltip.");
                Verify.IsTrue(rowsHost.Children[0].HasKeyboardFocus, "Escape must dismiss the tooltip without moving focus off the row.");

                MoveFocusOffTableBeforeTeardown();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the row tooltip is limited to row-level focus: a cell's own tooltip wins, and a focused cell without one shows nothing; on hover the row tooltip covers cells without their own.")]
        public void RowToolTipYieldsToCellToolTip()
        {
            // The Name column sets CellToolTipBinding="{Binding Name}"; Age sets none. GotFocus bubbles, so without
            // TableView's row-level filter ToolTipService would open the row tooltip over the focused Age cell.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!EnableRowToolTips()) { return; }
                DismissToolTip();

                if (!DrillIntoRow(BasicTable, 0)) { return; }
                VerifyOpenToolTipText(BasicName(0), "with keyboard focus on row 0's Name cell", notExpected: CityOf(0));

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                Verify.AreEqual(1, IndexOfFocusedCell(GetRowsHost(BasicTable).Children[0]), "Precondition: Right moved focus to the Age cell.");
                VerifyNoToolTipOpens("with keyboard focus on row 0's Age cell",
                    "A focused cell with no tooltip of its own must not show the row tooltip; it belongs to row-level focus.");

                MoveFocusOffTableBeforeTeardown();

                UIObject tableView = FindElement.ById(BasicTable);
                UIObject firstRow = GetRow(tableView, 0);
                HoverPoint(CellPoint(FindColumnHeader(tableView, "Name"), firstRow), "the Name cell of row 0");
                VerifyOpenToolTipText(BasicName(0), "hovering row 0's Name cell", notExpected: CityOf(0));

                HoverPoint(CellPoint(FindColumnHeader(tableView, "Age"), firstRow), "the Age cell of row 0");
                VerifyOpenToolTipText(CityOf(0), "hovering row 0's Age cell", notExpected: BasicName(0));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies opening an edit withdraws the cell and row tooltips, so the first Escape cancels the edit.")]
        public void ToolTipsDoNotInterceptEscapeWhileEditing()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!EnableRowToolTips()) { return; }
                DismissToolTip();

                if (!DrillIntoRow(BasicTable, 0)) { return; }
                VerifyOpenToolTipText(BasicName(0), "with keyboard focus on row 0's Name cell");

                KeyboardHelper.PressKey(Key.F2);
                KeyboardHelper.PressKey(Key.a, ModifierKey.Control);
                TextInput.SendText("Discarded");
                Wait.ForIdle();
                VerifyNoToolTipOpens("while editing", "An open edit must withdraw the cell and row tooltips.");

                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                Verify.AreEqual("Cancel;", ReadEditEndReport(), "The first Escape must cancel the edit, not close a tooltip.");
                Verify.AreEqual(BasicName(0), ReadFirstItemName(), "The cancelled edit must leave the value intact.");
                MoveFocusOffTableBeforeTeardown();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Escape during a column resize drag cancels the drag and leaves the row tooltip open during it alone.")]
        public void EscapeCancelsColumnResizeWithRowToolTipOn()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!EnableRowToolTips()) { return; }
                DismissToolTip();

                if (FocusRowAt(BasicTable, 0) == null) { return; }
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                UIObject tableView = FindElement.ById(BasicTable);
                UIObject nameHeader = FindColumnHeader(tableView, "Name");
                double widthBefore = nameHeader.BoundingRectangle.Width;

                // Inline rather than DragColumnBoundary, so the tooltip can be reopened mid-drag: the platform
                // closes it on pointer press, and the page's F4 reopens the focused row's tooltip.
                VerifyOpenToolTipText(CityOf(1), "before the drag starts");
                var bounds = nameHeader.BoundingRectangle;
                int startX = bounds.Left + bounds.Width - 1;
                int y = bounds.Top + (bounds.Height / 2);
                InputHelper.LeftMouseButtonDown(nameHeader, (bounds.Width / 2) - 1, 0);
                InputHelper.MoveMouse(new Point(startX + 45, y));
                InputHelper.MoveMouse(new Point(startX + 90, y));
                KeyboardHelper.PressKey(Key.F4);
                VerifyOpenToolTipText(CityOf(1), "mid-drag");
                double widthMidDrag = FindColumnHeader(tableView, "Name").BoundingRectangle.Width;
                Verify.IsGreaterThan(widthMidDrag, widthBefore + 30.0, "Precondition: the drag is resizing the column.");
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();
                Verify.IsNotNull(FindOpenToolTip("after Escape mid-drag"),
                    "Escape during a resize drag belongs to the drag; it must not also close the row tooltip.");
                InputHelper.LeftMouseButtonUp();
                Wait.ForIdle();

                double widthAfter = FindColumnHeader(tableView, "Name").BoundingRectangle.Width;
                Log.Comment("Name header width before={0}, after cancelled drag={1}.", widthBefore, widthAfter);
                Verify.IsLessThanOrEqual(Math.Abs(widthAfter - widthBefore), 8.0,
                    "Escape must cancel the resize drag rather than being consumed by tooltip dismissal.");
                MoveFocusOffTableBeforeTeardown();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Escape on a focused inline Hyperlink in a row-tooltip table neither crashes nor is swallowed by tooltip dismissal.")]
        public void EscapeOnFocusedHyperlinkInCellIsNotSwallowed()
        {
            // Focus on a TextElement has no visual parent; the tooltip walks must step through its host TextBlock.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(HyperlinkPivotItem))
                {
                    Verify.Fail("GoToHyperlinkButton was not found on the test page.");
                    return;
                }

                var focusButton = FindElement.ById<Button>(FocusTableHyperlinkButton);
                if (focusButton == null)
                {
                    Verify.Fail("FocusTableHyperlinkButton was not found on the test page.");
                    return;
                }

                focusButton.Invoke();
                Verify.IsTrue(WaitForStatusText(HyperlinkFocusedStatus, 5000), "Precondition: the inline Hyperlink took keyboard focus.");
                VerifyNoToolTipOpens("with focus on the Hyperlink",
                    "The row tooltip belongs to row-level focus and must not open over the Hyperlink.");

                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                Verify.IsNotNull(FindElement.ById(HyperlinkTable), "The app must survive Escape on a focused Hyperlink.");
                Verify.IsTrue(WaitForStatusText(EscapeReachedHostStatus, 5000),
                    "With no tooltip open, Escape on the Hyperlink must not be handled by the table and must reach the host.");
                MoveFocusOffTableBeforeTeardown();
            }
        }

        #endregion

        private static bool EnableRowToolTips()
        {
            var toggle = FindElement.ById<CheckBox>(RowToolTipCheckBox);
            if (toggle == null)
            {
                Verify.Fail("RowToolTipCheckBox was not found on the test page.");
                return false;
            }

            toggle.Check();
            Wait.ForIdle();
            return true;
        }

        private const int c_toolTipPollTimeoutMs = 6000;
        private const int c_toolTipPollIntervalMs = 250;

        // Polls until a tooltip carrying the expected text is open; the previous row's may still be closing.
        private static UIObject WaitForOpenToolTip(string phase, string expected)
        {
            var deadline = DateTime.Now.AddMilliseconds(c_toolTipPollTimeoutMs);
            UIObject toolTip;
            do
            {
                Wait.ForIdle();
                toolTip = FindOpenToolTip(phase);
                if (toolTip != null && ReadToolTipText(toolTip).Contains(expected))
                {
                    return toolTip;
                }
                Wait.ForMilliseconds(c_toolTipPollIntervalMs);
            }
            while (DateTime.Now < deadline);

            return toolTip;
        }

        private static bool WaitForToolTipClosed(string phase)
        {
            var deadline = DateTime.Now.AddMilliseconds(c_toolTipPollTimeoutMs);
            do
            {
                Wait.ForIdle();
                if (FindOpenToolTip(phase) == null)
                {
                    return true;
                }
                Wait.ForMilliseconds(c_toolTipPollIntervalMs);
            }
            while (DateTime.Now < deadline);

            return false;
        }

        // Absence can only be shown by watching past the point a tooltip would have opened.
        private static void VerifyNoToolTipOpens(string phase, string message)
        {
            var deadline = DateTime.Now.AddMilliseconds(c_toolTipDwellMs);
            do
            {
                Wait.ForIdle();
                Verify.IsNull(FindOpenToolTip(phase), message);
                Wait.ForMilliseconds(c_toolTipPollIntervalMs);
            }
            while (DateTime.Now < deadline);
        }

        private static void VerifyOpenToolTipText(string expected, string phase, string notExpected = null)
        {
            UIObject toolTip = WaitForOpenToolTip(phase, expected);
            if (toolTip == null)
            {
                Verify.Fail(string.Format("A tooltip carrying '{0}' must be open {1}.", expected, phase));
                return;
            }

            string text = ReadToolTipText(toolTip);
            Log.Comment("Tooltip {0}: '{1}'.", phase, text);
            Verify.IsTrue(text.Contains(expected), string.Format("The tooltip {0} must carry '{1}'; got '{2}'.", phase, expected, text));
            if (notExpected != null)
            {
                Verify.IsFalse(text.Contains(notExpected), string.Format("The tooltip {0} must not carry '{1}'; got '{2}'.", phase, notExpected, text));
            }
        }

        // The screen point over a given column's cell in a given row: the column's x from its HEADER, the row's
        // y from the ROW. See the note in CellToolTipAppearsOnHover for why this composition is what keeps the
        // test clear of finding #13.
        private static Point CellPoint(UIObject columnHeader, UIObject row)
        {
            var headerBounds = columnHeader.BoundingRectangle;
            var rowBounds = row.BoundingRectangle;
            return new Point(
                headerBounds.Left + (headerBounds.Width / 2),
                rowBounds.Top + (rowBounds.Height / 2));
        }

        // Moves the pointer onto the element's centre and dwells there. Absolute coordinates via PointerInput:
        // the UIObject overloads of InputHelper resolve their anchor through GetClickablePoint, which
        // access-violates on TableView peers (finding #14).
        private static void HoverOver(UIObject element, string label)
        {
            HoverPoint(CentreOf(element), label);
        }

        // Dwells on an absolute screen point.
        //
        // The pointer is parked away from the table first so that every hover is a fresh ENTER. Moving straight
        // from one target to the next leaves ToolTipService in its "already showing" state, where the second
        // tooltip can appear with no delay or not at all - either of which would be read as a product result
        // rather than as the gesture artifact it is.
        private static void HoverPoint(Point point, string label)
        {
            DismissToolTip();

            Log.Comment("Hovering {0} at ({1}, {2}) for {3}ms.", label, point.X, point.Y, c_toolTipDwellMs);

            PointerInput.Move(point);
            Wait.ForIdle();
            Wait.ForMilliseconds(c_toolTipDwellMs);
            Wait.ForIdle();
        }

        // Parks the pointer clear of the table and waits for any open tooltip to close.
        private static void DismissToolTip()
        {
            PointerInput.Move(new Point(10, 10));
            Wait.ForIdle();
            Wait.ForMilliseconds(c_toolTipDwellMs);
            Wait.ForIdle();
        }

        // Drags the body scroller's vertical ScrollBar thumb from the top of the track to well past the bottom,
        // pinning the list at maximum offset. Overshooting deliberately: the test needs the END of the list,
        // not a particular offset, and a drag that lands short would leave an unknown item at the bottom.
        //
        // Enumerating the scroller's own children is safe; only descending into a ROW's children trips
        // finding #13.
        private static bool DragVerticalScrollBarToBottom(UIObject tableView)
        {
            UIObject scrollBar = null;
            foreach (UIObject child in tableView.Children)
            {
                if (child.ClassName != "ScrollViewer")
                {
                    continue;
                }

                foreach (UIObject scrollerChild in child.Children)
                {
                    if (scrollerChild.ClassName == "ScrollBar" && scrollerChild.Name == "Vertical")
                    {
                        scrollBar = scrollerChild;
                        break;
                    }
                }
            }

            if (scrollBar == null)
            {
                return false;
            }

            var bounds = scrollBar.BoundingRectangle;
            if (bounds.Width <= 0 || bounds.Height <= 0)
            {
                return false;
            }

            int x = bounds.Left + (bounds.Width / 2);
            int startY = bounds.Top + (bounds.Height / 8);
            int endY = bounds.Top + (bounds.Height * 2);

            Log.Comment("Dragging the vertical ScrollBar thumb from ({0}, {1}) down to ({0}, {2}).", x, startY, endY);

            InputHelper.LeftMouseButtonDown(scrollBar, 0, -(bounds.Height * 3 / 8));
            InputHelper.MoveMouse(new Point(x, startY + (bounds.Height / 2)));
            InputHelper.MoveMouse(new Point(x, endY));
            InputHelper.LeftMouseButtonUp();
            Wait.ForIdle();

            // Double settle: the first idle returns once the drag is handled, which is before the scroll and
            // the re-realization it triggers have finished (AGENTS.md).
            Wait.ForIdle();

            return true;
        }

        // The realized row peer sitting lowest on screen, restricted to rows that actually overlap the table's
        // bounds. The bounds filter is the guard against stale rectangles: a row whose rect never followed the
        // scroll would report a position outside the table, and returning null there produces a clear harness
        // diagnosis instead of a hover at a meaningless coordinate.
        private static UIObject BottomMostRow(UIObject tableView)
        {
            UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
            var tableBounds = tableView.BoundingRectangle;

            UIObject bottom = null;
            int bottomTop = int.MinValue;

            foreach (UIObject child in rowsHost.Children)
            {
                if (child == null || child.ClassName == null || !child.ClassName.Contains("TableViewRow"))
                {
                    continue;
                }

                var bounds = child.BoundingRectangle;
                bool overlapsTable = bounds.Top < tableBounds.Bottom && bounds.Bottom > tableBounds.Top;
                if (!overlapsTable)
                {
                    continue;
                }

                // Must sit fully inside the viewport: a row clipped by the bottom edge can have its centre
                // outside the table, and hovering there would miss the cell entirely.
                if (bounds.Bottom > tableBounds.Bottom)
                {
                    continue;
                }

                if (bounds.Top > bottomTop)
                {
                    bottomTop = bounds.Top;
                    bottom = child;
                }
            }

            return bottom;
        }

        // Finds an open ToolTip, or returns null. Two candidate layers are searched, in order, because which
        // one a WinUI desktop tooltip lands on is exactly what this test is here to establish (plan §9.0):
        //
        //   1. A WINDOWED popup is a top-level element, so it is a child of UIObject.Root. Root.Children is a
        //      shallow match over desktop windows and never descends into app content - the same route
        //      Application.cs:129/:554 uses to find windows.
        //   2. An IN-FRAME popup lives under the app window, on a layer that is a SIBLING of the TableView.
        //      That is reachable with a bounded walk that skips the TableView outright.
        //
        // Both layers are logged whether or not the tooltip is found: a null result has to be attributable to
        // "nothing opened" rather than to "the search looked in the wrong place".
        private static UIObject FindOpenToolTip(string phase)
        {
            foreach (UIObject rootChild in UIObject.Root.Children)
            {
                if (!LooksLikePopup(rootChild))
                {
                    continue;
                }

                Log.Comment("[{0}] top-level popup candidate: class='{1}' name='{2}'.",
                    phase, rootChild.ClassName, rootChild.Name);

                UIObject found = FindToolTipWithin(rootChild, 0);
                if (found != null)
                {
                    Log.Comment("[{0}] tooltip located on the WINDOWED popup layer.", phase);
                    return found;
                }
            }

            UIObject window = TestEnvironment.Application.ApplicationFrameWindow
                ?? TestEnvironment.Application.CoreWindow;
            if (window == null)
            {
                Log.Warning("[{0}] no app window to search; only the windowed popup layer was checked.", phase);
                return null;
            }

            foreach (UIObject child in window.Children)
            {
                if (IsTableView(child))
                {
                    continue;
                }

                UIObject found = FindToolTipWithin(child, 0);
                if (found != null)
                {
                    Log.Comment("[{0}] tooltip located IN-FRAME under the app window.", phase);
                    return found;
                }
            }

            Log.Comment("[{0}] no tooltip on either layer.", phase);
            return null;
        }

        // Depth-bounded search that refuses to enter a TableView. The bound and the skip are independent
        // guards on the same hazard: entering a row peer takes the app down (finding #13), so neither is
        // removable just because the other looks sufficient.
        private static UIObject FindToolTipWithin(UIObject element, int depth)
        {
            if (element == null || depth > c_maxPopupSearchDepth || IsTableView(element))
            {
                return null;
            }

            if (IsToolTip(element))
            {
                return element;
            }

            foreach (UIObject child in element.Children)
            {
                UIObject found = FindToolTipWithin(child, depth + 1);
                if (found != null)
                {
                    return found;
                }
            }

            return null;
        }

        // The ToolTip peer reports its string content as its Name. When the content is not a string - or when
        // the peer reports nothing - the text is recovered from the descendants the popup is hosting, which is
        // still inside the popup's own small subtree and so still clear of the rows.
        private static string ReadToolTipText(UIObject toolTip)
        {
            if (!string.IsNullOrWhiteSpace(toolTip.Name))
            {
                return toolTip.Name;
            }

            var texts = new List<string>();
            CollectNames(toolTip, 0, texts);
            return string.Join(" ", texts);
        }

        private static void CollectNames(UIObject element, int depth, List<string> into)
        {
            if (element == null || depth > c_maxPopupSearchDepth || IsTableView(element))
            {
                return;
            }

            if (!string.IsNullOrWhiteSpace(element.Name))
            {
                into.Add(element.Name);
            }

            foreach (UIObject child in element.Children)
            {
                CollectNames(child, depth + 1, into);
            }
        }

        private static bool IsToolTip(UIObject element)
        {
            return string.Equals(element.ClassName, "ToolTip", StringComparison.Ordinal);
        }

        private static bool LooksLikePopup(UIObject element)
        {
            string className = element.ClassName ?? string.Empty;
            return className.IndexOf("Popup", StringComparison.OrdinalIgnoreCase) >= 0
                || className.IndexOf("ToolTip", StringComparison.OrdinalIgnoreCase) >= 0;
        }

        private static bool IsTableView(UIObject element)
        {
            string className = element.ClassName ?? string.Empty;
            return className.IndexOf("TableView", StringComparison.OrdinalIgnoreCase) >= 0
                || string.Equals(element.AutomationId, BasicTable, StringComparison.Ordinal);
        }
    }
}
