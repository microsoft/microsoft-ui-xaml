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
using System;
using System.Globalization;
using System.Linq;

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

        // BasicTableView columns: Name, Age, ReadOnlyCity (IsReadOnly), Score, Template (editing template),
        // Action (not editable). Tab commits and edits the next editable visible cell, Shift+Tab the previous one.

        [TestMethod]
        [TestProperty("Description", "Verifies Tab and Shift+Tab commit and edit the next and previous editable cell in the same row.")]
        public void TabAndShiftTabWhileEditingMoveWithinRow()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenNameEditorAndReplaceText(GetFirstRow(), "Tabbed");
                Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on Name.");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
                string probe = ReadEditorProbe();
                Log.Comment("Edit report: '{0}'. Probe: '{1}'.", ReadEditReport(), probe);
                Verify.AreEqual("Commit;", ReadEditEndReport(), "Tab must commit the open edit.");
                Verify.AreEqual("Tabbed", ReadFirstItemName(), "Tab must write the editor's value through to the item.");
                Verify.AreEqual("Name;Age;", ReadEditReport(), "Tab must edit the next editable cell.");
                Verify.AreEqual(BasicAge(0).ToString(), ProbeEntryField(probe, 1, 2), "The new editor must belong to row 0's Age.");
                Verify.AreEqual("focus=True", ProbeEntryField(probe, 1, 3), "Focus must move into the new editor.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();
                Verify.AreEqual("Name;Age;Name;", ReadEditReport(), "Shift+Tab must edit the previous editable cell.");
                Verify.AreEqual("Commit;Commit;", ReadEditEndReport(), "Each move must commit the cell it leaves.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab and Shift+Tab while editing skip read-only and collapsed columns.")]
        public void TabWhileEditingSkipsReadOnlyAndHiddenColumns()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var hideScore = FindElement.ById<CheckBox>(HideScoreColumnCheckBox);
                if (hideScore == null) { Verify.Fail("HideScoreColumnCheckBox was not found."); return; }

                try
                {
                    hideScore.Check();
                    Wait.ForIdle();

                    DoubleClickRowAtColumnOffset(GetFirstRow(), AgeCellRelativeX);
                    Verify.AreEqual("Age;", ReadEditReport(), "Precondition: the edit opened on Age.");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.AreEqual("Age;Template;", ReadEditReport(), "Tab must skip ReadOnlyCity and the collapsed Score.");

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.AreEqual("Age;Template;Age;", ReadEditReport(), "Shift+Tab must skip them too.");
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
        [TestProperty("Description", "Verifies Tab past a row's last editable cell wraps to the next row, and Shift+Tab wraps back.")]
        public void TabWhileEditingWrapsRowsBothWays()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                DoubleClickRowAtColumnOffset(GetFirstRow(), TemplateCellRelativeX);
                Verify.AreEqual("Template;", ReadEditReport(), "Precondition: the edit opened on Template.");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
                string probe = ReadEditorProbe();
                Log.Comment("Edit report: '{0}'. Probe: '{1}'.", ReadEditReport(), probe);
                Verify.AreEqual("Template;Name;", ReadEditReport(), "Tab must wrap to the next row's first editable cell.");
                Verify.AreEqual(BasicName(1), ProbeEntryField(probe, 1, 2), "The new editor must belong to row 1.");
                Verify.AreEqual("focus=True", ProbeEntryField(probe, 1, 3), "Focus must move into the new editor.");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();
                probe = ReadEditorProbe();
                Log.Comment("Edit report: '{0}'. Probe: '{1}'.", ReadEditReport(), probe);
                Verify.AreEqual("Template;Name;Template;", ReadEditReport(), "Shift+Tab must wrap to the previous row's last editable cell.");
                Verify.AreEqual(BasicName(0), ProbeEntryField(probe, 2, 2), "The new editor must belong to row 0.");
                Verify.AreEqual("Commit;Commit;", ReadEditEndReport(), "Each move must commit the cell it leaves.");

                CancelAnyOpenEdit();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Shift+Tab wrapping across a group header edits the previous group's last data row.")]
        public void ShiftTabWrapSkipsGroupHeader()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                var allowEditing = FindElement.ById<CheckBox>(AllowGroupedEditingCheckBox);
                if (allowEditing == null) { Verify.Fail("AllowGroupedEditingCheckBox was not found."); return; }

                try
                {
                    allowEditing.Check();
                    Wait.ForIdle();
                    if (!SelectPivotItem(GroupedPivotItem)) { Verify.Fail(GoToButton(GroupedPivotItem) + " was not found."); return; }

                    UIObject rowsHost = GetRowsHost(GroupedTable);
                    if (rowsHost == null) { return; }

                    int secondHeader = -1;
                    for (int i = 0, headers = 0; i < rowsHost.Children.Count && secondHeader < 0; i++)
                    {
                        if (IsGroupHeader(rowsHost.Children[i]) && ++headers == 2) { secondHeader = i; }
                    }
                    if (secondHeader < 1 || secondHeader + 1 >= rowsHost.Children.Count ||
                        !IsRow(rowsHost.Children[secondHeader - 1]) || !IsRow(rowsHost.Children[secondHeader + 1]))
                    {
                        Verify.Fail("Precondition: a second group header with data rows on both sides is needed (" + secondHeader + ").");
                        return;
                    }

                    UIObject previousRow = rowsHost.Children[secondHeader - 1];
                    string expectedCity = previousRow.Children.Count > 1 ? previousRow.Children[1].Name : null;
                    if (string.IsNullOrEmpty(expectedCity)) { Verify.Fail("Precondition: the previous row must expose its GroupedCity cell."); return; }

                    DoubleClickRowAtColumnOffset(rowsHost.Children[secondHeader + 1], 100);
                    if (UIObject.Focused?.ControlType != ControlType.Edit) { Verify.Fail("Precondition: double-click must open the GroupedName editor. Focus: " + DescribeFocused()); return; }

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();

                    UIObject editor = UIObject.Focused;
                    Log.Comment("Focus after Shift+Tab: {0}.", DescribeFocused());
                    Verify.AreEqual(ControlType.Edit, editor?.ControlType, "An editor must open past the group header.");
                    Verify.AreEqual(expectedCity, editor != null ? new Edit(editor).Value : null, "The editor must belong to the previous group's last row.");
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
        [TestProperty("Description", "Verifies Shift+Tab from the first editable cell of the first row commits and leaves the editor.")]
        public void ShiftTabWhileEditingFirstCellOfFirstRowExitsEdit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenNameEditorAndReplaceText(GetFirstRow(), "ShiftTabbed");

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();

                string focus;
                bool inEditor = IsFocusInBasicTableEditor(out focus);
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", ReadEditReport(), ReadEditEndReport(), focus);
                Verify.AreEqual("Commit;", ReadEditEndReport(), "Shift+Tab must commit.");
                Verify.AreEqual("ShiftTabbed", ReadFirstItemName(), "The commit must write the value.");
                Verify.AreEqual("Name;", ReadEditReport(), "No new edit may open.");
                Verify.IsFalse(inEditor, "Focus must leave the editor.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab from the last editable cell of the last row commits and leaves the editor.")]
        public void TabWhileEditingLastCellOfLastRowExitsEdit()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!DrillIntoRow(BasicTable, 0)) { return; }
                KeyboardHelper.PressKey(Key.End, ModifierKey.Control);
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.F2);
                Wait.ForIdle();
                Verify.AreEqual("Template;", ReadEditReport(), "Precondition: F2 opened the last row's Template editor.");
                Verify.AreEqual(BasicName(BasicItemCount - 1), ProbeEntryField(ReadEditorProbe(), 0, 2), "Precondition: the editor belongs to the last row.");

                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                string focus;
                bool inEditor = IsFocusInBasicTableEditor(out focus);
                Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", ReadEditReport(), ReadEditEndReport(), focus);
                Verify.AreEqual("Commit;", ReadEditEndReport(), "Tab must commit.");
                Verify.AreEqual("Template;", ReadEditReport(), "No new edit may open.");
                Verify.IsFalse(inEditor, "Focus must leave the editor.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tab keeps the editor open and focused when CellEditEnding cancels the commit.")]
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
                    OpenNameEditorAndReplaceText(GetFirstRow(), "Vetoed");

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string focus;
                    bool inEditor = IsFocusInBasicTableEditor(out focus);
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", ReadEditReport(), ReadEditEndReport(), focus);
                    Verify.AreEqual("Commit;", ReadEditEndReport(), "Tab must attempt the commit.");
                    Verify.AreEqual("Name;", ReadEditReport(), "A vetoed commit must not move the edit.");
                    Verify.AreEqual(BasicName(0), ReadFirstItemName(), "A vetoed commit must not write the value.");
                    Verify.IsTrue(inEditor, "Focus must stay in the editor.");
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
        [TestProperty("Description", "Verifies Tab moves within a multi-control editor before moving the edit, and Ctrl+Tab never moves the edit.")]
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

                    KeyboardHelper.PressKey(Key.End);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.F2);
                    Wait.ForIdle();
                    Verify.AreEqual("Pair;", ReadEditReport(), "Precondition: F2 opened an edit on Pair.");
                    Verify.AreEqual("PairFirstEditor", UIObject.Focused?.AutomationId, "Precondition: the first box has focus. Focus: " + DescribeFocused());

                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.AreEqual("PairSecondEditor", UIObject.Focused?.AutomationId, "Tab must reach the second box. Focus: " + DescribeFocused());
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.AreEqual("PairFirstEditor", UIObject.Focused?.AutomationId, "Shift+Tab must reach the first box. Focus: " + DescribeFocused());
                    Verify.AreEqual(string.Empty, ReadEditEndReport(), "Moving within the editor must not commit.");

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.AreEqual("Pair;Template;", ReadEditReport(), "Shift+Tab from the first box must commit and edit Template.");
                    Verify.AreEqual("Commit;", ReadEditEndReport(), "Leaving Pair must commit it once.");

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Control);
                    Wait.ForIdle();
                    Log.Comment("Edit report after Ctrl+Tab: '{0}'. Focus: {1}.", ReadEditReport(), DescribeFocused());
                    Verify.AreEqual("Pair;Template;", ReadEditReport(), "Ctrl+Tab must not move the edit.");
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
        [TestProperty("Description", "Verifies Shift+Tab wrapping to a row above the viewport scrolls it into view and opens its editor.")]
        public void ShiftTabWrapToRowOutsideViewportOpensEditorAfterScroll()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                if (!SelectPivotItem(ScrollingPivotItem)) { Verify.Fail(GoToButton(ScrollingPivotItem) + " was not found."); return; }

                UIObject rowsHost = GetRowsHost(ScrollingTable);
                if (rowsHost == null || rowsHost.Children.Count < 2) { Verify.Fail("Need realized rows."); return; }

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
                if (firstVisible < 1) { Verify.Fail("Precondition: a row must sit above the viewport (" + firstVisible + ")."); return; }

                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.F2);
                Wait.ForIdle();
                UIObject editor = UIObject.Focused;
                if (editor?.ControlType != ControlType.Edit) { Verify.Fail("Precondition: F2 must open the FrozenName editor. Focus: " + DescribeFocused()); return; }
                Verify.AreEqual(ScrollingName(firstVisible), new Edit(editor).Value, "Precondition: the editor belongs to the first fully visible row.");

                Log.Comment("Shift+Tab wraps to row {0}'s ScrollId, above the viewport.", firstVisible - 1);
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Wait.ForIdle();

                editor = UIObject.Focused;
                Log.Comment("Focus after Shift+Tab: {0}.", DescribeFocused());
                Verify.AreEqual(ControlType.Edit, editor?.ControlType, "An editor must open once the row scrolls into view.");
                Verify.AreEqual((firstVisible - 1).ToString(), editor != null ? new Edit(editor).Value : null, "The editor must belong to the row above the viewport.");

                CancelAnyOpenEdit();
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
                    DoubleClickRowAtColumnOffset(GetFirstRow(), NameCellRelativeX);
                    Verify.AreEqual("Name;", ReadEditReport(), "Precondition: the edit opened on row 0's Name.");

                    Log.Comment("The commit sorts by Age descending, moving Person 0 from the first row to the last.");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    string probe = ReadEditorProbe();
                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Probe: '{2}'.", ReadEditReport(), ReadEditEndReport(), probe);
                    Verify.AreEqual("Name;Age;", ReadEditReport(), "Tab must edit the record's next editable cell.");
                    Verify.AreEqual(BasicAge(0).ToString(), ProbeEntryField(probe, 1, 2),
                        "The new editor must belong to Person 0, not " + BasicName(BasicIdAtPositionWhenAgeDescending(0)) + ".");
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
        [TestProperty("Description", "Verifies Tab does not pull focus back when a CellEditEnding handler focuses a column header.")]
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

                    Log.Comment("Edit report: '{0}'. End report: '{1}'. Focus: {2}.", ReadEditReport(), ReadEditEndReport(), DescribeFocused());
                    Verify.AreEqual("Commit;", ReadEditEndReport(), "Tab must commit the open edit.");
                    Verify.AreEqual("Name;", ReadEditReport(), "Focus left the record, so no edit may open on Age.");
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

        // The probe writes one entry per opened edit, "<type>|<automationName>|<text>|focus=<bool>|align=<...>;".
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

        // GH #12354: the text must stay put when a real gesture opens the built-in editor, which then shows its
        // Focused and ButtonVisible states.
        [TestMethod]
        [TestProperty("Description", "Verifies a double-clicked, focused, non-empty editor draws its text line where the display text drew it, LTR and RTL.")]
        public void DoubleClickEditorKeepsTextInPlace()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                VerifyEditorAlignment(rtl: false);

                var toggle = FindElement.ById<Button>(ToggleBasicFlowDirectionButton);
                if (toggle == null) { Verify.Fail("ToggleBasicFlowDirectionButton was not found on the test page."); return; }
                toggle.InvokeAndWait();

                VerifyEditorAlignment(rtl: true);
                toggle.InvokeAndWait();
            }
        }

        private static void VerifyEditorAlignment(bool rtl)
        {
            string context = rtl ? "RTL" : "LTR";
            UIObject row = GetRow(FindElement.ById(BasicTable), 0);
            if (row == null) { Verify.Fail(context + ": the first row was not found."); return; }

            int nameX = BasicColumnCentreX(NameColumn);
            var bounds = row.BoundingRectangle;
            DoubleClickPoint(new Point(rtl ? bounds.Right - nameX : bounds.Left + nameX, bounds.Top + (bounds.Height / 2)));

            string[] entries = ReadEditorProbe().Split(new[] { ';' }, StringSplitOptions.RemoveEmptyEntries);
            string[] fields = entries.Length > 0 ? entries[entries.Length - 1].Split('|') : new string[0];
            Log.Comment("{0}: editor probe entry '{1}'.", context, string.Join("|", fields));
            Verify.IsTrue(fields.Length >= 5, context + ": the editor probe did not report an alignment.");
            if (fields.Length < 5) { return; }

            Verify.AreEqual(BasicName(0), fields[2], context + ": the editor must open on the Name cell with its non-empty value.");
            Verify.AreEqual("focus=True", fields[3], context + ": the editor must be focused.");

            const string alignPrefix = "align=dx:";
            Verify.IsTrue(fields[4].StartsWith(alignPrefix, StringComparison.Ordinal), context + ": the probe found no display cell to compare: '" + fields[4] + "'.");
            if (!fields[4].StartsWith(alignPrefix, StringComparison.Ordinal)) { return; }

            var align = fields[4].Substring("align=".Length).Split(',')
                .Select(p => p.Split(':'))
                .Where(p => p.Length == 2)
                .ToDictionary(p => p[0], p => p[1]);
            if (!align.TryGetValue("dx", out var dxText) || !align.TryGetValue("dy", out var dyText) || !align.TryGetValue("lines", out var lines))
            {
                Verify.Fail(context + ": the probe alignment is missing dx, dy or lines: '" + fields[4] + "'.");
                return;
            }
            double dx = double.Parse(dxText, CultureInfo.InvariantCulture);
            double dy = double.Parse(dyText, CultureInfo.InvariantCulture);
            Log.Comment("{0}: dx={1} dy={2} lines(display/editor)={3}", context, dx, dy, lines);
            Verify.IsLessThanOrEqual(Math.Abs(dx), 0.5, context + ": the editor text must not move horizontally.");
            Verify.IsLessThanOrEqual(Math.Abs(dy), 0.5, context + ": the editor text must not move vertically.");

            KeyboardHelper.PressKey(Key.Escape);
            Wait.ForIdle();
        }
    }
}
