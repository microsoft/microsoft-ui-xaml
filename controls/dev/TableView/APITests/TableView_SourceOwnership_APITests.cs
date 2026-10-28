// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewSortingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewSizingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Handing a single-owner TableViewSource between tables (TableViewSource.idl), and the state that
    // must not follow the source to its next owner.
    [TestClass]
    public class TableViewSourceOwnershipTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies an ItemsSource assigned while the previous source is being handed to a waiting table wins over the assignment that released it.")]
        public void VerifyItemsSourceAssignedDuringHandoffWins()
        {
            TableView owner = null;
            TableView waiter = null;
            TableViewSource shared = null;
            List<ShapedPerson> reentrant = null;

            RunOnUIThread.Execute(() =>
            {
                shared = TableViewSource.From(MakeShapedPeople());
                owner = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                waiter = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                var host = new StackPanel();
                host.Children.Add(owner);
                host.Children.Add(waiter);
                LoadContent(host);
            });

            SettleLayout(owner);
            SettleLayout(waiter);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, GetRowsRepeater(waiter).ItemsSourceView?.Count ?? 0, "Precondition: the waiter is refused.");

                reentrant = new List<ShapedPerson>
                {
                    new ShapedPerson("Lena", "Engineer", "Platform"),
                    new ShapedPerson("Tom", "Designer", "Studio"),
                };

                // The waiter is handed the source synchronously from inside the owner's ItemsSource
                // change, and an app reacting to that assigns the owner's ItemsSource again.
                var repeater = GetRowsRepeater(waiter);
                bool reassigned = false;
                long token = repeater.RegisterPropertyChangedCallback(ItemsRepeater.ItemsSourceProperty, (sender, dp) =>
                {
                    if (!reassigned && ((ItemsRepeater)sender).ItemsSource != null)
                    {
                        reassigned = true;
                        owner.ItemsSource = reentrant;
                    }
                });

                owner.ItemsSource = new List<ShapedPerson> { new ShapedPerson("Kai", "Architect", "Research") };
                repeater.UnregisterPropertyChangedCallback(ItemsRepeater.ItemsSourceProperty, token);

                Verify.IsTrue(reassigned, "Precondition: the waiter must claim the source during the owner's reassignment.");
                Verify.AreSame(reentrant, owner.ItemsSource, "Precondition: the nested assignment is the owner's ItemsSource.");
            });

            SettleLayout(owner);
            SettleLayout(waiter);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Lena", "Tom" }, RowValues(owner, person => person.Name),
                    "the owner must render the newest ItemsSource, not the one whose assignment released the shared source");
                VerifySequence(SourceOrderNames, RowValues(waiter, person => person.Name),
                    "the waiter must render the source it claimed");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a comparer sort a former owner left on a TableViewSource is cleared by the next owner and not driven by the former owner's later sorts.")]
        public void VerifyComparerSortDoesNotFollowSourceToNextOwner()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource shared = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                shared = TableViewSource.From(MakeShapedPeople());
                first = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                first.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);
            RunOnUIThread.Execute(() =>
                Verify.IsTrue(first.SortByColumn(first.Columns[0], SortDirection.Ascending), "Precondition: the comparer column sorts."));
            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Owen", "Rafa", "Diego" }, RowValues(first, person => person.Name),
                    "precondition: the comparer orders the shared source");

                // The same item objects, so ranks the first table computes later would also key the
                // shared source if its comparer state were still attached there.
                first.ItemsSource = TableViewSource.From(GetProjectedItems(first).Cast<ShapedPerson>().OrderBy(person => person.Name).ToList());

                second = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                second.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();
                host.Children.Add(second);
            });

            SettleLayout(first);
            SettleLayout(second);

            RunOnUIThread.Execute(() =>
                Verify.IsTrue(first.SortByColumn(first.Columns[0], SortDirection.Descending), "The former owner sorts its new source."));
            SettleLayout(first);

            // Re-project the shared source so it re-reads whatever sort keys it still has.
            RunOnUIThread.Execute(() => shared.Filter(item => Person(item).Name != "Asha"));
            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Diego", "Rafa", "Owen", "Ines", "Asha", "Mei" }, RowValues(first, person => person.Name),
                    "the former owner's new source follows its own comparer sort");
                VerifySequence(new[] { "Diego", "Mei", "Rafa", "Ines", "Owen" }, RowValues(second, person => person.Name),
                    "the shared source must be unsorted once its new owner has dropped the former owner's comparer axis");
                foreach (var column in second.Columns)
                {
                    Verify.AreEqual(SortDirection.None, column.SortDirection,
                        "The new owner must not report a sort for an axis it cleared.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clearing a column on an unloaded table leaves the sort of the shared TableViewSource its new owner shows.")]
        public void VerifyUnloadedTableClearKeepsSharedSourceSort()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource shared = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                shared = TableViewSource.From(MakeShapedPeople());
                first = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);
            RunOnUIThread.Execute(() => shared.Sort("Name", SortDirection.Descending));
            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Descending, first.Columns[0].SortDirection, "Precondition: the source sort lights the first table.");
                host.Children.Remove(first);
                second = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host.Children.Add(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(second, person => person.Name),
                    "precondition: the new owner shows the source sort");
                first.SortByColumn(first.Columns[0], SortDirection.None);
                Verify.AreEqual(SortDirection.None, first.Columns[0].SortDirection, "The unloaded table clears its own column.");
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(second, person => person.Name),
                    "a table that released the shared source must not clear the sort its new owner shows");
                Verify.AreEqual(SortDirection.Descending, second.Columns[0].SortDirection, "The new owner keeps its indicator.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a clear made before the table first loads still holds after it loads.")]
        public void VerifyClearBeforeLoadedStaysCleared()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(TableViewSource.From(MakeShapedPeople()), headers: new[] { "Name", "Role" }, width: 500, height: 300);
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending), "Precondition: the unloaded table sorts.");
                Verify.IsTrue(tableView.ClearSort(), "Precondition: the unloaded table clears its sort.");
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection, "The cleared column must stay cleared once the table loads.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name), "the rows must stay in source order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sorting an unloaded table leaves the shared TableViewSource its new owner shows unchanged.")]
        public void VerifyUnloadedTableSortKeepsSharedSourceOrder()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource shared = null;
            StackPanel host = null;
            int sortingCount = 0;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                shared = TableViewSource.From(MakeShapedPeople());
                first = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);
            RunOnUIThread.Execute(() => shared.Sort("Name", SortDirection.Descending));
            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Descending, first.Columns[0].SortDirection, "Precondition: the source sort lights the first table.");
                host.Children.Remove(first);
                second = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host.Children.Add(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(second, person => person.Name),
                    "precondition: the new owner shows the source sort");
                first.Sorting += (s, e) => sortingCount++;
                first.Sorted += (s, e) => sortedCount++;
                Verify.IsFalse(first.SortByColumn(first.Columns[0], SortDirection.Ascending),
                    "A sort the shared source cannot take must report failure.");
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(second, person => person.Name),
                    "a table that released the shared source must not re-sort the rows its new owner shows");
                Verify.AreEqual(SortDirection.Descending, second.Columns[0].SortDirection, "The new owner keeps its indicator.");
                Verify.AreEqual(0, sortingCount, "A refused sort must not raise Sorting.");
                Verify.AreEqual(0, sortedCount, "A refused sort must not raise Sorted.");
                Verify.AreEqual(SortDirection.Descending, first.Columns[0].SortDirection, "A refused sort leaves the column state unchanged.");
            });
        }
    }

    [TestClass]
    public class TableViewResizeCancelTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a canceled resize applies gripper availability changes deferred while the drag was in flight.")]
        public void VerifyCanceledResizeRefreshesGripperAvailability()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateResizableTable(
                    ("Name", new GridLength(150.0, GridUnitType.Pixel)),
                    ("Role", new GridLength(150.0, GridUnitType.Pixel)));
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var gripper = RequireGripper(tableView, 0);
                var siblingGripper = RequireGripper(tableView, 1);
                Verify.AreEqual(Visibility.Visible, siblingGripper.Visibility, "Precondition: the sibling column is resizable.");

                gripper.BeginDrag();
                // Pins the sibling while availability refreshes are held for the drag.
                tableView.Columns[1].MinWidth = 150.0;
                tableView.Columns[1].MaxWidth = 150.0;
                gripper.EndDrag(true /* canceled */);

                Verify.AreEqual(Visibility.Collapsed, siblingGripper.Visibility,
                    "A canceled drag must still apply the availability change it held back.");
            });
        }
    }
}
