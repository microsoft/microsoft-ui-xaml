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

        // Focuses the row at index and drills into its first cell with Right. Returns false (after failing) if
        // either step does not take; callers then stop, since every later assertion would be unmeasured.
        internal static bool DrillIntoRow(string tableAutomationId, int rowIndex)
        {
            UIObject rowsHost = GetRowsHost(tableAutomationId);
            if (rowsHost == null) { return false; }
            if (rowsHost.Children.Count <= rowIndex) { Verify.Fail(string.Format("Need at least {0} realized rows.", rowIndex + 1)); return false; }

            UIObject row = rowsHost.Children[rowIndex];
            row.SetFocus();
            Wait.ForIdle();
            if (!row.HasKeyboardFocus) { Verify.Fail(string.Format("Precondition: row {0} did not take focus.", rowIndex)); return false; }

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
    }
}
