// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using MUXTestInfra.Shared.Infra;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Point = System.Drawing.Point;

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class TableViewContextFlyoutsInteractionTests
    {
        private const string ContextTable = "ContextFlyoutsTableView";

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext) => TestEnvironment.Initialize(testContext);

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
            RestartAppIfLongRunning();
        }

        [TestMethod]
        public void RightClickSharedCellMenuUsesInvokingRowWithoutSelectingIt()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenContextTable();
                string log = "";
                RightClickCell(1, "ContextText", padding: true);
                ExpectMenu(ref log, "Text", "Context row 1");
                ActivateAction("Text action");
                Verify.AreEqual("Text|Context row 1|selected=Context row 0;",
                    ReadPageReadout("ContextFlyoutActionReport"), "The command must act on the invoking, unselected row.");

                RightClickCell(2, "ContextText", padding: true);
                ExpectMenu(ref log, "Text", "Context row 2");
                ActivateAction("Text action");
                Verify.AreEqual(
                    "Text|Context row 1|selected=Context row 0;Text|Context row 2|selected=Context row 0;",
                    ReadPageReadout("ContextFlyoutActionReport"),
                    "The same menu must get the next invoking row, not retain the previous row or use selection.");
            }
        }

        [TestMethod]
        public void TemplateWrapperPaddingUsesColumnMenuAndChildKeepsItsOwnMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenContextTable();
                string log = "";
                RightClickCell(1, "ContextTemplate", padding: true);
                ExpectMenu(ref log, "Template", "Context row 1");
                KeyboardHelper.PressKey(Key.Escape);

                var header = RequireHeader("ContextTemplate").BoundingRectangle;
                var row = RequireRow(1).BoundingRectangle;
                // The template child occupies x=16..136 DIPs; its horizontal centre scales with the header.
                RightClickPoint(new Point(header.Left + header.Width * 76 / 260, row.Top + row.Height / 2));
                ExpectMenu(ref log, "Child", "Context row 1");
                ActivateAction("Child action");
                Verify.AreEqual("Child|Context row 1|selected=Context row 0;",
                    ReadPageReadout("ContextFlyoutActionReport"));
            }
        }

        [TestMethod]
        public void HeaderPaddingAndFocusedHeaderUseColumnMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenContextTable();
                string log = "";
                foreach (var column in new[] { "ContextText", "ContextTemplate" })
                {
                    string kind = column == "ContextText" ? "TextHeader" : "TemplateHeader";
                    var header = RequireHeader(column);
                    var bounds = header.BoundingRectangle;
                    RightClickPoint(new Point(bounds.Left + 3, bounds.Top + 3));
                    ExpectMenu(ref log, kind, "<none>");
                    KeyboardHelper.PressKey(Key.Escape);

                    header = RequireHeader(column);
                    header.SetFocus();
                    Wait.ForIdle();
                    Verify.IsTrue(header.HasKeyboardFocus, "The generated header, not a template child, must hold focus.");
                    KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);
                    ExpectMenu(ref log, kind, "<none>");
                    KeyboardHelper.PressKey(Key.Escape);
                }
            }
        }

        [TestMethod]
        public void FocusedCellShiftF10UsesColumnMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenContextTable();
                Verify.IsTrue(DrillIntoRow(ContextTable, 1));
                string log = "";
                KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);
                ExpectMenu(ref log, "Text", "Context row 1");
                KeyboardHelper.PressKey(Key.Escape);
                KeyboardHelper.PressKey(Key.Right);
                VerifyFocusedCell(ContextTable, 1, 1, "Right must focus the template column's generated wrapper.");
                KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);
                ExpectMenu(ref log, "Template", "Context row 1");
                KeyboardHelper.PressKey(Key.Escape);
            }
        }

        [TestMethod]
        public void ClearingColumnMenusRestoresNativeRowAndTableFallback()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenContextTable();
                string log = "";
                RightClickCell(1, "ContextText", padding: true);
                ExpectMenu(ref log, "Text", "Context row 1");
                KeyboardHelper.PressKey(Key.Escape);
                InvokePageButton("ClearContextFlyoutsButton");

                RightClickCell(1, "ContextText", padding: true);
                ExpectMenu(ref log, "Row", "Context row 1");
                KeyboardHelper.PressKey(Key.Escape);
                RightClickCell(1, "ContextFallback", padding: true);
                ExpectMenu(ref log, "Row", "Context row 1");
                KeyboardHelper.PressKey(Key.Escape);

                var header = RequireHeader("ContextText").BoundingRectangle;
                RightClickPoint(new Point(header.Left + 3, header.Top + 3));
                ExpectMenu(ref log, "Table", "<none>");
                KeyboardHelper.PressKey(Key.Escape);
                InvokePageButton("RestoreContextFlyoutsButton");
                RightClickCell(1, "ContextText", padding: true);
                ExpectMenu(ref log, "Text", "Context row 1");
                KeyboardHelper.PressKey(Key.Escape);
            }
        }

        [TestMethod]
        public void NativeTextEditorMenuWinsOverColumnMenu()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenContextTable();
                Verify.IsTrue(DrillIntoRow(ContextTable, 1));
                KeyboardHelper.PressKey(Key.F2);
                var editor = FindElement.ById<Edit>("ContextNativeEditor");
                Verify.IsNotNull(editor, "F2 must realize the text column's native TextBox editor.");
                Verify.IsTrue(editor.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.a, ModifierKey.Control);
                KeyboardHelper.PressKey(Key.F10, ModifierKey.Shift);
                Verify.IsNotNull(FindElement.ByName<Button>("Copy"), "The editor's native text menu must open.");
                Verify.AreEqual("", ReadPageReadout("ContextFlyoutReport"), "No column or ancestor menu may also open.");
                KeyboardHelper.PressKey(Key.Escape);

                editor = FindElement.ById<Edit>("ContextNativeEditor");
                var bounds = editor.BoundingRectangle;
                RightClickPoint(new Point(bounds.Left + bounds.Width / 2, bounds.Top + bounds.Height / 2));
                Verify.IsNotNull(FindElement.ByName<Button>("Copy"));
                Verify.AreEqual("", ReadPageReadout("ContextFlyoutReport"));
                KeyboardHelper.PressKey(Key.Escape);
                KeyboardHelper.PressKey(Key.Escape);
            }
        }

        [TestMethod]
        public void OpenSharedMenuDoesNotRetargetWhenRowDataContextChanges()
        {
            using (var setup = new TestSetupHelper(PageName))
            {
                OpenContextTable();
                string log = "";
                RightClickCell(1, "ContextText", padding: true);
                ExpectMenu(ref log, "Text", "Context row 1");

                // UIA Invoke on this focus-neutral hook mutates the row, without clicking outside the popup.
                InvokePageButton("RetargetContextRowButton");
                string result = ReadPageReadout("ContextFlyoutLifetimeReport");
                Log.Comment("Lifetime outcome: {0}. Closure proves no-retarget safety, not open-menu continuity.", result);
                Verify.IsTrue(result == "Snapshot|Replacement row" || result == "Closed|Replacement row",
                    "An open menu must retain its invoking-row snapshot or close, never silently target Replacement row. Got: " + result);
                if (result == "Snapshot|Replacement row")
                {
                    ActivateAction("Text action");
                    Verify.AreEqual("Text|Context row 1|selected=Context row 0;",
                        ReadPageReadout("ContextFlyoutActionReport"));
                }

                RightClickCell(1, "ContextText", padding: true);
                ExpectMenu(ref log, "Text", "Replacement row");
                KeyboardHelper.PressKey(Key.Escape);
            }
        }

        private static void OpenContextTable()
        {
            InvokePageButton("GoToContextFlyoutsButton");
            Verify.IsNotNull(GetTable(ContextTable));
            Verify.IsNotNull(RequireRow(2), "All three fixture rows must be realized.");
        }

        private static void InvokePageButton(string id)
        {
            var button = FindElement.ById<Button>(id);
            Verify.IsNotNull(button);
            button.InvokeAndWait();
            Wait.ForIdle();
        }

        private static void ActivateAction(string name)
        {
            var item = FindElement.ByName(name);
            Verify.IsNotNull(item);
            InputHelper.LeftClick(item);
            Wait.ForIdle();
        }

        private static UIObject RequireHeader(string name)
        {
            var header = GetHeader(ContextTable, name);
            Verify.IsNotNull(header);
            return header;
        }

        private static UIObject RequireRow(int index)
        {
            var row = GetRow(GetTable(ContextTable), index);
            Verify.IsNotNull(row);
            return row;
        }

        private static void RightClickCell(int rowIndex, string column, bool padding)
        {
            var header = RequireHeader(column).BoundingRectangle;
            var row = RequireRow(rowIndex).BoundingRectangle;
            RightClickPoint(new Point(
                padding ? header.Left + 3 : header.Left + header.Width / 2,
                row.Top + row.Height / 2));
        }

        private static void RightClickPoint(Point point)
        {
            PointerInput.Move(point);
            PointerInput.Press(PointerButtons.Secondary);
            PointerInput.Release(PointerButtons.Secondary);
            Wait.ForIdle();
        }

        private static void ExpectMenu(ref string log, string kind, string row)
        {
            log += kind + "|" + row + "|selected=Context row 0;";
            Verify.AreEqual(log, ReadPageReadout("ContextFlyoutReport"),
                "Only the native winning menu should open, with the invoking context and unchanged selection.");
        }
    }
}
