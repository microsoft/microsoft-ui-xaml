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
using VirtualizationFacts = Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewVirtualizationPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView large-data / virtualization interaction tests, on TableViewVirtualizationPage: one editable
    // 2000-item table in a fixed-height frame, so only a viewport's worth of rows is ever realized.
    //
    // All state is read from the page's in-process readout (format: TableViewVirtualizationPageFacts.Readout).
    // Which item a realized row shows, whether a row holds an editor, and which rows are on screen are not
    // observable from UIA without walking every row's cells, and the page computes them from the visual tree.
    [TestClass]
    public class TableViewVirtualizationInteractionTests
    {
        // One mouse-wheel notch. Matches the WHEEL_DELTA the input stack injects.
        private const int mouseWheelDelta = 120;

        // Far beyond the first viewport plus ItemsRepeater's cache on either side, so every row that was
        // realized around item 0 has been recycled for other items by the time it is reached.
        private const int scrolledAwayFirstVisibleId = 60;

        private const int maxWheelSteps = 80;

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
        [TestProperty("Description", "Verifies an open edit whose row is scrolled far out of view stays open on its own item, leaks no editor into recycled rows, and commits to that item when scrolled back.")]
        public void OpenEditSurvivesScrollingItsRowOutOfViewAndBack()
        {
            // Contract, from the code:
            // - The editor holds focus (EditorGetsFocusOnBeginEdit). ItemsRepeater pins the child that contains
            //   focus (Repeater/ViewManager.cpp:959-1011, UpdateFocusedElement) and a pinned element is moved to
            //   the pinned pool instead of being cleared (ViewManager.cpp:98-106 and 928-950), so
            //   TableView::OnRowElementClearing (TableView.cpp:2409-2438) is NOT reached for the edited row and
            //   the edit is not terminated by scrolling.
            // - Nothing else ends the edit on scroll: focus never leaves the editor, so the click-away commit
            //   (TableView_EditingInput.cpp:98-198) does not run, and CellEditEnding is raised only by a close
            //   (TableView_Editing.cpp:232-264).
            // - The editing binding is Explicit, so the typed value stays in the editor until commit
            //   (TableView_Editing.cpp:835-837).
            // What the user must see: the typed text is not lost, no recycled row - each now showing a
            // different item - ever shows an editor, and Enter writes to the item that was edited and no other.
            using (var setup = new TestSetupHelper(new[] { PageName, VirtualizationFacts.NavigateButton }))
            {
                UIObject table = GetTable(VirtualizationFacts.LargeTable);
                if (table == null) { return; }

                UIObject row = GetRow(table, 0);
                if (row == null) { Verify.Fail("The first realized TableViewRow peer was not found."); return; }

                string readout = ReadVirtualizationReadout();
                Log.Comment("Initial readout: '{0}'.", readout);
                Verify.AreEqual("False", Field(readout, VirtualizationFacts.EditingField), "Precondition: no edit is open.");
                Verify.AreEqual(0, VisibleFirstId(readout), "Precondition: the table starts scrolled to item 0.");

                OpenNameEditorAndReplaceText(row, "Typed");

                readout = ReadVirtualizationReadout();
                Log.Comment("After opening the edit and typing: '{0}'.", readout);
                Verify.AreEqual("True", Field(readout, VirtualizationFacts.EditingField), "Precondition: the double-click must open an edit.");
                Verify.AreEqual("0/Typed", Field(readout, VirtualizationFacts.EditorsField),
                    "Precondition: exactly one editor, on item 0, holding the typed text.");
                Verify.AreEqual(VirtualizationFacts.ItemName(0), Field(readout, VirtualizationFacts.FirstItemField),
                    "Precondition: typing alone must not reach the item - the editing binding is Explicit.");

                readout = WheelUntil(table, -5 * mouseWheelDelta, r => VisibleFirstId(r) >= scrolledAwayFirstVisibleId);
                Log.Comment("After scrolling away: '{0}'.", readout);

                int firstVisible = VisibleFirstId(readout);
                Verify.IsTrue(firstVisible >= scrolledAwayFirstVisibleId,
                    string.Format("Precondition: the body must scroll item 0 far out of view (first visible item {0}, needed >= {1}); otherwise nothing below was virtualized.",
                        firstVisible, scrolledAwayFirstVisibleId));

                Verify.AreEqual("True", Field(readout, VirtualizationFacts.EditingField),
                    "Scrolling the edited row out of view must not end the edit: focus is still in the editor.");
                Verify.AreEqual(VirtualizationFacts.NoneValue, Field(readout, VirtualizationFacts.EditEndingField),
                    "No CellEditEnding may be raised by scrolling alone.");
                Verify.AreEqual("0/Typed", Field(readout, VirtualizationFacts.EditorsField),
                    "The only editor must still belong to item 0; a recycled row showing another item must never carry it.");
                Verify.AreEqual(VirtualizationFacts.NoneValue, Field(readout, VirtualizationFacts.ModifiedField),
                    "Scrolling must write nothing to any item.");

                readout = WheelUntil(table, 5 * mouseWheelDelta, r => VisibleFirstId(r) == 0 && Field(r, VirtualizationFacts.OffsetField) == "0");
                Log.Comment("After scrolling back: '{0}'.", readout);
                Verify.AreEqual(0, VisibleFirstId(readout), "Precondition: the body must scroll back to item 0.");

                Verify.AreEqual("True", Field(readout, VirtualizationFacts.EditingField), "The edit must still be open after scrolling back.");
                Verify.AreEqual("0/Typed", Field(readout, VirtualizationFacts.EditorsField),
                    "Item 0's editor must come back with the typed text, and no other realized row may show an editor.");

                Log.Comment("Press Enter to commit.");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();

                readout = ReadVirtualizationReadout();
                Log.Comment("After Enter: '{0}'.", readout);
                Verify.AreEqual("False", Field(readout, VirtualizationFacts.EditingField), "Enter must close the edit.");
                Verify.AreEqual("Commit/0", Field(readout, VirtualizationFacts.EditEndingField),
                    "Exactly one CellEditEnding, a Commit for item 0.");
                Verify.AreEqual("Typed", Field(readout, VirtualizationFacts.FirstItemField),
                    "The typed value must reach item 0, the item the edit was opened on.");
                Verify.AreEqual("0", Field(readout, VirtualizationFacts.ModifiedField),
                    "No item other than the edited one may change.");
                Verify.AreEqual(VirtualizationFacts.NoneValue, Field(readout, VirtualizationFacts.EditorsField),
                    "No editor may remain in any realized row.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an open edit whose row is recycled out from under it is closed by the control: the pending value is committed to the edited item, CellEditEnding reports Commit, and no editor survives in any row.")]
        public void OpenEditIsClosedWhenItsRowIsRecycledByRemovingItsItem()
        {
            // Contract, from the code:
            // - Removing the item clears its row with isClearedDueToCollectionChange, which bypasses the focus
            //   pin (Repeater/ViewManager.cpp:597 and 931-932, ClearElementToPinnedPool), so TableView's ElementClearing
            //   handler (registered TableView.cpp:940-945) runs for the edited row.
            // - OnRowElementClearing (TableView.cpp:2409-2438) calls TerminateEditWithoutVisualRestore(true)
            //   (TableView_Editing.cpp:512-532), which is TerminateEditForReset(force = true)
            //   (TableView_Editing.cpp:822-871): a forced close that cannot be vetoed, transfers the editor's
            //   value to the EDITED item first, and reports Commit unless nothing was written or the item has
            //   blocking validation errors (this item has none). It is NOT a cancel.
            // - CellEditEnding is raised once, with that action, and because the close happens inside a layout
            //   pass it is posted to the dispatcher rather than raised inline (TableView_Editing.cpp:232-264,
            //   545-574). FinishEditTeardown does not write a second time (TableView_Editing.cpp:640-646).
            // The removal is driven from a focus-neutral page button, so nothing but the recycle can close the
            // edit: a focus move would close it through the click-away commit instead and prove nothing here.
            using (var setup = new TestSetupHelper(new[] { PageName, VirtualizationFacts.NavigateButton }))
            {
                UIObject table = GetTable(VirtualizationFacts.LargeTable);
                if (table == null) { return; }

                UIObject row = GetRow(table, 0);
                if (row == null) { Verify.Fail("The first realized TableViewRow peer was not found."); return; }

                var removeFirstItem = FindElement.ById<Button>(VirtualizationFacts.RemoveFirstItemButton);
                if (removeFirstItem == null) { Verify.Fail(VirtualizationFacts.RemoveFirstItemButton + " was not found."); return; }

                OpenNameEditorAndReplaceText(row, "Typed");

                string readout = ReadVirtualizationReadout();
                Log.Comment("After opening the edit and typing: '{0}'.", readout);
                Verify.AreEqual("True", Field(readout, VirtualizationFacts.EditingField), "Precondition: the double-click must open an edit.");
                Verify.AreEqual("0/Typed", Field(readout, VirtualizationFacts.EditorsField),
                    "Precondition: exactly one editor, on item 0, holding the typed text.");
                Verify.AreEqual(VirtualizationFacts.ItemName(0), Field(readout, VirtualizationFacts.FirstItemField),
                    "Precondition: typing alone must not reach the item - the editing binding is Explicit.");

                Log.Comment("Remove item 0 from the source through the focus-neutral button.");
                removeFirstItem.InvokeAndWait();
                Wait.ForIdle();

                readout = ReadVirtualizationReadout();
                Log.Comment("After removing item 0: '{0}'.", readout);

                Verify.AreEqual(1, VisibleFirstId(readout),
                    "Precondition: item 0 must be gone from the rows, with item 1 now first; otherwise its row was never recycled.");
                Verify.AreEqual("False", Field(readout, VirtualizationFacts.EditingField),
                    "Recycling the edited row must close the edit (TableView.cpp:2409-2438).");
                Verify.AreEqual(VirtualizationFacts.NoneValue, Field(readout, VirtualizationFacts.EditorsField),
                    "No realized row may keep the editor; a recycled row carrying it would edit a different item.");
                Verify.AreEqual("Commit/0", Field(readout, VirtualizationFacts.EditEndingField),
                    "The forced close must raise exactly one CellEditEnding, a Commit for the edited item (TableView_Editing.cpp:822-871).");
                Verify.AreEqual("Typed", Field(readout, VirtualizationFacts.FirstItemField),
                    "A forced close commits the pending value to the item the edit was opened on.");
                Verify.AreEqual("0", Field(readout, VirtualizationFacts.ModifiedField),
                    "The value must not land on any other item - in particular not on the item now shown by the recycled row.");
            }
        }

        #region Helpers

        private static readonly int NameCellRelativeX = VirtualizationFacts.ColumnCentreX(VirtualizationFacts.NameColumn);

        private static string ReadVirtualizationReadout() => ReadPageReadout(VirtualizationFacts.Readout);

        // The value of one "key=value" field of the readout, or a diagnostic string the assertion will print.
        private static string Field(string readout, string key)
        {
            if (string.IsNullOrEmpty(readout))
            {
                return "<no readout>";
            }

            foreach (string part in readout.Split(';'))
            {
                int split = part.IndexOf('=');
                if (split > 0 && string.Equals(part.Substring(0, split), key, StringComparison.Ordinal))
                {
                    return part.Substring(split + 1);
                }
            }

            return "<no field '" + key + "' in '" + readout + "'>";
        }

        // The Id of the first item whose row intersects the body viewport, or -1.
        private static int VisibleFirstId(string readout)
        {
            string visible = Field(readout, VirtualizationFacts.VisibleField);
            int dash = visible.IndexOf('-');
            int id;
            return dash > 0 && int.TryParse(visible.Substring(0, dash), out id) ? id : -1;
        }

        // Rotates the wheel over the table body, a step at a time, until the readout satisfies done or the step
        // budget runs out. Aimed at the table's centre, which is below row 0, so the pointer never sits on the
        // editor. Returns the last readout.
        private static string WheelUntil(UIObject table, int deltaPerStep, Func<string, bool> done)
        {
            Point centre = CentreOf(table);
            string readout = ReadVirtualizationReadout();
            for (int step = 0; step < maxWheelSteps && !done(readout); step++)
            {
                Log.Comment("Rotate wheel by {0} at ({1}, {2}).", deltaPerStep, centre.X, centre.Y);
                PointerInput.Move(centre);
                MouseWheelInput.RotateWheel(deltaPerStep);
                Wait.ForIdle();
                readout = ReadVirtualizationReadout();
            }

            return readout;
        }

        // Select-all first: the editor opens pre-populated, so typing without selecting would concatenate.
        private static void OpenNameEditorAndReplaceText(UIObject row, string newText)
        {
            Log.Comment("Double-click the Name cell, select all, and type '{0}'.", newText);
            DoubleClickRowAtColumnOffset(row, NameCellRelativeX);

            KeyboardHelper.PressKey(Key.a, ModifierKey.Control);
            TextInput.SendText(newText);
            Wait.ForIdle();
        }

        #endregion
    }
}
