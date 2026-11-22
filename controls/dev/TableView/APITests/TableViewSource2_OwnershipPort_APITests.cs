// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewSortingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2, Milestone 9: the TableViewSourceOwnershipTests scenarios
    // (TableView_SourceOwnership_APITests.cs) run against a TableViewSource2.
    [TestClass]
    public class TableViewSource2OwnershipTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies an ItemsSource assigned while the previous TableViewSource2 is being handed to a waiting table wins over the assignment that released it.")]
        public void VerifyItemsSourceAssignedDuringHandoffWins()
        {
            TableView owner = null;
            TableView waiter = null;
            TableViewSource2 shared = null;
            List<ShapedPerson> reentrant = null;

            RunOnUIThread.Execute(() =>
            {
                shared = TableViewSource2.From(MakeShapedPeople());
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

        // Adapted (drift #64): a v2 TableView reads the applied spec and does not rewrite a source it
        // adopts, so the former owner's comparer level stays on the shared source. What must still
        // hold is that the new owner lights no chevron for it and that the former owner's later
        // sorts do not drive the shared source.
        [TestMethod]
        [TestProperty("Description", "Verifies a comparer sort a former owner left on a TableViewSource2 lights no chevron in the next owner and is not driven by the former owner's later sorts.")]
        public void VerifyComparerSortDoesNotFollowSourceToNextOwner()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource2 shared = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                shared = TableViewSource2.From(MakeShapedPeople());
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

                first.ItemsSource = TableViewSource2.From(GetProjectedItems(first).Cast<ShapedPerson>().OrderBy(person => person.Name).ToList());

                second = CreateTableView(shared, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                second.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();
                host.Children.Add(second);
            });

            SettleLayout(first);
            SettleLayout(second);

            RunOnUIThread.Execute(() =>
                Verify.IsTrue(first.SortByColumn(first.Columns[0], SortDirection.Descending), "The former owner sorts its new source."));
            SettleLayout(first);

            // Re-project the shared source so it re-reads whatever sort levels it still has.
            RunOnUIThread.Execute(() => shared.Filter(item => Person(item).Name != "Asha"));
            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Diego", "Rafa", "Owen", "Ines", "Asha", "Mei" }, RowValues(first, person => person.Name),
                    "the former owner's new source follows its own comparer sort");
                VerifySequence(new[] { "Mei", "Ines", "Owen", "Rafa", "Diego" }, RowValues(second, person => person.Name),
                    "the shared source keeps the ascending comparer level it was left with");
                foreach (var column in second.Columns)
                {
                    Verify.AreEqual(SortDirection.None, column.SortDirection,
                        "The new owner must not claim a sort level another table's comparer applied.");
                }
            });
        }
    }
}
