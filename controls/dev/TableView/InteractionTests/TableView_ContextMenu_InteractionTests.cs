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
using Point = System.Drawing.Point;

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView context menu interaction tests: real right-click, Shift+F10 and Menu key input, end to end.
    //
    // Every test runs on the page's ContextMenu host (ContextMenuTable, laid out as TableViewTestPageFacts
    // describes), which gives each resolution level its own menu. Which menu opened, for which item and with focus
    // where, is read from ContextMenuReadout (written by the menus' Opening handlers); the open menu itself is
    // confirmed through UIA by its item's AutomationId. Resolution order, DataContext and recycling are covered by
    // the API tests; these cover input routing.
    [TestClass]
    public class TableViewContextMenuInteractionTests
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

        #region Pointer

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on a cell's text opens the column's CellContextFlyout for that row, focuses the cell and leaves selection alone.")]
        public void RightClickOnCellTextOpensCellMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                RightClickRow(table, 3, ContextMenuNameTextX);

                VerifyMenuOpened(CellMenu, ContextMenuName(3), CellFocus(3, ContextMenuNameColumn));
                Verify.AreEqual(NoneValue, ReadContextMenuField(SelectedField), "A right-click must not select the row.");
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on a cell's blank padding, away from its text, still opens the column's CellContextFlyout.")]
        public void RightClickOnCellPaddingOpensCellMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                RightClickRow(table, 2, ContextMenuNamePaddingX);

                VerifyMenuOpened(CellMenu, ContextMenuName(2), CellFocus(2, ContextMenuNameColumn));
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a ContextFlyout set inside a cell template wins over the row's menu.")]
        public void RightClickOnTemplateCellOpensTemplateMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                RightClickRow(table, 1, ContextMenuColumnCentreX(ContextMenuCityColumn));

                // Focus is not asserted: input on template content raises ContextRequested at that content, and the
                // framework shows the content's own ContextFlyout during the route, before the TableView sees the
                // request, so the TableView's focus move does not run.
                VerifyMenuOpened(TemplateMenu, ContextMenuName(1), expectedFocus: null);
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on a cell with no menu of its own opens the row's ContextFlyout, with focus on the clicked cell.")]
        public void RightClickOnPlainCellOpensRowMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                RightClickRow(table, 4, ContextMenuColumnCentreX(ContextMenuAgeColumn));

                VerifyMenuOpened(RowMenu, ContextMenuName(4), CellFocus(4, ContextMenuAgeColumn));
                Verify.AreEqual("TableViewRow", ReadContextMenuField(TargetField), "The row menu must be shown at the row.");
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on a row's blank space past the last column opens the row's ContextFlyout.")]
        public void RightClickOnBlankRowSpaceOpensRowMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                RightClickRow(table, 0, ContextMenuBlankRowX);

                VerifyMenuOpened(RowMenu, ContextMenuName(0), expectedFocus: null);
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on a column header opens the column's HeaderContextFlyout, with no row item.")]
        public void RightClickOnHeaderOpensHeaderMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                RightClickHeader(ContextMenuColumns[ContextMenuNameColumn]);

                VerifyMenuOpened(HeaderMenu, NoneValue, expectedFocus: null);
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on a header whose column has no HeaderContextFlyout falls back to the table's ContextFlyout.")]
        public void RightClickOnHeaderWithoutMenuOpensTableMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                RightClickHeader(ContextMenuColumns[ContextMenuAgeColumn]);

                VerifyMenuOpened(TableMenu, NoneValue, expectedFocus: null);
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on the empty area below the last row opens the table's ContextFlyout.")]
        public void RightClickBelowRowsOpensTableMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                var bounds = table.BoundingRectangle;
                RightClickPoint(new Point(bounds.Left + 80, bounds.Top + bounds.Height - 24));

                VerifyMenuOpened(TableMenu, NoneValue, expectedFocus: null);
                CloseMenu();
            }
        }

        #endregion

        #region Keyboard

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+F10 on a focused cell opens the column's CellContextFlyout for that row.")]
        public void ShiftF10OnCellOpensCellMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectContextMenuHostAndGetTable() == null) { return; }
                if (!DrillIntoRow(ContextMenuTable, 4)) { return; }

                KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);

                VerifyMenuOpened(CellMenu, ContextMenuName(4), CellFocus(4, ContextMenuNameColumn));
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the Menu (Apps) key on a focused cell opens the column's CellContextFlyout for that row.")]
        public void MenuKeyOnCellOpensCellMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectContextMenuHostAndGetTable() == null) { return; }
                if (!DrillIntoRow(ContextMenuTable, 4)) { return; }

                TextInput.SendText("{APPS}");
                Wait.ForIdle();

                VerifyMenuOpened(CellMenu, ContextMenuName(4), CellFocus(4, ContextMenuNameColumn));
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+F10 on a focused row (not a cell) opens the row's menu, not a column's cell menu.")]
        public void ShiftF10OnRowOpensRowMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }

                UIObject row = GetRow(table, 3);
                if (row == null) { Verify.Fail("Row 3 was not found."); return; }
                row.SetFocus();
                Wait.ForIdle();
                if (!row.HasKeyboardFocus) { Verify.Fail("Precondition: row 3 did not take focus."); return; }

                KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);

                VerifyMenuOpened(RowMenu, ContextMenuName(3), RowFocus(3));
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+F10 on a focused cell with no menu of its own opens the row's menu.")]
        public void ShiftF10OnPlainCellOpensRowMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectContextMenuHostAndGetTable() == null) { return; }
                if (!DrillIntoRow(ContextMenuTable, 2)) { return; }

                KeyboardHelper.PressKey(Key.Right, ModifierKey.None, numPresses: (uint)ContextMenuAgeColumn);
                Wait.ForIdle();
                VerifyFocusedCell(ContextMenuTable, 2, ContextMenuAgeColumn, "Precondition: focus must reach row 2's Age cell.");

                KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);

                VerifyMenuOpened(RowMenu, ContextMenuName(2), CellFocus(2, ContextMenuAgeColumn));
                Verify.AreEqual("TableViewRow", ReadContextMenuField(TargetField), "The row menu must stay targeted at the row.");
                UIObject row = GetRow(GetTable(ContextMenuTable), 2);
                VerifyMenuPlacedAtCell(row, row.Children[ContextMenuAgeColumn], RowMenu);
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the Menu (Apps) key on a focused cell with no menu of its own opens the row's menu at that cell, still targeted at the row.")]
        public void MenuKeyOnPlainCellOpensRowMenuAtCell()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectContextMenuHostAndGetTable() == null) { return; }
                if (!DrillIntoRow(ContextMenuTable, 3)) { return; }

                KeyboardHelper.PressKey(Key.Right, ModifierKey.None, numPresses: (uint)ContextMenuAgeColumn);
                Wait.ForIdle();
                VerifyFocusedCell(ContextMenuTable, 3, ContextMenuAgeColumn, "Precondition: focus must reach row 3's Age cell.");

                TextInput.SendText("{APPS}");
                Wait.ForIdle();

                VerifyMenuOpened(RowMenu, ContextMenuName(3), CellFocus(3, ContextMenuAgeColumn));
                Verify.AreEqual("TableViewRow", ReadContextMenuField(TargetField), "The row menu must stay targeted at the row.");
                UIObject row = GetRow(GetTable(ContextMenuTable), 3);
                VerifyMenuPlacedAtCell(row, row.Children[ContextMenuAgeColumn], RowMenu);
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+F10 on a focused column header opens the column's HeaderContextFlyout.")]
        public void ShiftF10OnHeaderOpensHeaderMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectContextMenuHostAndGetTable() == null) { return; }

                string name = ContextMenuColumns[ContextMenuNameColumn];
                UIObject header = GetHeader(ContextMenuTable, name);
                if (header == null) { Verify.Fail("The Name header was not found."); return; }
                header.SetFocus();
                Wait.ForIdle();
                VerifyFocusedHeader(ContextMenuTable, name, "Precondition: the Name header must take focus.");

                KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);

                VerifyMenuOpened(HeaderMenu, NoneValue, HeaderFocus(name));
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Escape closes a keyboard-opened cell menu and returns focus to the cell it was opened on.")]
        public void EscapeReturnsFocusToMenuTarget()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectContextMenuHostAndGetTable() == null) { return; }
                if (!DrillIntoRow(ContextMenuTable, 4)) { return; }

                KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);
                VerifyMenuOpened(CellMenu, ContextMenuName(4), CellFocus(4, ContextMenuNameColumn));

                CloseMenu();

                ElementCache.Clear();
                VerifyFocusedCell(ContextMenuTable, 4, ContextMenuNameColumn, "Escape must return focus to row 4's Name cell.");
            }
        }

        #endregion

        #region Editing

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on the editor of a cell being edited shows no TableView menu, and that the editor's own menu neither ends the edit while open nor when Escape closes it.")]
        public void RightClickOnEditorOpensNoTableViewMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }
                if (!BeginEditOnNameCell(3)) { return; }

                RightClickRow(table, 3, ContextMenuNameTextX);
                VerifyNoMenuOpened();
                VerifyEditorMenuOpen();

                // Escape inside the editor's menu closes only the menu; focus returns to the editor.
                KeyboardHelper.PressKey(Key.Escape);
                VerifyStillEditing("Closing the editor's menu must not end the edit.");

                KeyboardHelper.PressKey(Key.Escape);
                Verify.AreEqual("False", WaitForContextMenuField(EditingField, "False"),
                    "Escape in the editor must still cancel the edit.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a command picked from the editor's own menu closes the menu without ending the edit, and Escape still cancels afterwards.")]
        public void EditorMenuCommandKeepsEdit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }
                if (!BeginEditOnNameCell(3)) { return; }

                RightClickRow(table, 3, ContextMenuNameTextX);
                VerifyEditorMenuOpen();

                FindElement.ByName<Button>(EditorMenuSelectAll).InvokeAndWait();
                VerifyStillEditing("Picking a command from the editor's menu must not end the edit.");

                KeyboardHelper.PressKey(Key.Escape);
                Verify.AreEqual("False", WaitForContextMenuField(EditingField, "False"),
                    "Escape in the editor must still cancel the edit.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that after the editor's own menu closes, moving to another cell still commits the typed value.")]
        public void ClickAwayAfterEditorMenuCommitsEdit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }
                if (!BeginEditOnNameCell(3)) { return; }

                RightClickRow(table, 3, ContextMenuNameTextX);
                VerifyEditorMenuOpen();
                KeyboardHelper.PressKey(Key.Escape);
                VerifyStillEditing("Closing the editor's menu must not end the edit.");

                const string typed = "EditedAfterMenu";
                KeyboardHelper.PressKey(Key.a, ModifierKey.Control);
                TextInput.SendText(typed);
                Wait.ForIdle();

                ClickRowAtColumnOffset(GetRow(table, 5), ContextMenuColumnCentreX(ContextMenuAgeColumn));
                Verify.AreEqual("False", WaitForContextMenuField(EditingField, "False"),
                    "Moving to another cell must end the edit.");

                // The row menu reports its row's item, so it shows whether the typed name was kept.
                ElementCache.Clear();
                RightClickRow(GetTable(ContextMenuTable), 3, ContextMenuColumnCentreX(ContextMenuAgeColumn));
                VerifyMenuOpened(RowMenu, typed, "cell:" + typed + "/" + ContextMenuAgeColumn);
                CloseMenu();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on another row's cell while a cell is editing opens no menu: neither the cell's, the row's nor the table's.")]
        public void RightClickOnOtherRowWhileEditingOpensNoMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }
                if (!BeginEditOnNameCell(1)) { return; }

                RightClickRow(table, 4, ContextMenuNameTextX);
                VerifyNoMenuOpened();

                RightClickRow(table, 4, ContextMenuColumnCentreX(ContextMenuAgeColumn));
                VerifyNoMenuOpened();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on a column header while a cell is editing opens no menu: neither the header's nor the table's.")]
        public void RightClickOnHeaderWhileEditingOpensNoMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (SelectContextMenuHostAndGetTable() == null) { return; }
                if (!BeginEditOnNameCell(2)) { return; }

                RightClickHeader(ContextMenuColumns[ContextMenuNameColumn]);
                VerifyNoMenuOpened();

                RightClickHeader(ContextMenuColumns[ContextMenuAgeColumn]);
                VerifyNoMenuOpened();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on the empty area below the rows while a cell is editing does not open the table's ContextFlyout.")]
        public void RightClickBelowRowsWhileEditingOpensNoMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject table = SelectContextMenuHostAndGetTable();
                if (table == null) { return; }
                if (!BeginEditOnNameCell(2)) { return; }

                var bounds = table.BoundingRectangle;
                RightClickPoint(new Point(bounds.Left + 80, bounds.Top + bounds.Height - 24));
                VerifyNoMenuOpened();
            }
        }

        #endregion

        #region Helpers

        // Focuses the row's Name cell and opens its editor with F2.
        private static bool BeginEditOnNameCell(int rowIndex)
        {
            if (!DrillIntoRow(ContextMenuTable, rowIndex)) { return false; }

            KeyboardHelper.PressKey(Key.F2);
            Wait.ForIdle();
            if (WaitForContextMenuField(EditingField, "True") != "True")
            {
                Verify.Fail(string.Format("Precondition: F2 must open the editor on row {0}'s Name cell.", rowIndex));
                return false;
            }
            return true;
        }

        // No TableView menu opened since the reset, and the cell is still being edited.
        private static void VerifyNoMenuOpened()
        {
            Wait.ForIdle();
            Log.Comment("Readout: {0}", ReadPageReadout(ContextMenuReadout));
            Verify.AreEqual("0", ReadContextMenuField(OpensField), "No TableView menu may open while a cell is editing.");
            VerifyStillEditing("The request must not end the edit.");
        }

        // The editing readout refreshes on a dispatcher hop after the edit ends, so one read can be stale. Watch it
        // across several idles and fail if it ever reports the edit ended.
        private static void VerifyStillEditing(string message)
        {
            for (int attempt = 0; attempt < 4; attempt++)
            {
                Wait.ForIdle();
                Verify.AreEqual("True", ReadContextMenuField(EditingField), message);
            }
        }

        // The text editor's own menu (TextCommandBarFlyout) is open.
        private const string EditorMenuSelectAll = "Select All";

        private static void VerifyEditorMenuOpen()
        {
            ElementCache.Clear();
            Verify.IsNotNull(FindElement.ByName(EditorMenuSelectAll), "The editor's own menu must open.");
        }

        // Switches to the ContextMenu host, resets its readout and selection, and returns ContextMenuTable.
        private static UIObject SelectContextMenuHostAndGetTable()
        {
            if (!SelectPivotItem(ContextMenuPivotItem))
            {
                Verify.Fail(GoToButton(ContextMenuPivotItem) + " was not found.");
                return null;
            }

            var reset = FindElement.ById<Button>(ResetContextMenuButton);
            if (reset == null)
            {
                Verify.Fail(ResetContextMenuButton + " was not found.");
                return null;
            }

            reset.InvokeAndWait();
            Wait.ForIdle();
            ElementCache.Clear();

            UIObject table = GetTable(ContextMenuTable);
            if (table == null)
            {
                return null;
            }

            if (ReadContextMenuField(OpensField) != "0")
            {
                Verify.Fail("Precondition: the readout must start with no menu opened.");
                return null;
            }

            return table;
        }

        private static void RightClickPoint(Point point)
        {
            Log.Comment("Right-click at absolute point ({0}, {1}).", point.X, point.Y);
            PointerInput.Move(point);
            PointerInput.Press(PointerButtons.Secondary);
            PointerInput.Release(PointerButtons.Secondary);
            Wait.ForIdle();
        }

        private static void RightClickRow(UIObject table, int rowIndex, int rowRelativeX)
        {
            UIObject row = GetRow(table, rowIndex);
            if (row == null) { Verify.Fail(string.Format("Row {0} was not found.", rowIndex)); return; }
            RightClickPoint(RowPointAtColumnOffset(row, rowRelativeX));
        }

        private static void RightClickHeader(string headerText)
        {
            UIObject header = GetHeader(ContextMenuTable, headerText);
            if (header == null) { Verify.Fail("The " + headerText + " header was not found."); return; }
            RightClickPoint(CentreOf(header));
        }

        // Exactly one menu opened since the reset, it is the expected one, it is open in the UIA tree, and it saw the
        // expected item. expectedFocus, when given, is the focused element at the moment the menu opened.
        private static void VerifyMenuOpened(string expectedMenu, string expectedItem, string expectedFocus)
        {
            string opens = WaitForContextMenuField(OpensField, "1");
            string menu = ReadContextMenuField(MenuField);
            string open = ReadContextMenuField(OpenField);
            string item = ReadContextMenuField(ItemField);
            string focus = ReadContextMenuField(FocusField);
            UIObject menuItem = TryFindElement.ById(MenuItemId(expectedMenu));
            Log.Comment("Readout: {0}", ReadPageReadout(ContextMenuReadout));

            Verify.AreEqual("1", opens, "Exactly one menu must open.");
            Verify.AreEqual(expectedMenu, menu, "Wrong menu opened.");
            Verify.AreEqual("True", open, "The menu must still be open.");
            Verify.IsNotNull(menuItem, "The open menu's item must be in the UIA tree: " + MenuItemId(expectedMenu) + ".");
            Verify.AreEqual(expectedItem, item, "The menu must see the requested row's item.");
            if (expectedFocus != null)
            {
                Verify.AreEqual(expectedFocus, focus, "Wrong element focused when the menu opened.");
            }
        }

        // A keyboard request on a cell opens the menu at that cell - starting within the cell's columns, directly
        // below or above it - even when the menu belongs to the row. Row-anchored placement would put the menu at the
        // row's left edge or centred on the row, so both are ruled out where they differ from the cell.
        private static void VerifyMenuPlacedAtCell(UIObject row, UIObject cell, string menu)
        {
            UIObject menuItem = TryFindElement.ById(MenuItemId(menu));
            if (row == null || cell == null || menuItem == null) { Verify.Fail("The row, the cell or the open menu was not found."); return; }

            var rowBounds = row.BoundingRectangle;
            var cellBounds = cell.BoundingRectangle;
            var itemBounds = menuItem.BoundingRectangle;
            Log.Comment("Row bounds={0}; cell bounds={1}; menu item bounds={2}.", rowBounds, cellBounds, itemBounds);

            const int Tolerance = 16;
            Verify.IsTrue(itemBounds.Left >= cellBounds.Left - Tolerance && itemBounds.Left <= cellBounds.Right,
                "The menu must start horizontally within the focused cell, not at the row.");
            Verify.IsTrue(itemBounds.Top >= cellBounds.Bottom - Tolerance || itemBounds.Bottom <= cellBounds.Top + Tolerance,
                "The menu must open next to the focused cell without covering it.");

            if (cellBounds.Left - rowBounds.Left > 2 * Tolerance)
            {
                Verify.IsTrue(Math.Abs(itemBounds.Left - rowBounds.Left) > Tolerance,
                    "The menu must not be placed at the row's left edge.");
            }

            int rowCentre = rowBounds.Left + rowBounds.Width / 2;
            int itemCentre = itemBounds.Left + itemBounds.Width / 2;
            if (rowCentre < cellBounds.Left - Tolerance || rowCentre > cellBounds.Right + Tolerance)
            {
                Verify.IsTrue(Math.Abs(itemCentre - rowCentre) > Tolerance,
                    "The menu must not be centred on the row.");
            }
        }

        private static void CloseMenu()
        {
            KeyboardHelper.PressKey(Key.Escape);
            Wait.ForIdle();
            Verify.AreEqual("False", WaitForContextMenuField(OpenField, "False"), "Escape must close the menu.");
        }

        // The value of one "key=value" field of ContextMenuReadout, or null if the field is absent.
        private static string ReadContextMenuField(string field)
        {
            string readout = ReadPageReadout(ContextMenuReadout);
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

        // A few idle waits let a menu's Opening/Closed land. Returns the last value read.
        private static string WaitForContextMenuField(string field, string expected)
        {
            string value = ReadContextMenuField(field);
            for (int attempt = 0; attempt < 5 && value != expected; attempt++)
            {
                Wait.ForIdle();
                value = ReadContextMenuField(field);
            }
            return value;
        }

        #endregion
    }
}
