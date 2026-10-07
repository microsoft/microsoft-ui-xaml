// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using MUXTestInfra.Shared.Infra;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView interaction tests. See docs\design-notes\TabularControls\TableView-interaction-test-plan.md.
    //
    // These run out of process and can only reach the control through the real UIA provider tree,
    // which is precisely the point: they assert that gestures and a UIA client reach the state
    // machine the API tests cover in process.
    [TestClass]
    public class TableViewTests
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

        #region 0. Page load

        [TestMethod]
        [TestProperty("Description", "Verifies a TableView authored in compiled markup loads and renders its authored column headers and rows.")]
        public void TestPageLoadsAndRendersStaticTable()
        {
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = FindElement.ById("BasicTableView");
                if (tableView == null)
                {
                    Verify.Fail("The TableView authored in compiled markup was not found on the test page.");
                    return;
                }

                // Headers authored as child content in compiled markup must reach the visual tree.
                VerifyElement.Found("Name", FindBy.Name);
                VerifyElement.Found("Age", FindBy.Name);
                VerifyElement.Found("ReadOnlyCity", FindBy.Name);
                VerifyElement.Found("Template", FindBy.Name);

                // Rows must be realized from the source the page bound in code-behind.
                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                Verify.IsGreaterThan(rowsHost.Children.Count, 0, "At least one row should be realized.");
                Verify.AreEqual(
                    "Microsoft.UI.Xaml.Controls.Tabular.TableViewRow",
                    rowsHost.Children[0].ClassName,
                    "The realized row containers should be TableViewRow.");
            }
        }

        #endregion

        #region 10. Accessibility scan and out-of-proc UIA

        [TestMethod]
        [TestProperty("Description", "Verifies an Axe accessibility scan of the TableView test page reports no issues.")]
        public void VerifyAxeScanPasses()
        {
            // Interaction plan §10 VerifyAxeScanPasses.
            // Repo convention: 12 controls run this exact shape from their interaction tests, and it exists at no
            //   other tier. TableViewPage is already registered for it (TableViewPage.xaml.cs:90).
            // Failure means: the table trips a general accessibility rule - contrast, a missing name, a wrong
            //   role - of a kind no per-peer assertion looks for, because a per-peer test asserts the value it
            //   was written to expect and Axe asserts the rules nobody thought to check.
            //
            // EXPECTED TO FAIL ON PRODUCT FINDING #13, and left that way deliberately. Axe walks the whole
            // provider tree of the app process, which means it asks a TableViewRow peer for its children and
            // the app fail-fasts with 0xC0000420 before any result is reported. This is the same root cause as
            // VerifyTableIsNavigableByAUiaClient below, reached by a different client - which is itself the
            // finding's significance: it is not one test's unusual walk, it is any conforming UIA client.
            using (var setup = new TestSetupHelper("TableView-Axe"))
            {
                AxeTestHelper.TestForAxeIssues();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a UIA client outside the app can walk from the TableView down to a cell.")]
        public void VerifyTableIsNavigableByAUiaClient()
        {
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = FindElement.ById("BasicTableView");
                if (tableView == null)
                {
                    Verify.Fail("The TableView was not found on the test page.");
                    return;
                }

                UIObject headersHost = tableView.Children[0];
                Verify.IsGreaterThan(headersHost.Children.Count, 0, "The header host should expose header peers.");
                Verify.AreEqual("Name", headersHost.Children[0].Name, "The first header peer should carry its column header.");

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                Verify.IsGreaterThan(rowsHost.Children.Count, 0, "The rows host should expose row peers.");

                UIObject firstRow = rowsHost.Children[0];

                // A client walking the tree asks each row for its children. This is the step that
                // reaches TableViewRowAutomationPeer::GetChildrenCore through the provider stack.
                Verify.IsGreaterThan(
                    firstRow.Children.Count,
                    0,
                    "A row peer should expose one cell peer per visible column to an out-of-proc client.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies adding and removing a column raises a StructureChanged event that reaches a UIA client outside the app.")]
        public void VerifyStructureChangedEventsReachAUiaClient()
        {
            // Interaction plan §10 VerifyStructureChangedEventsReachAUiaClient.
            // Contract: adding or removing a column changes the SHAPE of the grid - the column count, and the
            //   set of children every row exposes. A UIA client that has cached the tree has no way to learn
            //   that except from a StructureChanged event. The product already accepts this obligation for
            //   other shape changes: TableView::OnTableViewSourceShapingChanged raises it for shaping
            //   (TableView.cpp:1021-1046) and TableViewAutomationPeer::RaiseStructureChangedForGroupExpansion
            //   for group expansion (TableViewAutomationPeer.cpp:119).
            // Why this tier: API §12.6 records that automation EVENTS are not API-testable - an in-proc peer
            //   test can call the raise method, but it cannot show anything arrived. Only a client on the other
            //   side of the process boundary can.
            // Failure means: assistive technology never learns the grid changed shape. It keeps reading the old
            //   column set, and a screen-reader user is told about a column that is gone or never hears about
            //   one that appeared.
            using (var setup = new TestSetupHelper("TableView Tests"))
            {
                UIObject tableView = FindElement.ById("BasicTableView");
                var addColumn = FindElement.ById<Button>("AddColumnButton");
                var removeColumn = FindElement.ById<Button>("RemoveColumnButton");
                if (tableView == null || addColumn == null || removeColumn == null)
                {
                    Verify.Fail("BasicTableView, AddColumnButton and/or RemoveColumnButton were not found on the test page.");
                    return;
                }

                // Scope.Element, not Subtree. The registration itself must not give UIA a reason to walk into
                // the rows: asking a TableViewRow peer for its children fail-fasts the app (finding #13). The
                // events under test are raised on the TableView's own peer
                // (TableViewAutomationPeer::RaiseStructureChanged -> RaiseStructureChangedEvent), so element
                // scope is where they arrive anyway.
                using (var waiter = new StructureChangedEventWaiter(tableView, Scope.Element))
                {
                    // POSITIVE CONTROL, and it is load-bearing. Sorting is a shape change the product already
                    // raises for, so it proves the whole chain works: a listener is registered (the product
                    // checks ListenerExists before raising at all - TableView.cpp:1024), the provider marshals,
                    // and this waiter sees it. Without this, "no event arrived" for a column change would be
                    // indistinguishable from "this test never listens properly", and the finding would be
                    // worthless.
                    UIObject nameHeader = FindColumnHeader(tableView, "Name");
                    if (nameHeader == null)
                    {
                        Verify.Fail("The 'Name' column header was not found, so the positive control could not run.");
                        return;
                    }

                    InputHelper.LeftClick(nameHeader);
                    Wait.ForIdle();

                    bool sortRaised = waiter.TryWait(TimeSpan.FromSeconds(5));
                    Verify.IsTrue(
                        sortRaised,
                        "Precondition: sorting must raise StructureChanged, which TableView.cpp:1038-1041 does " +
                        "for a re-order. If this does not arrive, nothing below can be read as a product " +
                        "result - it would only show that this client never hears anything.");

                    // SUBJECT 1: adding a column.
                    waiter.Reset();
                    addColumn.InvokeAndWait();
                    Wait.ForIdle();

                    bool addRaised = waiter.TryWait(TimeSpan.FromSeconds(5));

                    // SUBJECT 2: removing it again. Run before asserting so both results are logged from one
                    // run - knowing whether the gap is symmetric is what says where to look in the product.
                    waiter.Reset();
                    removeColumn.InvokeAndWait();
                    Wait.ForIdle();

                    bool removeRaised = waiter.TryWait(TimeSpan.FromSeconds(5));

                    Log.Comment("StructureChanged observed: sort={0} addColumn={1} removeColumn={2}.",
                        sortRaised, addRaised, removeRaised);

                    Verify.IsTrue(
                        addRaised,
                        "Adding a column must raise StructureChanged to a UIA client. Column changes go through " +
                        "QueueRebuildHeaders (TableView.cpp:1775/:1786), which rebuilds the header band and " +
                        "every row's cells but raises nothing - so a client keeps the stale column set.");

                    Verify.IsTrue(
                        removeRaised,
                        "Removing a column must raise StructureChanged to a UIA client. A client that still " +
                        "reports the removed column hands assistive technology a column that no longer exists.");
                }
            }
        }

        // Returns the header peer for the named column, or null. Enumerating the header host's children is the
        // measured-safe descent; this never asks a row peer for its children (finding #13).
        private static UIObject FindColumnHeader(UIObject tableView, string headerText)
        {
            UIObject headerHost = tableView.Children[0];
            foreach (UIObject child in headerHost.Children)
            {
                if (child.Name == headerText)
                {
                    return child;
                }
            }

            return null;
        }

        #endregion
    }
}
