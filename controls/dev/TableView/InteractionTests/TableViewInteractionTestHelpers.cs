// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using MUXTestInfra.Shared.Infra;
using Point = System.Drawing.Point;

using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // Helpers shared by every TableView interaction-test class. Page facts (AutomationIds, column layout, data) come
    // from TableViewTestPageFacts, which the test page itself is built from.
    //
    // Structure of the TableView provider tree, as a client sees it: the header host is the TableView peer's FIRST
    // child (its children are the column header peers) and the rows host is its LAST child (its children are row
    // and group-header peers in projection order). Each row peer's children are its cell peers, one per visible
    // column in visible order.
    //
    // History: these helpers used to avoid ever reading a row's children, because doing so fail-fasted the app
    // (finding #13), and avoided every InputHelper offset overload, because GetClickablePoint access-violated on
    // TableView peers (finding #14). #11820 rebuilt the peers and cell peers are now read safely. Pointer input is
    // still expressed as ABSOLUTE screen points: the offset overloads anchor inconsistently (some at the clickable
    // point, some at the top-left), so a row-relative column x cannot be expressed reliably as an offset.
    internal static class TableViewInteractionTestHelpers
    {
        // ---------- Finding elements ----------

        internal static UIObject GetTable(string tableAutomationId)
        {
            UIObject tableView = FindElement.ById(tableAutomationId);
            if (tableView == null) { Verify.Fail(tableAutomationId + " was not found."); return null; }
            if (tableView.Children.Count < 1) { Verify.Fail(tableAutomationId + " exposed no children."); return null; }
            return tableView;
        }

        internal static UIObject GetHeaderHost(UIObject tableView) => tableView.Children[0];

        internal static UIObject GetRowsHost(UIObject tableView) => tableView.Children[tableView.Children.Count - 1];

        internal static UIObject GetRowsHost(string tableAutomationId)
        {
            UIObject tableView = GetTable(tableAutomationId);
            return tableView == null ? null : GetRowsHost(tableView);
        }

        // The header peer whose Name is the column header text, or null.
        internal static UIObject FindColumnHeader(UIObject tableView, string headerText)
        {
            foreach (UIObject child in GetHeaderHost(tableView).Children)
            {
                if (child.Name == headerText)
                {
                    return child;
                }
            }

            return null;
        }

        internal static UIObject GetHeader(string tableAutomationId, string headerText)
        {
            UIObject tableView = GetTable(tableAutomationId);
            return tableView == null ? null : FindColumnHeader(tableView, headerText);
        }

        internal static bool IsRow(UIObject element) =>
            element != null && element.ClassName != null && element.ClassName.Contains("TableViewRow");

        // The group header's class name is "TableViewGroupHeader" (TableViewGroupHeaderAutomationPeer::GetClassNameCore).
        internal static bool IsGroupHeader(UIObject element) =>
            element != null && element.ClassName != null && element.ClassName.Contains("GroupHeader");

        // The rows-host child at index if it is a TableViewRow, otherwise null.
        internal static UIObject GetRow(UIObject tableView, int index)
        {
            UIObject rowsHost = GetRowsHost(tableView);
            if (rowsHost == null || index >= rowsHost.Children.Count)
            {
                return null;
            }

            UIObject candidate = rowsHost.Children[index];
            return IsRow(candidate) ? candidate : null;
        }

        // Counts the TableViewRow peers under the rows host. Clears the element cache first: a collapse driven from
        // OUTSIDE the table (a page button) changes the rows host's children with no interaction inside the tree to
        // invalidate MITA's cache, so a cached walk can report the pre-collapse count.
        internal static int CountRows(UIObject rowsHost)
        {
            ElementCache.Clear();

            int count = 0;
            foreach (UIObject child in rowsHost.Children)
            {
                if (IsRow(child))
                {
                    count++;
                }
            }

            return count;
        }

        internal static UIObject GetFirstGroupHeader(UIObject rowsHost) => GetGroupHeaderAt(rowsHost, 0);

        // A collapse or expand recycles the group header container, so callers re-read the header after each toggle
        // instead of reusing the UIObject captured before it. Fails (and returns null) if it is gone or names another group.
        internal static UIObject RefetchFirstGroupHeader(UIObject tableView, string expectedName)
        {
            Wait.ForIdle();
            ElementCache.Clear();
            UIObject header = GetFirstGroupHeader(GetRowsHost(tableView));
            if (header == null)
            {
                Verify.Fail("The first group header must still be realized after the toggle.");
                return null;
            }

            Verify.AreEqual(expectedName, header.Name, "The first group header must still name the same group.");
            return header;
        }

        // Injected touch is delivered asynchronously, so a single idle wait can observe the header before the tap
        // lands. Polls until the first group header reports the expected state, then falls back to the strict refetch.
        internal static UIObject WaitForFirstGroupHeaderState(UIObject tableView, string expectedName, ExpandCollapseState expected)
        {
            for (int attempt = 0; attempt < 20; attempt++)
            {
                Wait.ForIdle();
                ElementCache.Clear();
                UIObject header = GetFirstGroupHeader(GetRowsHost(tableView));
                if (header != null &&
                    header.Name == expectedName &&
                    new ExpandCollapseImplementation(header).ExpandCollapseState == expected)
                {
                    Log.Comment("Group header reached {0} after {1} poll(s).", expected, attempt + 1);
                    return header;
                }

                Wait.ForMilliseconds(100);
            }

            return RefetchFirstGroupHeader(tableView, expectedName);
        }

        // The index-th group header among the rows host's own children, or null.
        internal static UIObject GetGroupHeaderAt(UIObject rowsHost, int index)
        {
            int seen = 0;
            foreach (UIObject child in rowsHost.Children)
            {
                if (IsGroupHeader(child))
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

        // Depth-first search for a named element (for example, a control hosted in a cell).
        internal static UIObject FindDescendantByName(UIObject root, string name)
        {
            foreach (UIObject child in root.Children)
            {
                if (child.Name == name)
                {
                    return child;
                }
                UIObject nested = FindDescendantByName(child, name);
                if (nested != null)
                {
                    return nested;
                }
            }
            return null;
        }

        // ---------- Focus ----------

        // The focused element's UIA PositionInSet (1-based, within the group), or -1. Unlike a scroll
        // percentage, it distinguishes a page from a single step.
        internal static int FocusedPositionInSet()
        {
            AutomationElement focused = AutomationElement.FocusedElement;
            if (focused == null) { return -1; }

            object value = focused.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
            return value is int position ? position : -1;
        }

        internal static UIObject FindFocusedRow(UIObject rowsHost)
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

        // Index of the row's cell peer that holds keyboard focus, or -1.
        internal static int IndexOfFocusedCell(UIObject row)
        {
            if (row == null) { return -1; }
            int index = 0;
            foreach (UIObject cell in row.Children)
            {
                if (cell.HasKeyboardFocus)
                {
                    return index;
                }
                index++;
            }
            return -1;
        }

        // The realized row that holds focus itself or through one of its cells, or null.
        internal static UIObject FindFocusedBodyElement(UIObject rowsHost)
        {
            if (rowsHost == null) { return null; }
            foreach (UIObject row in rowsHost.Children)
            {
                if (row.HasKeyboardFocus || IndexOfFocusedCell(row) >= 0)
                {
                    return row;
                }
            }
            return null;
        }

        // The header peer of the named table that holds keyboard focus, or null.
        internal static UIObject FindFocusedHeader(string tableAutomationId)
        {
            UIObject tableView = GetTable(tableAutomationId);
            if (tableView == null) { return null; }
            foreach (UIObject header in GetHeaderHost(tableView).Children)
            {
                if (header.HasKeyboardFocus)
                {
                    return header;
                }
            }
            return null;
        }

        // Which realized row/cell holds keyboard focus; both are -1 when no cell does.
        internal static void FindFocusedCell(string tableAutomationId, out int rowIndex, out int cellIndex)
        {
            rowIndex = -1;
            cellIndex = -1;
            UIObject rowsHost = GetRowsHost(tableAutomationId);
            if (rowsHost == null) { return; }

            int r = 0;
            foreach (UIObject row in rowsHost.Children)
            {
                int c = IndexOfFocusedCell(row);
                if (c >= 0)
                {
                    rowIndex = r;
                    cellIndex = c;
                    return;
                }
                r++;
            }
        }

        internal static void VerifyFocusedCell(string tableAutomationId, int expectedRow, int expectedCell, string message)
        {
            int rowIndex, cellIndex;
            FindFocusedCell(tableAutomationId, out rowIndex, out cellIndex);
            Log.Comment("Focused cell: row={0}, cell={1} (expected row={2}, cell={3}).", rowIndex, cellIndex, expectedRow, expectedCell);
            Verify.IsTrue(rowIndex == expectedRow && cellIndex == expectedCell, message);
        }

        internal static void VerifyFocusedHeader(string tableAutomationId, string expectedHeader, string message)
        {
            UIObject focused = FindFocusedHeader(tableAutomationId);
            Log.Comment("Focused header: {0} (expected {1}).", focused == null ? "<none>" : focused.Name, expectedHeader);
            Verify.IsTrue(focused != null && focused.Name == expectedHeader, message);
        }

        // Focuses the row at index through UIA. Returns null (after failing) if it is missing or does not take focus.
        internal static UIObject FocusRowAt(string tableAutomationId, int rowIndex)
        {
            UIObject rowsHost = GetRowsHost(tableAutomationId);
            if (rowsHost == null) { return null; }
            if (rowsHost.Children.Count <= rowIndex) { Verify.Fail(string.Format("Need at least {0} realized rows.", rowIndex + 1)); return null; }

            UIObject row = rowsHost.Children[rowIndex];
            row.SetFocus();
            Wait.ForIdle();
            if (!row.HasKeyboardFocus) { Verify.Fail(string.Format("Precondition: row {0} did not take focus.", rowIndex)); return null; }
            return row;
        }

        // Focuses the row at index and drills into its first cell with Right. Returns false (after failing) if
        // either step does not take; callers then stop, since every later assertion would be unmeasured.
        internal static bool DrillIntoRow(string tableAutomationId, int rowIndex)
        {
            if (FocusRowAt(tableAutomationId, rowIndex) == null) { return false; }

            KeyboardHelper.PressKey(Key.Right);
            Wait.ForIdle();
            if (IndexOfFocusedCell(GetRowsHost(tableAutomationId).Children[rowIndex]) != 0)
            {
                Verify.Fail(string.Format("Precondition: Right did not drill into row {0}'s first cell.", rowIndex));
                return false;
            }
            return true;
        }

        // ---------- Selection ----------

        // Selection has two channels out of proc: the SelectionItem pattern, and the underlying UIA property as a
        // fallback. Returns false only when NEITHER answers, so callers can tell "not selected" apart from
        // "cannot be observed" instead of passing vacuously.
        internal static bool TryGetIsSelected(UIObject row, out bool isSelected)
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

        internal static bool IsSelected(UIObject row)
        {
            bool isSelected;
            return TryGetIsSelected(row, out isSelected) && isSelected;
        }

        internal static void SelectRow(UIObject row)
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

        internal static UIObject FindSelectedRow(UIObject rowsHost)
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

        // Position of the selected row among the realized rows, or -1. Re-resolves the rows host on every call so the
        // walk reflects the post-sort order. Deliberately does NOT clear the element cache: UIA re-queries children
        // on access, which is enough here.
        internal static int IndexOfSelectedRow(string tableAutomationId)
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

        internal static void VerifyNoRowSelected(string when, string tableAutomationId = BasicTable)
        {
            UIObject rowsHost = GetRowsHost(tableAutomationId);
            if (rowsHost == null) { return; }
            foreach (UIObject row in rowsHost.Children)
            {
                bool isSelected;
                if (!TryGetIsSelected(row, out isSelected))
                {
                    Verify.Fail("Selection could not be observed on a row while checking " + when + ".");
                    return;
                }
                Verify.IsFalse(isSelected, "No row may be selected merely by " + when + ".");
            }
        }

        // ---------- Pointer (absolute screen points) ----------

        internal static string DescribeFocused()
        {
            try
            {
                UIObject focused = UIObject.Focused;
                return focused == null ? "<none>" : string.Format("'{0}' [{1}]", focused.Name, focused.ClassName);
            }
            catch (Exception e)
            {
                return "<unavailable: " + e.GetType().Name + ">";
            }
        }

        // Vertical only: MITA has no horizontal wheel and Shift does not make one; use DragHorizontalScrollBar.
        internal static void WheelAtPoint(Point point, int delta)
        {
            Log.Comment("Rotate wheel by {0} at ({1}, {2}).", delta, point.X, point.Y);

            PointerInput.Move(point);
            MouseWheelInput.RotateWheel(delta);
            Wait.ForIdle();
        }

        internal static Point CentreOf(UIObject element)
        {
            var bounds = element.BoundingRectangle;
            return new Point(bounds.Left + (bounds.Width / 2), bounds.Top + (bounds.Height / 2));
        }

        internal static Point RowPointAtColumnOffset(UIObject row, int rowRelativeX)
        {
            var bounds = row.BoundingRectangle;
            return new Point(bounds.Left + rowRelativeX, bounds.Top + (bounds.Height / 2));
        }

        internal static void ClickPoint(Point point)
        {
            Log.Comment("Click at absolute point ({0}, {1}).", point.X, point.Y);
            PointerInput.Move(point);
            PointerInput.Press(PointerButtons.Primary);
            PointerInput.Release(PointerButtons.Primary);
            Wait.ForIdle();
        }

        // Clicks the row at a row-relative x, so a caller can name a column (see BasicColumnCentreX).
        internal static void ClickRowAtColumnOffset(UIObject row, int rowRelativeX)
        {
            ClickPoint(RowPointAtColumnOffset(row, rowRelativeX));
        }

        // The two press/release pairs are issued back to back with no idle wait between them: a Wait.ForIdle in the
        // middle can exceed the system double-click time and turn the gesture into two single clicks.
        internal static void DoubleClickRowAtColumnOffset(UIObject row, int rowRelativeX)
        {
            var point = RowPointAtColumnOffset(row, rowRelativeX);

            Log.Comment("Double-click at absolute point ({0}, {1}).", point.X, point.Y);
            PointerInput.Move(point);
            PointerInput.Press(PointerButtons.Primary);
            PointerInput.Release(PointerButtons.Primary);
            PointerInput.Press(PointerButtons.Primary);
            PointerInput.Release(PointerButtons.Primary);
            Wait.ForIdle();
        }

        // Clicks a header once, re-finding it first: a previous click can rebuild the header band, which invalidates
        // any UIObject the caller still holds.
        internal static void ClickHeader(string tableAutomationId, string columnHeader)
        {
            UIObject header = GetHeader(tableAutomationId, columnHeader);
            if (header == null) { Verify.Fail("The " + columnHeader + " header was not found."); return; }
            InputHelper.LeftClick(header);
            Wait.ForIdle();
        }

        internal static void ClickHeaderTwice(string tableAutomationId, string columnHeader)
        {
            ClickHeader(tableAutomationId, columnHeader);
            ClickHeader(tableAutomationId, columnHeader);
        }

        // Presses just inside the column's LTR trailing edge, where the ResizeGripper straddles the boundary, and
        // drags right by widenBy so the column WIDENS, optionally pressing Escape mid-gesture, then releases. LTR only:
        // under RTL use a boundary measured from the two adjacent headers. MITA offsets are CENTRE-relative, so the
        // press offset is Width/2 - 1. The moves use absolute points because the header's own rectangle grows during
        // the drag, so a relative offset would drift. Two steps clear the 0.5 DIP deadband (dev-spec:123).
        internal static void DragColumnBoundary(UIObject header, int widenBy, bool cancelWithEscape = false)
        {
            var bounds = header.BoundingRectangle;

            int grabOffsetX = (bounds.Width / 2) - 1;
            int startX = bounds.Left + bounds.Width - 1;
            int y = bounds.Top + (bounds.Height / 2);

            InputHelper.LeftMouseButtonDown(header, grabOffsetX, 0);
            InputHelper.MoveMouse(new Point(startX + (widenBy / 2), y));
            InputHelper.MoveMouse(new Point(startX + widenBy, y));

            if (cancelWithEscape)
            {
                KeyboardHelper.PressKey(Key.Escape);
            }

            InputHelper.LeftMouseButtonUp();
            Wait.ForIdle();
        }

        // dx is a screen delta; positive scrolls the content right. Returns false if the ScrollBar was not found.
        // The ScrollBars are direct children of the body scroller; enumerating those is safe, unlike a row's children.
        internal static bool DragHorizontalScrollBar(UIObject tableView, int dx)
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

            // A quarter along the track is inside the thumb at offset 0: the viewport is ~62% of the extent.
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

        // ---------- Test page ----------

        // MUXTestInfra's AppWatcherThread kills the test host once ONE app instance has run for 15 minutes
        // (Application.cs: "Test app process ran for an unexpectedly long time"). A TableView class with ~30 passing
        // tests keeps a single instance alive past that, so the test that happens to straddle the mark fails with
        // 0xE0434352 before it starts. Call from every TestCleanup: once the instance is older than the threshold, the
        // next TestSetupHelper restarts it.
        private static readonly TimeSpan s_appRestartThreshold = TimeSpan.FromMinutes(10);

        internal static void RestartAppIfLongRunning()
        {
            try
            {
                var process = TestEnvironment.Application.Process;
                if (process != null && !process.HasExited && DateTime.Now - process.StartTime > s_appRestartThreshold)
                {
                    Log.Comment("Test app has run for over {0} minutes; scheduling a restart before the AppWatcherThread limit.",
                        s_appRestartThreshold.TotalMinutes);
                    TestEnvironment.ShouldRestartApplication = true;
                }
            }
            catch (Exception e) when (e is InvalidOperationException || e is System.ComponentModel.Win32Exception)
            {
                // The process could not be queried; leave the restart decision to the infra.
            }
        }

        // A TAEF timeout kills the host and skips cleanup, and the next host's TestEnvironment.Initialize would re-attach
        // to the possibly hung app through UIA. Call first in ClassInitialize: kill any test app older than this host.
        internal static void KillTestAppLeftByPreviousTestHost()
        {
            DateTime hostStart = System.Diagnostics.Process.GetCurrentProcess().StartTime;
            foreach (var process in System.Diagnostics.Process.GetProcessesByName("MUXControlsTestApp"))
            {
                try
                {
                    if (!process.HasExited && process.StartTime < hostStart)
                    {
                        Log.Comment("Test app {0} predates this test host; killing it so the class starts on a fresh instance.",
                            process.Id);
                        process.Kill();
                        process.WaitForExit(10000);
                    }
                }
                catch (Exception e) when (e is InvalidOperationException || e is System.ComponentModel.Win32Exception)
                {
                    Log.Comment("Could not kill test app {0}: {1}", process.Id, e.Message);
                }
                finally
                {
                    process.Dispose();
                }
            }
        }

        // Call last inside the TestSetupHelper scope of a test that leaves a column header focused. On 25H2 the UIA
        // call after GoBack (back-button InvokeAndWait, or the next test's page lookup) has blocked until the TAEF
        // timeout when the page was torn down with a header holding keyboard focus; the app itself stayed responsive.
        // Parking focus on a page button and settling first keeps that teardown off the header path. Best-effort.
        internal static void MoveFocusOffTableBeforeTeardown()
        {
            UIObject outside = TryFindElement.ById(DummyButton);
            if (outside == null)
            {
                Log.Comment(DummyButton + " was not found; leaving focus where it is for teardown.");
                return;
            }

            outside.SetFocus();
            Wait.ForIdle();
            if (!outside.HasKeyboardFocus)
            {
                Log.Comment("Could not move keyboard focus to " + DummyButton + " before teardown.");
            }
        }

        // Declare inside the TestSetupHelper scope so it runs before teardown, including on early return or failure.
        internal static IDisposable MoveFocusOffTableOnExit() => new FocusOffTableOnExit();

        private sealed class FocusOffTableOnExit : IDisposable
        {
            public void Dispose()
            {
                try
                {
                    MoveFocusOffTableBeforeTeardown();
                }
                catch (Exception e)
                {
                    Log.Comment("Could not move focus off the table before teardown: " + e.Message);
                }
            }
        }

        // Selects a Pivot item by invoking the page's GoTo<Item>Button, found by AutomationId rather than by header
        // name. Returns false if the button could not be found; the caller decides whether that fails the test.
        internal static bool SelectPivotItem(string pivotItem)
        {
            var goTo = FindElement.ById<Button>(GoToButton(pivotItem));
            if (goTo == null)
            {
                return false;
            }

            goTo.InvokeAndWait();
            Wait.ForIdle();
            return true;
        }

        // The page button used to reach the Grouped pivot, kept so a keyboard test can start a Tab walk from it
        // without a second FindElement once the grouped table is realized.
        internal static Button GroupedAnchorButton { get; private set; }

        // Switches the page Pivot to the Grouped item and returns GroupedTableView.
        internal static UIObject SelectGroupedPivotAndGetTable()
        {
            var goToGrouped = FindElement.ById<Button>(GoToButton(GroupedPivotItem));
            if (goToGrouped == null)
            {
                Verify.Fail(GoToButton(GroupedPivotItem) + " was not found.");
                return null;
            }

            GroupedAnchorButton = goToGrouped;
            goToGrouped.InvokeAndWait();
            Wait.ForIdle();

            UIObject tableView = FindElement.ById(GroupedTable);
            if (tableView == null)
            {
                Verify.Fail(GroupedTable + " was not found after selecting the Grouped pivot item.");
                return null;
            }

            return tableView;
        }

        // Reads a page readout TextBox's Value. Resolved fresh each call but WITHOUT ElementCache.Clear(); UIA property
        // reads are live, so a plain Value read already sees the current text.
        internal static string ReadPageReadout(string automationId)
        {
            Edit report = FindElement.ById<Edit>(automationId);
            return report == null ? null : report.Value;
        }

        // The value of one "key=value" field of a ';'-separated page readout, or null if the field is absent.
        internal static string ReadReadoutField(string readoutId, string field)
        {
            string readout = ReadPageReadout(readoutId);
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

        // Pages refresh readouts on a dispatcher hop; a few idle waits let one settle. Returns the last value read, so
        // the caller asserts on what was actually observed.
        internal static string WaitForReadoutField(string readoutId, string field, string expected)
        {
            string value = ReadReadoutField(readoutId, field);
            for (int attempt = 0; attempt < 5 && value != expected; attempt++)
            {
                Wait.ForIdle();
                value = ReadReadoutField(readoutId, field);
            }

            Log.Comment("Readout {0} = '{1}' (expected '{2}').", field, value, expected);
            return value;
        }

        internal static bool WaitForStatusText(string expected, int timeoutMs = 10000)
        {
            var status = FindElement.ById<TextBlock>(StatusText);
            string value = null;
            DateTime deadline = DateTime.Now.AddMilliseconds(timeoutMs);
            while (status != null)
            {
                value = status.DocumentText;
                if (value == expected || DateTime.Now >= deadline)
                {
                    break;
                }
                Wait.ForMilliseconds(100);
            }

            Log.Comment("Status = '{0}' (expected '{1}').", value, expected);
            return value == expected;
        }

        internal static string ReadEditReport() => ReadPageReadout(EditColumnReport);

        internal static string ReadEditorProbe() => ReadPageReadout(EditorProbe);

        internal static string ReadEditEndReport() => ReadPageReadout(EditEndReport);

        internal static string ReadFirstItemName() => ReadPageReadout(FirstItemName);

        // Reads the page's scroll/frozen-geometry readout (format: TableViewPage.ReportScrollOffsets).
        // Header-sync and frozen-column tests assert on these page-published values, not UIA
        // BoundingRectangle: the header scrolls via PART_HeaderScroller's offset and frozen cells are
        // pinned with a composition-only Translation, neither of which the peer rectangles reflect.
        // *X is from TransformToVisual, which already includes Translation; *T is diagnostic only
        // (adding it to *X double-counts the pin).
        internal static string ReadScrollOffsets()
        {
            var readout = FindElement.ById<TextBlock>(ScrollOffsets);
            return readout == null ? "<no readout>" : readout.DocumentText;
        }

        // Returns double.NaN when the readout is missing or malformed.
        internal static double ReadScrollOffsetComponent(string offsets, string name)
        {
            if (string.IsNullOrEmpty(offsets)) { return double.NaN; }

            foreach (string part in offsets.Split(';'))
            {
                int split = part.IndexOf('=');
                if (split <= 0) { continue; }

                if (string.Equals(part.Substring(0, split).Trim(), name, StringComparison.Ordinal))
                {
                    return double.TryParse(
                        part.Substring(split + 1).Trim(),
                        System.Globalization.NumberStyles.Float,
                        System.Globalization.CultureInfo.InvariantCulture,
                        out double value) ? value : double.NaN;
                }
            }
            return double.NaN;
        }

        internal static string VerticalOffsetOf(string offsets)
        {
            foreach (string field in offsets.Split(';'))
            {
                if (field.StartsWith("V=", StringComparison.Ordinal))
                {
                    return field;
                }
            }
            return offsets;
        }
    }
}
