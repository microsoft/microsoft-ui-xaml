// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewEditingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 10.1 of the TableView test plan: editing gates.
    //
    // Spec basis: TableView.idl:493-530. IsReadOnly is "[MUX_DEFAULT_VALUE(true)]" and editing is
    // "opt-in: while true (the default) no cell can be edited, whatever the column says". IsEditing is
    // "true while an edit is in flight on a cell", and explicitly "also covers the brief windows where
    // an edit is still opening and where a close is already underway".
    //
    // How these tests reach editing without a gesture: the IDL says there is no programmatic BeginEdit,
    // but TableViewCellAutomationPeer implements IValueProvider and its SetValue drives the real edit
    // lifecycle (BeginEdit -> write -> CommitEdit) precisely so that "a programmatic set must not be
    // able to do what a user cannot". The Value pattern is advertised only when editing is actually
    // possible, so the gates below are observable on the peer without opening an edit at all.
    //
    // NOT IMPLEMENTED HERE:
    //   VerifyEditingDefaultsAreReadOnly - TableViewTests.cs:130-131 asserts IsReadOnly == true and
    //                                   IsEditing == false on a fresh control, :160-161 asserts
    //                                   TableViewColumn.IsReadOnly == false and CellEditingTemplate
    //                                   == null, and :246/:272-273/:323-325/:388-393 cover the DP
    //                                   statics and settable roundtrips.
    //   Gesture-driven editor tests    - double-click, F2, Enter, Esc, focus loss, editor focus and the
    //                                   editor's initial value are all in section 11. SetValue
    //                                   overwrites the initial value inside the same call, so no API
    //                                   test can observe it.
    [TestClass]
    public class TableViewEditingGateTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a read-only TableView advertises no Value pattern and refuses SetValue, leaving the item untouched.")]
        public void VerifyReadOnlyTableViewOffersNoValuePatternAndRejectsEditing()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            AutomationPeer cellPeer = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);

                // Deliberately left at the default. The IDL calls this the opt-in gate.
                Verify.IsTrue(tableView.IsReadOnly, "The test relies on IsReadOnly defaulting to true.");

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                cellPeer = GetCellPeer(tableView, items[0], columnIndex: 0);

                var pattern = cellPeer.GetPattern(PatternInterface.Value);
                Verify.IsTrue(pattern == null, "A read-only TableView must not advertise the Value pattern on its cells.");

                // The peer still implements IValueProvider; it reports itself read-only and rejects
                // the call. Assistive technology that asks anyway must be told no, not silently fail.
                var provider = (IValueProvider)cellPeer;
                Verify.IsTrue(provider.IsReadOnly, "IValueProvider.IsReadOnly must be true while the TableView is read-only.");

                var error = CaptureSetValue(provider, "Rejected");
                Verify.IsTrue(error != null, "SetValue must throw on a read-only cell rather than editing it.");
                Log.Comment($"SetValue threw as expected: {error.Message}");

                Verify.AreEqual("Asha", items[0].Name, "A rejected SetValue must not change the bound item.");
                Verify.IsFalse(tableView.IsEditing, "A rejected SetValue must not leave an edit open.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a writable TableView with a writable column advertises the Value pattern.")]
        public void VerifyWritableTableViewAndColumnOfferValuePattern()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;

                Verify.IsFalse(tableView.Columns[0].IsReadOnly, "Columns are writable by default.");

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var cellPeer = GetCellPeer(tableView, items[0], columnIndex: 0);

                var provider = cellPeer.GetPattern(PatternInterface.Value) as IValueProvider;
                Verify.IsTrue(provider != null, "A writable cell must advertise the Value pattern so assistive technology can edit it.");
                Verify.IsFalse(provider.IsReadOnly, "IValueProvider.IsReadOnly must be false once the app has opted in to editing.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a read-only column blocks only its own cells while sibling columns stay editable.")]
        public void VerifyPerColumnReadOnlyBlocksOnlyThatColumn()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                tableView.Columns[0].IsReadOnly = true;

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var lockedPeer = GetCellPeer(tableView, items[0], columnIndex: 0);
                Verify.IsTrue(
                    lockedPeer.GetPattern(PatternInterface.Value) == null,
                    "A cell in a read-only column must not advertise the Value pattern.");

                var lockedError = CaptureSetValue((IValueProvider)lockedPeer, "Blocked");
                Verify.IsTrue(lockedError != null, "SetValue on a read-only column must throw.");
                Verify.AreEqual("Asha", items[0].Name, "A read-only column must not let a value reach the item.");

                var openPeer = GetCellPeer(tableView, items[0], columnIndex: 1);
                var openProvider = openPeer.GetPattern(PatternInterface.Value) as IValueProvider;
                Verify.IsTrue(openProvider != null, "A writable sibling column must stay editable when another column is read-only.");

                openProvider.SetValue("Principal");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Principal", items[0].Role, "The writable column's edit must reach the item.");
                Verify.AreEqual("Asha", items[0].Name, "Editing one column must not disturb another.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a column carrying a CellEditingTemplate does not advertise the text Value pattern.")]
        public void VerifyColumnWithCellEditingTemplateOffersNoValuePattern()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;

                // A custom editor replaces the built-in TextBox, so a text-valued SetValue has nothing
                // it can reliably write. Advertising the pattern would open an edit on screen and only
                // then fail.
                tableView.Columns[0].CellEditingTemplate = MakeCustomEditorTemplate();

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var cellPeer = GetCellPeer(tableView, items[0], columnIndex: 0);

                Verify.IsTrue(
                    cellPeer.GetPattern(PatternInterface.Value) == null,
                    "A column with a CellEditingTemplate must not advertise the text Value pattern.");
                Verify.IsTrue(
                    ((IValueProvider)cellPeer).IsReadOnly,
                    "The peer must report itself read-only when it cannot set a text value.");

                // The column is still editable by gesture - only the text-value contract is withheld.
                Verify.IsFalse(tableView.Columns[0].IsReadOnly, "The column itself is still writable.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies IsEditing is false before an edit, true while one is closing, and false once it has closed.")]
        public void VerifyIsEditingIsTrueOnlyWhileAnEditIsOpen()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            bool? isEditingInsideHandler = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;

                tableView.CellEditEnding += (s, e) =>
                {
                    isEditingInsideHandler = tableView.IsEditing;
                };

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, "IsEditing must be false before any edit starts.");

                var provider = GetValueProvider(tableView, items[0], columnIndex: 0);
                provider.SetValue("Asha Renamed");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(isEditingInsideHandler.HasValue, "CellEditEnding must have been raised.");
                Verify.IsTrue(
                    isEditingInsideHandler.Value,
                    "IsEditing must be true inside CellEditEnding - the IDL says it covers the window where a close is already underway.");
                Verify.IsFalse(tableView.IsEditing, "IsEditing must return to false once the edit has closed.");
            });
        }
    }

    // Category 10.3 of the TableView test plan: commit and cancel.
    //
    // Spec basis: TableView.idl:493-530. CommitEdit and CancelEdit are cell-scoped and return Boolean -
    // "False when it did not close: a veto or failed validation leaves the edit open and IsEditing
    // true." CellEditEnded reports the close once it has happened.
    [TestClass]
    public class TableViewEditCommitTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a committed edit reaches the bound item, the peer's Value, and the displayed cell text.")]
        public void VerifySetValueCommitsThroughToTheBoundItem()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha", items[0].Name, "Pre-edit value.");

                var provider = GetValueProvider(tableView, items[0], columnIndex: 0);
                provider.SetValue("Asha Kapoor");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha Kapoor", items[0].Name, "A committed edit must write the new value to the bound item property.");
                Verify.IsFalse(tableView.IsEditing, "A successful commit must close the edit.");

                var provider = GetValueProvider(tableView, items[0], columnIndex: 0);
                Verify.AreEqual("Asha Kapoor", provider.Value, "IValueProvider.Value must report the committed value.");

                var texts = GetCellTexts(tableView, items[0]);
                Verify.IsTrue(
                    texts.Contains("Asha Kapoor"),
                    $"The displayed cell must show the committed value. Saw: {string.Join(" | ", texts)}");

                Verify.AreEqual("Diego", items[1].Name, "Editing one row must not touch another.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CommitEdit and CancelEdit return false and raise nothing when no edit is open, in both read-only and writable configurations.")]
        public void VerifyCommitAndCancelEditReturnFalseWhenNoEditIsOpen()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            int beginningEditCount = 0;
            int cellEditEndingCount = 0;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.BeginningEdit += (s, e) => beginningEditCount++;
                tableView.CellEditEnding += (s, e) => cellEditEndingCount++;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Read-only configuration.
                Verify.IsFalse(tableView.CommitEdit(), "CommitEdit must return false when no edit is open (read-only).");
                Verify.IsFalse(tableView.CancelEdit(), "CancelEdit must return false when no edit is open (read-only).");

                // Writable configuration - still nothing open.
                tableView.IsReadOnly = false;
                Verify.IsFalse(tableView.CommitEdit(), "CommitEdit must return false when no edit is open (writable).");
                Verify.IsFalse(tableView.CancelEdit(), "CancelEdit must return false when no edit is open (writable).");

                // Calling twice in a row must stay stable - an app may commit defensively before navigating.
                Verify.IsFalse(tableView.CommitEdit(), "A repeated CommitEdit must stay false.");

                Verify.IsFalse(tableView.IsEditing, "Closing nothing must not open anything.");
                Verify.AreEqual(0, beginningEditCount, "No edit was started, so BeginningEdit must not fire.");
                Verify.AreEqual(0, cellEditEndingCount, "No edit was open, so CellEditEnding must not fire.");
                Verify.AreEqual("Asha", items[0].Name, "No item value may change.");
            });
        }
    }

    // Category 10.4 of the TableView test plan: editing events.
    //
    // Spec basis: TableView.idl:274-290. TableViewBeginningEditEventArgs carries Item, Column and a
    // settable Cancel; TableViewCellEditEndingEventArgs carries Item, Column, EditAction and a settable
    // Cancel. TableViewEditAction (:59-63) has exactly two values, Commit = 0 and Cancel = 1. The IDL
    // states that a veto "leaves the edit open and IsEditing true".
    [TestClass]
    public class TableViewEditEventTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies BeginningEdit reports the exact item and column instances being edited.")]
        public void VerifyBeginningEditReportsItemAndColumn()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            int callCount = 0;
            object reportedItem = null;
            TableViewColumn reportedColumn = null;
            bool cancelDefault = true;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;

                tableView.BeginningEdit += (s, e) =>
                {
                    callCount++;
                    reportedItem = e.Item;
                    reportedColumn = e.Column;
                    cancelDefault = e.Cancel;
                };

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var provider = GetValueProvider(tableView, items[2], columnIndex: 1);
                provider.SetValue("Lead Architect");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, callCount, "BeginningEdit must fire exactly once for one edit.");
                Verify.IsFalse(cancelDefault, "BeginningEdit.Cancel must default to false, so an app that ignores the event does not block editing.");

                // Reference identity, not equality: the IDL calls SelectedItem "the selected data item",
                // and the same rule applies here. A wrapper would break every app that keys off the item.
                Verify.AreSame(items[2], reportedItem, "BeginningEdit.Item must be the data item itself, not a wrapper.");
                Verify.AreSame(tableView.Columns[1], reportedColumn, "BeginningEdit.Column must be the column object being edited.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies cancelling BeginningEdit blocks the edit entirely: nothing opens, nothing is written, and CellEditEnding does not fire.")]
        public void VerifyBeginningEditCancelPreventsTheEdit()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            int endingCount = 0;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;

                tableView.BeginningEdit += (s, e) => e.Cancel = true;
                tableView.CellEditEnding += (s, e) => endingCount++;

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var provider = GetValueProvider(tableView, items[0], columnIndex: 0);

                var error = CaptureSetValue(provider, "Should not stick");
                Verify.IsTrue(error != null, "A vetoed edit must report failure rather than silently succeeding.");
                Log.Comment($"SetValue threw as expected: {error.Message}");

                Verify.AreEqual("Asha", items[0].Name, "A vetoed edit must not write to the item.");
                Verify.IsFalse(tableView.IsEditing, "A vetoed edit must leave nothing open.");
                Verify.AreEqual(0, endingCount, "Nothing opened, so no edit can end: CellEditEnding must not fire.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a successful commit raises CellEditEnding once with the Commit action and the edited item and column.")]
        public void VerifyCellEditEndingReportsCommitAction()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            int callCount = 0;
            TableViewEditAction reportedAction = TableViewEditAction.Cancel;
            object reportedItem = null;
            TableViewColumn reportedColumn = null;
            bool cancelDefault = true;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;

                tableView.CellEditEnding += (s, e) =>
                {
                    callCount++;
                    reportedAction = e.EditAction;
                    reportedItem = e.Item;
                    reportedColumn = e.Column;
                    cancelDefault = e.Cancel;
                };

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                GetValueProvider(tableView, items[1], columnIndex: 0).SetValue("Diego Munoz");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, callCount, "CellEditEnding must fire exactly once per closing edit.");
                Verify.AreEqual(TableViewEditAction.Commit, reportedAction, "A commit must be reported as TableViewEditAction.Commit.");
                Verify.IsFalse(cancelDefault, "CellEditEnding.Cancel must default to false.");
                Verify.AreSame(items[1], reportedItem, "CellEditEnding.Item must be the edited data item.");
                Verify.AreSame(tableView.Columns[0], reportedColumn, "CellEditEnding.Column must be the edited column.");
                Verify.AreEqual("Diego Munoz", items[1].Name, "The commit must still reach the item.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a commit raises IsEditing changes, CellEditEnding with the live editor, then CellEditEnded once after the value is written and IsEditing is false.")]
        public void VerifyCellEditEventOrderAndEditingElement()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            var log = new List<string>();
            var editorTypeInEnding = string.Empty;
            string nameAtEnded = null;
            object endedItem = null;
            TableViewColumn endedColumn = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                tableView.RegisterPropertyChangedCallback(TableView.IsEditingProperty, (s, dp) => log.Add("IsEditing=" + tableView.IsEditing));
                tableView.CellEditEnding += (s, e) =>
                {
                    log.Add("Ending");
                    editorTypeInEnding = e.EditingElement?.GetType().Name ?? "null";
                };
                tableView.CellEditEnded += (s, e) =>
                {
                    log.Add("Ended:" + e.EditAction + ":IsEditing=" + tableView.IsEditing);
                    nameAtEnded = items[0].Name;
                    endedItem = e.Item;
                    endedColumn = e.Column;
                };
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => GetValueProvider(tableView, items[0], columnIndex: 0).SetValue("Asha K"));

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("IsEditing=True,Ending,IsEditing=False,Ended:Commit:IsEditing=False", string.Join(",", log),
                    "The edit must open, raise CellEditEnding, close, and only then raise CellEditEnded exactly once.");
                Verify.AreEqual(nameof(TextBox), editorTypeInEnding, "CellEditEnding.EditingElement must be the live text editor.");
                Verify.AreEqual("Asha K", nameAtEnded, "The value must be written before CellEditEnded is raised.");
                Verify.AreSame(items[0], endedItem, "CellEditEnded.Item must be the edited item.");
                Verify.AreSame(tableView.Columns[0], endedColumn, "CellEditEnded.Column must be the edited column.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CellEditEnded is not raised while CellEditEnding keeps the edit open, and is raised once with Cancel when the edit is then cancelled.")]
        public void VerifyCellEditEndedWaitsForTheEditToClose()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            var ended = new List<TableViewEditAction>();
            var veto = true;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                tableView.CellEditEnding += (s, e) => e.Cancel = veto;
                tableView.CellEditEnded += (s, e) => ended.Add(e.EditAction);
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Asha K");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the vetoed commit keeps the edit open.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, ended.Count, "CellEditEnded must not be raised while the edit stays open.");
                veto = false;
                Verify.IsTrue(tableView.CancelEdit(), "Precondition: the edit cancels.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, ended.Count, "CellEditEnded must be raised once when the edit closes.");
                Verify.AreEqual(TableViewEditAction.Cancel, ended.FirstOrDefault(), "The close must be reported as Cancel.");
                Verify.AreEqual("Asha", items[0].Name, "A cancelled edit writes nothing.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a C#-derived column's CommitCellEditCore override receives the editor and decides the commit: true closes, false or a throw keeps the edit open.")]
        [TestProperty("data:Mode", "{Accept, Reject, Throw}")]
        public void VerifyCommitCellEditCoreOverrideDecidesTheCommit()
        {
            var mode = ((string)TestContext.DataRow["Mode"]).Trim();
            TableView tableView = null;
            List<EditablePerson> items = null;
            OverridingCommitTextColumn column = null;
            Exception error = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                column = new OverridingCommitTextColumn
                {
                    Header = "Name",
                    Mode = mode,
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                };
                tableView = CreateTableView(items, headers: new string[0], width: 500, height: 300);
                tableView.Columns.Add(column);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "asha k"));

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, column.CommitCount, $"{mode}: the override must be called once for the commit.");
                Verify.AreEqual(nameof(TextBox), column.EditingElementType, $"{mode}: the override must receive the editor.");
                if (mode == "Accept")
                {
                    Verify.IsNull(error, "An accepted commit must succeed.");
                    Verify.IsFalse(tableView.IsEditing, "An accepted commit must close the edit.");
                    Verify.AreEqual("ASHA K", items[0].Name, "The override's write must reach the item instead of the default binding write.");
                }
                else
                {
                    Verify.IsNotNull(error, $"{mode}: a failed commit must be reported to the caller.");
                    Verify.IsTrue(tableView.IsEditing, $"{mode}: a failed commit must keep the edit open.");
                    Verify.AreEqual("Asha", items[0].Name, $"{mode}: a failed commit must write nothing.");
                    column.Mode = "Accept";
                    Verify.IsTrue(tableView.CommitEdit(), $"{mode}: the edit must still commit once the override accepts.");
                    Verify.AreEqual("ASHA K", items[0].Name, $"{mode}: the retried commit must write the value.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies cancelling CellEditEnding blocks the value and leaves the edit open, per the IDL's stay-open rule.")]
        public void VerifyCellEditEndingCancelBlocksCommitAndLeavesEditOpen()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            bool vetoing = true;
            var actions = new List<TableViewEditAction>();

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;

                tableView.CellEditEnding += (s, e) =>
                {
                    actions.Add(e.EditAction);
                    e.Cancel = vetoing;
                };

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Vetoed value");
                Verify.IsTrue(error != null, "A vetoed commit must report failure to the caller.");
                Log.Comment($"SetValue threw as expected: {error.Message}");

                Verify.AreEqual("Asha", items[0].Name, "A vetoed commit must not let the value reach the item.");

                // The IDL is explicit: "a veto or failed validation leaves the edit open and IsEditing true".
                Verify.IsTrue(tableView.IsEditing, "A vetoed commit must leave the edit open so the user can correct or discard it.");
                Verify.AreEqual(1, actions.Count, "CellEditEnding must have been raised once for the vetoed commit.");
                Verify.AreEqual(TableViewEditAction.Commit, actions[0], "The vetoed attempt was a commit.");

                // Stop vetoing and close the still-open edit. A normal cancel is itself vetoable, so the
                // handler has to stand down first.
                vetoing = false;
                Verify.IsTrue(tableView.CancelEdit(), "CancelEdit must close an edit that is genuinely open.");
                Verify.IsFalse(tableView.IsEditing, "The edit must be closed afterwards.");
                Verify.AreEqual("Asha", items[0].Name, "The cancel must leave the pre-edit value in place.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies closing CanUserSortColumns from a BeginningEdit handler, cancelled or not, keeps the applied sort and raises no Sorting or Sorted.")]
        [TestProperty("data:Change", "{CanUserSortColumnsCancel, CanUserSortColumnsContinue}")]
        public void VerifySortOptOutDuringBeginningEditKeepsSort()
        {
            var change = ((string)TestContext.DataRow["Change"]).Trim();
            TableView tableView = null;
            List<EditablePerson> items = null;
            bool requested = false;
            int sortEvents = 0;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending), "Precondition: the Name column sorts.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorting += (s, e) => sortEvents++;
                tableView.Sorted += (s, e) => sortEvents++;
                tableView.BeginningEdit += (s, e) =>
                {
                    if (requested)
                    {
                        return;
                    }
                    requested = true;
                    tableView.CanUserSortColumns = false;
                    e.Cancel = change != "CanUserSortColumnsContinue";
                };

                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 1), "Pilot");
                Log.Comment($"SetValue: {error?.Message ?? "succeeded"}");
                Verify.IsTrue(requested, "Precondition: BeginningEdit closed the gate.");
                if (change == "CanUserSortColumnsContinue")
                {
                    Verify.IsNull(error, "An edit BeginningEdit let continue must accept the value.");
                    Verify.AreEqual("Pilot", items[0].Role, "The value must be committed.");
                    Verify.IsFalse(tableView.IsEditing, "The committed edit must be closed.");
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection, $"{change}: the gate gates user input only, so the sort stays.");
                Verify.AreEqual(0, sortEvents, $"{change}: no Sorting or Sorted may be raised.");
                var names = GetProjectedItems(tableView).Select(item => ((EditablePerson)item).Name).ToList();
                Verify.AreEqual("Mei,Diego,Asha", string.Join(",", names), $"{change}: the rows must stay sorted.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies closing CanUserSortColumns from a BeginningEdit handler that lets the edit open keeps both the edit and the sort.")]
        public void VerifySortOptOutDuringBeginningEditKeepsTheEditOpen()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            bool requested = false;
            bool vetoClose = true;
            int sortEvents = 0;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending), "Precondition: the Name column sorts.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorting += (s, e) => sortEvents++;
                tableView.Sorted += (s, e) => sortEvents++;
                tableView.BeginningEdit += (s, e) =>
                {
                    if (!requested)
                    {
                        requested = true;
                        tableView.CanUserSortColumns = false;
                    }
                };
                // Holds the edit open after SetValue so its state can be observed.
                tableView.CellEditEnding += (s, e) => e.Cancel = vetoClose;

                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 1), "Pilot");
                Verify.IsTrue(requested, "Precondition: BeginningEdit closed the gate.");
                Verify.IsNotNull(error, "Precondition: the vetoed commit is reported.");
                Verify.IsTrue(tableView.IsEditing, "The opened edit must stay open.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.IsEditing, "Closing the gate must not close the open edit.");
                vetoClose = false;
                Verify.IsTrue(tableView.CommitEdit(), "The edit commits.");
                Verify.AreEqual("Pilot", items[0].Role, "The value must be committed.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection, "The sort must survive the gate and the edit.");
                Verify.AreEqual(0, sortEvents, "No Sorting or Sorted may be raised.");
            });
        }
    }

    // Category 10.5 of the TableView test plan: validation, teardown and reentrancy.
    //
    // Spec basis: TableView.idl:493-530 - "False when it did not close: a veto or failed validation
    // leaves the edit open and IsEditing true", and IsEditing covers the opening and closing windows
    // "because the control uses this to reject re-entrant edit operations from inside consumer
    // callbacks".
    //
    // A validation failure is the only API-reachable way to obtain a persistently OPEN edit, since every
    // other route into one is a gesture. The teardown tests below use it for exactly that reason.
    //
    // Spec gap: the IDL does not name INotifyDataErrorInfo, nor state that errors are scoped to the
    // edited property rather than the whole object. Both are asserted here from the product's stated
    // "failed validation" behaviour plus the recovery requirement, and both deserve IDL text.
    [TestClass]
    public class TableViewEditValidationTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a validation error blocks the commit, holds the edit open, and rolls the item back to its pre-edit value.")]
        public void VerifyValidationErrorBlocksCommitAndLeavesItemUnchanged()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                foreach (var item in items)
                {
                    item.RejectPrefix = InvalidPrefix;
                }

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(error != null, "A value the item's own validator rejects must not be reported as applied.");
                Log.Comment($"SetValue threw as expected: {error.Message}");

                Verify.IsTrue(tableView.IsEditing, "Failed validation must leave the edit open so the user can correct the value.");

                // The rollback matters as much as the block: without it the item holds data its own
                // validator rejects while the UI shows something else.
                Verify.AreEqual("Asha", items[0].Name, "A rejected value must be rolled back off the item.");
            });

            // Leave no open edit behind for the next test in the class.
            RunOnUIThread.Execute(() => tableView.CancelEdit());
            IdleSynchronizer.Wait();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a forced teardown rolls back a pending value that fails validation instead of leaving it on the item.")]
        public void VerifyForcedTeardownRollsBackInvalidPendingValue()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                items[0].RejectPrefix = InvalidPrefix;

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // A rejected value holds the edit open, so the editor stays in the tree.
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: an edit is open.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var editor = GetEditorTextBox(tableView, items[0]);
                editor.Text = InvalidPrefix + "NopeX";

                tableView.ItemsSource = new List<EditablePerson>();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha", items[0].Name, "A forced teardown must not leave a rejected value on the item.");
                Verify.IsFalse(tableView.IsEditing, "A forced teardown must close the edit.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a forced teardown that keeps the edited item in the source transfers an invalid pending value, rejects it and leaves the item unchanged.")]
        public void VerifyForcedTeardownWithItemPresentRollsBackInvalidPendingValue()
        {
            var items = MakeEditableItems();
            items[0].RejectPrefix = InvalidPrefix;
            var tableView = LoadEditableTable(items);

            // The vetoed commit wrote nothing, so the only invalid value is the one typed below.
            var actions = OpenEditHoldingValidValue(tableView, items[0], "Zed");

            RunOnUIThread.Execute(() =>
            {
                var editor = GetEditorTextBox(tableView, items[0]);
                editor.Text = InvalidPrefix + "Nope";

                // Forced and outside layout, with the item still in the source: the pending value is
                // transferred, then rejected by validation.
                tableView.IsReadOnly = true;
            });
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Cancel, items[0], "Asha", "an invalid pending value must be rolled back off an item that stays in the source");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies recycling the edited row during layout rolls back a pending value that fails validation, raising Ending(Cancel) with the live editor and then Ended(Cancel) once; a SortByColumn refused by the failing value raises no Sorting.")]
        [TestProperty("data:Queued", "{None, Sort}")]
        public void VerifyRecycleDuringLayoutRollsBackInvalidPendingValue()
        {
            var queued = ((string)TestContext.DataRow["Queued"]).Trim();
            TableView tableView = null;
            List<EditablePerson> items = null;
            Button focusSink = null;

            RunOnUIThread.Execute(() =>
            {
                var names = new string[200];
                names[0] = "Asha";
                for (int i = 1; i < names.Length; i++)
                {
                    names[i] = "Person " + i;
                }
                items = MakeEditableItems(names);
                items[0].RejectPrefix = InvalidPrefix;

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                focusSink = LoadWithFocusSink(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: an edit is open.");
            });

            IdleSynchronizer.Wait();

            // ItemsRepeater never recycles the row holding focus, so move focus off the editor. The
            // focus-loss commit of an invalid value is rejected and leaves the edit open.
            RunOnUIThread.Execute(() =>
            {
                var editor = GetEditorTextBox(tableView, items[0]);
                editor.Text = InvalidPrefix + "NopeX";
                focusSink.Focus(FocusState.Programmatic);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotEqual(FocusState.Unfocused, focusSink.FocusState, "Precondition: focus left the editor.");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the rejected focus-loss commit keeps the edit open.");

                var editor = GetEditorTextBox(tableView, items[0]);
                editor.Text = InvalidPrefix + "NopeY";
            });

            var log = new List<string>();
            RunOnUIThread.Execute(() =>
            {
                tableView.CellEditEnding += (s, e) => log.Add($"Ending:{e.EditAction}:{(e.EditingElement != null ? "editor" : "null")}");
                tableView.CellEditEnded += (s, e) => log.Add($"Ended:{e.EditAction}:IsEditing={tableView.IsEditing}");
                tableView.Sorting += (s, e) => log.Add("Sorting");
                if (queued == "Sort")
                {
                    // The sort's cancellable commit fails validation, so the sort is refused, not queued.
                    Verify.IsFalse(tableView.SortByColumn(tableView.Columns[1], SortDirection.Descending), "A sort blocked by a failing edit must be refused.");
                    Verify.IsTrue(tableView.IsEditing, "The failing value must keep the edit open.");
                }
            });

            // Row 0 leaves the realization window, so the repeater recycles it inside measure.
            ScrollBodyTo(tableView, ScrollAxis.Vertical, 3000);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha", items[0].Name, "Recycling the edited row must not leave a rejected value on the item.");
                Verify.IsFalse(tableView.IsEditing, "Recycling the edited row must close the edit.");

                var closing = log.SkipWhile(entry => !entry.StartsWith("Ending:Cancel", StringComparison.Ordinal)).ToList();
                Verify.AreEqual("Ending:Cancel:editor,Ended:Cancel:IsEditing=False", string.Join(",", closing),
                    $"{queued}: the forced close must raise Ending with the live editor, then Ended once; log [{string.Join(", ", log)}].");
                Verify.IsFalse(log.Contains("Sorting"), $"{queued}: no Sorting may be raised.");
                Verify.AreEqual(SortDirection.None, tableView.Columns[1].SortDirection, $"{queued}: the refused sort must not light its column.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort queued behind an edit that a CellEditEnded handler reopened drains after that edit closes (by recycle or CommitEdit): Ended first, then Sorting, and a throwing Sorting handler does not crash.")]
        [TestProperty("data:Close", "{Recycle, Commit}")]
        public void VerifyQueuedSortBehindReopenedEditDrainsAfterEnded()
        {
            var close = ((string)TestContext.DataRow["Close"]).Trim();
            TableView tableView = null;
            List<EditablePerson> items = null;
            Button focusSink = null;
            var log = new List<string>();
            var vetoNext = false;
            var reopened = false;

            RunOnUIThread.Execute(() =>
            {
                var names = new string[200];
                names[0] = "Asha";
                for (int i = 1; i < names.Length; i++)
                {
                    names[i] = "Person " + i;
                }
                items = MakeEditableItems(names);
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                focusSink = LoadWithFocusSink(tableView);
            });

            IdleSynchronizer.Wait();

            // Cell A holds a valid pending value, so the sort's commit of A succeeds.
            OpenEditHoldingValidValue(tableView, items[0], "Zed");

            RunOnUIThread.Execute(() =>
            {
                tableView.CellEditEnding += (s, e) => e.Cancel = vetoNext;
                tableView.CellEditEnded += (s, e) =>
                {
                    log.Add($"Ended:{e.EditAction}:{((EditablePerson)e.Item).Name}");
                    if (reopened)
                    {
                        return;
                    }

                    // Reopening an edit here queues the sort behind cell B.
                    reopened = true;
                    vetoNext = true;
                    CaptureSetValue(GetValueProvider(tableView, items[1], columnIndex: 0), "Bea");
                };
                tableView.Sorting += (s, e) =>
                {
                    log.Add("Sorting");
                    throw new InvalidOperationException("Sorting handler fault");
                };

                tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(reopened, "Precondition: CellEditEnded reopened an edit on cell B.");
                Verify.IsTrue(tableView.IsEditing, "Precondition: cell B is being edited.");
                Verify.IsFalse(log.Contains("Sorting"), "The sort must wait behind cell B's edit.");
                if (close == "Recycle")
                {
                    // ItemsRepeater never recycles the row holding focus; the vetoed focus-loss commit keeps B open.
                    focusSink.Focus(FocusState.Programmatic);
                }
            });

            IdleSynchronizer.Wait();

            if (close == "Recycle")
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(tableView.IsEditing, "Precondition: cell B is still being edited.");
                    vetoNext = false;
                });
                ScrollBodyTo(tableView, ScrollAxis.Vertical, 3000);
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    vetoNext = false;
                    Verify.IsTrue(tableView.CommitEdit(), "Cell B commits.");
                });
            }

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, $"{close}: every edit must be closed.");
                var endedB = log.FindIndex(entry => entry.StartsWith("Ended:", StringComparison.Ordinal) && entry != "Ended:Commit:Zed");
                var sorting = log.IndexOf("Sorting");
                Verify.AreEqual(1, log.Count(entry => entry == "Sorting"), $"{close}: the queued sort must raise Sorting once; log [{string.Join(", ", log)}].");
                Verify.IsTrue(endedB >= 0 && endedB < sorting, $"{close}: Ended for cell B must come before Sorting; log [{string.Join(", ", log)}].");
                Verify.AreEqual(close == "Commit" ? TableViewEditAction.Commit.ToString() : TableViewEditAction.Cancel.ToString(),
                    log[endedB].Split(':')[1], $"{close}: cell B must close with the expected action.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus moving into a flyout the editor owns keeps the edit open, and the edit commits once focus leaves for good.")]
        public void VerifyFocusInEditorOwnedFlyoutKeepsEditOpen()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            Button focusSink = null;
            Flyout flyout = null;
            Button flyoutButton = null;

            OpenEditHeldByValidator(out tableView, out items, out focusSink);

            RunOnUIThread.Execute(() =>
            {
                var editor = GetEditorTextBox(tableView, items[0]);
                editor.Text = "Bea";
                flyoutButton = new Button { Content = "Inside" };
                flyout = new Flyout { Content = flyoutButton };
                FlyoutBase.SetAttachedFlyout(editor, flyout);
                FlyoutBase.ShowAttachedFlyout(editor);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => flyoutButton.Focus(FocusState.Programmatic));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(flyout.IsOpen, "Precondition: the editor's flyout is open.");
                Verify.AreNotEqual(FocusState.Unfocused, flyoutButton.FocusState, "Precondition: focus is inside the flyout.");
                Verify.IsTrue(tableView.IsEditing, "Focus inside a flyout the editor owns must not commit the edit.");
                Verify.AreEqual("Asha", items[0].Name, "The pending value must not reach the item while the flyout is open.");
                flyout.Hide();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => focusSink.Focus(FocusState.Programmatic));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, "Focus leaving the editor after the flyout closes must commit the edit.");
                Verify.AreEqual("Bea", items[0].Name, "The committed value must reach the item.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus moving into a popup the editor did not open commits the edit.")]
        public void VerifyFocusInUnrelatedPopupCommitsEdit()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            Button focusSink = null;
            Popup popup = null;
            Button popupButton = null;

            OpenEditHeldByValidator(out tableView, out items, out focusSink);

            RunOnUIThread.Execute(() =>
            {
                GetEditorTextBox(tableView, items[0]).Text = "Bea";
                popupButton = new Button { Content = "Elsewhere" };
                popup = new Popup { Child = popupButton, XamlRoot = tableView.XamlRoot };
                popup.IsOpen = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => popupButton.Focus(FocusState.Programmatic));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotEqual(FocusState.Unfocused, popupButton.FocusState, "Precondition: focus is inside the unrelated popup.");
                Verify.IsFalse(tableView.IsEditing, "Focus inside a popup the editor did not open must commit the edit.");
                Verify.AreEqual("Bea", items[0].Name, "The committed value must reach the item.");
                popup.IsOpen = false;
            });
            IdleSynchronizer.Wait();
        }

        // Opens an edit on the first item that stays open because its value fails validation.
        private void OpenEditHeldByValidator(out TableView tableView, out List<EditablePerson> items, out Button focusSink)
        {
            TableView table = null;
            List<EditablePerson> source = null;
            Button sink = null;

            RunOnUIThread.Execute(() =>
            {
                source = MakeEditableItems();
                source[0].RejectPrefix = InvalidPrefix;

                table = CreateEditableTable(source);
                table.IsReadOnly = false;
                sink = LoadWithFocusSink(table);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(table, source[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(table.IsEditing, "Precondition: an edit is open.");
                Verify.IsTrue(GetEditorTextBox(table, source[0]).Focus(FocusState.Programmatic), "Precondition: the editor takes focus.");
            });
            IdleSynchronizer.Wait();

            tableView = table;
            items = source;
            focusSink = sink;
        }

        private static TextBox GetEditorTextBox(TableView tableView, object item)
        {
            var editors = FindVisualChildrenByType<TextBox>(GetRowForItem(tableView, item));
            Verify.AreEqual(1, editors.Count, "The edited row must host exactly one editor TextBox.");
            return editors[0];
        }

        // Loads the table after a focusable button, which tests use to move focus off the editor.
        private Button LoadWithFocusSink(TableView tableView)
        {
            var sink = new Button { Content = "Sink" };
            var host = new StackPanel();
            host.Children.Add(sink);
            host.Children.Add(tableView);
            LoadContent(host);
            return sink;
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a rollback posted from a layout pass does not overwrite a value the app wrote before it ran, when the column's value cannot be snapshotted.")]
        public void VerifyPostedRollbackDoesNotOverwriteNewerAppWrite()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            Button focusSink = null;
            int appWrites = 0;

            RunOnUIThread.Execute(() =>
            {
                var names = new string[200];
                names[0] = "Asha";
                for (int i = 1; i < names.Length; i++)
                {
                    names[i] = "Person " + i;
                }
                items = MakeEditableItems(names);
                items[0].RejectPrefix = InvalidPrefix;

                // An explicit binding Source leaves the control no editing path, so it cannot compare
                // the item's value at rollback time - the same blind spot as an enum or decimal path.
                tableView = CreateTableViewShell(items, 500, 300);
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Name",
                    Binding = new Binding { Path = new PropertyPath("Name"), Source = items[0], Mode = BindingMode.OneWay },
                });
                tableView.IsReadOnly = false;

                // A forced close from layout delivers CellEditEnding on the dispatcher ahead of the
                // posted rollback, so this is an app write landing between the two.
                tableView.CellEditEnding += (s, e) =>
                {
                    if (e.EditAction == TableViewEditAction.Cancel && appWrites == 0)
                    {
                        appWrites++;
                        items[0].Name = "Rewritten";
                    }
                };

                focusSink = LoadWithFocusSink(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: an edit is open.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var editor = GetEditorTextBox(tableView, items[0]);
                editor.Text = InvalidPrefix + "NopeX";
                focusSink.Focus(FocusState.Programmatic);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.IsEditing, "Precondition: the rejected focus-loss commit keeps the edit open.");
                var editor = GetEditorTextBox(tableView, items[0]);
                editor.Text = InvalidPrefix + "NopeY";
            });

            // Recycling row 0 inside measure transfers the invalid value and posts its rollback.
            ScrollBodyTo(tableView, ScrollAxis.Vertical, 3000);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, "Recycling the edited row must close the edit.");
                Verify.AreEqual(1, appWrites, "Precondition: the app wrote while the rollback was pending.");
                Verify.AreEqual("Rewritten", items[0].Name, "A posted rollback must not overwrite a newer app write.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the text column's editor TextBox takes its UIA name from the column header text.")]
        public void VerifyTextColumnEditorNameComesFromHeader()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                items[0].RejectPrefix = InvalidPrefix;

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // A rejected value holds the edit open, so the editor stays in the tree.
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: an edit is open.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var editor = GetEditorTextBox(tableView, items[0]);
                Verify.AreEqual("Name", AutomationProperties.GetName(editor), "The editor's UIA name must be the column header text.");
            });

            RunOnUIThread.Execute(() => tableView.CancelEdit());
            IdleSynchronizer.Wait();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the text column's editor TextBox is named from a templated object header's rendered text.")]
        public void VerifyTextColumnEditorNameComesFromTemplatedObjectHeader()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                items[0].RejectPrefix = InvalidPrefix;

                tableView = CreateTableViewShell(items, 500, 300);
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = new TitledColumnHeader { Title = "Full name" },
                    HeaderTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">
                              <TextBlock Text=""{Binding Title}"" />
                          </DataTemplate>"),
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                });
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: an edit is open.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var editor = GetEditorTextBox(tableView, items[0]);
                Verify.AreEqual("Full name", AutomationProperties.GetName(editor),
                    "An object header has no string form; the editor must take the name its rendered header exposes.");
                Verify.AreEqual("Full name", FrameworkElementAutomationPeer.CreatePeerForElement(editor).GetName(),
                    "The editor's peer must report the same name.");
            });

            RunOnUIThread.Execute(() => tableView.CancelEdit());
            IdleSynchronizer.Wait();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies FontWeight set on the TableView reaches the text column's editor TextBox.")]
        public void VerifyTableViewFontWeightReachesTextEditor()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                items[0].RejectPrefix = InvalidPrefix;

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                tableView.FontWeight = Microsoft.UI.Text.FontWeights.Bold;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: an edit is open.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var editor = GetEditorTextBox(tableView, items[0]);
                Verify.AreEqual(Microsoft.UI.Text.FontWeights.Bold.Weight, editor.FontWeight.Weight,
                    "The editor must not set a local FontWeight that blocks the TableView's value.");
            });

            RunOnUIThread.Execute(() => tableView.CancelEdit());
            IdleSynchronizer.Wait();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cell blocked by validation recovers: a subsequent acceptable value commits and closes the edit.")]
        public void VerifyClearingValidationErrorAllowsTheCommit()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            var actions = new List<TableViewEditAction>();

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                foreach (var item in items)
                {
                    item.RejectPrefix = InvalidPrefix;
                }

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                tableView.CellEditEnding += (s, e) => actions.Add(e.EditAction);
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the cell is blocked and the edit is open.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Asha Kapoor");
                Verify.IsTrue(error == null, $"A value the validator accepts must commit. Instead: {error?.Message}");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha Kapoor", items[0].Name, "The accepted value must reach the item.");
                Verify.IsFalse(tableView.IsEditing, "A validation failure must not be terminal - the cell recovers once the value is acceptable.");
                Verify.IsTrue(actions.Count > 0, "CellEditEnding must have been raised.");
                Verify.AreEqual(TableViewEditAction.Commit, actions[actions.Count - 1], "The final close must be reported as a commit.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CancelEdit closes a validation-blocked edit, reporting the Cancel action and preserving the pre-edit value.")]
        public void VerifyCancelEditClosesAValidationBlockedEditWithCancelAction()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            var actions = new List<TableViewEditAction>();

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                foreach (var item in items)
                {
                    item.RejectPrefix = InvalidPrefix;
                }

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                tableView.CellEditEnding += (s, e) => actions.Add(e.EditAction);
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the edit is held open by the validator.");
                actions.Clear();

                Verify.IsTrue(tableView.CancelEdit(), "CancelEdit must close an open edit and report that it closed.");
                Verify.IsFalse(tableView.IsEditing, "The blocked edit must be gone.");

                Verify.AreEqual(1, actions.Count, "The cancel must raise CellEditEnding exactly once.");
                Verify.AreEqual(TableViewEditAction.Cancel, actions[0], "A cancel must be reported as TableViewEditAction.Cancel.");
                Verify.AreEqual("Asha", items[0].Name, "Discarding the edit must leave the pre-edit value on the item.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing ItemsSource while an edit is open closes the edit and leaves the control usable.")]
        public void VerifyItemsSourceResetWhileEditingClosesEditSafely()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            List<EditablePerson> replacement = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                foreach (var item in items)
                {
                    item.RejectPrefix = InvalidPrefix;
                }

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: an edit is open over the first row.");

                replacement = MakeEditableItems("Nadia", "Omar", "Priya");
                tableView.ItemsSource = replacement;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // The row the editor sat over no longer exists. Leaving IsEditing true would wedge the
                // control: every later edit is rejected as reentrant, and a recycled row keeps a live
                // editor over unrelated data.
                Verify.IsFalse(tableView.IsEditing, "Replacing the source must close any edit that was open over the old items.");

                var rows = GetRealizedRows(tableView);
                Verify.IsTrue(rows.Count > 0, "The replacement source must render.");

                // The real proof the control is not wedged: it can still edit.
                var error = CaptureSetValue(GetValueProvider(tableView, replacement[0], columnIndex: 0), "Nadia Okoye");
                Verify.IsTrue(error == null, $"Editing must still work after a reset during an edit. Instead: {error?.Message}");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Nadia Okoye", replacement[0].Name, "The post-reset edit must commit normally.");
                Verify.AreEqual("Asha", items[0].Name, "The abandoned edit must not have written to the discarded item.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing the edited item from the source force-closes the edit as a Cancel and writes nothing to the removed item.")]
        public void VerifyForcedCloseCancelsWhenEditedItemIsRemoved()
        {
            var items = new ObservableCollection<EditablePerson>(MakeEditableItems());
            var tableView = LoadEditableTable(items);
            var edited = items[0];
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            RunOnUIThread.Execute(() => items.Remove(edited));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Cancel, edited, "Asha", "a removed item must not receive the pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies filtering the edited item out of the projection force-closes the edit as a Cancel and writes nothing to it.")]
        public void VerifyForcedCloseCancelsWhenEditedItemIsFilteredOut()
        {
            var items = MakeEditableItems();
            var source = SourceFrom(items);
            var tableView = LoadEditableTable(source);
            var edited = items[0];
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            RunOnUIThread.Execute(() => source.Filter(new TableViewPredicate(o => ((EditablePerson)o).Name != "Asha")));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Cancel, edited, "Asha", "a filtered-out item must not receive the pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing ItemsSource force-closes a valid pending edit as a Cancel and writes nothing to the old item.")]
        public void VerifyForcedCloseCancelsWhenItemsSourceIsReplaced()
        {
            var items = MakeEditableItems();
            var tableView = LoadEditableTable(items);
            var edited = items[0];
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            RunOnUIThread.Execute(() => tableView.ItemsSource = MakeEditableItems("Nadia", "Omar", "Priya"));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Cancel, edited, "Asha", "an item from the replaced source must not receive the pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a forced close from a DP change sets IsEditing false synchronously and raises one CellEditEnded(Cancel) after the triggering call returns.")]
        [TestProperty("data:Trigger", "{ItemsSource, IsReadOnly}")]
        public void VerifyForcedCloseRaisesOneCellEditEndedOutsideTheCallback()
        {
            var trigger = ((string)TestContext.DataRow["Trigger"]).Trim();
            var items = MakeEditableItems();
            var tableView = LoadEditableTable(items);
            OpenEditHoldingValidValue(tableView, items[0], "Zed");
            var ended = new List<string>();
            var inTrigger = false;
            bool? isEditingAfterTrigger = null;
            var editorInEnding = "not raised";

            RunOnUIThread.Execute(() =>
            {
                tableView.CellEditEnding += (s, e) => editorInEnding = e.EditingElement?.GetType().Name ?? "null";
                tableView.CellEditEnded += (s, e) => ended.Add(e.EditAction + (inTrigger ? ":inside" : ":outside"));
                inTrigger = true;
                if (trigger == "ItemsSource")
                {
                    tableView.ItemsSource = MakeEditableItems("Nadia", "Omar");
                }
                else
                {
                    tableView.IsReadOnly = true;
                }
                inTrigger = false;
                isEditingAfterTrigger = tableView.IsEditing;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(false, isEditingAfterTrigger, $"{trigger}: IsEditing must be false as soon as the forced close returns.");
                Verify.AreEqual("Cancel:outside", string.Join(",", ended),
                    $"{trigger}: exactly one CellEditEnded(Cancel), raised after the DP callback.");
                Verify.AreEqual(nameof(TextBox), editorInEnding, $"{trigger}: CellEditEnding must still see the live editor.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CellEditEnded handler that opens an edit on the next row leaves exactly one editor and a consistent IsEditing.")]
        public void VerifyCellEditEndedHandlerCanOpenTheNextEdit()
        {
            var items = MakeEditableItems();
            var tableView = LoadEditableTable(items);
            var vetoNext = false;
            var reopened = false;

            RunOnUIThread.Execute(() =>
            {
                tableView.CellEditEnding += (s, e) => e.Cancel = vetoNext;
                tableView.CellEditEnded += (s, e) =>
                {
                    if (reopened)
                    {
                        return;
                    }
                    reopened = true;
                    vetoNext = true;
                    CaptureSetValue(GetValueProvider(tableView, items[1], columnIndex: 0), "Diego K");
                };
                GetValueProvider(tableView, items[0], columnIndex: 0).SetValue("Asha K");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(reopened, "Precondition: the CellEditEnded handler ran.");
                Verify.AreEqual("Asha K", items[0].Name, "The first edit must have committed.");
                Verify.IsTrue(tableView.IsEditing, "The edit opened from CellEditEnded must be open.");
                Verify.AreEqual(1, FindVisualChildrenByType<TextBox>(tableView).Count, "Exactly one editor may exist.");
                GetEditorTextBox(tableView, items[1]);

                vetoNext = false;
                Verify.IsTrue(tableView.CancelEdit(), "The second edit cancels.");
                Verify.IsFalse(tableView.IsEditing, "IsEditing must be false after the cancel.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, FindVisualChildrenByType<TextBox>(tableView).Count, "No editor may be left behind.");
                Verify.AreEqual("Diego", items[1].Name, "The cancelled second edit writes nothing.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort requested while an edit is open, whose CellEditEnded handler opens a new edit, still ends with one editor and the sort applied once the edits close.")]
        public void VerifySortWhileEditingWithReopeningCellEditEndedHandler()
        {
            var items = MakeEditableItems();
            var tableView = LoadEditableTable(items);
            OpenEditHoldingValidValue(tableView, items[0], "Zed");
            var vetoNext = false;
            var reopened = false;

            RunOnUIThread.Execute(() =>
            {
                tableView.CellEditEnding += (s, e) => e.Cancel = vetoNext;
                tableView.CellEditEnded += (s, e) =>
                {
                    if (reopened)
                    {
                        return;
                    }
                    reopened = true;
                    vetoNext = true;
                    CaptureSetValue(GetValueProvider(tableView, items[1], columnIndex: 0), "Diego K");
                };
                tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(reopened, "Precondition: the sort closed the first edit and CellEditEnded reopened one.");
                Verify.AreEqual("Zed", items[0].Name, "The first edit must have committed before the sort.");
                Verify.IsTrue(FindVisualChildrenByType<TextBox>(tableView).Count <= 1, "At most one editor may exist.");
                Verify.AreEqual(tableView.IsEditing, FindVisualChildrenByType<TextBox>(tableView).Count == 1,
                    "IsEditing must agree with the realized editor.");

                vetoNext = false;
                tableView.CancelEdit();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, "Every edit must be closed.");
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection, "The sort must be applied.");
                var names = GetProjectedItems(tableView).Select(item => ((EditablePerson)item).Name).ToList();
                Verify.AreEqual("Zed,Mei,Diego", string.Join(",", names), "The rows must be sorted by name descending.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a forced close that keeps the edited item (an app sort on the source) still commits a valid pending value.")]
        public void VerifyForcedCloseCommitsValidValueWhenItemStillPresent()
        {
            var items = MakeEditableItems();
            var source = SourceFrom(items);
            var tableView = LoadEditableTable(source);
            var edited = items[0];
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Descending));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Commit, edited, "Zed", "an item still in the source must receive the valid pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a forced close that leaves the edited item only under a collapsed parent (an app ParentBy) still commits a valid pending value.")]
        public void VerifyForcedCloseCommitsWhenEditedItemIsOnlyCollapsed()
        {
            var items = MakeEditableItems();
            var source = SourceFrom(items);
            var tableView = LoadEditableTable(source);
            var parent = items[0];
            var edited = items[1];
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            // Nodes start collapsed, so the edited child leaves the visible rows but stays in the tree.
            RunOnUIThread.Execute(() => source.ParentBy(
                new TableViewKeySelector(o => o),
                new TableViewKeySelector(o => ReferenceEquals(o, edited) ? parent : null)));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Commit, edited, "Zed", "an item only hidden under a collapsed parent must receive the valid pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing the edited item from a grouped source force-closes the edit as a Cancel and writes nothing to the removed item.")]
        public void VerifyForcedCloseCancelsWhenEditedItemIsRemovedWhileGrouped()
        {
            var items = new ObservableCollection<EditablePerson>(MakeEditableItems("Asha", "Diego", "Mei", "Ravi"));
            var source = GroupByRole(items);
            var tableView = LoadEditableTable(source);
            var edited = items[0];
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            RunOnUIThread.Execute(() => items.Remove(edited));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Cancel, edited, "Asha", "a removed item must not receive the pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies filtering the edited item out of a grouped projection force-closes the edit as a Cancel and writes nothing to it.")]
        public void VerifyForcedCloseCancelsWhenEditedItemIsFilteredOutWhileGrouped()
        {
            var items = MakeEditableItems("Asha", "Diego", "Mei", "Ravi");
            var source = GroupByRole(items);
            var tableView = LoadEditableTable(source);
            var edited = items[0];
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            RunOnUIThread.Execute(() => source.Filter(new TableViewPredicate(o => ((EditablePerson)o).Name != "Asha")));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Cancel, edited, "Asha", "a filtered-out item must not receive the pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clearing the parent relation while editing an item that stays in the flat projection still commits a valid pending value.")]
        public void VerifyForcedCloseCommitsWhenClearParentByKeepsEditedItem()
        {
            var items = MakeEditableItems();
            var parent = items[0];
            var child = items[1];
            var source = SourceFrom(items);
            RunOnUIThread.Execute(() => source.ParentBy(
                new TableViewKeySelector(o => o),
                new TableViewKeySelector(o => ReferenceEquals(o, child) ? parent : null)));
            var tableView = LoadEditableTable(source);
            var actions = OpenEditHoldingValidValue(tableView, parent, "Zed");

            RunOnUIThread.Execute(() => source.ClearParentBy());
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Commit, parent, "Zed", "an item still in the flat projection must receive the valid pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies filtering an edited child out of a ParentBy projection force-closes the edit as a Cancel and writes nothing to it.")]
        public void VerifyForcedCloseCancelsWhenEditedItemIsFilteredOutUnderParentBy()
        {
            var items = MakeEditableItems();
            var parent = items[0];
            var edited = items[1];
            var source = ParentEditedUnder(items, parent, edited, items[2]);
            var tableView = LoadExpandedTable(source);
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            RunOnUIThread.Execute(() => source.Filter(new TableViewPredicate(o => ((EditablePerson)o).Name != "Diego")));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Cancel, edited, "Diego", "a filtered-out child must not receive the pending value");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing an edited child from a ParentBy source force-closes the edit as a Cancel and writes nothing to it.")]
        public void VerifyForcedCloseCancelsWhenEditedItemIsRemovedUnderParentBy()
        {
            var items = new ObservableCollection<EditablePerson>(MakeEditableItems());
            var parent = items[0];
            var edited = items[1];
            var source = ParentEditedUnder(items, parent, edited, items[2]);
            var tableView = LoadExpandedTable(source);
            var actions = OpenEditHoldingValidValue(tableView, edited, "Zed");

            RunOnUIThread.Execute(() => items.Remove(edited));
            IdleSynchronizer.Wait();

            VerifyForcedClose(tableView, actions, TableViewEditAction.Cancel, edited, "Diego", "a removed child must not receive the pending value");
        }

        // The sibling keeps the parent's child list non-empty once the edited child leaves it.
        private static TableViewSource ParentEditedUnder(IEnumerable<EditablePerson> items, EditablePerson parent, EditablePerson child, EditablePerson sibling)
        {
            var source = SourceFrom(items);
            RunOnUIThread.Execute(() => source.ParentBy(
                new TableViewKeySelector(o => o),
                new TableViewKeySelector(o => ReferenceEquals(o, child) || ReferenceEquals(o, sibling) ? parent : null)));
            return source;
        }

        // Nodes start collapsed, so expand them to give the child a realized row to edit.
        private TableView LoadExpandedTable(TableViewSource source)
        {
            var tableView = LoadEditableTable(source);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            IdleSynchronizer.Wait();
            return tableView;
        }

        // TableViewSource must be created on the UI thread.
        private static TableViewSource SourceFrom(object items)
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = TableViewSource.From(items));
            return source;
        }

        private static TableViewSource GroupByRole(IEnumerable<EditablePerson> items)
        {
            var source = SourceFrom(items);
            RunOnUIThread.Execute(() => source.GroupBy(o => ((EditablePerson)o).Role));
            return source;
        }

        private TableView LoadEditableTable(object itemsSource)
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(itemsSource, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            return tableView;
        }

        // A vetoed commit keeps the editor open holding a valid value that has not reached the item, so
        // only the forced close decides whether it is written.
        private static List<TableViewEditAction> OpenEditHoldingValidValue(TableView tableView, EditablePerson item, string pendingValue)
        {
            var actions = new List<TableViewEditAction>();
            bool veto = true;
            RunOnUIThread.Execute(() =>
            {
                var originalName = item.Name;
                tableView.CellEditEnding += (s, e) =>
                {
                    actions.Add(e.EditAction);
                    e.Cancel = veto;
                };

                CaptureSetValue(GetValueProvider(tableView, item, columnIndex: 0), pendingValue);
                veto = false;

                Verify.IsTrue(tableView.IsEditing, "Precondition: the vetoed commit keeps the edit open.");
                Verify.AreEqual(originalName, item.Name, "Precondition: the vetoed commit wrote nothing.");
                actions.Clear();
            });
            IdleSynchronizer.Wait();
            return actions;
        }

        private static void VerifyForcedClose(TableView tableView, List<TableViewEditAction> actions, TableViewEditAction expected, EditablePerson item, string expectedName, string reason)
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, "The forced close must end the edit.");
                Verify.AreEqual(1, actions.Count, "The forced close must raise CellEditEnding exactly once.");
                Verify.AreEqual(expected, actions[0], "The forced close must report " + expected + ".");
                Verify.AreEqual(expectedName, item.Name, "After a forced close " + reason + ".");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies unloading the TableView while an edit is open tears down cleanly and the control edits again after reload.")]
        public void VerifyUnloadWhileEditingClosesEditSafely()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                foreach (var item in items)
                {
                    item.RejectPrefix = InvalidPrefix;
                }

                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), InvalidPrefix + "Nope");
                Verify.IsTrue(tableView.IsEditing, "Precondition: an edit is open.");

                // Navigating away mid-edit is ordinary user behaviour in a paged app.
                Content = null;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, "A reloaded TableView must not still report an edit from before the unload.");

                var error = CaptureSetValue(GetValueProvider(tableView, items[1], columnIndex: 0), "Diego Munoz");
                Verify.IsTrue(error == null, $"Editing must work again after an unload during an edit. Instead: {error?.Message}");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Diego Munoz", items[1].Name, "The post-reload edit must commit normally.");
                Verify.AreEqual("Asha", items[0].Name, "The edit abandoned at unload must not have written anything.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CommitEdit and CancelEdit called from inside BeginningEdit and CellEditEnding are rejected without corrupting edit state.")]
        public void VerifyReentrantEditCallsAreRejectedAndLeaveStateCoherent()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;

            bool? commitFromBeginning = null;
            bool? cancelFromBeginning = null;
            bool? commitFromEnding = null;
            bool? cancelFromEnding = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;

                tableView.BeginningEdit += (s, e) =>
                {
                    if (commitFromBeginning.HasValue)
                    {
                        return;
                    }

                    // The IDL says IsEditing is true here so the control can reject reentrant calls.
                    commitFromBeginning = tableView.CommitEdit();
                    cancelFromBeginning = tableView.CancelEdit();
                };

                tableView.CellEditEnding += (s, e) =>
                {
                    if (commitFromEnding.HasValue)
                    {
                        return;
                    }

                    commitFromEnding = tableView.CommitEdit();
                    cancelFromEnding = tableView.CancelEdit();
                };

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Asha Kapoor");
                Verify.IsTrue(error == null, $"Reentrant calls from handlers must not derail the outer edit. Instead: {error?.Message}");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(commitFromBeginning.HasValue, "BeginningEdit must have run.");
                Verify.IsFalse(commitFromBeginning.Value, "CommitEdit from inside BeginningEdit must be rejected - the edit is still opening.");
                Verify.IsFalse(cancelFromBeginning.Value, "CancelEdit from inside BeginningEdit must be rejected.");

                Verify.IsTrue(commitFromEnding.HasValue, "CellEditEnding must have run.");
                Verify.IsFalse(commitFromEnding.Value, "CommitEdit from inside CellEditEnding must be rejected - the close is already underway.");
                Verify.IsFalse(cancelFromEnding.Value, "CancelEdit from inside CellEditEnding must be rejected.");

                Verify.AreEqual("Asha Kapoor", items[0].Name, "The outer edit must still complete on its own terms.");
                Verify.IsFalse(tableView.IsEditing, "The state machine must be back at rest.");
            });

            // An independent edit afterwards is the real coherence check: a corrupted state machine
            // would reject it as reentrant.
            RunOnUIThread.Execute(() =>
            {
                var error = CaptureSetValue(GetValueProvider(tableView, items[1], columnIndex: 1), "Staff Engineer");
                Verify.IsTrue(error == null, $"A later, independent edit must work. Instead: {error?.Message}");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Staff Engineer", items[1].Role, "The later edit must commit.");
                Verify.IsFalse(tableView.IsEditing, "And must close.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a BeginEdit whose editor creation throws rolls back: no edit is left open, the display cell and focus are restored, and a later edit works.")]
        public void VerifyBeginEditThatThrowsRollsBackAndAllowsALaterEdit()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            var converter = new ArmableThrowingConverter();

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateTableViewShell(items, 500, 300);
                tableView.IsReadOnly = false;

                // An explicit Source makes the editor's binding evaluate inside the editor factory, so
                // an armed converter throws out of BeginEdit itself rather than into a swallowed refresh.
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Name",
                    Binding = new Binding { Path = new PropertyPath("Name"), Source = items[0], Converter = converter, Mode = BindingMode.OneWay },
                });
                tableView.Columns.Add(MakeTextColumn("Role"));
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            Grid cell = null;
            UIElement displayContent = null;
            RunOnUIThread.Execute(() =>
            {
                cell = GetRowCell(GetRowForItem(tableView, items[0]), 0);
                displayContent = GetCellContent(cell);
                GetCellPeer(tableView, items[0], columnIndex: 0).SetFocus();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(ReferenceEquals(cell, FocusManager.GetFocusedElement(tableView.XamlRoot)),
                    "Precondition: the cell has focus before the edit is attempted.");

                converter.Armed = true;
                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Asha Kapoor");
                converter.Armed = false;

                Verify.IsTrue(error != null, "The throwing editor factory must surface as a failed edit.");
                Verify.IsFalse(tableView.IsEditing, "A BeginEdit that threw must not leave an edit open.");
                Verify.IsTrue(ReferenceEquals(displayContent, GetCellContent(cell)), "The cell must show its display content again.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(ReferenceEquals(cell, FocusManager.GetFocusedElement(tableView.XamlRoot)),
                    "Focus must be back on the cell after the rollback.");

                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Asha Kapoor");
                Verify.IsTrue(error == null, $"A later edit on the same cell must start and commit. Instead: {error?.Message}");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha Kapoor", items[0].Name, "The later edit must commit.");
                Verify.IsFalse(tableView.IsEditing, "And must close.");
            });
        }
    }

    // Throws from Convert only while armed, so the display cells build normally.
    internal sealed class ArmableThrowingConverter : IValueConverter
    {
        public bool Armed { get; set; }

        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (Armed)
            {
                throw new InvalidOperationException("Deliberate editor-binding failure.");
            }
            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language) => value;
    }

    // Commits by upper-casing the editor text into the item itself, so a passing test proves the
    // override ran rather than the default binding write.
    internal sealed partial class OverridingCommitTextColumn : TableViewTextColumn
    {
        public string Mode { get; set; } = "Accept";

        public int CommitCount { get; private set; }

        public string EditingElementType { get; private set; }

        protected override bool CommitCellEditCore(FrameworkElement editingElement)
        {
            CommitCount++;
            EditingElementType = editingElement?.GetType().Name;

            switch (Mode)
            {
                case "Reject":
                    return false;
                case "Throw":
                    throw new InvalidOperationException("The app rejected the value.");
            }

            ((EditablePerson)editingElement.DataContext).Name = ((TextBox)editingElement).Text.ToUpperInvariant();
            return true;
        }
    }

    internal static class TableViewEditingTestHelpers
    {
        // Any value starting with this is rejected by EditablePerson's validator once RejectPrefix is set.
        internal const string InvalidPrefix = "!";

        internal static List<EditablePerson> MakeEditableItems(params string[] names)
        {
            if (names == null || names.Length == 0)
            {
                names = new[] { "Asha", "Diego", "Mei" };
            }

            var roles = new[] { "Designer", "Engineer", "Architect" };

            var result = new List<EditablePerson>();
            for (int i = 0; i < names.Length; i++)
            {
                result.Add(new EditablePerson { Name = names[i], Role = roles[i % roles.Length] });
            }

            return result;
        }

        // Two writable text columns. The TableView is left read-only so each test states its own gate.
        internal static TableView CreateEditableTable(IList<EditablePerson> items)
            => CreateTableView(items, headers: new[] { "Name", "Role" }, width: 500, height: 300);

        // A non-TextBox editor, so the column can no longer honour a text-valued SetValue.
        internal static DataTemplate MakeCustomEditorTemplate()
        {
            return (DataTemplate)XamlReader.Load(
                @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <ToggleSwitch />
                  </DataTemplate>");
        }

        // Fails the test if the pattern is not advertised - a test that needs to edit has no fallback.
        internal static IValueProvider GetValueProvider(TableView tableView, object item, int columnIndex)
        {
            var peer = GetCellPeer(tableView, item, columnIndex);
            var provider = peer.GetPattern(PatternInterface.Value) as IValueProvider;
            Verify.IsTrue(provider != null, "The cell must advertise the Value pattern for this test to drive an edit.");
            return provider;
        }

        internal static Exception CaptureSetValue(IValueProvider provider, string value)
        {
            try
            {
                provider.SetValue(value);
                return null;
            }
            catch (Exception e)
            {
                return e;
            }
        }

        internal static List<string> GetCellTexts(TableView tableView, object item)
        {
            var row = GetRowForItem(tableView, item);
            return FindVisualChildrenByType<TextBlock>(row).Select(t => t.Text).ToList();
        }
    }

    // A data item that can be edited and can reject values, so the validation path is reachable without
    // any product-internal hook. Implements the XAML INotifyPropertyChanged so a committed value reaches
    // the OneWay display binding, and the XAML INotifyDataErrorInfo so the control's validation gate has
    // something to consult.
    internal sealed class EditablePerson : INotifyPropertyChanged, INotifyDataErrorInfo
    {
        private string _name;
        private string _role;

        public event PropertyChangedEventHandler PropertyChanged;

        public event EventHandler<DataErrorsChangedEventArgs> ErrorsChanged;

        // When set, any Name beginning with this prefix is reported as invalid.
        public string RejectPrefix { get; set; }

        public string Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    Raise(nameof(Name));
                }
            }
        }

        public string Role
        {
            get => _role;
            set
            {
                if (_role != value)
                {
                    _role = value;
                    Raise(nameof(Role));
                }
            }
        }

        public bool HasErrors => IsNameInvalid;

        public IEnumerable GetErrors(string propertyName)
        {
            if (string.IsNullOrEmpty(propertyName) || propertyName == nameof(Name))
            {
                if (IsNameInvalid)
                {
                    return new object[] { $"'{_name}' is not an acceptable {nameof(Name)}." };
                }
            }

            return null;
        }

        private bool IsNameInvalid =>
            !string.IsNullOrEmpty(RejectPrefix) &&
            _name != null &&
            _name.StartsWith(RejectPrefix, StringComparison.Ordinal);

        private void Raise(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
            ErrorsChanged?.Invoke(this, new DataErrorsChangedEventArgs(propertyName));
        }
    }
}
