// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using MUXTestInfra.Shared.Infra;

using Point = System.Drawing.Point;

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView layout interaction tests: pointer resize, scrolling and frozen columns, and right-to-left.
    //
    // These run out of process against the real UIA provider tree. They own the *gesture route* into the
    // column-sizing and scroll-sync contracts the API tests (TableView_Sizing_APITests.cs and API 4.x)
    // already cover programmatically: the API test proves the state machine, this file proves real pointer
    // and wheel input reaches it.
    //
    // Product finding #13 (measured this session) constrains every test here: an out-of-proc client that
    // asks a TableViewRow peer for its children fail-fasts the app (0xC0000420 in Microsoft.UI.Xaml.dll —
    // TableViewRowAutomationPeer::GetChildrenCore manufactures fresh cell peers per call). Therefore
    // NOTHING below descends into a row. Column width and frozen-column position are read exclusively from
    // the *header* peers, which enumerate safely, and from row-level bounding rectangles. Header peers are
    // reached by the measured-safe descent: tableView.Children[0] is the header host whose children are the
    // per-column header peers (.Name == the column header text).
    //
    // Resize is driven by coordinate-based pointer input derived from the header peer's BoundingRectangle
    // (grab just inside its right edge, where the ResizeGripper straddles the column boundary) rather than
    // by searching for the gripper element: whether a header peer's own children enumerate safely is
    // UNVERIFIED, and a search that walked into a row would crash.
    [TestClass]
    public class TableViewLayoutInteractionTests
    {
        // One mouse-wheel notch. Matches the WHEEL_DELTA the input stack injects.
        private const int mouseWheelDelta = 120;

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

        #region 6. Pointer resize

        [TestMethod]
        [TestProperty("Description", "Verifies dragging a column's resize gripper with the pointer changes that column's width.")]
        public void PointerResizeDragChangesColumnWidth()
        {
            // Derives from TableView-dev-spec.md:127 ("the table owns ... clamping the reported delta to
            // MinWidth/MaxWidth, writing Width") and the resize Protocol paragraph (:129):
            // BeginDrag -> DragDelta -> EndDrag, with TotalDelta applied against the captured anchor.
            // API 4.x proves ResizeGripper.BeginDrag/TryDrag/EndDrag drive that write; this test proves a
            // real pointer drag on the gripper reaches it. A failure means gripper hit-testing, pointer
            // capture, or the manipulation->TryDrag handler is broken even though the width engine is fine.
            using (var setup = new TestSetupHelper(PageName))
            {
                // Basic pivot is selected by default; its TableView authors CanUserResizeColumns="True"
                // and every column defaults CanResize="true" (TableView.idl:140, :469).
                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                UIObject nameHeader = FindColumnHeader(tableView, "Name");
                if (nameHeader == null)
                {
                    Verify.Fail("The 'Name' column header peer was not found on the header host.");
                    return;
                }

                double widthBefore = nameHeader.BoundingRectangle.Width;

                // Drag the boundary to the right by ~80px. Positive grows in reading order (dev-spec:131),
                // which under LTR is rightward, so the column must get wider.
                const int dragBy = 80;
                DragColumnBoundary(nameHeader, dragBy, cancelWithEscape: false);

                Wait.ForIdle();
                double widthAfter = FindColumnHeader(tableView, "Name").BoundingRectangle.Width;

                Log.Comment("Name column header width before={0}, after={1}, dragBy={2}.", widthBefore, widthAfter, dragBy);

                // Compare the delta, not an absolute (bounding rectangles are screen-relative). Allow slack
                // for the gripper width, the 0.5 DIP deadband, and manipulation rounding — the claim is that
                // the drag reached the width engine and grew the column, not that it grew by exactly 80.
                Verify.IsGreaterThan(
                    widthAfter,
                    widthBefore + 20.0,
                    "A rightward pointer drag on the gripper must widen the column (dev-spec:127 writes Width from the drag delta).");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product finding #17: Escape during a pointer resize drag does not restore the authored width.
        [TestProperty("Description", "Verifies pressing Escape during a pointer resize drag restores the column's authored width.")]
        public void PointerResizeEscapeCancelsResize()
        {
            // Derives from the Protocol paragraph at TableView-dev-spec.md:129: "DragCompleted.Canceled
            // distinguishes a torn-down gesture (Escape ...) from a release: on cancel the host restores the
            // authored GridLength." The Basic 'Name' column authors Width="160" (with MinWidth="60"), so a
            // cancelled drag must return the header to ~160 regardless of how far the pointer travelled.
            // A failure means Escape does not reach the gripper's cancel path, or the host applied the drag
            // delta permanently instead of reverting to the authored value.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                UIObject nameHeader = FindColumnHeader(tableView, "Name");
                if (nameHeader == null)
                {
                    Verify.Fail("The 'Name' column header peer was not found on the header host.");
                    return;
                }

                double widthBefore = nameHeader.BoundingRectangle.Width;

                // Drag well past the deadband so the width is visibly changing mid-gesture, then Escape to
                // cancel before releasing the pointer.
                DragColumnBoundary(nameHeader, 90, cancelWithEscape: true);

                Wait.ForIdle();
                double widthAfter = FindColumnHeader(tableView, "Name").BoundingRectangle.Width;

                Log.Comment("Name column header width before={0}, after cancelled drag={1}.", widthBefore, widthAfter);

                // The authored width is what a cancel restores; the pre-drag width equals it, so the header
                // must land back within a small tolerance of where it started.
                Verify.IsLessThanOrEqual(
                    Math.Abs(widthAfter - widthBefore),
                    8.0,
                    "Escape during a pointer drag must restore the authored width (dev-spec:129 cancel semantics).");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies unloading the TableView mid-drag releases pointer capture and leaves no wedged resize state.")]
        public void UnloadDuringPendingResizeLeavesNoWedgedState()
        {
            // Moved from API 16.4. A "pending resize" is a captured pointer between BeginDrag and EndDrag;
            // the API can drive that lifecycle but only a real drag holds the pointer CAPTURE, which is the
            // half that can wedge. dev-spec:129 lists "a canceled contact, the header rebuilt mid-drag" among
            // the torn-down gestures the primitive must survive; an unload is exactly that teardown. Nothing
            // in the peer surface opens or observes capture, so this is interaction-only.
            //
            // Shape: the DelayedUnloadButton removes the Basic table 1.5s after its click and re-adds it 1.5s
            // later, so the unload lands while the pointer is captured mid-drag. Proof of "not wedged" is that
            // a normal resize works again after the reload - identical to PointerResizeDragChangesColumnWidth.
            // StatusTextBlock ("Unloaded"/"Reloaded") is page state used only as a timing story; it is never
            // asserted - the assertion is the width change of the reloaded control.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                if (tableView == null)
                {
                    Verify.Fail("BasicTableView was not found on the test page.");
                    return;
                }

                UIObject nameHeader = FindColumnHeader(tableView, "Name");
                if (nameHeader == null)
                {
                    Verify.Fail("The 'Name' column header peer was not found on the header host.");
                    return;
                }

                UIObject unloadButton = FindElement.ById(DelayedUnloadButton);
                if (unloadButton == null)
                {
                    Verify.Fail("DelayedUnloadButton was not found on the test page.");
                    return;
                }

                // Arm the delayed unload (removes the table at +1.5s, re-adds it at +3.0s).
                InputHelper.Tap(unloadButton);

                // Begin a real resize drag and hold it open past the unload point.
                var bounds = nameHeader.BoundingRectangle;
                int grabOffsetX = (bounds.Width / 2) - 1;
                int startX = bounds.Left + bounds.Width - 1;
                int y = bounds.Top + (bounds.Height / 2);

                InputHelper.LeftMouseButtonDown(nameHeader, grabOffsetX, 0);
                InputHelper.MoveMouse(new Point(startX + 40, y));

                // Hold with the pointer captured while the table is torn out from under the drag (~1.5s).
                Wait.ForMilliseconds(2200);

                // Continue and release into empty space; the moves use absolute points so nothing dereferences
                // the now-detached header peer.
                InputHelper.MoveMouse(new Point(startX + 80, y));
                InputHelper.LeftMouseButtonUp();
                Wait.ForIdle();

                // Wait for the same instance to be re-added (reload at +3.0s from the click).
                Wait.ForMilliseconds(2000);
                Wait.ForIdle();

                // The old UIObject is stale after the remove/add; re-query the reloaded control.
                UIObject reloaded = FindElement.ById(BasicTable);
                if (reloaded == null)
                {
                    Verify.Fail("BasicTableView did not come back after the delayed reload.");
                    return;
                }

                UIObject reloadedHeader = FindColumnHeader(reloaded, "Name");
                if (reloadedHeader == null)
                {
                    Verify.Fail("The 'Name' column header peer was not found after reload.");
                    return;
                }

                double widthBefore = reloadedHeader.BoundingRectangle.Width;

                // A normal resize must succeed - if capture wedged, the gripper would ignore this drag.
                DragColumnBoundary(reloadedHeader, 80, cancelWithEscape: false);
                Wait.ForIdle();

                double widthAfter = FindColumnHeader(FindElement.ById(BasicTable), "Name").BoundingRectangle.Width;

                Log.Comment("After mid-drag unload/reload: Name width before={0}, after normal resize={1}.", widthBefore, widthAfter);

                Verify.IsGreaterThan(
                    widthAfter,
                    widthBefore + 20.0,
                    "A resize after a mid-drag unload/reload must still widen the column - i.e. no capture wedged the gripper (dev-spec:129).");
            }
        }

        #endregion

        #region 7. Scrolling

        [TestMethod]
        [TestProperty("Ignore", "True")] // Finding #20: header BoundingRectangle does not reflect scroll offset, so this tier cannot observe it.
        [TestProperty("Description", "Verifies a non-frozen header scrolls horizontally when the body is scrolled by pointer.")]
        public void HorizontalScrollKeepsHeaderAligned()
        {
            // Derives from TableView-dev-spec.md "Sticky headers": the control drives PART_HeaderScroller's
            // horizontal offset from PART_BodyScroller.ViewChanged, so the header band tracks horizontal body
            // scrolling. This test proves real pointer scroll moves the non-frozen header; a failure means the
            // header<->body horizontal sync never runs off the input path.
            //
            // LIMITATION (finding #13): the plan item's full claim is that headers move "in lockstep with
            // cells". Reading an individual cell's x requires descending into a row peer, which crashes the
            // app, so the cell half is unreachable out-of-proc. This asserts the header tracks the body scroll,
            // which is the observable half.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem))
                {
                    Verify.Fail("Could not select the 'Scrolling' pivot item.");
                    return;
                }

                UIObject tableView = FindElement.ById(ScrollingTable);
                if (tableView == null)
                {
                    Verify.Fail("ScrollingTableView was not found on the test page.");
                    return;
                }

                UIObject scrollHeader = FindColumnHeader(tableView, "ScrollCity");
                if (scrollHeader == null)
                {
                    Verify.Fail("The 'ScrollCity' column header peer was not found on the header host.");
                    return;
                }

                double leftBefore = scrollHeader.BoundingRectangle.Left;
                string offsetsBefore = ReadScrollOffsets();

                // The Scrolling table is 520px wide with ~840px of columns, so it overflows horizontally.
                // Horizontal scrolling is driven by dragging the body scroller's horizontal ScrollBar thumb:
                // that is the only real pointer route the mouse has here. Shift+wheel does NOT work - measured,
                // it scrolls VERTICALLY instead (offsets went H=0;V=39) - and MITA exposes no horizontal wheel.
                // A small 30px thumb drag keeps ScrollCity inside the viewport; a header scrolled clear out
                // reports an empty rectangle whose Left reads 0, which would satisfy the assertion falsely.
                if (!DragHorizontalScrollBar(tableView, 30))
                {
                    Verify.Fail("The body scroller's horizontal ScrollBar was not found.");
                    return;
                }

                // The header scroller's own offset (HeaderH) is published alongside the body's, so this test can
                // tell "the sync never ran" from "the sync ran but the header did not move".
                //
                // Do NOT call ElementCache.Clear() here. The next FindElement.ById would miss and run
                // ElementCache.Refresh(), which walks window.Descendants reading .Name on every node
                // (FindElement.cs:414); computing a TableViewRow peer's name manufactures cell peers and trips
                // finding #13 (0xC0000420), taking the app down mid-test. It also buys nothing: FindColumnHeader
                // walks tableView.Children live on every call, so the header rectangles below are already
                // re-read from the tree rather than served from that cache.
                double leftAfter = FindColumnHeader(tableView, "ScrollCity").BoundingRectangle.Left;
                string offsetsAfter = ReadScrollOffsets();

                Log.Comment("ScrollCity header Left before={0}, after horizontal scroll={1}. Body offsets '{2}' -> '{3}'.",
                    leftBefore, leftAfter, offsetsBefore, offsetsAfter);

                // Precondition: the body must have scrolled horizontally, or the header assertion is vacuous.
                Verify.AreNotEqual(
                    offsetsBefore,
                    offsetsAfter,
                    "Dragging the horizontal ScrollBar thumb must scroll PART_BodyScroller; identical offsets mean the input never reached the scroller.");

                Verify.IsGreaterThan(
                    leftAfter,
                    0.0,
                    "The 'ScrollCity' header must still be on screen after the drag, otherwise the movement assertion below would be satisfied by the header vanishing.");

                Verify.IsLessThan(
                    leftAfter,
                    leftBefore - 10.0,
                    "A non-frozen header must move LEFT when the body is scrolled right (dev-spec Sticky headers).");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the header row stays pinned vertically while the body is scrolled by pointer.")]
        public void VerticalScrollKeepsHeaderSticky()
        {
            // Derives from TableView-dev-spec.md "Sticky headers" / header/body layout: the header band is a
            // separate scroller from the body, so vertical body scroll must not move the header down or off.
            // A failure means the header participates in vertical scrolling and slides away from the top.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem))
                {
                    Verify.Fail("Could not select the 'Scrolling' pivot item.");
                    return;
                }

                UIObject tableView = FindElement.ById(ScrollingTable);
                if (tableView == null)
                {
                    Verify.Fail("ScrollingTableView was not found on the test page.");
                    return;
                }

                UIObject frozenHeader = FindColumnHeader(tableView, "FrozenName");
                if (frozenHeader == null)
                {
                    Verify.Fail("The 'FrozenName' column header peer was not found on the header host.");
                    return;
                }

                double topBefore = frozenHeader.BoundingRectangle.Top;

                // Precondition material: the body must actually scroll, or "the header did not move" is vacuous.
                // A row peer's Name came back empty when this was measured, so the page publishes
                // PART_BodyScroller's own offsets instead.
                string offsetsBefore = ReadScrollOffsets();

                // 200 items in a 300px-tall table: several wheel notches scroll the body well past a viewport.
                // The header must not follow.
                WheelAtPoint(CentreOf(tableView), -3 * mouseWheelDelta);

                // No ElementCache.Clear() here - see HorizontalScrollKeepsHeaderAligned: clearing forces a
                // Refresh() that reads .Name across the whole tree and trips finding #13. The freshness this
                // negative assertion needs comes from FindColumnHeader's live walk of tableView.Children.
                double topAfter = FindColumnHeader(tableView, "FrozenName").BoundingRectangle.Top;
                string offsetsAfter = ReadScrollOffsets();

                Log.Comment("Header Top before={0}, after vertical scroll={1}. Body offsets '{2}' -> '{3}'.",
                    topBefore, topAfter, offsetsBefore, offsetsAfter);

                Verify.AreNotEqual(
                    offsetsBefore,
                    offsetsAfter,
                    "The wheel must scroll PART_BodyScroller vertically; identical offsets would make the assertion below vacuous.");

                Verify.IsLessThanOrEqual(
                    Math.Abs(topAfter - topBefore),
                    4.0,
                    "The header row must stay pinned at the top during vertical scroll (dev-spec Sticky headers).");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Finding #20: composition translation/clip are invisible to UIA BoundingRectangle at this tier.
        [TestProperty("Description", "Verifies a FrozenEdge.Leading column stays pinned while unfrozen columns scroll horizontally under pointer input.")]
        public void FrozenColumnStaysPinnedUnderPointerScroll()
        {
            // Derives from TableView-dev-spec.md "Frozen leading columns" (:149): leading-frozen header and
            // body cells are counter-translated against the horizontal scroll offset, so a FrozenEdge.Leading
            // column keeps its on-screen x while non-frozen columns shift. FrozenName authors
            // FrozenEdge="Leading" on the Scrolling table (TableView.idl:150). API 4.x sets the offset directly
            // via ChangeView; this test drives real wheel and drag so the frozen layout runs off the input
            // path. A failure means frozen pinning is applied only from the programmatic scroll path.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem))
                {
                    Verify.Fail("Could not select the 'Scrolling' pivot item.");
                    return;
                }

                UIObject tableView = FindElement.ById(ScrollingTable);
                if (tableView == null)
                {
                    Verify.Fail("ScrollingTableView was not found on the test page.");
                    return;
                }

                UIObject frozenHeader = FindColumnHeader(tableView, "FrozenName");
                UIObject scrollHeader = FindColumnHeader(tableView, "ScrollCity");
                if (frozenHeader == null || scrollHeader == null)
                {
                    Verify.Fail("The 'FrozenName' and/or 'ScrollCity' header peers were not found on the header host.");
                    return;
                }

                double frozenLeftBefore = frozenHeader.BoundingRectangle.Left;
                double scrollLeftBefore = scrollHeader.BoundingRectangle.Left;
                string offsetsBefore = ReadScrollOffsets();

                // Shift+wheel is NOT a horizontal scroll here - measured, it scrolls vertically - so the drag of
                // the body scroller's horizontal ScrollBar thumb is the real pointer route. Kept small so
                // ScrollCity stays inside the viewport: a header scrolled clear out reports an empty rectangle
                // whose Left reads 0, which would satisfy the "it moved" precondition falsely.
                if (!DragHorizontalScrollBar(tableView, 30))
                {
                    Verify.Fail("The body scroller's horizontal ScrollBar was not found.");
                    return;
                }

                // No ElementCache.Clear() here - see HorizontalScrollKeepsHeaderAligned for why it crashes the
                // app (finding #13) without making these rectangles any fresher.
                double frozenLeftAfter = FindColumnHeader(tableView, "FrozenName").BoundingRectangle.Left;
                double scrollLeftAfter = FindColumnHeader(tableView, "ScrollCity").BoundingRectangle.Left;
                string offsetsAfter = ReadScrollOffsets();

                Log.Comment("FrozenName Left {0}->{1}; ScrollCity Left {2}->{3}; body offsets '{4}' -> '{5}'.",
                    frozenLeftBefore, frozenLeftAfter, scrollLeftBefore, scrollLeftAfter, offsetsBefore, offsetsAfter);

                Verify.AreNotEqual(
                    offsetsBefore,
                    offsetsAfter,
                    "Dragging the horizontal ScrollBar thumb must scroll PART_BodyScroller horizontally; identical offsets mean the input never reached the scroller.");

                // The unfrozen column must actually have moved — otherwise the pinned assertion is vacuous.
                Verify.IsGreaterThan(
                    scrollLeftAfter,
                    0.0,
                    "The unfrozen 'ScrollCity' header must still be on screen; a header scrolled out reports an empty rectangle and would fake the movement below.");

                Verify.IsGreaterThan(
                    Math.Abs(scrollLeftAfter - scrollLeftBefore),
                    10.0,
                    "The unfrozen 'ScrollCity' column must shift under horizontal scroll (precondition for the frozen assertion).");

                // The frozen column's on-screen x must not move (dev-spec:149 counter-translation).
                Verify.IsLessThanOrEqual(
                    Math.Abs(frozenLeftAfter - frozenLeftBefore),
                    4.0,
                    "A FrozenEdge.Leading column must stay pinned while the body scrolls horizontally (dev-spec:149).");
            }
        }

        #endregion

        #region 8. Right-to-left

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product finding #19: RTL pointer resize gripper placement and delta are not mirrored.
        [TestProperty("Description", "Verifies a pointer resize drag is direction-mirrored under RTL: a leftward drag widens the leading column.")]
        public void RightToLeftResizeMirrors()
        {
            // Derives from TableView-dev-spec.md:131: "positive always grows in reading order ... only the
            // horizontal axis mirrors under RTL." Under RTL the leading column (RtlName) is laid out at the
            // screen RIGHT and its gripper sits on its screen-LEFT edge, so the drag that WIDENS it moves LEFT
            // (-x) - the mirror of the LTR rightward widen proven by PointerResizeDragChangesColumnWidth. The
            // sign is the whole point: a failure means the primitive did not mirror the horizontal axis and a
            // leftward drag shrinks (or does nothing to) the column under RTL.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(RtlPivotItem))
                {
                    Verify.Fail("Could not select the 'Rtl' pivot item.");
                    return;
                }

                UIObject tableView = FindElement.ById(RtlTable);
                if (tableView == null)
                {
                    Verify.Fail("RtlTableView was not found on the test page.");
                    return;
                }

                UIObject rtlNameHeader = FindColumnHeader(tableView, "RtlName");
                if (rtlNameHeader == null)
                {
                    Verify.Fail("The 'RtlName' column header peer was not found on the header host.");
                    return;
                }

                UIObject rtlCityHeader = FindColumnHeader(tableView, "RtlCity");
                if (rtlCityHeader == null)
                {
                    Verify.Fail("The 'RtlCity' column header peer was not found on the header host.");
                    return;
                }

                double nameWidthBefore = rtlNameHeader.BoundingRectangle.Width;
                double cityWidthBefore = rtlCityHeader.BoundingRectangle.Width;

                // Diagnostic: log every header's bounds so the actual RTL layout order, and therefore the real
                // column boundary the gripper straddles, is on record. Header peers are not row peers, so this
                // does not trip finding #13.
                foreach (UIObject header in tableView.Children[0].Children)
                {
                    Log.Comment("RTL header '{0}' bounds={1}.", header.Name, header.BoundingRectangle);
                }

                // The gripper is an 8px band straddling a column boundary, and which SIDE of the boundary it
                // occupies is the placement question this test exists to pin. Pressing "1px inside my own
                // header" therefore begs the question: it lands on the band only if the band is placed the way
                // the test assumed. So press the boundary itself, from RtlName's side first and RtlCity's side
                // second, and report which side responded and which column moved. Only a gesture that cannot
                // miss the affordance can prove anything about its direction.
                //
                // Under RTL, RtlName is the reading-order-FIRST column and is laid out at the screen RIGHT, so
                // the boundary it shares with RtlCity is RtlName's screen-LEFT edge, and the drag that widens
                // RtlName moves LEFT (-x) - the mirror of the LTR rightward widen.
                int boundaryX = rtlNameHeader.BoundingRectangle.Left;
                const int dragBy = 80;

                DragAtAbsoluteX(rtlNameHeader, boundaryX + 2, -dragBy);
                double nameAfterLeadingSide = FindColumnHeader(tableView, "RtlName").BoundingRectangle.Width;
                double cityAfterLeadingSide = FindColumnHeader(tableView, "RtlCity").BoundingRectangle.Width;
                Log.Comment("Boundary x={0}, pressed RtlName's side (+2), dragged {1}px: RtlName {2}->{3}, RtlCity {4}->{5}.",
                    boundaryX, -dragBy, nameWidthBefore, nameAfterLeadingSide, cityWidthBefore, cityAfterLeadingSide);

                // Take every observation BEFORE the assertion: a failed Verify is terminal in this suite, so
                // anything logged after it is lost on the run that most needs it.
                double nameAfterTrailingSide = nameAfterLeadingSide;
                double cityAfterTrailingSide = cityAfterLeadingSide;
                if (Math.Abs(nameAfterLeadingSide - nameWidthBefore) < 1.0
                    && Math.Abs(cityAfterLeadingSide - cityWidthBefore) < 1.0)
                {
                    DragAtAbsoluteX(FindColumnHeader(tableView, "RtlCity"), boundaryX - 2, -dragBy);
                    nameAfterTrailingSide = FindColumnHeader(tableView, "RtlName").BoundingRectangle.Width;
                    cityAfterTrailingSide = FindColumnHeader(tableView, "RtlCity").BoundingRectangle.Width;
                    Log.Comment("Nothing moved; pressed RtlCity's side (-2), dragged {0}px: RtlName {1}->{2}, RtlCity {3}->{4}.",
                        -dragBy, nameWidthBefore, nameAfterTrailingSide, cityWidthBefore, cityAfterTrailingSide);
                }

                // Last resort: the header's screen-RIGHT edge is the table's OUTER edge under RTL and borders
                // no column at all. A width change there means the gripper was left on the column's physical
                // right edge, i.e. placed as if the layout were still LTR.
                double nameAfterOuterEdgeProbe = nameAfterTrailingSide;
                if (Math.Abs(nameAfterTrailingSide - nameWidthBefore) < 1.0)
                {
                    var nameBounds = FindColumnHeader(tableView, "RtlName").BoundingRectangle;
                    DragAtAbsoluteX(FindColumnHeader(tableView, "RtlName"), nameBounds.Left + nameBounds.Width - 2, dragBy);
                    nameAfterOuterEdgeProbe = FindColumnHeader(tableView, "RtlName").BoundingRectangle.Width;
                    Log.Comment("Still nothing; probed the table's OUTER edge with a rightward drag: RtlName {0}->{1}.",
                        nameWidthBefore, nameAfterOuterEdgeProbe);
                }

                double widthAfter = Math.Max(nameAfterLeadingSide, nameAfterTrailingSide);

                Verify.IsGreaterThan(
                    widthAfter,
                    nameWidthBefore + 20.0,
                    "Under RTL a LEFTWARD pointer drag on the RtlName/RtlCity boundary must WIDEN RtlName, the reading-order-first column (dev-spec:131 horizontal mirror).");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies keyboard resize is direction-mirrored under RTL: Alt+Left widens the column and Alt+Right shrinks it, without moving header focus.")]
        public void RightToLeftKeyboardResizeMirrors()
        {
            // Previously named RightToLeftKeyboardNavigationMirrors.
            // dev-spec:133 - TryKeyboardStep owns direction and the RTL mirror; dev-spec:131 - positive grows in
            // reading order, so under RTL the forward chord is Alt+LEFT. The Alt+Arrow chord is the #11820 model
            // (spec debt: dev-spec:135 still says bare Left/Right). Focus is re-checked so a chord that navigates
            // instead of resizing cannot pass on a clipped header rectangle.
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(RtlPivotItem))
                {
                    Verify.Fail("Could not select the 'Rtl' pivot item.");
                    return;
                }

                UIObject tableView = FindElement.ById(RtlTable);
                if (tableView == null)
                {
                    Verify.Fail("RtlTableView was not found on the test page.");
                    return;
                }

                UIObject rtlNameHeader = FindColumnHeader(tableView, "RtlName");
                if (rtlNameHeader == null)
                {
                    Verify.Fail("The 'RtlName' column header peer was not found on the header host.");
                    return;
                }

                rtlNameHeader.SetFocus();
                Wait.ForIdle();
                if (!rtlNameHeader.HasKeyboardFocus)
                {
                    Verify.Fail("The 'RtlName' header did not take keyboard focus; the resize keyboard step cannot be exercised.");
                    return;
                }

                double widthStart = rtlNameHeader.BoundingRectangle.Width;

                // Alt+Left = reading-order-forward under RTL => widen.
                KeyboardHelper.PressKey(Key.Left, ModifierKey.Alt, numPresses: 6);
                Wait.ForIdle();
                UIObject afterLeft = FindColumnHeader(tableView, "RtlName");
                double widthAfterLeft = afterLeft.BoundingRectangle.Width;
                bool focusKeptLeft = afterLeft.HasKeyboardFocus;

                Log.Comment("RTL keyboard resize: width start={0}, after Alt+Left x6={1}, focus kept={2}.", widthStart, widthAfterLeft, focusKeptLeft);

                Verify.IsTrue(focusKeptLeft, "Alt+Left must resize RtlName, not move focus to another header.");
                Verify.IsGreaterThan(
                    widthAfterLeft,
                    widthStart + 2.0,
                    "Under RTL, Alt+Left on the header must WIDEN the column (dev-spec:133 RTL mirror + dev-spec:131 reading-order growth).");

                // Alt+Right = reading-order-backward under RTL => shrink.
                KeyboardHelper.PressKey(Key.Right, ModifierKey.Alt, numPresses: 6);
                Wait.ForIdle();
                UIObject afterRight = FindColumnHeader(tableView, "RtlName");
                double widthAfterRight = afterRight.BoundingRectangle.Width;

                Log.Comment("RTL keyboard resize: after Alt+Right x6={0} (was {1}), focus kept={2}.", widthAfterRight, widthAfterLeft, afterRight.HasKeyboardFocus);

                Verify.IsTrue(afterRight.HasKeyboardFocus, "Alt+Right must resize RtlName, not move focus to another header.");
                Verify.IsLessThan(
                    widthAfterRight,
                    widthAfterLeft - 2.0,
                    "Under RTL, Alt+Right on the header must SHRINK the column - the mirror of Alt+Left (dev-spec:133).");
            }
        }

        #endregion

        // ---- helpers ---------------------------------------------------------------------------------

        // Presses at an ABSOLUTE screen x on the header band and drags dx pixels horizontally, releasing at the
        // end. anchorHeader only supplies the peer to press through and the y centre - the x comes from the
        // caller, so the gesture is aimed at a measured boundary rather than at an assumed edge of an element.
        //
        // MITA offsets are CENTRE-relative, not top-left relative, which is why the offset is computed against
        // the anchor's centre. Getting this backwards silently presses the middle of the header, where there is
        // no gripper, and reads as a product failure.
        private static void DragAtAbsoluteX(UIObject anchorHeader, int absoluteX, int dx)
        {
            var bounds = anchorHeader.BoundingRectangle;
            int centreX = bounds.Left + (bounds.Width / 2);
            int y = bounds.Top + (bounds.Height / 2);

            InputHelper.LeftMouseButtonDown(anchorHeader, absoluteX - centreX, 0);

            // Move in a couple of steps so the manipulation is recognized past the 0.5 DIP deadband.
            InputHelper.MoveMouse(new Point(absoluteX + (dx / 2), y));
            InputHelper.MoveMouse(new Point(absoluteX + dx, y));
            InputHelper.LeftMouseButtonUp();
            Wait.ForIdle();
        }

        // Rotates the wheel with the pointer parked over the given point. Vertical only: MITA exposes no
        // horizontal wheel, and holding Shift does not turn this into one (measured - the body scrolled
        // VERTICALLY with Shift down). Use DragHorizontalScrollBar for the horizontal axis.
        private static void WheelAtPoint(Point point, int delta)
        {
            Log.Comment("Rotate wheel by {0} at ({1}, {2}).", delta, point.X, point.Y);

            PointerInput.Move(point);
            MouseWheelInput.RotateWheel(delta);
            Wait.ForIdle();
        }

        // Scrolls the body horizontally by mouse-dragging the body scroller's horizontal ScrollBar thumb.
        // dx is a screen delta: positive drags the thumb right, scrolling the content right so the headers
        // travel left. Returns false if the ScrollBar was not found.
        //
        // The body scroller is the TableView's SECOND ScrollViewer child (the first is the header band), and
        // it exposes 'Vertical' and 'Horizontal' ScrollBar children alongside the rows. Enumerating the
        // scroller's own children is safe; only descending into a ROW's children trips finding #13.
        private static bool DragHorizontalScrollBar(UIObject tableView, int dx)
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
                    if (scrollerChild.ClassName == "ScrollBar" && scrollerChild.Name == "Horizontal")
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

            // Press a quarter of the way along the track, which is inside the thumb while the offset is 0
            // The viewport is ~62% of the extent, so the thumb covers the left ~62% of the track.
            int startX = bounds.Left + (bounds.Width / 4);
            int y = bounds.Top + (bounds.Height / 2);

            Log.Comment("Dragging the horizontal ScrollBar thumb from ({0}, {1}) by {2}px.", startX, y, dx);

            InputHelper.LeftMouseButtonDown(scrollBar, -(bounds.Width / 4), 0);
            InputHelper.MoveMouse(new Point(startX + (dx / 2), y));
            InputHelper.MoveMouse(new Point(startX + dx, y));
            InputHelper.LeftMouseButtonUp();
            Wait.ForIdle();

            return true;
        }

    }
}
