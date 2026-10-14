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

        #region Helpers

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
