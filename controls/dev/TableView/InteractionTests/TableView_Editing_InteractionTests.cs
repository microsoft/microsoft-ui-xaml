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

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView editing-gesture interaction tests.
    // Editing mixes routes by nature - the edit is opened with a pointer or with F2 and
    // then driven from the keyboard - so it gets its own file rather than being split across the pointer and
    // keyboard files.
    //
    // Product finding #13 (a client asking a row peer for its children fail-fasted the app) shaped the older tests in
    // this file: they observe rows at row level, point at cells by coordinates, and read editors and visual states
    // through in-process page readouts. #11820 fixed the peers, and cell peers are now read safely; see the history
    // note in TableViewInteractionTestHelpers. The older tests are kept as written - their techniques still work.
    [TestClass]
    [TestProperty("Ignore", "True")] // Disabled pending TableView page leak fix: #12491.
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
            RestartAppIfLongRunning();
        }

        #region 5. Editing gestures

        [TestMethod]
        [TestProperty("Description", "Verifies a double-click inside an editable column begins an edit on the column under the pointer.")]
        public void PointerDoubleClickBeginsEditWhenEditable()
        {
            // Spec: functional-spec:61 - "double-click / double-tap and F2 begin an edit".
            //       dev-spec:284 - the gesture is driven from PointerPressed with click-count tracking, not from a
            //       DoubleTapped handler, so that a row marking the press handled for selection cannot kill it.
            // Observed through the page's BeginningEdit report (TableViewBeginningEditEventArgs.Column is public IDL).
            using (var setup = new TestSetupHelper(PageName))
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
            // Spec: the per-column gate is checked before BeginningEdit is raised, so a correct implementation is
            //   silent rather than cancelling - hence the assertion is that the event does not fire at all.
            // The second leg is a POSITIVE CONTROL: without it a double-click that missed the table entirely would
            //   look like correct gating.
            using (var setup = new TestSetupHelper(PageName))
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
            // Distinct from PointerDoubleClickBeginsEditWhenEditable: that one proves the GESTURE reaches the edit
            //   state machine (BeginningEdit fired, on the right column). This one proves the state machine then
            //   produces an EDITOR - TableViewTextColumn's documented editing visual is a TextBox
            //   (TableView.idl:208 - "A column with no CellEditingTemplate and no built-in editor is not editable").
            //   An implementation that raised the event and swapped in nothing would pass the other test and fail
            //   this one.
            using (var setup = new TestSetupHelper(PageName))
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
            // Spec: functional-spec:61 names F2 as a begin-edit gesture alongside double-click. This is the second
            //   documented route into the same editor, so it is a separate test: a regression can break one route
            //   while leaving the other intact (they enter from OnKeyDown and OnPointerPressed respectively).
            // A single click first, to establish which cell is current - F2 acts on the current cell.
            using (var setup = new TestSetupHelper(PageName))
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
        [TestProperty("Description", "Verifies a keyboard-only user can choose the edit column: Tab onto the header row, Down into the body, Right into the cells, F2 edits the focused cell.")]
        public void KeyboardOnlyF2EditsChosenColumn()
        {
            // No pointer input, so only cell-level keyboard navigation can choose the column; else F2 edits Name.
            using (var setup = new TestSetupHelper(PageName))
            {
                var selectionMode = FindElement.ById<ComboBox>(SelectionModeComboBox);
                if (selectionMode == null) { Verify.Fail("SelectionModeComboBox was not found."); return; }
                selectionMode.SetFocus();
                Wait.ForIdle();

                Verify.AreEqual(string.Empty, ReadEditReport(), "Precondition: no edit should have been reported before the test acts.");

                Log.Comment("Tab onto the header row, then Down into the body.");
                for (int attempt = 0; attempt < 4 && FindFocusedHeader(BasicTable) == null; attempt++)
                {
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                }
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                UIObject focusedRow = FindFocusedRow(GetRowsHost(BasicTable));
                if (focusedRow == null) { Verify.Fail("Precondition: Tab and Down never put focus on a body row."); return; }

                Log.Comment("Right drills into the first cell, Right again moves to the Age cell, then F2.");
                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();

                int rowIndex, cellIndex;
                FindFocusedCell(BasicTable, out rowIndex, out cellIndex);
                Verify.AreEqual(AgeColumn, cellIndex, "Precondition: Right, Right from the row must focus the Age cell.");

                KeyboardHelper.PressKey(Key.F2);
                Wait.ForIdle();

                Verify.AreEqual("Age;", ReadEditReport(),
                    "F2 must edit the cell the keyboard focused, not fall back to the first editable column.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a column with a CellEditingTemplate opens that template's content as its editor.")]
        public void TemplateColumnEditorUsesCellEditingTemplateContent()
        {
            // Spec: TableView.idl:208-218 - CellEditingTemplate is "the supported way to customise an editor".
            // Not API-testable: such a column deliberately does not advertise the Value pattern, so SetValue cannot
            //   open it. API §10.1 covers that advertisement gate; only a gesture reaches the editor itself.
            // The page's template column authors its editing template as a TextBox with AutomationProperties.Name
            //   "TemplateCellEditor", which is what distinguishes it from the built-in text-column editor: both are
            //   TextBoxes, so type alone would not prove the template was used.
            using (var setup = new TestSetupHelper(PageName))
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
            // Not API-testable: SetValue overwrites the initial value inside the same call, so there is no moment at
            //   which an API test could observe it. The probe reads the editor's text at the instant it opens.
            // The first item's Name is "Person 0" (the page seeds "Person " + index).
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();

                Log.Comment("Double-click inside the Name column of the first row.");
                DoubleClickRowAtColumnOffset(row, NameCellRelativeX);

                string probe = ReadEditorProbe();
                Log.Comment("Editor probe: '{0}'.", probe);

                Verify.AreEqual(BasicName(0), ProbeField(probe, 2),
                    "The editor must open showing the cell's current value; an empty or stale editor would silently discard data on commit.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus moves into the editor when an edit opens.")]
        public void EditorGetsFocusOnBeginEdit()
        {
            // Separate from the editor-exists tests: an editor that is created but never focused sends the user's
            //   typing to the table's key handling instead of into the cell, and every keyboard leg below
            //   (Enter commits, Escape cancels) silently depends on this.
            // The probe compares the focused element against the editor it found, walking up parents so an editor
            //   whose inner content part takes focus still counts.
            using (var setup = new TestSetupHelper(PageName))
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
            // Two independent observations, both needed: CellEditEnding must report EditAction.Commit (the control's
            //   own account of the outcome) AND the bound item's Name must hold the typed text (the edit actually
            //   reached the data). An implementation that closed the editor and reported Commit without pushing the
            //   value would pass the first and fail the second.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();

                Verify.AreEqual(BasicName(0), ReadFirstItemName(), "Precondition: the first item's Name is the seeded value.");

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
            // The mirror of EnterKeyCommitsEdit, and it is the more important half: a cancel that still wrote the
            //   value is silent data loss. Both observations are taken before either is asserted, because Verify
            //   throws in this suite and a failure on the first would hide the second.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();

                Verify.AreEqual(BasicName(0), ReadFirstItemName(), "Precondition: the first item's Name is the seeded value.");

                OpenNameEditorAndReplaceText(row, "Discarded");

                Log.Comment("Press Escape to cancel.");
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                string endReport = ReadEditEndReport();
                string itemName = ReadFirstItemName();
                Log.Comment("Edit-end report: '{0}'. First item Name: '{1}'.", endReport, itemName);

                Verify.AreEqual("Cancel;", endReport, "Escape must end the edit with EditAction.Cancel.");
                Verify.AreEqual(BasicName(0), itemName, "Escape must leave the item's pre-edit value intact; writing it anyway is silent data loss.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies moving focus out of the editor commits the edit rather than discarding it.")]
        public void FocusLossCommitsEdit()
        {
            // Focus leaves by a real click on a button OUTSIDE the table, which is how a user loses an editor in
            //   practice. The contract is commit, not cancel: a user who types and then clicks elsewhere expects to
            //   keep the edit, and the failure mode is silent data loss.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();

                // AfterTableButton is a no-op button on the page, outside the table and ahead of the readouts in
                // its panel, so it stays on screen. The bounds guard is not decoration: a button pushed off the
                // panel reports an empty rectangle and the click below would silently land at (0, 0), which is
                // exactly how this test failed the first time it ran.
                UIObject focusSink = FindElement.ById(AfterTableButton);
                if (focusSink == null)
                {
                    Verify.Fail("AfterTableButton was not found on the test page.");
                    return;
                }

                var sinkBounds = focusSink.BoundingRectangle;
                Log.Comment("Focus sink bounds: {0}.", sinkBounds);
                Verify.IsTrue(sinkBounds.Width > 0 && sinkBounds.Height > 0,
                    "The focus target is not laid out on screen, so the click below could not move focus.");

                Verify.AreEqual(BasicName(0), ReadFirstItemName(), "Precondition: the first item's Name is the seeded value.");

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

        #region Tab / Shift+Tab while editing (ADO 64540419)

        // Expected behavior follows WPF DataGrid: Tab commits and opens the next editable visible cell, Shift+Tab
        // the previous one; read-only and non-editable columns are skipped; the end of a row wraps to the next
        // row; past the last editable cell the edit closes and Tab leaves normally; a vetoed commit keeps the editor.
        // BasicTableView columns: Name, Age, ReadOnlyCity (IsReadOnly), Score, Template (editing template),
        // Action (no editing template, so not editable).

        [TestMethod]
        [TestProperty("Description", "Verifies Tab in an open editor commits it and opens the next editable cell in the same row.")]
        public void TabWhileEditingMovesToNextEditableCell()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();

                OpenNameEditorAndReplaceText(row, "Tabbed");
                Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on Name.");

                Log.Comment("Press Tab inside the Name editor.");
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                string editReport = ReadEditReport();
                string endReport = ReadEditEndReport();
                string itemName = ReadFirstItemName();
                string probe = ReadEditorProbe();
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Item Name: '{2}'. Probe: '{3}'.", editReport, endReport, itemName, probe);

                Verify.AreEqual("Commit;", endReport, "Tab must commit the open edit.");
                Verify.AreEqual("Tabbed", itemName, "Tab must write the editor's value through to the item.");
                Verify.AreEqual("Name;Age;", editReport, "Tab must open an edit on the next editable cell (Age).");
                Verify.AreEqual(BasicAge(0).ToString(), ProbeEntryField(probe, 1, 2), "The new editor must belong to row 0's Age cell.");
                Verify.AreEqual("focus=True", ProbeEntryField(probe, 1, 3), "Focus must move into the new editor.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+Tab in an open editor commits it and opens the previous editable cell in the same row.")]
        public void ShiftTabWhileEditingMovesToPreviousEditableCell()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();

                DoubleClickRowAtColumnOffset(row, AgeCellRelativeX);
                Verify.AreEqual("Age;", ReadEditReport(), "Precondition: the edit opened on Age.");

                Log.Comment("Press Shift+Tab inside the Age editor.");
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();

                string editReport = ReadEditReport();
                string endReport = ReadEditEndReport();
                string probe = ReadEditorProbe();
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Probe: '{2}'.", editReport, endReport, probe);

                Verify.AreEqual("Commit;", endReport, "Shift+Tab must commit the open edit.");
                Verify.AreEqual("Age;Name;", editReport, "Shift+Tab must open an edit on the previous editable cell (Name).");
                Verify.AreEqual(BasicName(0), ProbeEntryField(probe, 1, 2), "The new editor must belong to row 0's Name cell.");
                Verify.AreEqual("focus=True", ProbeEntryField(probe, 1, 3), "Focus must move into the new editor.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab and Shift+Tab while editing skip a read-only column in both directions.")]
        public void TabWhileEditingSkipsReadOnlyColumn()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();

                DoubleClickRowAtColumnOffset(row, AgeCellRelativeX);
                Verify.AreEqual("Age;", ReadEditReport(), "Precondition: the edit opened on Age.");

                Log.Comment("Tab from Age must skip ReadOnlyCity and land on Score.");
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
                string afterTab = ReadEditReport();
                Log.Comment("Edit report after Tab: '{0}'.", afterTab);
                Verify.AreEqual("Age;Score;", afterTab, "Tab must skip the IsReadOnly column and edit Score.");

                Log.Comment("Shift+Tab from Score must skip ReadOnlyCity and land back on Age.");
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();
                string afterShiftTab = ReadEditReport();
                Log.Comment("Edit report after Shift+Tab: '{0}'. End report: '{1}'.", afterShiftTab, ReadEditEndReport());
                Verify.AreEqual("Age;Score;Age;", afterShiftTab, "Shift+Tab must skip the IsReadOnly column and edit Age.");
                Verify.AreEqual("Commit;Commit;", ReadEditEndReport(), "Each move must commit the cell it leaves.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab from the last editable cell of a row wraps to the first editable cell of the next row, still editing.")]
        public void TabWhileEditingAtEndOfRowWrapsToNextRow()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();

                Log.Comment("Double-click the Template cell, the last editable cell of row 0 (Action is not editable).");
                DoubleClickRowAtColumnOffset(row, TemplateCellRelativeX);
                Verify.AreEqual("Template;", ReadEditReport(), "Precondition: the edit opened on Template.");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                string editReport = ReadEditReport();
                string endReport = ReadEditEndReport();
                string probe = ReadEditorProbe();
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Probe: '{2}'.", editReport, endReport, probe);

                Verify.AreEqual("Commit;", endReport, "Tab must commit the open edit.");
                Verify.AreEqual("Template;Name;", editReport, "Tab past the last editable cell must wrap to the next row's first editable cell.");
                Verify.AreEqual(BasicName(1), ProbeEntryField(probe, 1, 2), "The new editor must belong to row 1, not row 0.");
                Verify.AreEqual("focus=True", ProbeEntryField(probe, 1, 3), "Focus must move into the new editor.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab from the last editable cell of the last row commits, ends editing, and opens no new edit.")]
        public void TabWhileEditingLastCellOfLastRowExitsEdit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!DrillIntoRow(BasicTable, 0)) { return; }

                Log.Comment("Ctrl+End to the last cell of the last row, Left to its Template cell, F2.");
                KeyboardHelper.PressKey(Key.End, ModifierKey.Control);
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.F2);
                Wait.ForIdle();

                Verify.AreEqual("Template;", ReadEditReport(), "Precondition: F2 opened an edit on the last row's Template cell.");
                Verify.AreEqual(BasicName(BasicItemCount - 1), ProbeEntryField(ReadEditorProbe(), 0, 2), "Precondition: the editor belongs to the last row.");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                string editReport = ReadEditReport();
                string endReport = ReadEditEndReport();
                string focus;
                bool inEditor = IsFocusInBasicTableEditor(out focus);
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", editReport, endReport, focus);

                Verify.AreEqual("Commit;", endReport, "Tab from the last editable cell must commit.");
                Verify.AreEqual("Template;", editReport, "There is no next editable cell, so no new edit may open.");
                Verify.IsFalse(inEditor, "Focus must leave the editor so Tab continues normally.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+Tab from the first editable cell of the first row commits, ends editing, and opens no new edit.")]
        public void ShiftTabWhileEditingFirstCellOfFirstRowExitsEdit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();
                OpenNameEditorAndReplaceText(row, "ShiftTabbed");
                Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on row 0's Name.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();

                string editReport = ReadEditReport();
                string endReport = ReadEditEndReport();
                string itemName = ReadFirstItemName();
                string focus;
                bool inEditor = IsFocusInBasicTableEditor(out focus);
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Item Name: '{2}'. Focus: {3}.", editReport, endReport, itemName, focus);

                Verify.AreEqual("Commit;", endReport, "Shift+Tab from the first editable cell must commit.");
                Verify.AreEqual("ShiftTabbed", itemName, "The commit must write the editor's value through to the item.");
                Verify.AreEqual("Name;", editReport, "There is no previous editable cell, so no new edit may open.");
                Verify.IsFalse(inEditor, "Focus must leave the editor so Shift+Tab continues normally.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab and Shift+Tab while editing under RTL follow logical column order.")]
        public void TabWhileEditingUnderRtlFollowsLogicalOrder()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }

                try
                {
                    if (!SelectPivotItem(RtlPivotItem)) { Verify.Fail(GoToButton(RtlPivotItem) + " was not found."); return; }

                    UIObject rowsHost = GetRowsHost(RtlTable);
                    if (rowsHost == null) { return; }
                    if (rowsHost.Children.Count < 2) { Verify.Fail("Need realized rows in RtlTableView."); return; }

                    rowsHost.Children[1].SetFocus();
                    Wait.ForIdle();
                    Log.Comment("Under RTL, Left drills into the first logical cell (RtlName); F2 edits it.");
                    KeyboardHelper.PressKey(Key.Left);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();

                    UIObject editor = UIObject.Focused;
                    if (editor == null || editor.ControlType != ControlType.Edit) { Verify.Fail("Precondition: F2 must open the RtlName editor. Focus: " + DescribeFocused()); return; }
                    Verify.AreEqual(ScrollingName(1), new Edit(editor).Value, "Precondition: the editor belongs to row 1's RtlName cell.");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    editor = UIObject.Focused;
                    Log.Comment("Focus after Tab: {0}.", DescribeFocused());
                    Verify.IsTrue(editor != null && editor.ControlType == ControlType.Edit, "Tab must open the next logical cell's editor.");
                    if (editor != null && editor.ControlType == ControlType.Edit)
                    {
                        Verify.AreEqual(CityOf(1), new Edit(editor).Value, "Under RTL, Tab must move to RtlCity, the next cell in logical order.");
                    }

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    editor = UIObject.Focused;
                    Log.Comment("Focus after Shift+Tab: {0}.", DescribeFocused());
                    Verify.IsTrue(editor != null && editor.ControlType == ControlType.Edit, "Shift+Tab must open the previous logical cell's editor.");
                    if (editor != null && editor.ControlType == ControlType.Edit)
                    {
                        Verify.AreEqual(ScrollingName(1), new Edit(editor).Value, "Under RTL, Shift+Tab must move back to RtlName.");
                    }
                }
                finally
                {
                    CancelAnyOpenEdit();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab from the last editable cell of the last row keeps the editor open when the commit is vetoed.")]
        public void TabWhileEditingLastCellOfLastRowStaysWhenCommitVetoed()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var veto = FindElement.ById<CheckBox>(VetoCommitCheckBox);
                if (veto == null) { Verify.Fail("VetoCommitCheckBox was not found."); return; }

                try
                {
                    veto.Check();
                    Wait.ForIdle();
                    if (!DrillIntoRow(BasicTable, 0)) { return; }

                    KeyboardHelper.PressKey(Key.End, ModifierKey.Control);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Left);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();
                    Verify.AreEqual("Template;", ReadEditReport(), "Precondition: F2 opened an edit on the last row's Template cell.");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string endReport = ReadEditEndReport();
                    string focus;
                    bool inEditor = IsFocusInBasicTableEditor(out focus);
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", ReadEditReport(), endReport, focus);

                    Verify.AreEqual("Commit;", endReport, "Tab must attempt the commit, which the page cancels.");
                    Verify.IsTrue(inEditor, "A vetoed commit must keep focus in the editor instead of leaving the table.");
                }
                finally
                {
                    veto.Uncheck();
                    Wait.ForIdle();
                    CancelAnyOpenEdit();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies two quick Tabs while editing move the edit two editable cells, across a row wrap.")]
        public void DoubleTabWhileEditingMovesTwoCells()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();
                DoubleClickRowAtColumnOffset(row, ScoreCellRelativeX);
                Verify.AreEqual("Score;", ReadEditReport(), "Precondition: the edit opened on Score.");

                KeyboardHelper.PressKey(Key.Tab);
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                string editReport = ReadEditReport();
                string probe = ReadEditorProbe();
                string focus;
                bool inEditor = IsFocusInBasicTableEditor(out focus);
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Probe: '{2}'. Focus: {3}.", editReport, ReadEditEndReport(), probe, focus);

                Verify.AreEqual("Score;Template;Name;", editReport, "Two Tabs must move Score -> Template -> the next row's Name.");
                Verify.AreEqual(BasicName(1), ProbeEntryField(probe, 2, 2), "The last editor must belong to row 1.");
                Verify.IsTrue(inEditor, "Focus must stay in the table's editor.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab while editing skips a collapsed column.")]
        public void TabWhileEditingSkipsHiddenColumn()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var hideScore = FindElement.ById<CheckBox>(HideScoreColumnCheckBox);
                if (hideScore == null) { Verify.Fail("HideScoreColumnCheckBox was not found."); return; }

                try
                {
                    hideScore.Check();
                    Wait.ForIdle();

                    UIObject row = GetFirstRow();
                    DoubleClickRowAtColumnOffset(row, AgeCellRelativeX);
                    Verify.AreEqual("Age;", ReadEditReport(), "Precondition: the edit opened on Age.");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    string editReport = ReadEditReport();
                    Log.Comment("Edit report: '{0}'. End report: '{1}'.", editReport, ReadEditEndReport());
                    Verify.AreEqual("Age;Template;", editReport, "Tab must skip ReadOnlyCity and the collapsed Score column.");
                }
                finally
                {
                    CancelAnyOpenEdit();
                    hideScore.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Tab wrap keeps its new editor open when the commit changes a live-grouped key.")]
        public void TabWrapKeepsEditorWhenCommitReshapesLiveGroups()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }

                try
                {
                    if (!SelectPivotItem(GroupedPivotItem)) { Verify.Fail(GoToButton(GroupedPivotItem) + " was not found."); return; }

                    UIObject rowsHost = GetRowsHost(GroupedTable);
                    if (rowsHost == null) { return; }

                    int first = -1;
                    for (int i = 0; i < rowsHost.Children.Count; i++)
                    {
                        if (IsRow(rowsHost.Children[i])) { first = i; break; }
                    }
                    int next = first + 1;
                    while (next >= 0 && next < rowsHost.Children.Count && !IsRow(rowsHost.Children[next])) { next++; }
                    if (first < 0 || next >= rowsHost.Children.Count) { Verify.Fail("Precondition: two realized data rows are needed."); return; }
                    string expectedName = rowsHost.Children[next].Children.Count > 0 ? rowsHost.Children[next].Children[0].Name : null;

                    Log.Comment("Edit the first row's GroupedCity (the live group key), then Tab wraps to the next data row.");
                    DoubleClickRowAtColumnOffset(rowsHost.Children[first], 280);
                    UIObject editor = UIObject.Focused;
                    if (editor == null || editor.ControlType != ControlType.Edit) { Verify.Fail("Precondition: double-click must open the GroupedCity editor. Focus: " + DescribeFocused()); return; }
                    KeyboardHelper.PressKey(Key.a, ModifierKey.Control);
                    TextInput.SendText("Zzz");
                    Wait.ForIdle();
                    Log.Comment("Focus before Tab: {0}. Expected next row: '{1}'.", DescribeFocused(), expectedName);

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    editor = UIObject.Focused;
                    Log.Comment("Focus after Tab: {0}.", DescribeFocused());
                    Verify.IsTrue(editor != null && editor.ControlType == ControlType.Edit, "The live regroup must not close the wrapped edit.");
                    if (editor != null && editor.ControlType == ControlType.Edit)
                    {
                        Verify.AreEqual(expectedName, new Edit(editor).Value, "The editor must belong to the next data row's GroupedName.");
                    }
                }
                finally
                {
                    CancelAnyOpenEdit();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a pending Shift+Tab wrap opens no editor when its target row is removed first.")]
        public void ShiftTabWrapOpensNoEditorWhenTargetRemoved()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }
                var removeAbove = FindElement.ById<CheckBox>(RemoveRowAboveOnCommitCheckBox);
                if (removeAbove == null) { Verify.Fail("RemoveRowAboveOnCommitCheckBox was not found."); return; }

                try
                {
                    removeAbove.Check();
                    Wait.ForIdle();
                    if (!SelectPivotItem(ScrollingPivotItem)) { Verify.Fail(GoToButton(ScrollingPivotItem) + " was not found."); return; }

                    UIObject rowsHost = GetRowsHost(ScrollingTable);
                    if (rowsHost == null || rowsHost.Children.Count < 2) { Verify.Fail("Need realized rows."); return; }

                    rowsHost.Children[0].SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.PageDown);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.PageDown);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.PageUp);
                    Wait.ForIdle();

                    int firstVisible = FocusedPositionInSet() - 1;
                    if (firstVisible < 1) { Verify.Fail("Precondition: a row must sit above the viewport (" + firstVisible + ")."); return; }

                    KeyboardHelper.PressKey(Key.Right);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();
                    UIObject editor = UIObject.Focused;
                    if (editor == null || editor.ControlType != ControlType.Edit) { Verify.Fail("Precondition: F2 must open the FrozenName editor. Focus: " + DescribeFocused()); return; }

                    Log.Comment("Shift+Tab wraps to the row above the viewport; the page removes that row before it is ready.");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();

                    editor = UIObject.Focused;
                    Log.Comment("Focus after Shift+Tab: {0}.", DescribeFocused());
                    if (editor != null && editor.ControlType == ControlType.Edit)
                    {
                        Verify.AreNotEqual((firstVisible - 1).ToString(), new Edit(editor).Value, "No editor may open on the removed row.");
                    }
                }
                finally
                {
                    CancelAnyOpenEdit();
                    removeAbove.Uncheck();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab does not pull focus back when a CellEditEnding handler focuses a column header inside the commit.")]
        public void TabWhileEditingStopsWhenCommitHandlerFocusesHeader()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var focusHeader = FindElement.ById<CheckBox>(FocusHeaderInCommitCheckBox);
                if (focusHeader == null) { Verify.Fail("FocusHeaderInCommitCheckBox was not found."); return; }

                try
                {
                    focusHeader.Check();
                    Wait.ForIdle();

                    OpenNameEditorAndReplaceText(GetFirstRow(), "Moved");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string editReport = ReadEditReport();
                    string focus;
                    bool inEditor = IsFocusInBasicTableEditor(out focus);
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", editReport, ReadEditEndReport(), focus);

                    Verify.AreEqual("Commit;", ReadEditEndReport(), "Tab must commit the open edit.");
                    Verify.AreEqual("Name;", editReport, "Focus left the record during the commit, so no edit may open on Age.");
                    Verify.IsFalse(inEditor, "The table must not pull focus back from the header.");
                    Verify.AreEqual("TableViewColumnHeader", UIObject.Focused?.ClassName, "Focus must stay on the column header.");
                }
                finally
                {
                    focusHeader.Uncheck();
                    Wait.ForIdle();
                    CancelAnyOpenEdit();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab opens no edit when a CellEditEnding handler re-sorts the rows and then focuses a column header.")]
        public void TabWhileEditingStopsWhenCommitHandlerSortsAndFocusesHeader()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var sortOnCommit = FindElement.ById<CheckBox>(SortOnCommitCheckBox);
                var focusHeader = FindElement.ById<CheckBox>(FocusHeaderInCommitCheckBox);
                if (sortOnCommit == null || focusHeader == null) { Verify.Fail("SortOnCommitCheckBox or FocusHeaderInCommitCheckBox was not found."); return; }

                try
                {
                    sortOnCommit.Check();
                    focusHeader.Check();
                    Wait.ForIdle();

                    DoubleClickRowAtColumnOffset(GetFirstRow(), NameCellRelativeX);
                    Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on Name.");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", ReadEditReport(), ReadEditEndReport(), DescribeFocused());
                    Verify.AreEqual("Commit;", ReadEditEndReport(), "Tab must commit the open edit.");
                    Verify.AreEqual("Name;", ReadEditReport(), "Focus left the body during the commit, so no edit may open.");
                    // The re-sort's own focus restore then lands on a body row; Enter does the same (followups.md).
                    Verify.AreNotEqual(ControlType.Edit, UIObject.Focused?.ControlType, "No editor may take focus after the move stopped.");
                }
                finally
                {
                    sortOnCommit.Uncheck();
                    focusHeader.Uncheck();
                    Wait.ForIdle();
                    CancelAnyOpenEdit();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a second Shift+Tab with nothing past the pending wrap target lets focus leave the body instead of swallowing the key.")]
        public void SecondShiftTabPastPendingWrapLeavesBody()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }
                var single = FindElement.ById<CheckBox>(SingleEditableScrollingCheckBox);
                if (single == null) { Verify.Fail("SingleEditableScrollingCheckBox was not found."); return; }

                try
                {
                    single.Check();
                    Wait.ForIdle();
                    if (!SelectPivotItem(ScrollingPivotItem)) { Verify.Fail(GoToButton(ScrollingPivotItem) + " was not found."); return; }

                    UIObject rowsHost = GetRowsHost(ScrollingTable);
                    if (rowsHost == null || rowsHost.Children.Count < 2) { Verify.Fail("Need realized rows."); return; }

                    Log.Comment("PageDown twice, then Up until row 1 is focused: the table scrolls just enough to show row 1, so row 0 sits above the viewport.");
                    rowsHost.Children[0].SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.PageDown);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.PageDown);
                    Wait.ForIdle();
                    for (int i = 0; i < 60 && FocusedPositionInSet() > 2; i++)
                    {
                        KeyboardHelper.PressKey(Key.Up);
                        Wait.ForIdle();
                    }
                    if (FocusedPositionInSet() != 2) { Verify.Fail("Precondition: row 1 must be focused (" + FocusedPositionInSet() + ")."); return; }

                    KeyboardHelper.PressKey(Key.Right);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();
                    UIObject editor = UIObject.Focused;
                    if (editor == null || editor.ControlType != ControlType.Edit) { Verify.Fail("Precondition: F2 must open row 1's FrozenName editor. Focus: " + DescribeFocused()); return; }

                    Log.Comment("The first Shift+Tab wraps to row 0 above the viewport; the second has nowhere further to go.");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();

                    UIObject focused = UIObject.Focused;
                    Log.Comment("Focus after two Shift+Tabs: {0}.", DescribeFocused());
                    Verify.IsTrue(focused != null && focused.ControlType != ControlType.Edit, "No editor may hold focus after the second Shift+Tab.");
                    Verify.IsTrue(focused != null && focused.ClassName != "TableViewCell" && focused.ClassName != "Microsoft.UI.Xaml.Controls.Tabular.TableViewRow",
                        "The second Shift+Tab must move focus out of the body, not be swallowed.");
                }
                finally
                {
                    CancelAnyOpenEdit();
                    single.Uncheck();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab and Shift+Tab move between the controls of a multi-control editing template before moving the edit.")]
        public void TabInMultiControlEditorMovesWithinEditorFirst()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var showExtra = FindElement.ById<CheckBox>(ShowExtraEditColumnsCheckBox);
                if (showExtra == null) { Verify.Fail("ShowExtraEditColumnsCheckBox was not found."); return; }

                try
                {
                    showExtra.Check();
                    Wait.ForIdle();
                    if (!DrillIntoRow(BasicTable, 0)) { return; }

                    Log.Comment("End to XBind, Left to Pair, F2.");
                    KeyboardHelper.PressKey(Key.End);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Left);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();
                    Verify.AreEqual("Pair;", ReadEditReport(), "Precondition: F2 opened an edit on Pair.");
                    Verify.AreEqual("PairFirstEditor", UIObject.Focused?.AutomationId, "Precondition: the first box has focus. Focus: " + DescribeFocused());

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.AreEqual("PairSecondEditor", UIObject.Focused?.AutomationId, "Tab from the first box must reach the second box. Focus: " + DescribeFocused());
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.AreEqual("PairFirstEditor", UIObject.Focused?.AutomationId, "Shift+Tab from the second box must reach the first box. Focus: " + DescribeFocused());
                    Verify.AreEqual(string.Empty, ReadEditEndReport(), "Moving within the editor must not commit.");
                    Verify.AreEqual("Pair;", ReadEditReport(), "Moving within the editor must not move the edit.");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.AreEqual("Pair;XBind;", ReadEditReport(), "Tab from the second box must commit and edit XBind.");
                    Verify.AreEqual("Commit;", ReadEditEndReport(), "Leaving Pair must commit it once.");

                    Log.Comment("Esc closes XBind; Left to Pair and F2 reopens it on the first box.");
                    KeyboardHelper.PressKey(Key.Escape);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Left);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();
                    Verify.AreEqual("PairFirstEditor", UIObject.Focused?.AutomationId, "Precondition: Pair reopened on its first box. Focus: " + DescribeFocused());

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.AreEqual("Pair;XBind;Pair;Template;", ReadEditReport(), "Shift+Tab from the first box must commit and edit Template.");
                }
                finally
                {
                    CancelAnyOpenEdit();
                    showExtra.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab from an {x:Bind} editor the base commit cannot see keeps the editor open rather than leaving the table.")]
        public void TabFromXBindEditorKeepsEditorOpen()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var showExtra = FindElement.ById<CheckBox>(ShowExtraEditColumnsCheckBox);
                if (showExtra == null) { Verify.Fail("ShowExtraEditColumnsCheckBox was not found."); return; }

                try
                {
                    showExtra.Check();
                    Wait.ForIdle();
                    if (!DrillIntoRow(BasicTable, 0)) { return; }

                    KeyboardHelper.PressKey(Key.End);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();
                    Verify.AreEqual("XBind;", ReadEditReport(), "Precondition: F2 opened an edit on XBind.");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", ReadEditReport(), ReadEditEndReport(), DescribeFocused());

                    Verify.AreEqual("Commit;", ReadEditEndReport(), "Tab must attempt the commit.");
                    Verify.AreEqual("XBind;", ReadEditReport(), "A commit that writes nothing keeps the edit, so the move must not proceed.");
                    Verify.AreEqual("XBindCellEditor", UIObject.Focused?.AutomationId, "Focus must stay in the XBind editor.");
                }
                finally
                {
                    CancelAnyOpenEdit();
                    showExtra.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies two quick Shift+Tabs that wrap to a row above the viewport move the edit two cells once the row is ready.")]
        public void DoubleShiftTabWrapToRowOutsideViewportMovesTwoCells()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }

                try
                {
                    int firstVisible = OpenFirstVisibleScrollingNameEditor();
                    if (firstVisible < 1) { return; }

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();

                    UIObject editor = UIObject.Focused;
                    Log.Comment("Focus after two Shift+Tabs: {0}.", DescribeFocused());
                    Verify.IsTrue(editor != null && editor.ControlType == ControlType.Edit, "An editor must open on the wrapped row.");
                    if (editor != null && editor.ControlType == ControlType.Edit)
                    {
                        Verify.AreEqual(ScrollingAge(firstVisible - 1).ToString(), new Edit(editor).Value,
                            "Two Shift+Tabs must land on ScrollAge of the row above the viewport.");
                    }
                }
                finally
                {
                    CancelAnyOpenEdit();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a pending Shift+Tab wrap to a row above the viewport does not pull focus back after the app moves it away.")]
        public void ShiftTabWrapOutsideViewportDoesNotPullFocusBack()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }
                var focusAway = FindElement.ById<CheckBox>(FocusAwayOnCommitCheckBox);
                if (focusAway == null) { Verify.Fail("FocusAwayOnCommitCheckBox was not found."); return; }

                try
                {
                    focusAway.Check();
                    Wait.ForIdle();
                    int firstVisible = OpenFirstVisibleScrollingNameEditor();
                    if (firstVisible < 1) { return; }

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();

                    Log.Comment("Focus after Shift+Tab: {0}.", DescribeFocused());
                    Verify.AreEqual(AfterTableButton, UIObject.Focused?.AutomationId, "The pending edit must not pull focus back into the table.");
                }
                finally
                {
                    focusAway.Uncheck();
                    CancelAnyOpenEdit();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Tab wrap to a row below the viewport keeps its editor when the commit changes a live group key, and the regroup runs once the edit closes.")]
        public void TabWrapOutsideViewportKeepsEditorWhenCommitReshapesLiveGroups()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }
                var liveGroup = FindElement.ById<CheckBox>(LiveGroupScrollingCheckBox);
                if (liveGroup == null) { Verify.Fail("LiveGroupScrollingCheckBox was not found."); return; }

                try
                {
                    liveGroup.Check();
                    Wait.ForIdle();
                    if (!SelectPivotItem(ScrollingPivotItem)) { Verify.Fail(GoToButton(ScrollingPivotItem) + " was not found."); return; }

                    UIObject rowsHost = GetRowsHost(ScrollingTable);
                    if (rowsHost == null) { return; }
                    UIObject firstRow = null;
                    foreach (UIObject child in rowsHost.Children)
                    {
                        if (IsRow(child)) { firstRow = child; break; }
                    }
                    if (firstRow == null) { Verify.Fail("Precondition: a realized data row is needed."); return; }

                    firstRow.SetFocus();
                    Wait.ForIdle();
                    Log.Comment("PageDown puts focus on the last fully visible row, so the next data row is below the viewport.");
                    KeyboardHelper.PressKey(Key.PageDown);
                    Wait.ForIdle();
                    if (IsGroupHeader(UIObject.Focused))
                    {
                        KeyboardHelper.PressKey(Key.Up);
                        Wait.ForIdle();
                    }
                    KeyboardHelper.PressKey(Key.Right);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Right);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();

                    UIObject editor = UIObject.Focused;
                    if (editor == null || editor.ControlType != ControlType.Edit) { Verify.Fail("Precondition: F2 must open the ScrollCity editor. Focus: " + DescribeFocused()); return; }
                    string editedCity = new Edit(editor).Value;
                    Log.Comment("Editing ScrollCity '{0}'.", editedCity);

                    KeyboardHelper.PressKey(Key.a, ModifierKey.Control);
                    TextInput.SendText("Zzz");
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    editor = UIObject.Focused;
                    Log.Comment("Focus after Tab: {0}.", DescribeFocused());
                    Verify.IsTrue(editor != null && editor.ControlType == ControlType.Edit, "The live regroup must not close the wrapped edit.");
                    if (editor != null && editor.ControlType == ControlType.Edit)
                    {
                        Verify.IsTrue(new Edit(editor).Value.StartsWith("Scroll ", System.StringComparison.Ordinal), "The editor must be the next data row's FrozenName: " + new Edit(editor).Value);
                    }

                    Log.Comment("Esc closes the edit; the held regroup then moves the edited row into a 'Zzz' group.");
                    KeyboardHelper.PressKey(Key.Escape);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.End, ModifierKey.Control);
                    Wait.ForIdle();

                    bool foundZzz = false;
                    foreach (UIObject child in GetRowsHost(ScrollingTable).Children)
                    {
                        if (IsGroupHeader(child) && child.Name.StartsWith("Zzz", System.StringComparison.Ordinal)) { foundZzz = true; break; }
                    }
                    Verify.IsTrue(foundZzz, "The regroup must run once the edit closes. Focus: " + DescribeFocused());
                }
                finally
                {
                    CancelAnyOpenEdit();
                    liveGroup.Uncheck();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        // Scrolls ScrollingTableView a page and opens the FrozenName editor on the first fully visible row, so a row
        // sits above the viewport. Returns that row's index, or -1 (after failing) when the setup does not hold.
        private static int OpenFirstVisibleScrollingNameEditor()
        {
            if (!SelectPivotItem(ScrollingPivotItem)) { Verify.Fail(GoToButton(ScrollingPivotItem) + " was not found."); return -1; }

            UIObject rowsHost = GetRowsHost(ScrollingTable);
            if (rowsHost == null || rowsHost.Children.Count < 2) { Verify.Fail("Need realized rows."); return -1; }

            rowsHost.Children[0].SetFocus();
            Wait.ForIdle();
            KeyboardHelper.PressKey(Key.PageDown);
            Wait.ForIdle();
            KeyboardHelper.PressKey(Key.PageDown);
            Wait.ForIdle();
            KeyboardHelper.PressKey(Key.PageUp);
            Wait.ForIdle();

            int firstVisible = FocusedPositionInSet() - 1;
            if (firstVisible < 1) { Verify.Fail("Precondition: a row must sit above the viewport (" + firstVisible + ")."); return -1; }

            KeyboardHelper.PressKey(Key.Right);
            Wait.ForIdle();
            KeyboardHelper.PressKey(Key.F2);
            Wait.ForIdle();
            UIObject editor = UIObject.Focused;
            if (editor == null || editor.ControlType != ControlType.Edit) { Verify.Fail("Precondition: F2 must open the FrozenName editor. Focus: " + DescribeFocused()); return -1; }
            Verify.AreEqual(ScrollingName(firstVisible), new Edit(editor).Value, "Precondition: the editor belongs to the first fully visible row.");
            return firstVisible;
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab while editing keeps the editor open and focused when CellEditEnding cancels the commit.")]
        public void TabWhileEditingStaysInEditorWhenCommitVetoed()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var veto = FindElement.ById<CheckBox>(VetoCommitCheckBox);
                if (veto == null) { Verify.Fail("VetoCommitCheckBox was not found."); return; }

                try
                {
                    veto.Check();
                    Wait.ForIdle();

                    UIObject row = GetFirstRow();
                    OpenNameEditorAndReplaceText(row, "Vetoed");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string editReport = ReadEditReport();
                    string endReport = ReadEditEndReport();
                    string itemName = ReadFirstItemName();
                    string focus;
                    bool inEditor = IsFocusInBasicTableEditor(out focus);
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Item Name: '{2}'. Focus: {3}.", editReport, endReport, itemName, focus);

                    Verify.AreEqual("Commit;", endReport, "Tab must attempt a commit, which the page cancels.");
                    Verify.AreEqual("Name;", editReport, "A vetoed commit must not move on to another cell.");
                    Verify.AreEqual(BasicName(0), itemName, "A vetoed commit must not write the value.");
                    Verify.IsTrue(inEditor, "A vetoed commit must leave focus in the current editor.");
                }
                finally
                {
                    veto.Uncheck();
                    Wait.ForIdle();
                    CancelAnyOpenEdit();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+Tab from the first editable cell of a row wraps to the last editable cell of the previous row, still editing.")]
        public void ShiftTabWhileEditingAtStartOfRowWrapsToPreviousRow()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = FindElement.ById(BasicTable);
                UIObject secondRow = tableView != null ? GetRow(tableView, 1) : null;
                if (secondRow == null) { Verify.Fail("The second realized TableViewRow peer was not found."); return; }

                DoubleClickRowAtColumnOffset(secondRow, NameCellRelativeX);
                Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on row 1's Name.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();

                string editReport = ReadEditReport();
                string endReport = ReadEditEndReport();
                string probe = ReadEditorProbe();
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Probe: '{2}'.", editReport, endReport, probe);

                Verify.AreEqual("Commit;", endReport, "Shift+Tab must commit the open edit.");
                Verify.AreEqual("Name;Template;", editReport, "Shift+Tab before the first editable cell must wrap to the previous row's last editable cell.");
                Verify.AreEqual(BasicName(0), ProbeEntryField(probe, 1, 2), "The new editor must belong to row 0, not row 1.");
                Verify.AreEqual("focus=True", ProbeEntryField(probe, 1, 3), "Focus must move into the new editor.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Tab wrap whose new editor loses focus to the app is committed and does not pull focus back.")]
        public void TabWrapDoesNotPullFocusBackAfterFocusMovesAway()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var focusAway = FindElement.ById<CheckBox>(FocusAwayOnCommitCheckBox);
                if (focusAway == null) { Verify.Fail("FocusAwayOnCommitCheckBox was not found."); return; }

                try
                {
                    focusAway.Check();
                    Wait.ForIdle();

                    UIObject row = GetFirstRow();
                    DoubleClickRowAtColumnOffset(row, TemplateCellRelativeX);
                    Verify.AreEqual("Template;", ReadEditReport(), "Precondition: the edit opened on Template.");

                    Log.Comment("Tab wraps to row 1; the page moves focus to AfterTableButton before the wrapped edit can open.");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string editReport = ReadEditReport();
                    string endReport = ReadEditEndReport();
                    string focus = DescribeFocused();
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", editReport, endReport, focus);

                    Verify.AreEqual("Commit;Commit;", endReport, "Tab commits, then focus leaving the new editor commits it too.");
                    Verify.AreEqual("Template;Name;", editReport, "The wrapped row is in view, so its edit opens at once.");
                    Verify.AreEqual(AfterTableButton, UIObject.Focused?.AutomationId, "The wrapped edit must not pull focus back into the table.");
                }
                finally
                {
                    focusAway.Uncheck();
                    Wait.ForIdle();
                    CancelAnyOpenEdit();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab after a commit that re-sorts the edited row edits the same record, not the one now at its old position.")]
        public void TabWhileEditingFollowsRecordResortedByCommit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var sortOnCommit = FindElement.ById<CheckBox>(SortOnCommitCheckBox);
                if (sortOnCommit == null) { Verify.Fail("SortOnCommitCheckBox was not found."); return; }

                try
                {
                    sortOnCommit.Check();
                    Wait.ForIdle();

                    UIObject row = GetFirstRow();
                    DoubleClickRowAtColumnOffset(row, NameCellRelativeX);
                    Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on row 0's Name.");

                    Log.Comment("Tab commits; the commit sorts by Age descending, moving Person 0 from the first row to the last.");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string editReport = ReadEditReport();
                    string endReport = ReadEditEndReport();
                    string probe = ReadEditorProbe();
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Probe: '{2}'.", editReport, endReport, probe);

                    Verify.AreEqual("Commit;", endReport, "Tab must commit the open edit.");
                    Verify.AreEqual("Name;Age;", editReport, "Tab must open the edited record's next editable cell.");
                    Verify.AreEqual(BasicAge(0).ToString(), ProbeEntryField(probe, 1, 2),
                        "The new editor must belong to Person 0, not to the record now at its old position (" +
                        BasicName(BasicIdAtPositionWhenAgeDescending(0)) + ").");
                }
                finally
                {
                    sortOnCommit.Uncheck();
                    Wait.ForIdle();
                    CancelAnyOpenEdit();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Tab wrap whose new editor loses focus to a column header does not pull focus back.")]
        public void TabWrapDoesNotPullFocusBackAfterFocusMovesWithinTable()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var focusHeader = FindElement.ById<CheckBox>(FocusHeaderOnCommitCheckBox);
                if (focusHeader == null) { Verify.Fail("FocusHeaderOnCommitCheckBox was not found."); return; }

                try
                {
                    focusHeader.Check();
                    Wait.ForIdle();

                    UIObject row = GetFirstRow();
                    DoubleClickRowAtColumnOffset(row, TemplateCellRelativeX);
                    Verify.AreEqual("Template;", ReadEditReport(), "Precondition: the edit opened on Template.");

                    Log.Comment("Tab wraps to row 1; the page focuses the Name column header before the wrapped edit can open.");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string editReport = ReadEditReport();
                    string endReport = ReadEditEndReport();
                    string focus;
                    bool inEditor = IsFocusInBasicTableEditor(out focus);
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", editReport, endReport, focus);

                    Verify.AreEqual("Commit;Commit;", endReport, "Tab commits, then focus leaving the new editor commits it too.");
                    Verify.AreEqual("Template;Name;", editReport, "The wrapped row is in view, so its edit opens at once.");
                    Verify.IsFalse(inEditor, "The wrapped edit must not pull focus back from the header.");
                }
                finally
                {
                    focusHeader.Uncheck();
                    Wait.ForIdle();
                    CancelAnyOpenEdit();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Ctrl+Tab while editing is not a cell move.")]
        public void CtrlTabWhileEditingDoesNotMoveTheEdit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject row = GetFirstRow();
                DoubleClickRowAtColumnOffset(row, NameCellRelativeX);
                Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on Name.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Control);
                Wait.ForIdle();

                string editReport = ReadEditReport();
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", editReport, ReadEditEndReport(), DescribeFocused());
                Verify.AreEqual("Name;", editReport, "Ctrl+Tab must not open an edit on another cell.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+Tab wrapping to a row above the viewport scrolls it into view and opens its editor there.")]
        public void ShiftTabWrapToRowOutsideViewportOpensEditorAfterScroll()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }

                try
                {
                    ShiftTabWrapToRowOutsideViewportCore();
                }
                finally
                {
                    CancelAnyOpenEdit();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        private static void ShiftTabWrapToRowOutsideViewportCore()
        {
            if (!SelectPivotItem(ScrollingPivotItem)) { Verify.Fail(GoToButton(ScrollingPivotItem) + " was not found."); return; }

            UIObject rowsHost = GetRowsHost(ScrollingTable);
            if (rowsHost == null) { return; }
            if (rowsHost.Children.Count < 2) { Verify.Fail("Need realized rows."); return; }

            Log.Comment("PageDown twice scrolls a page; PageUp then lands on the first fully visible row without scrolling.");
            rowsHost.Children[0].SetFocus();
            Wait.ForIdle();
            KeyboardHelper.PressKey(Key.PageDown);
            Wait.ForIdle();
            KeyboardHelper.PressKey(Key.PageDown);
            Wait.ForIdle();
            KeyboardHelper.PressKey(Key.PageUp);
            Wait.ForIdle();

            int firstVisible = FocusedPositionInSet() - 1;
            if (firstVisible < 1) { Verify.Fail("Precondition: the table must be scrolled so a row sits above the viewport (" + firstVisible + ")."); return; }
            int target = firstVisible - 1;

            KeyboardHelper.PressKey(Key.Right);
            Wait.ForIdle();
            Verify.AreEqual(ScrollingName(firstVisible), UIObject.Focused?.Name,
                "Precondition: Right must drill into the FrozenName cell of the first fully visible row. Focus: " + DescribeFocused());
            KeyboardHelper.PressKey(Key.F2);
            Wait.ForIdle();

            UIObject editor = UIObject.Focused;
            if (editor == null || editor.ControlType != ControlType.Edit) { Verify.Fail("Precondition: F2 must open the FrozenName editor. Focus: " + DescribeFocused()); return; }
            Verify.AreEqual(ScrollingName(firstVisible), new Edit(editor).Value, "Precondition: the editor belongs to the first fully visible row.");

            Log.Comment("Shift+Tab wraps to row {0}'s last editable cell (ScrollId), above the viewport.", target);
            KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
            Wait.ForIdle();

            editor = UIObject.Focused;
            Log.Comment("Focus after Shift+Tab: {0}.", DescribeFocused());
            Verify.IsTrue(editor != null && editor.ControlType == ControlType.Edit, "An editor must open on the wrapped row once it scrolls into view.");
            if (editor != null && editor.ControlType == ControlType.Edit)
            {
                Verify.AreEqual(target.ToString(), new Edit(editor).Value, "The editor must belong to the row above the viewport.");
            }
        }

        // Scrolling, Grouped and Rtl default to IsReadOnly; the page toggle lets them edit. Null (after failing) if missing.
        private static CheckBox EnableSecondaryTableEditing()
        {
            var allowEditing = FindElement.ById<CheckBox>(AllowSecondaryTableEditingCheckBox);
            if (allowEditing == null)
            {
                Verify.Fail("AllowSecondaryTableEditingCheckBox was not found.");
                return null;
            }

            allowEditing.Check();
            Wait.ForIdle();
            return allowEditing;
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab onto a cell whose BeginningEdit is cancelled ends the move there instead of skipping ahead.")]
        public void TabStopsOnCellWhoseBeginningEditIsCancelled()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var cancelAge = FindElement.ById<CheckBox>(CancelAgeBeginEditCheckBox);
                if (cancelAge == null) { Verify.Fail("CancelAgeBeginEditCheckBox was not found."); return; }

                try
                {
                    cancelAge.Check();
                    Wait.ForIdle();

                    UIObject row = GetFirstRow();
                    DoubleClickRowAtColumnOffset(row, NameCellRelativeX);
                    Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on Name.");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string editReport = ReadEditReport();
                    string endReport = ReadEditEndReport();
                    string focus;
                    bool inEditor = IsFocusInBasicTableEditor(out focus);
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", editReport, endReport, focus);

                    Verify.AreEqual("Commit;", endReport, "Tab must commit the open edit.");
                    Verify.AreEqual("Name;Age;", editReport, "The move must end on Age when its BeginningEdit is cancelled, not continue to Score.");
                    Verify.IsFalse(inEditor, "No editor may be open after the refused edit.");
                }
                finally
                {
                    cancelAge.Uncheck();
                    Wait.ForIdle();
                    CancelAnyOpenEdit();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+Tab wrapping across a group header lands on the previous group's last data row, still editing.")]
        public void ShiftTabWrapSkipsGroupHeader()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                CheckBox allowEditing = EnableSecondaryTableEditing();
                if (allowEditing == null) { return; }

                try
                {
                    ShiftTabWrapSkipsGroupHeaderCore();
                }
                finally
                {
                    CancelAnyOpenEdit();
                    allowEditing.Uncheck();
                    Wait.ForIdle();
                }
            }
        }

        private static void ShiftTabWrapSkipsGroupHeaderCore()
        {
            if (!SelectPivotItem(GroupedPivotItem)) { Verify.Fail(GoToButton(GroupedPivotItem) + " was not found."); return; }

            UIObject rowsHost = GetRowsHost(GroupedTable);
            if (rowsHost == null) { return; }

            int secondHeader = -1;
            int headersSeen = 0;
            for (int i = 0; i < rowsHost.Children.Count; i++)
            {
                if (IsGroupHeader(rowsHost.Children[i]) && ++headersSeen == 2)
                {
                    secondHeader = i;
                    break;
                }
            }
            if (secondHeader < 1 || secondHeader + 1 >= rowsHost.Children.Count ||
                !IsRow(rowsHost.Children[secondHeader - 1]) || !IsRow(rowsHost.Children[secondHeader + 1]))
            {
                Verify.Fail("Precondition: a second group header with data rows on both sides is needed (" + secondHeader + ").");
                return;
            }

            UIObject previousRow = rowsHost.Children[secondHeader - 1];
            string expectedCity = previousRow.Children.Count > 1 ? previousRow.Children[1].Name : null;
            if (string.IsNullOrEmpty(expectedCity)) { Verify.Fail("Precondition: the previous group's last row must expose its GroupedCity cell."); return; }

            DoubleClickRowAtColumnOffset(rowsHost.Children[secondHeader + 1], 100);
            UIObject editor = UIObject.Focused;
            if (editor == null || editor.ControlType != ControlType.Edit) { Verify.Fail("Precondition: double-click must open the GroupedName editor. Focus: " + DescribeFocused()); return; }

            Log.Comment("Shift+Tab from the second group's first GroupedName must skip the header and edit GroupedCity above it.");
            KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
            Wait.ForIdle();

            editor = UIObject.Focused;
            Log.Comment("Focus after Shift+Tab: {0}.", DescribeFocused());
            Verify.IsTrue(editor != null && editor.ControlType == ControlType.Edit, "An editor must open past the group header.");
            if (editor != null && editor.ControlType == ControlType.Edit)
            {
                Verify.AreEqual(expectedCity, new Edit(editor).Value, "The editor must belong to the previous group's last row.");
            }
        }

        #endregion

        #region Helpers

        // Row-relative x inside the Name column (0-160), which is editable.
        private static readonly int NameCellRelativeX = BasicColumnCentreX(NameColumn);

        // Row-relative x inside the Age column (160-260), which is editable.
        private static readonly int AgeCellRelativeX = BasicColumnCentreX(AgeColumn);

        // Row-relative x inside the ReadOnlyCity column (260-420), authored IsReadOnly="True".
        private static readonly int ReadOnlyCellRelativeX = BasicColumnCentreX(ReadOnlyCityColumn);

        // Row-relative x inside the Template column (520-720), which authors a CellEditingTemplate.
        private static readonly int TemplateCellRelativeX = BasicColumnCentreX(TemplateColumn);

        private static readonly int ScoreCellRelativeX = BasicColumnCentreX(ScoreColumn);

        private static UIObject GetFirstRow()
        {
            UIObject tableView = FindElement.ById(BasicTable);
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

        // Field of the entry'th probe entry (0-based), or a diagnostic string the assertion will print.
        private static string ProbeEntryField(string probe, int entry, int field)
        {
            string[] entries = (probe ?? string.Empty).Split(new[] { ';' }, System.StringSplitOptions.RemoveEmptyEntries);
            if (entry >= entries.Length)
            {
                return "<no probe entry " + entry + ": '" + probe + "'>";
            }

            string[] fields = entries[entry].Split('|');
            return field < fields.Length ? fields[field] : "<probe entry has no field " + field + ": '" + entries[entry] + "'>";
        }

        // True when UIA focus is on an Edit inside BasicTableView, i.e. in an open cell editor.
        private static bool IsFocusInBasicTableEditor(out string description)
        {
            description = DescribeFocused();
            UIObject focused = UIObject.Focused;
            if (focused == null || focused.ControlType != ControlType.Edit)
            {
                return false;
            }

            UIObject ancestor = focused.Parent;
            for (int depth = 0; ancestor != null && depth < 16; depth++, ancestor = ancestor.Parent)
            {
                if (ancestor.AutomationId == BasicTable)
                {
                    return true;
                }
            }

            return false;
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

        #endregion
    }
}
