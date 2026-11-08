// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
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
    // Injected touch must reach the same selection, group toggle and scroll-sync paths the mouse tests cover.
    [TestClass]
    [TestProperty("Ignore", "True")] // Disabled pending TableView page leak fix: #12491.
    public class TableViewTouchInteractionTests
    {
        private const string NavigateToTouchButton = "navigateToTouch";
        private const string TouchBasicTable = "TouchBasicTableView";
        private const string TouchGroupedTable = "TouchGroupedTableView";
        private const string TouchScrollingTable = "TouchScrollingTableView";
        private const string TouchScrollOffsets = "TouchScrollOffsetTextBlock";
        private const string TouchScrollFirstHeader = "TouchScrollName";

        private const int TappedRowIndex = 2;
        private const int PanDistance = 150;

        // Same sync tolerance as TableView_Layout_InteractionTests.HorizontalScrollKeepsHeaderAligned.
        private const double HeaderSyncTolerance = 1.0;
        private const double HeaderStickyTolerance = 4.0;

        // Screen pixels; grid lines are 1px and both bands snap the same way.
        private const double MidPanSkewTolerance = 1.0;

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

        [TestMethod]
        [TestProperty("Description", "Verifies a touch tap on a row selects that row and moves keyboard focus into it.")]
        public void TouchTapSelectsAndFocusesRow()
        {
            using (var setup = new TestSetupHelper(new[] { PageName, NavigateToTouchButton }))
            {
                UIObject tableView = GetTable(TouchBasicTable);
                if (tableView == null)
                {
                    return;
                }

                UIObject row = GetRow(tableView, TappedRowIndex);
                if (row == null)
                {
                    Verify.Fail(string.Format("Realized row {0} was not found on {1}.", TappedRowIndex, TouchBasicTable));
                    return;
                }

                VerifyNoRowSelected("navigating to the page", TouchBasicTable);
                Verify.AreEqual(-1, IndexOfFocusedBodyRow(TouchBasicTable), "Precondition: no row of the table may hold focus before the tap.");

                InputHelper.Tap(row);
                Wait.ForIdle();

                int selectedIndex = IndexOfSelectedRow(TouchBasicTable);
                int focusedIndex = IndexOfFocusedBodyRow(TouchBasicTable);
                Log.Comment("After the tap: selected row={0}, focused row={1}.", selectedIndex, focusedIndex);

                Verify.AreEqual(TappedRowIndex, selectedIndex, "A touch tap must select the tapped row.");
                Verify.AreEqual(TappedRowIndex, focusedIndex, "A touch tap must move keyboard focus to the tapped row (or one of its cells).");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a touch tap on a group header collapses its group and a second tap expands it again.")]
        public void TouchTapOnGroupHeaderTogglesExpansion()
        {
            using (var setup = new TestSetupHelper(new[] { PageName, NavigateToTouchButton }))
            {
                UIObject tableView = GetTable(TouchGroupedTable);
                if (tableView == null)
                {
                    return;
                }

                UIObject rowsHost = GetRowsHost(tableView);
                int baselineRows = CountRows(rowsHost);
                Verify.IsGreaterThan(baselineRows, 0, "The grouped table should realize rows before any collapse.");

                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                string headerName = groupHeader.Name;
                Verify.AreEqual(ExpandCollapseState.Expanded, new ExpandCollapseImplementation(groupHeader).ExpandCollapseState, "Baseline: the group header should report Expanded.");

                InputHelper.Tap(groupHeader);
                groupHeader = WaitForFirstGroupHeaderState(tableView, headerName, ExpandCollapseState.Collapsed);
                if (groupHeader == null) { return; }

                int collapsedRows = CountRows(GetRowsHost(tableView));
                Verify.IsLessThan(collapsedRows, baselineRows, "Tapping the group header must collapse its group and drop the realized row count.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, new ExpandCollapseImplementation(groupHeader).ExpandCollapseState, "After the tap the header must report Collapsed.");

                InputHelper.Tap(groupHeader);
                groupHeader = WaitForFirstGroupHeaderState(tableView, headerName, ExpandCollapseState.Expanded);
                if (groupHeader == null) { return; }

                Verify.AreEqual(baselineRows, CountRows(GetRowsHost(tableView)), "A second tap must restore the original realized row count.");
                Verify.AreEqual(ExpandCollapseState.Expanded, new ExpandCollapseImplementation(groupHeader).ExpandCollapseState, "After the second tap the header must report Expanded again.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a touch pan scrolls the body vertically with the header pinned, and horizontally with the header offset tracking the body.")]
        public void TouchPanScrollsBodyAndKeepsHeaderAligned()
        {
            using (var setup = new TestSetupHelper(new[] { PageName, NavigateToTouchButton }))
            {
                UIObject tableView = GetTable(TouchScrollingTable);
                if (tableView == null)
                {
                    return;
                }

                UIObject header = FindColumnHeader(tableView, TouchScrollFirstHeader);
                if (header == null)
                {
                    Verify.Fail("The '" + TouchScrollFirstHeader + "' column header peer was not found on the header host.");
                    return;
                }

                string offsetsBefore = ReadSettledTouchScrollOffsets();
                Verify.AreEqual(0.0, ReadScrollOffsetComponent(offsetsBefore, "H"), "Precondition: the body must start at H=0. Raw: " + offsetsBefore);
                Verify.AreEqual(0.0, ReadScrollOffsetComponent(offsetsBefore, "V"), "Precondition: the body must start at V=0. Raw: " + offsetsBefore);
                Verify.AreEqual(0.0, ReadScrollOffsetComponent(offsetsBefore, "HeaderH"), "Precondition: the header must start at HeaderH=0. Raw: " + offsetsBefore);

                double headerTopBefore = header.BoundingRectangle.Top;
                var bounds = tableView.BoundingRectangle;

                Log.Comment("Pan the body up (finger moves North) to scroll vertically.");
                var verticalStart = new Point(bounds.Left + (bounds.Width / 2), bounds.Top + (bounds.Height * 3 / 4));
                InputHelper.Pan(tableView, verticalStart, PanDistance, Direction.North);

                string offsetsAfterVertical = ReadSettledTouchScrollOffsets();
                double headerTopAfter = FindColumnHeader(tableView, TouchScrollFirstHeader).BoundingRectangle.Top;
                Log.Comment("Offsets '{0}' -> '{1}'; header Top {2} -> {3}.", offsetsBefore, offsetsAfterVertical, headerTopBefore, headerTopAfter);

                Verify.IsGreaterThan(
                    ReadScrollOffsetComponent(offsetsAfterVertical, "V"),
                    0.0,
                    "A vertical touch pan must scroll PART_BodyScroller vertically.");
                Verify.IsLessThanOrEqual(
                    Math.Abs(headerTopAfter - headerTopBefore),
                    HeaderStickyTolerance,
                    "The header row must stay pinned at the top during a vertical touch pan.");

                double bodyHBefore = ReadScrollOffsetComponent(offsetsAfterVertical, "H");

                Log.Comment("Pan the body left (finger moves West) to scroll horizontally.");
                var horizontalStart = new Point(bounds.Right - 40, bounds.Top + (bounds.Height / 2));
                InputHelper.Pan(tableView, horizontalStart, PanDistance, Direction.West);

                string offsetsAfterHorizontal = ReadSettledTouchScrollOffsets();
                double bodyHAfter = ReadScrollOffsetComponent(offsetsAfterHorizontal, "H");
                double headerHAfter = ReadScrollOffsetComponent(offsetsAfterHorizontal, "HeaderH");
                Log.Comment("Offsets '{0}' -> '{1}'.", offsetsAfterVertical, offsetsAfterHorizontal);

                Verify.IsGreaterThan(bodyHAfter, bodyHBefore, "A horizontal touch pan must scroll PART_BodyScroller horizontally.");
                Verify.IsLessThan(
                    Math.Abs(headerHAfter - bodyHAfter),
                    HeaderSyncTolerance,
                    string.Format("The header offset ({0}) must track the body offset ({1}) within the sync tolerance.", headerHAfter, bodyHAfter));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies header cells stay over their body cells on every frame of a horizontal touch pan, not only once it settles (ADO 64513223).")]
        public void TouchPanKeepsHeaderOverBodyDuringPan()
        {
            using (var setup = new TestSetupHelper(new[] { PageName, NavigateToTouchButton }))
            {
                UIObject tableView = GetTable(TouchScrollingTable);
                if (tableView == null)
                {
                    return;
                }

                UIObject header = FindColumnHeader(tableView, TouchScrollFirstHeader);
                UIObject rowsHost = GetRowsHost(TouchScrollingTable);
                if (header == null || rowsHost == null || rowsHost.Children.Count == 0)
                {
                    Verify.Fail("The first column header or the first row of " + TouchScrollingTable + " was not found.");
                    return;
                }

                // One pixel row inside the header band below its text, one inside the first row above its text;
                // both cross only the vertical grid lines. The scroll bar is left out.
                var bounds = tableView.BoundingRectangle;
                int headerY = header.BoundingRectangle.Bottom - 3;
                int bodyY = rowsHost.Children[0].BoundingRectangle.Top + 2;
                int left = bounds.Left + 2;
                int width = bounds.Width - 24;

                var (headerLinesBefore, bodyLinesBefore) = CaptureGridLines(left, width, headerY, bodyY);
                Log.Comment("Grid lines at rest: header [{0}], body [{1}].", string.Join(",", headerLinesBefore), string.Join(",", bodyLinesBefore));
                Verify.IsGreaterThan(headerLinesBefore.Count, 0, "Precondition: the header band must show vertical grid lines.");
                Verify.IsLessThanOrEqual(MaxSkew(headerLinesBefore, bodyLinesBefore), MidPanSkewTolerance, "Precondition: header and body grid lines must line up at rest.");

                var pan = System.Threading.Tasks.Task.Run(() => InputHelper.Pan(
                    tableView, new Point(bounds.Right - 40, bounds.Top + (bounds.Height / 2)), PanDistance, Direction.West, waitForIdle: false));

                // Sampling continues through inertia, which runs on the compositor alone, until the body stops.
                // Mid-pan, columns the body has not realized yet draw no line; those samples cannot be paired.
                double worstSkew = 0.0;
                int pairedSamples = 0;
                string worstSample = "<none>";
                var previousBodyLines = bodyLinesBefore;
                var lastMove = System.Diagnostics.Stopwatch.StartNew();
                while (!pan.IsCompleted || lastMove.ElapsedMilliseconds < 500)
                {
                    var (headerLines, bodyLines) = CaptureGridLines(left, width, headerY, bodyY);
                    if (!bodyLines.SequenceEqual(previousBodyLines))
                    {
                        previousBodyLines = bodyLines;
                        lastMove.Restart();
                        if (headerLines.Count > 0 && headerLines.Count == bodyLines.Count)
                        {
                            pairedSamples++;
                            double skew = MaxSkew(headerLines, bodyLines);
                            if (skew > worstSkew)
                            {
                                worstSkew = skew;
                                worstSample = string.Format("header [{0}], body [{1}]", string.Join(",", headerLines), string.Join(",", bodyLines));
                            }
                        }
                    }
                }
                pan.Wait();

                string offsets = ReadSettledTouchScrollOffsets();
                var (headerLinesAfter, bodyLinesAfter) = CaptureGridLines(left, width, headerY, bodyY);
                Log.Comment("Offsets after the pan: '{0}'. {1} paired moving samples; worst skew {2} px: {3}.", offsets, pairedSamples, worstSkew, worstSample);
                Log.Comment("Grid lines after the pan: header [{0}], body [{1}].", string.Join(",", headerLinesAfter), string.Join(",", bodyLinesAfter));

                Verify.IsGreaterThan(ReadScrollOffsetComponent(offsets, "H"), 0.0, "Precondition: the pan must scroll the body horizontally.");
                Verify.IsGreaterThanOrEqual(pairedSamples, 3, "Precondition: the screen must be sampled while the body is moving.");
                Verify.IsLessThanOrEqual(worstSkew, MidPanSkewTolerance,
                    "Mid-pan, every header grid line must be drawn over its body grid line in the same frame.");
                Verify.IsLessThanOrEqual(MaxSkew(headerLinesAfter, bodyLinesAfter), MidPanSkewTolerance,
                    "Once the pan settles, the header grid lines must sit over the body grid lines.");
                Verify.IsLessThan(Math.Abs(ReadScrollOffsetComponent(offsets, "HeaderH") - ReadScrollOffsetComponent(offsets, "H")), HeaderSyncTolerance,
                    "Once the pan settles, XAML's view of the header offset (hit-testing, UIA) must match the body.");
            }
        }

        // Lines are paired by index, so a header a whole column behind cannot pair with the wrong line.
        private static double MaxSkew(List<double> headerLines, List<double> bodyLines)
            => headerLines.Count == 0 || headerLines.Count != bodyLines.Count
                ? double.PositiveInfinity
                : headerLines.Zip(bodyLines, (h, b) => Math.Abs(h - b)).Max();

        // Both rows come from one BitBlt, so they belong to the same composed frame.
        private static (List<double> header, List<double> body) CaptureGridLines(int left, int width, int headerY, int bodyY)
        {
            IntPtr screen = NativeMethods.GetDC(IntPtr.Zero);
            IntPtr memory = NativeMethods.CreateCompatibleDC(screen);
            int height = bodyY - headerY + 1;
            IntPtr bitmap = NativeMethods.CreateCompatibleBitmap(screen, width, height);
            IntPtr previous = NativeMethods.SelectObject(memory, bitmap);
            try
            {
                NativeMethods.BitBlt(memory, 0, 0, width, height, screen, left, headerY, 0x00CC0020 /* SRCCOPY */ | 0x40000000 /* CAPTUREBLT */);
                return (FindGridLines(memory, width, 0), FindGridLines(memory, width, height - 1));
            }
            finally
            {
                NativeMethods.SelectObject(memory, previous);
                NativeMethods.DeleteObject(bitmap);
                NativeMethods.DeleteDC(memory);
                NativeMethods.ReleaseDC(IntPtr.Zero, screen);
            }
        }

        // Thin runs of pixels that stand out from the row's dominant colour; wider runs are not grid lines.
        private static List<double> FindGridLines(IntPtr dc, int width, int y)
        {
            var luminance = new int[width];
            for (int x = 0; x < width; x++)
            {
                uint p = NativeMethods.GetPixel(dc, x, y);
                luminance[x] = (int)(((p & 0xFF) * 299 + ((p >> 8) & 0xFF) * 587 + ((p >> 16) & 0xFF) * 114) / 1000);
            }

            int background = luminance.OrderBy(v => v).ElementAt(width / 2);
            var lines = new List<double>();
            for (int x = 0; x < width; x++)
            {
                if (Math.Abs(luminance[x] - background) < 8) { continue; }
                int start = x;
                while (x + 1 < width && Math.Abs(luminance[x + 1] - background) >= 8) { x++; }
                // A line clipped by either end of the strip could show in one band and not the other.
                if (x - start <= 2 && start > 1 && x < width - 2) { lines.Add((start + x) / 2.0); }
            }
            return lines;
        }

        private static class NativeMethods
        {
            [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr hWnd);
            [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr hWnd, IntPtr hdc);
            [DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleDC(IntPtr hdc);
            [DllImport("gdi32.dll")] public static extern bool DeleteDC(IntPtr hdc);
            [DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleBitmap(IntPtr hdc, int width, int height);
            [DllImport("gdi32.dll")] public static extern IntPtr SelectObject(IntPtr hdc, IntPtr obj);
            [DllImport("gdi32.dll")] public static extern bool DeleteObject(IntPtr obj);
            [DllImport("gdi32.dll")] public static extern bool BitBlt(IntPtr dest, int x, int y, int width, int height, IntPtr source, int sourceX, int sourceY, uint rop);
            [DllImport("gdi32.dll")] public static extern uint GetPixel(IntPtr hdc, int x, int y);
        }
        // Index among the rows host's children of the row that holds focus itself or through a cell, or -1.
        private static int IndexOfFocusedBodyRow(string tableAutomationId)
        {
            UIObject rowsHost = GetRowsHost(tableAutomationId);
            if (rowsHost == null) { return -1; }

            int index = 0;
            foreach (UIObject row in rowsHost.Children)
            {
                if (row.HasKeyboardFocus || IndexOfFocusedCell(row) >= 0)
                {
                    return index;
                }
                index++;
            }
            return -1;
        }

        private static string ReadTouchScrollOffsets()
        {
            var readout = FindElement.ById<TextBlock>(TouchScrollOffsets);
            return readout == null ? "<no readout>" : readout.DocumentText;
        }

        // Inertia outlasts Wait.ForIdle and the page re-publishes the header 250 ms after a scroll, so poll until two reads further apart than that agree.
        private static string ReadSettledTouchScrollOffsets()
        {
            string previous = ReadTouchScrollOffsets();
            var timer = Stopwatch.StartNew();
            while (timer.ElapsedMilliseconds < 4000)
            {
                Wait.ForMilliseconds(400);
                string current = ReadTouchScrollOffsets();
                if (current == previous)
                {
                    return current;
                }
                previous = current;
            }
            return previous;
        }
    }
}
