// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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
    // TableView editing-gesture interaction tests.
    // Scope: interaction test plan (docs\design-notes\TabularControls\TableView-interaction-test-plan.md) §5
    // "Editing gestures". Editing mixes routes by nature - the edit is opened with a pointer or with F2 and
    // then driven from the keyboard - so it gets its own file rather than being split across the pointer and
    // keyboard files.
    //
    // CRASH CONSTRAINT (product finding #13): an out-of-process client must never ask a TableViewRow peer for
    // its children; TableViewRowAutomationPeer::GetChildrenCore manufactures fresh cell peers per call and
    // fail-fasts the app. So nothing here resolves a cell or an editor peer. Cells are reached by clicking the
    // row at a row-relative x, and the editor is observed through the page's in-process probe
    // (EditorProbeTextBlock), which walks the visual tree in process and therefore creates no peers at all -
    // the same technique the row and group-header state logs already use.
    [TestClass]
    public class TableViewEditingInteractionTests
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

        #region 5. Editing gestures

        [TestMethod]
        [TestProperty("Description", "Verifies a double-click inside an editable column begins an edit on the column under the pointer.")]
        public void PointerDoubleClickBeginsEditWhenEditable()
        {
            // Interaction plan §5 PointerDoubleClickBeginsEditWhenEditable.
            // Spec: functional-spec:61 - "double-click / double-tap and F2 begin an edit".
            //       dev-spec:284 - the gesture is driven from PointerPressed with click-count tracking, not from a
            //       DoubleTapped handler, so that a row marking the press handled for selection cannot kill it.
            // Observed through the page's BeginningEdit report (TableViewBeginningEditEventArgs.Column is public IDL).
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Verify.AreEqual(string.Empty, ReadEditReport(), "Precondition: no edit should have been reported before the test acts.");

                Log.Comment("Double-click inside the editable Age column.");
                DoubleClickRowAtColumnOffset(row, AgeCellRelativeX);

                Verify.AreEqual("Age;", ReadEditReport(),
                    "A double-click on an editable cell must begin an edit on the column under the pointer (functional-spec:61, dev-spec:284).");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a double-click inside a column authored IsReadOnly begins no edit, while the same gesture on an editable column still does.")]
        public void PointerDoubleClickDoesNothingWhenReadOnly()
        {
            // Interaction plan §5 PointerDoubleClickDoesNothingWhenReadOnly.
            // Spec: the per-column gate is checked before BeginningEdit is raised, so a correct implementation is
            //   silent rather than cancelling - hence the assertion is that the event does not fire at all.
            // The second leg is a POSITIVE CONTROL: without it a double-click that missed the table entirely would
            //   look like correct gating.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Verify.AreEqual(string.Empty, ReadEditReport(), "Precondition: no edit should have been reported before the test acts.");

                Log.Comment("Double-click inside the ReadOnlyCity column, which is authored IsReadOnly=True.");
                DoubleClickRowAtColumnOffset(row, ReadOnlyCellRelativeX);

                Verify.AreEqual(string.Empty, ReadEditReport(),
                    "A double-click on a column authored IsReadOnly must not begin an edit, so BeginningEdit must not fire at all.");

                Log.Comment("Positive control: the same gesture on the editable Age column must report.");
                DoubleClickRowAtColumnOffset(row, AgeCellRelativeX);

                Verify.AreEqual("Age;", ReadEditReport(),
                    "Positive control failed: the double-click gesture never reached an editable cell either, so the read-only assertion above proved nothing.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies double-clicking an editable text cell puts a TextBox editor in the cell, not just a begin-edit event.")]
        public void TextColumnDoubleClickCreatesTextBox()
        {
            // Interaction plan §5 TextColumnDoubleClickCreatesTextBox.
            // Distinct from PointerDoubleClickBeginsEditWhenEditable: that one proves the GESTURE reaches the edit
            //   state machine (BeginningEdit fired, on the right column). This one proves the state machine then
            //   produces an EDITOR - TableViewTextColumn's documented editing visual is a TextBox
            //   (TableView.idl:208 - "A column with no CellEditingTemplate and no built-in editor is not editable").
            //   An implementation that raised the event and swapped in nothing would pass the other test and fail
            //   this one.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Log.Comment("Double-click inside the editable Name column.");
                DoubleClickRowAtColumnOffset(row, NameCellRelativeX);

                string probe = ReadEditorProbe();
                Log.Comment("Editor probe: '{0}'.", probe);

                Verify.AreEqual("TextBox", ProbeField(probe, 0),
                    "A text column's editor must be a TextBox; the probe found no editor or a different element.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies F2 on the current editable text cell puts a TextBox editor in the cell.")]
        public void TextColumnF2CreatesTextBox()
        {
            // Interaction plan §5 TextColumnF2CreatesTextBox.
            // Spec: functional-spec:61 names F2 as a begin-edit gesture alongside double-click. This is the second
            //   documented route into the same editor, so it is a separate test: a regression can break one route
            //   while leaving the other intact (they enter from OnKeyDown and OnPointerPressed respectively).
            // A single click first, to establish which cell is current - F2 acts on the current cell.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Log.Comment("Single-click the Name cell to make it current, then press F2.");
                ClickRowAtColumnOffset(row, NameCellRelativeX);
                KeyboardHelper.PressKey(Key.F2);
                Wait.ForIdle();

                Verify.AreEqual("Name;", ReadEditReport(), "F2 must begin an edit on the current cell's column.");

                string probe = ReadEditorProbe();
                Log.Comment("Editor probe: '{0}'.", probe);

                Verify.AreEqual("TextBox", ProbeField(probe, 0),
                    "F2 must produce the same TextBox editor the double-click route produces.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a column with a CellEditingTemplate opens that template's content as its editor.")]
        public void TemplateColumnEditorUsesCellEditingTemplateContent()
        {
            // Interaction plan §5 TemplateColumnEditorUsesCellEditingTemplateContent.
            // Spec: TableView.idl:208-218 - CellEditingTemplate is "the supported way to customise an editor".
            // Not API-testable: such a column deliberately does not advertise the Value pattern, so SetValue cannot
            //   open it. API §10.1 covers that advertisement gate; only a gesture reaches the editor itself.
            // The page's template column authors its editing template as a TextBox with AutomationProperties.Name
            //   "TemplateCellEditor", which is what distinguishes it from the built-in text-column editor: both are
            //   TextBoxes, so type alone would not prove the template was used.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Log.Comment("Row bounds width is {0}; the Template column starts at x=520.", row.BoundingRectangle.Width);
                Verify.IsTrue(row.BoundingRectangle.Width > TemplateCellRelativeX,
                    "The Template column is not inside the row's bounds, so the gesture below could not land on it.");

                Log.Comment("Double-click inside the Template column.");
                DoubleClickRowAtColumnOffset(row, TemplateCellRelativeX);

                string probe = ReadEditorProbe();
                Log.Comment("Editor probe: '{0}'.", probe);

                Verify.AreEqual("TemplateCellEditor", ProbeField(probe, 1),
                    "The editor a template column opens must be the content of its CellEditingTemplate (TableView.idl:208-218).");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the editor opens pre-populated with the cell's current value.")]
        public void EditorReceivesInitialValue()
        {
            // Interaction plan §5 EditorReceivesInitialValue.
            // Not API-testable: SetValue overwrites the initial value inside the same call, so there is no moment at
            //   which an API test could observe it. The probe reads the editor's text at the instant it opens.
            // The first item's Name is "Person 0" (the page seeds "Person " + index).
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Log.Comment("Double-click inside the Name column of the first row.");
                DoubleClickRowAtColumnOffset(row, NameCellRelativeX);

                string probe = ReadEditorProbe();
                Log.Comment("Editor probe: '{0}'.", probe);

                Verify.AreEqual("Person 0", ProbeField(probe, 2),
                    "The editor must open showing the cell's current value; an empty or stale editor would silently discard data on commit.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus moves into the editor when an edit opens.")]
        public void EditorGetsFocusOnBeginEdit()
        {
            // Interaction plan §5 EditorGetsFocusOnBeginEdit.
            // Separate from the editor-exists tests: an editor that is created but never focused sends the user's
            //   typing to the table's key handling instead of into the cell, and every keyboard leg below
            //   (Enter commits, Escape cancels) silently depends on this.
            // The probe compares the focused element against the editor it found, walking up parents so an editor
            //   whose inner content part takes focus still counts.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Log.Comment("Double-click inside the Name column of the first row.");
                DoubleClickRowAtColumnOffset(row, NameCellRelativeX);

                string probe = ReadEditorProbe();
                Log.Comment("Editor probe: '{0}'.", probe);

                Verify.AreEqual("focus=True", ProbeField(probe, 3),
                    "Focus must move into the editor when an edit opens, otherwise typing never reaches the cell.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Enter commits the open edit and writes the typed value through to the item.")]
        public void EnterKeyCommitsEdit()
        {
            // Interaction plan §5 EnterKeyCommitsEdit.
            // Two independent observations, both needed: CellEditEnding must report EditAction.Commit (the control's
            //   own account of the outcome) AND the bound item's Name must hold the typed text (the edit actually
            //   reached the data). An implementation that closed the editor and reported Commit without pushing the
            //   value would pass the first and fail the second.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Verify.AreEqual("Person 0", ReadFirstItemName(), "Precondition: the first item's Name is the seeded value.");

                OpenNameEditorAndReplaceText(row, "Renamed");

                Log.Comment("Press Enter to commit.");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();

                string endReport = ReadEditEndReport();
                string itemName = ReadFirstItemName();
                Log.Comment("Edit-end report: '{0}'. First item Name: '{1}'.", endReport, itemName);

                Verify.AreEqual("Commit;", endReport, "Enter must end the edit with EditAction.Commit.");
                Verify.AreEqual("Renamed", itemName, "Enter must write the editor's value through to the bound item.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Escape cancels the open edit and leaves the item's pre-edit value intact.")]
        public void EscapeKeyCancelsEdit()
        {
            // Interaction plan §5 EscapeKeyCancelsEdit.
            // The mirror of EnterKeyCommitsEdit, and it is the more important half: a cancel that still wrote the
            //   value is silent data loss. Both observations are taken before either is asserted, because Verify
            //   throws in this suite and a failure on the first would hide the second.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                Verify.AreEqual("Person 0", ReadFirstItemName(), "Precondition: the first item's Name is the seeded value.");

                OpenNameEditorAndReplaceText(row, "Discarded");

                Log.Comment("Press Escape to cancel.");
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                string endReport = ReadEditEndReport();
                string itemName = ReadFirstItemName();
                Log.Comment("Edit-end report: '{0}'. First item Name: '{1}'.", endReport, itemName);

                Verify.AreEqual("Cancel;", endReport, "Escape must end the edit with EditAction.Cancel.");
                Verify.AreEqual("Person 0", itemName, "Escape must leave the item's pre-edit value intact; writing it anyway is silent data loss.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies moving focus out of the editor commits the edit rather than discarding it.")]
        public void FocusLossCommitsEdit()
        {
            // Interaction plan §5 FocusLossCommitsEdit.
            // Focus leaves by a real click on a button OUTSIDE the table, which is how a user loses an editor in
            //   practice. The contract is commit, not cancel: a user who types and then clicks elsewhere expects to
            //   keep the edit, and the failure mode is silent data loss.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject row = GetFirstRow();

                // AfterTableButton is a no-op button on the page, outside the table and ahead of the readouts in
                // its panel, so it stays on screen. The bounds guard is not decoration: a button pushed off the
                // panel reports an empty rectangle and the click below would silently land at (0, 0), which is
                // exactly how this test failed the first time it ran.
                UIObject focusSink = FindElement.ById("AfterTableButton");
                if (focusSink == null)
                {
                    Verify.Fail("AfterTableButton was not found on the test page.");
                    return;
                }

                var sinkBounds = focusSink.BoundingRectangle;
                Log.Comment("Focus sink bounds: {0}.", sinkBounds);
                Verify.IsTrue(sinkBounds.Width > 0 && sinkBounds.Height > 0,
                    "The focus target is not laid out on screen, so the click below could not move focus.");

                Verify.AreEqual("Person 0", ReadFirstItemName(), "Precondition: the first item's Name is the seeded value.");

                OpenNameEditorAndReplaceText(row, "CommittedByFocusLoss");

                Log.Comment("Click the focus sink button outside the table.");
                ClickPoint(new Point(sinkBounds.Left + (sinkBounds.Width / 2), sinkBounds.Top + (sinkBounds.Height / 2)));

                string endReport = ReadEditEndReport();
                string itemName = ReadFirstItemName();
                Log.Comment("Edit-end report: '{0}'. First item Name: '{1}'.", endReport, itemName);

                Verify.AreEqual("Commit;", endReport, "Losing focus must end the edit with EditAction.Commit.");
                Verify.AreEqual("CommittedByFocusLoss", itemName, "Losing focus must keep the typed value, not discard it.");
            }
        }

        #endregion

        #region Helpers

        // Row-relative x inside the Name column (0-160), which is editable.
        private const int NameCellRelativeX = 80;

        // Row-relative x inside the Age column (160-260), which is editable.
        private const int AgeCellRelativeX = 210;

        // Row-relative x inside the ReadOnlyCity column (260-420), authored IsReadOnly="True".
        private const int ReadOnlyCellRelativeX = 340;

        // Row-relative x inside the Template column (520-720), which authors a CellEditingTemplate.
        private const int TemplateCellRelativeX = 620;

        private static UIObject GetFirstRow()
        {
            UIObject tableView = FindElement.ById("BasicTableView");
            if (tableView == null)
            {
                Verify.Fail("BasicTableView was not found on the test page.");
                return null;
            }

            UIObject row = GetRow(tableView, 0);
            if (row == null)
            {
                Verify.Fail("The first realized TableViewRow peer was not found under the rows host.");
            }

            return row;
        }

        // Opens the Name editor on the given row and replaces its whole contents with newText.
        // Select-all first: the editor opens pre-populated (EditorReceivesInitialValue asserts exactly that), so
        // typing without selecting would leave a concatenation and make the commit assertions ambiguous.
        private static void OpenNameEditorAndReplaceText(UIObject row, string newText)
        {
            Log.Comment("Double-click the Name cell, select all, and type '{0}'.", newText);
            DoubleClickRowAtColumnOffset(row, NameCellRelativeX);

            KeyboardHelper.PressKey(Key.a, ModifierKey.Control);
            TextInput.SendText(newText);
            Wait.ForIdle();
        }

        // Escapes any edit the test left open, so the app is in a known state for the next test in the class.
        private static void CancelAnyOpenEdit()
        {
            KeyboardHelper.PressKey(Key.Escape);
            Wait.ForIdle();
        }

        // Page readouts. Resolved fresh each call but WITHOUT ElementCache.Clear(): clearing the cache forces the
        // next FindElement to re-walk the whole visual tree, and that walk descends into TableViewRow children -
        // the peer-manufacturing path of finding #13 - which asserts the app. UIA property reads are live, so a
        // plain Value read already sees the current text.
        private static string ReadEditReport()
        {
            Edit report = FindElement.ById<Edit>("EditColumnReportTextBlock");
            return report == null ? null : report.Value;
        }

        private static string ReadEditorProbe()
        {
            Edit report = FindElement.ById<Edit>("EditorProbeTextBlock");
            return report == null ? null : report.Value;
        }

        private static string ReadEditEndReport()
        {
            Edit report = FindElement.ById<Edit>("EditEndReportTextBlock");
            return report == null ? null : report.Value;
        }

        private static string ReadFirstItemName()
        {
            Edit report = FindElement.ById<Edit>("FirstItemNameTextBlock");
            return report == null ? null : report.Value;
        }

        // The probe writes one entry per opened edit, "<type>|<automationName>|<text>|focus=<bool>;".
        // Returns the requested field of the FIRST entry, or a diagnostic string the assertion will print.
        private static string ProbeField(string probe, int field)
        {
            if (string.IsNullOrEmpty(probe))
            {
                return "<no probe entry: no edit opened>";
            }

            string firstEntry = probe.Split(';')[0];
            string[] fields = firstEntry.Split('|');
            return field < fields.Length ? fields[field] : "<probe entry has no field " + field + ": '" + firstEntry + "'>";
        }

        // ALL pointer input below is expressed as ABSOLUTE SCREEN POINTS derived from BoundingRectangle, never as
        // a UIObject + offset: every InputHelper offset overload resolves its anchor with
        // IUIAutomationElement::GetClickablePoint, and that call access-violates the app on a TableViewRow peer
        // (product finding #14) because UIA computes the point by walking into the row's children.
        private static void ClickPoint(Point point)
        {
            Log.Comment("Click at absolute point ({0}, {1}).", point.X, point.Y);
            PointerInput.Move(point);
            PointerInput.Press(PointerButtons.Primary);
            PointerInput.Release(PointerButtons.Primary);
            Wait.ForIdle();
        }

        private static void ClickRowAtColumnOffset(UIObject row, int rowRelativeX)
        {
            var bounds = row.BoundingRectangle;
            ClickPoint(new Point(bounds.Left + rowRelativeX, bounds.Top + (bounds.Height / 2)));
        }

        // The two press/release pairs are issued back to back with no idle wait between them: a Wait.ForIdle in
        // the middle can exceed the system double-click time and turn the gesture into two single clicks.
        private static void DoubleClickRowAtColumnOffset(UIObject row, int rowRelativeX)
        {
            var bounds = row.BoundingRectangle;
            var point = new Point(bounds.Left + rowRelativeX, bounds.Top + (bounds.Height / 2));

            Log.Comment("Double-click at absolute point ({0}, {1}).", point.X, point.Y);
            PointerInput.Move(point);
            PointerInput.Press(PointerButtons.Primary);
            PointerInput.Release(PointerButtons.Primary);
            PointerInput.Press(PointerButtons.Primary);
            PointerInput.Release(PointerButtons.Primary);
            Wait.ForIdle();
        }

        // Returns the row peer at index, or null. Never descends into the row's own children.
        private static UIObject GetRow(UIObject tableView, int index)
        {
            UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
            if (rowsHost == null || index >= rowsHost.Children.Count)
            {
                return null;
            }

            UIObject candidate = rowsHost.Children[index];
            if (candidate == null || candidate.ClassName == null || !candidate.ClassName.Contains("TableViewRow"))
            {
                return null;
            }

            return candidate;
        }

        #endregion
    }
}
