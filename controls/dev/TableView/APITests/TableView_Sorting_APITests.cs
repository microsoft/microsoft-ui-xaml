// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.CompilerServices;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewSortingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 7 of the TableView test plan: sorting.
    //
    // SUBJECT. The control-level sort API - the state that lives on TableViewColumn.SortDirection,
    // the three verbs and their documented Boolean returns, the Sorting/Sorted events, the two
    // click-to-sort gates, and the reconciliation rule with TableViewSource.Sort. Category 8 owns
    // TableViewSource.Sort itself. Plan section 7.0 writes the split up in full; the short version
    // is that a test belongs here if it would still be meaningful with TableViewSource never
    // mentioned in the test body.
    //
    // KEY RESOLUTION. Several tests reach GetSortMemberPathCore(), which is `overridable` in the
    // IDL and therefore projects as `protected virtual`. A test class cannot call it, so the probe
    // columns at the bottom of this file expose a public passthrough to base.GetSortMemberPathCore().
    [TestClass]
    public class TableViewSortingTests : TableViewApiTestBase
    {
        #region 7.1 Column sort surface

        [TestMethod]
        [TestProperty("Description", "Verifies a fresh column reports the sort defaults declared in TableView.idl.")]
        public void VerifyColumnSortDefaults()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new TableViewColumn();

                Verify.IsTrue(column.CanSort, "CanSort defaults to true (TableView.idl:157).");
                Verify.AreEqual(TableViewSortCycle.AscendingDescending, column.SortCycle,
                    "SortCycle defaults to AscendingDescending (TableView.idl:165).");
                Verify.AreEqual(string.Empty, column.SortMemberPath, "SortMemberPath defaults to empty.");
                Verify.IsNull(column.CustomSortComparer, "CustomSortComparer defaults to null.");
                Verify.AreEqual(SortDirection.None, column.SortDirection,
                    "SortDirection defaults to None (TableView.idl:185).");

                var templateColumn = new TableViewTemplateColumn();

                Verify.IsTrue(templateColumn.CanSort, "A template column shares the base defaults.");
                Verify.AreEqual(SortDirection.None, templateColumn.SortDirection, "A template column starts unsorted.");
                Verify.AreEqual(string.Empty, templateColumn.SortMemberPath, "A template column starts with no explicit path.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the base column returns SortMemberPath verbatim from GetSortMemberPathCore.")]
        public void VerifyGetSortMemberPathReturnsExplicitPath()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new SortPathProbeColumn { SortMemberPath = "DepartmentName" };

                Verify.AreEqual("DepartmentName", column.ReadSortMemberPath(),
                    "The base column returns SortMemberPath verbatim (TableView.idl:192-193).");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a base column with no path and no comparer reports no sort key.")]
        public void VerifyBaseColumnWithNoPathAndNoComparerHasNoSortKey()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new SortPathProbeColumn();

                Verify.AreEqual(string.Empty, column.ReadSortMemberPath(),
                    "The base column reports an empty key when nothing was configured (TableView.idl:175-177).");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the sort pipeline consults a custom column's GetSortMemberPathCore override.")]
        public void VerifyCustomColumnSortMemberPathOverrideDrivesSort()
        {
            TableView tableView = null;
            ProbeColumn column = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 500, 700);

                // The column never mentions Role in a Binding: the only route to it is the override.
                column = new ProbeColumn { Header = "Custom", ValuePath = "Role" };
                tableView.Columns.Add(column);

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(column, SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var roles = RowValues(tableView, person => person.Role);

                VerifySequence(
                    new[] { "Architect", "Designer", "Designer", "Engineer", "Engineer", "Engineer" },
                    roles,
                    "an overridden GetSortMemberPathCore must drive the sort key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the base column infers its sort key from DisplayMemberBinding's path, and an explicit SortMemberPath wins.")]
        public void VerifyDisplayMemberBindingPathIsSortKeyUnlessSortMemberPathIsSet()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new SortPathProbeColumn { DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                Verify.AreEqual("Name", column.ReadSortMemberPath(), "With no SortMemberPath, the displayed path is the sort key.");

                column.SortMemberPath = "Role";
                Verify.AreEqual("Role", column.ReadSortMemberPath(), "An explicit SortMemberPath must win over the binding path.");

                var sourced = new SortPathProbeColumn
                {
                    DisplayMemberBinding = new Binding { Source = new ShapedPerson("X", "Y", "Z"), Path = new PropertyPath("Name") },
                };
                Verify.AreEqual(string.Empty, sourced.ReadSortMemberPath(),
                    "A path relative to an explicit Source is not a member of the row item, so it is not a sort key.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies invoking the header of a DisplayMemberBinding column sorts by the binding path, or by SortMemberPath when set.")]
        public void VerifyHeaderInvokeSortsDisplayMemberBindingColumn()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                tableView.Columns.Add(new TableViewColumn { Header = "Name", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } });
                tableView.Columns.Add(new TableViewColumn
                {
                    Header = "Shown as name",
                    DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") },
                    SortMemberPath = "Role",
                });
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => InvokeHeader(tableView, 0));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The header Invoke must sort the column.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the binding path must be the sort key");

                InvokeHeader(tableView, 1);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[1].SortDirection, "The header Invoke must sort the column.");
                VerifySequence(new[] { "Architect", "Designer", "Designer", "Engineer", "Engineer", "Engineer" },
                    RowValues(tableView, person => person.Role), "SortMemberPath, not the displayed path, must be the sort key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changing a sorted column's DisplayMemberBinding path re-sorts the rows by the new path.")]
        public void VerifyChangingSortedColumnDisplayMemberBindingResorts()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                tableView.Columns.Add(new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } });
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Baseline: the column must sort."));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "baseline: the rows must be sorted by Name");
                tableView.Columns[0].DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The column must keep its direction.");
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the new binding path");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clearing a sorted column's DisplayMemberBinding with no SortMemberPath clears its sort.")]
        public void VerifyClearingSortedColumnDisplayMemberBindingClearsSort()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                tableView.Columns.Add(new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } });
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Baseline: the column must sort."));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "baseline: the rows must be sorted by Name");
                tableView.Columns[0].DisplayMemberBinding = null;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection, "A column left with no sort key must not stay sorted.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name), "clearing the sort must restore source order");
                var indicator = FindSortIndicator(tableView, 0);
                Verify.IsTrue(indicator == null || indicator.Direction == SortIndicatorDirection.None, "The header arrow must be cleared.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Sorting handler that replaces the sorted column's DisplayMemberBinding does not re-enter the sort, and the rows follow the final key.")]
        public void VerifySortingHandlerReplacingSortKeyDoesNotRecurse()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            int sortingCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                tableView.Sorting += (sender, args) =>
                {
                    // Bounded so the pre-fix recursion fails the count instead of overflowing the stack.
                    if (++sortingCount < 50)
                    {
                        column.DisplayMemberBinding = new Binding { Path = new PropertyPath("DepartmentName") };
                    }
                };
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, sortingCount, "A key change made from Sorting must not start another sort.");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(new[] { "Diego", "Rafa", "Owen", "Mei", "Asha", "Ines" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the key the handler left");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changing a sorted column's key raises exactly one Sorting and one Sorted in the existing direction.")]
        public void VerifySortKeyChangeRaisesOneSortTransition()
        {
            VerifySortKeyChangeTransition(cancel: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies cancelling the Sorting raised by a sort key change keeps the previous order and arrow.")]
        public void VerifyCancelledSortKeyChangeKeepsOrder()
        {
            VerifySortKeyChangeTransition(cancel: true);
        }

        private void VerifySortKeyChangeTransition(bool cancel)
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                tableView.Sorting += (sender, args) =>
                {
                    events.Add($"Sorting:{args.Direction}:{args.Column == column}");
                    args.Cancel = cancel;
                };
                tableView.Sorted += (sender, args) => events.Add($"Sorted:{args.Direction}:{args.Column == column}");
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(cancel ? new[] { "Sorting:Ascending:True" } : new[] { "Sorting:Ascending:True", "Sorted:Ascending:True" },
                    events, "a key change must be one transition in the current direction");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                var indicator = FindSortIndicator(tableView, 0);
                Verify.IsNotNull(indicator, "The header must keep its sort affordance.");
                Verify.AreEqual(SortIndicatorDirection.Ascending, indicator.Direction, "The header arrow must stay.");
                VerifySequence(cancel ? NameAscendingOrder : new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name),
                    cancel ? "a cancelled re-sort must keep the previous order" : "the rows must be sorted by the new key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies swapping a sorted column's CustomSortComparer re-sorts with one Sorting/Sorted pair.")]
        public void VerifyCustomSortComparerSwapRaisesOneSortTransition()
        {
            TableView tableView = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Baseline: the column must sort.");
                tableView.Sorting += (sender, args) => events.Add($"Sorting:{args.Direction}");
                tableView.Sorted += (sender, args) => events.Add($"Sorted:{args.Direction}");
                tableView.Columns[0].CustomSortComparer = new ReverseNameComparer();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending", "Sorted:Ascending" }, events, "a comparer swap must be one transition");
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(tableView, person => person.Name),
                    "the rows must follow the new comparer");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Sorting handler that changes the sort key and cancels the key-change re-sort keeps the order, and the dropped change never re-sorts later.")]
        public void VerifyCancellingSortingHandlerKeyChangeIsDropped()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            int sortingCount = 0;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                tableView.Sorting += (sender, args) =>
                {
                    if (++sortingCount == 1)
                    {
                        column.DisplayMemberBinding = new Binding { Path = new PropertyPath("DepartmentName") };
                    }
                    args.Cancel = true;
                };
                tableView.Sorted += (sender, args) => ++sortedCount;
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, sortingCount, "The cancelling handler's own key change must not start another sort.");
                Verify.AreEqual(0, sortedCount, "A cancelled re-sort must not raise Sorted.");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "a cancelled re-sort must keep the previous order");
                Verify.IsFalse(tableView.ClearSort(), "Precondition: the handler cancels the clear.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, sortingCount, "A cancelled clear must raise only its own Sorting.");
                Verify.AreEqual(0, sortedCount, "Nothing may re-sort after a cancelled clear.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a key change from a cancelled Sorting handler wins over a change deferred from an earlier Sorted handler, so the declined key never re-sorts.")]
        public void VerifyCancelledSortingKeyChangeOverridesDeferredChange()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var sortingPaths = new List<string>();
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                bool cancelled = false;
                tableView.Sorting += (sender, args) =>
                {
                    sortingPaths.Add(column.DisplayMemberBinding.Path.Path);
                    if (args.Direction == SortDirection.Descending && !cancelled)
                    {
                        cancelled = true;
                        column.DisplayMemberBinding = new Binding { Path = new PropertyPath("DepartmentName") };
                        args.Cancel = true;
                    }
                };
                tableView.Sorted += (sender, args) =>
                {
                    if (++sortedCount == 1)
                    {
                        column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
                    }
                };
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                Verify.IsFalse(tableView.SortByColumn(column, SortDirection.Descending), "Precondition: the handler cancels the descending sort.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Name", "Role" }, sortingPaths, "no Sorting may follow for the declined key");
                Verify.AreEqual(1, sortedCount, "Only the first sort may complete.");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "a cancelled sort must keep the previous order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a key change from a Sorting handler that cancels a sort nested in a Sorted handler does not re-sort the column later.")]
        public void VerifyNestedCancelledSortingKeyChangeIsNotApplied()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            int sortingCount = 0;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorting += (sender, args) =>
                {
                    ++sortingCount;
                    if (args.Direction == SortDirection.Descending)
                    {
                        column.SortMemberPath = "Role";
                        args.Cancel = true;
                    }
                };
                tableView.Sorted += (sender, args) =>
                {
                    if (++sortedCount == 1)
                    {
                        Verify.IsFalse(tableView.SortByColumn(column, SortDirection.Descending), "Precondition: the nested sort is cancelled.");
                    }
                };
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, sortingCount, "The sort and the cancelled nested sort only.");
                Verify.AreEqual(1, sortedCount, "Nothing may re-sort after the nested cancel.");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the rows must keep the order the outer sort built");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that after a Sorting handler declines a key, re-assigning that key is ignored and a different key re-sorts normally.")]
        public void VerifyKeyChangeAfterDeclinedKeyReSorts()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                bool cancelled = false;
                tableView.Sorting += (sender, args) =>
                {
                    events.Add($"Sorting:{args.Direction}");
                    if (!cancelled)
                    {
                        cancelled = true;
                        column.DisplayMemberBinding = new Binding { Path = new PropertyPath("DepartmentName") };
                        args.Cancel = true;
                    }
                };
                tableView.Sorted += (sender, args) => events.Add($"Sorted:{args.Direction}");
                Verify.IsFalse(tableView.SortByColumn(column, SortDirection.Descending), "Precondition: the handler cancels the sort.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("DepartmentName") };
                VerifySequence(new[] { "Sorting:Descending" }, events, "re-assigning the declined key must not re-sort");
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Descending", "Sorting:Ascending", "Sorted:Ascending" }, events,
                    "a new key must re-sort once in the current direction");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the new key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies returning to the installed key ends a decline, so assigning the declined key again re-sorts once.")]
        public void VerifyDeclinedKeyExpiresWhenKeyReturnsToInstalledKey()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                bool cancelled = false;
                tableView.Sorting += (sender, args) =>
                {
                    events.Add($"Sorting:{args.Direction}");
                    args.Cancel = !cancelled;
                    cancelled = true;
                };
                tableView.Sorted += (sender, args) => events.Add($"Sorted:{args.Direction}");
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending" }, events, "precondition: the key re-sort is cancelled");
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") };
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending", "Sorting:Ascending", "Sorted:Ascending" }, events,
                    "the declined key must re-sort once after the key returned to the installed one");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the new key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cancelled SortByColumn that leaves the key alone does not decline a key change deferred from a Sorted handler.")]
        public void VerifyUnrelatedCancelledSortKeepsDeferredKeyChange()
        {
            VerifyUnrelatedCancelKeepsDeferredKeyChange(clear: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cancelled ClearSort that leaves the key alone does not decline a key change deferred from a Sorted handler.")]
        public void VerifyUnrelatedCancelledClearKeepsDeferredKeyChange()
        {
            VerifyUnrelatedCancelKeepsDeferredKeyChange(clear: true);
        }

        private void VerifyUnrelatedCancelKeepsDeferredKeyChange(bool clear)
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                bool cancelNext = false;
                int sortedCount = 0;
                tableView.Sorting += (sender, args) =>
                {
                    events.Add($"Sorting:{args.Direction}");
                    args.Cancel = cancelNext;
                    cancelNext = false;
                };
                tableView.Sorted += (sender, args) =>
                {
                    events.Add($"Sorted:{args.Direction}");
                    if (++sortedCount == 1)
                    {
                        column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
                    }
                };
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                cancelNext = true;
                Verify.IsFalse(clear ? tableView.ClearSort() : tableView.SortByColumn(column, SortDirection.Descending),
                    "Precondition: the handler cancels the unrelated request.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending", "Sorted:Ascending", clear ? "Sorting:None" : "Sorting:Descending",
                    "Sorting:Ascending", "Sorted:Ascending" }, events, "the deferred key must still re-sort once in the installed direction");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the deferred key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing a column with itself keeps the key its Sorting handler declined.")]
        public void VerifyDeclinedKeySurvivesSameColumnReplacement()
        {
            VerifyDeclinedKeySurvivesColumnMove(replace: true);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing and re-inserting a column keeps the key its Sorting handler declined.")]
        public void VerifyDeclinedKeySurvivesColumnRemoveAndReinsert()
        {
            VerifyDeclinedKeySurvivesColumnMove(replace: false);
        }

        private void VerifyDeclinedKeySurvivesColumnMove(bool replace)
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                bool cancelled = false;
                tableView.Sorting += (sender, args) =>
                {
                    events.Add($"Sorting:{args.Direction}");
                    if (!cancelled)
                    {
                        cancelled = true;
                        column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
                        args.Cancel = true;
                    }
                };
                tableView.Sorted += (sender, args) => events.Add($"Sorted:{args.Direction}");
                Verify.IsFalse(tableView.SortByColumn(column, SortDirection.Descending), "Precondition: the handler cancels the sort.");
                if (replace)
                {
                    tableView.Columns[0] = column;
                }
                else
                {
                    tableView.Columns.RemoveAt(0);
                    tableView.Columns.Insert(0, column);
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Descending" }, events, "the moved column must not re-sort on its declined key");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The moved column must keep its direction.");
                Verify.AreEqual("Role", column.DisplayMemberBinding.Path.Path, "The column must keep the declined key.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the rows must keep the order built before the cancel");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort-key change made while the sorted column is out of Columns re-sorts once after it is re-inserted.")]
        public void VerifyDetachedKeyChangeAppliesOnReinsert()
        {
            VerifyDetachedKeyChangeAppliesOnReturn(replace: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort-key change made while the sorted column is out of Columns re-sorts once after it replaces another column.")]
        public void VerifyDetachedKeyChangeAppliesOnReplace()
        {
            VerifyDetachedKeyChangeAppliesOnReturn(replace: true);
        }

        private void VerifyDetachedKeyChangeAppliesOnReturn(bool replace)
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                tableView.Columns.Add(new TableViewColumn { Header = "Other", DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") } });
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                tableView.Sorting += (sender, args) => events.Add($"Sorting:{args.Direction}");
                tableView.Sorted += (sender, args) => events.Add($"Sorted:{args.Direction}");
                tableView.Columns.RemoveAt(0);
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
                if (replace)
                {
                    tableView.Columns[0] = column;
                }
                else
                {
                    tableView.Columns.Insert(0, column);
                }
                Verify.AreEqual(0, events.Count, "The re-sort must not run inside VectorChanged.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending", "Sorted:Ascending" }, events, "the detached key change must re-sort once");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the new key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changing a declined key away and back while the column is out of Columns ends the decline.")]
        public void VerifyDetachedKeyRoundTripEndsDecline()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                bool cancelled = false;
                tableView.Sorting += (sender, args) =>
                {
                    events.Add($"Sorting:{args.Direction}");
                    args.Cancel = !cancelled;
                    cancelled = true;
                };
                tableView.Sorted += (sender, args) => events.Add($"Sorted:{args.Direction}");
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending" }, events, "precondition: the key re-sort is cancelled");
                tableView.Columns.RemoveAt(0);
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") };
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
                tableView.Columns.Insert(0, column);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending", "Sorting:Ascending", "Sorted:Ascending" }, events,
                    "the key re-assigned while detached must re-sort once");
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the re-assigned key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SortByColumn in the column's current direction applies a declined key in place.")]
        public void VerifySameDirectionSortByColumnAppliesDeclinedKey()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                bool cancelled = false;
                tableView.Sorting += (sender, args) =>
                {
                    events.Add($"Sorting:{args.Direction}");
                    args.Cancel = !cancelled;
                    cancelled = true;
                };
                tableView.Sorted += (sender, args) => events.Add($"Sorted:{args.Direction}");
                column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending" }, events, "precondition: the key re-sort is cancelled");
                Verify.IsTrue(tableView.SortByColumn(column, column.SortDirection), "The declined key must be applied.");
                Verify.IsFalse(tableView.SortByColumn(column, column.SortDirection), "Once applied, the same request is a no-op.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Sorting:Ascending", "Sorting:Ascending", "Sorted:Ascending" }, events,
                    "the declined key must be applied with one Sorting/Sorted pair");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the declined key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sort-key changes made from successive Sorted handlers are each applied on a later turn, leaving the rows on the final key.")]
        public void VerifySortedHandlerKeyChangesApplyOnLaterTurns()
        {
            TableView tableView = null;
            TableViewColumn column = null;
            int sortingCount = 0;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                tableView.Columns.Add(column);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorting += (sender, args) => ++sortingCount;
                tableView.Sorted += (sender, args) =>
                {
                    switch (++sortedCount)
                    {
                        case 1: column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") }; break;
                        case 2: column.DisplayMemberBinding = new Binding { Path = new PropertyPath("DepartmentName") }; break;
                    }
                };
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "The column must sort.");
                Verify.AreEqual(1, sortingCount, "A Sorted handler's key change must not re-sort inside the sort.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(3, sortingCount, "The sort and one re-sort per Sorted key change.");
                Verify.AreEqual(3, sortedCount, "Each re-sort must complete.");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The column must keep its direction.");
                var indicator = FindSortIndicator(tableView, 0);
                Verify.IsNotNull(indicator, "The header must keep its sort affordance.");
                Verify.AreEqual(SortIndicatorDirection.Ascending, indicator.Direction, "The header arrow must stay.");
                VerifySequence(new[] { "Diego", "Rafa", "Owen", "Mei", "Asha", "Ines" },
                    RowValues(tableView, person => person.Name), "the rows must be sorted by the last key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort-key change deferred from a Sorted handler is not applied by a table the column has since moved out of.")]
        public void VerifyDeferredSortKeyChangeSkipsColumnMovedToAnotherTable()
        {
            TableView first = null;
            TableView second = null;
            TableViewColumn column = null;
            int firstSortingCount = 0;

            RunOnUIThread.Execute(() =>
            {
                first = CreateTableViewShell(MakeShapedPeople(), 600, 300);
                second = CreateTableViewShell(MakeShapedPeople(), 600, 300);
                column = new TableViewColumn { Header = "Value", DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") } };
                first.Columns.Add(column);
                var host = new StackPanel();
                host.Children.Add(first);
                host.Children.Add(second);
                LoadContent(host);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                bool changed = false;
                first.Sorted += (sender, args) =>
                {
                    if (!changed)
                    {
                        changed = true;
                        column.DisplayMemberBinding = new Binding { Path = new PropertyPath("Role") };
                    }
                };
                Verify.IsTrue(first.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");
                first.Sorting += (sender, args) => { if (args.Column == column) { ++firstSortingCount; } };
                first.Columns.Remove(column);
                second.Columns.Add(column);
                Verify.IsTrue(second.SortByColumn(column, SortDirection.Ascending), "Precondition: the new table sorts the column.");
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, firstSortingCount, "The old table must not sort a column it no longer owns.");
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection, "The new table's sort must stand.");
                VerifySequence(new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(second, person => person.Name), "the new table sorts by the column's current key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies giving a keyless column a DisplayMemberBinding makes its header sortable.")]
        public void VerifySettingDisplayMemberBindingMakesHeaderSortable()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);
                tableView.Columns.Add(new TableViewColumn { Header = "Value" });
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(FindSortIndicator(tableView, 0), "Baseline: a column with no sort key must not offer a sort affordance.");
                tableView.Columns[0].DisplayMemberBinding = new Binding { Path = new PropertyPath("Name") };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(FindSortIndicator(tableView, 0), "A column that gained a sort key must offer a sort affordance.");
                InvokeHeader(tableView, 0);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "Invoking the header must sort the column.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the rows must be sorted by the new binding path");
            });
        }

        #endregion

        #region 7.2 Programmatic sort state

        [TestMethod]
        [TestProperty("Description", "Verifies SortByColumn sets the requested direction on the column.")]
        [TestProperty("data:Direction", "{Ascending, Descending, None}")]
        public void VerifySortByColumnSetsDirection()
        {
            var directionName = ((string)TestContext.DataRow["Direction"]).Trim();
            var direction = (SortDirection)Enum.Parse(typeof(SortDirection), directionName);
            Log.Comment($"Direction under test: {directionName}.");

            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], direction));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual(direction, tableView.Columns[0].SortDirection,
                    $"{directionName}: the column must report the direction that was requested."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sorting does not narrow the header content, because its padding already reserves the indicator.")]
        public void VerifySortingDoesNotChangeHeaderContentWidth()
        {
            TableView tableView = null;
            double unsortedWidth = 0.0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].Width = new GridLength(200.0, GridUnitType.Pixel);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                unsortedWidth = GetHeaderPresenter(tableView, 0).ActualWidth;
                Verify.IsGreaterThan(unsortedWidth, 0.0, "The header content must have been laid out.");
                tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(unsortedWidth, GetHeaderPresenter(tableView, 0).ActualWidth,
                    "Sorting must not take the indicator's width from the header content a second time.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SortByColumn returns true only when the sort state actually changed.")]
        public void VerifySortByColumnReturnsWhetherStateChanged()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var column = tableView.Columns[0];

                Verify.IsFalse(tableView.SortByColumn(column, SortDirection.None),
                    "Clearing an already-unsorted column changes nothing (TableView.idl:587).");
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending),
                    "The first real sort changes the state.");
                Verify.IsFalse(tableView.SortByColumn(column, SortDirection.Ascending),
                    "Re-applying the same direction to the same column changes nothing.");
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Descending),
                    "Flipping the direction changes the state.");
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.None),
                    "Clearing a sorted column changes the state.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sorting a second column clears the first column's sort state.")]
        public void VerifySortByColumnReplacesPreviousColumnSort()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[1], SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "Sorting is single-column: the previous column must be cleared (TableView.idl:180-181).");
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[1].SortDirection,
                    "The newly sorted column holds the active state.");

                var roles = RowValues(tableView, person => person.Role);

                VerifySequence(
                    new[] { "Engineer", "Engineer", "Engineer", "Designer", "Designer", "Architect" },
                    roles,
                    "the rows must follow the column that currently owns the sort");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies repeated ToggleSortDirection calls walk the sequence named by the column's SortCycle, and wrap.")]
        [TestProperty("data:Cycle", "{AscendingDescending, AscendingDescendingNone, DescendingAscending, DescendingAscendingNone}")]
        public void VerifyToggleSortDirectionCyclesPerSortCycle()
        {
            var cycleName = ((string)TestContext.DataRow["Cycle"]).Trim();
            var cycle = (TableViewSortCycle)Enum.Parse(typeof(TableViewSortCycle), cycleName);
            Log.Comment($"Sort cycle under test: {cycleName}.");

            var expected = cycle switch
            {
                TableViewSortCycle.AscendingDescending => new[]
                {
                    SortDirection.Ascending, SortDirection.Descending, SortDirection.Ascending,
                },
                TableViewSortCycle.AscendingDescendingNone => new[]
                {
                    SortDirection.Ascending, SortDirection.Descending, SortDirection.None, SortDirection.Ascending,
                },
                TableViewSortCycle.DescendingAscending => new[]
                {
                    SortDirection.Descending, SortDirection.Ascending, SortDirection.Descending,
                },
                _ => new[]
                {
                    SortDirection.Descending, SortDirection.Ascending, SortDirection.None, SortDirection.Descending,
                },
            };

            TableView tableView = null;
            var actual = new List<SortDirection>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].SortCycle = cycle;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var column = tableView.Columns[0];

                for (var i = 0; i < expected.Length; i++)
                {
                    tableView.ToggleSortDirection(column);
                    actual.Add(column.SortDirection);
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var detail = $"Expected [{string.Join(", ", expected)}], saw [{string.Join(", ", actual)}].";

                for (var i = 0; i < expected.Length; i++)
                {
                    // The final step is the wrap: it proves the cycle repeats rather than terminating.
                    Verify.AreEqual(expected[i], actual[i],
                        $"{cycleName}: toggle {i + 1} of {expected.Length}. {detail}");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ToggleSortDirection does nothing to a column whose CanSort is false.")]
        public void VerifyToggleSortDirectionRespectsCanSort()
        {
            TableView tableView = null;
            var returned = true;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].CanSort = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => returned = tableView.ToggleSortDirection(tableView.Columns[0]));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(returned, "Toggling a column that opted out of click-to-sort changes no state.");
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A column with CanSort = false must not acquire a sort direction.");

                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "a column that opted out of sorting must leave the rows in source order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SortByColumn refuses a column whose CanSort is false.")]
        public void VerifySortByColumnRespectsCanSort()
        {
            TableView tableView = null;
            var returned = true;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].CanSort = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => returned = tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(returned, "SortByColumn must refuse a column with CanSort = false.");
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A column with CanSort = false must not acquire a sort direction.");

                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "a column that opted out of sorting must leave the rows in source order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies setting CanSort = false on the sorted column clears its sort and raises Sorting/Sorted.")]
        public void VerifyCanSortFalseClearsActiveSort()
        {
            TableView tableView = null;
            var sortingArgs = new List<TableViewSortingEventArgs>();
            var sortedArgs = new List<TableViewSortedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending),
                    "Baseline: the column must sort.");
                tableView.Sorting += (sender, args) => sortingArgs.Add(args);
                tableView.Sorted += (sender, args) => sortedArgs.Add(args);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "baseline: the rows must be sorted");
                tableView.Columns[0].CanSort = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var column = tableView.Columns[0];
                Verify.AreEqual(SortDirection.None, column.SortDirection,
                    "Opting a sorted column out of sorting must clear its sort direction.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "clearing the opted-out column's sort must restore source order");

                var indicator = FindSortIndicator(tableView, 0);
                Verify.IsTrue(indicator == null || indicator.Direction == SortIndicatorDirection.None,
                    "The header arrow must be cleared.");

                Verify.AreEqual(1, sortingArgs.Count, "The clear must raise Sorting once.");
                Verify.AreEqual(column, sortingArgs[0].Column, "Sorting must name the opted-out column.");
                Verify.AreEqual(SortDirection.None, sortingArgs[0].Direction, "Sorting must request None.");
                Verify.AreEqual(1, sortedArgs.Count, "The clear must raise Sorted once.");
                Verify.AreEqual(column, sortedArgs[0].Column, "Sorted must name the opted-out column.");
                Verify.AreEqual(SortDirection.None, sortedArgs[0].Direction, "Sorted must report None.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CanSort = false clear still removes the installed sort when the Sorting handler leaves the column without a sort path.")]
        public void VerifyCanSortFalseClearRemovesSortWhenHandlerDropsSortPath()
        {
            TableView tableView = null;
            var sortedArgs = new List<TableViewSortedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending),
                    "Baseline: the column must sort.");
                tableView.Sorting += (sender, args) =>
                {
                    if (args.Direction == SortDirection.None && args.Column is TableViewColumn column)
                    {
                        column.SortMemberPath = string.Empty;
                    }
                };
                tableView.Sorted += (sender, args) => sortedArgs.Add(args);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "baseline: the rows must be sorted");
                tableView.Columns[0].CanSort = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection, "The opted-out column's sort must be cleared.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "the clear must remove the sort the column installed");
                Verify.AreEqual(1, sortedArgs.Count, "The clear must raise Sorted once.");
                Verify.AreEqual(SortDirection.None, sortedArgs[0].Direction, "Sorted must report None.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a canceled CanSort = false clear keeps the sort, and SortByColumn(column, None) can still clear it.")]
        public void VerifyCanceledCanSortClearCanBeClearedBySortByColumnNone()
        {
            TableView tableView = null;
            var cancel = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Sorting += (sender, args) => args.Cancel = cancel;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var column = tableView.Columns[0];
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending), "Baseline: the column must sort.");

                cancel = true;
                column.CanSort = false;
                cancel = false;
                Verify.AreEqual(SortDirection.Ascending, column.SortDirection,
                    "A Sorting handler that cancels the opt-out clear keeps the sort.");

                Verify.IsFalse(tableView.SortByColumn(column, SortDirection.Descending),
                    "CanSort = false must still refuse a new direction.");
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.None),
                    "SortByColumn(column, None) must clear a sort the opted-out column still holds.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection, "The sort must be cleared.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "clearing the sort must restore source order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearSort resets every column and restores source order.")]
        public void VerifyClearSortClearsEveryColumnAndRestoresSourceOrder()
        {
            TableView tableView = null;
            var returned = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => returned = tableView.ClearSort());

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(returned, "ClearSort returns true when it cleared something (TableView.idl:592-593).");

                foreach (var column in tableView.Columns)
                {
                    Verify.AreEqual(SortDirection.None, column.SortDirection,
                        $"Column '{column.Header}' must report no sort direction after ClearSort.");
                }

                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "ClearSort must restore source order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearSort is a safe no-op that returns false when nothing is sorted.")]
        public void VerifyClearSortReturnsFalseWhenNothingIsSorted()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.IsFalse(tableView.ClearSort(), "Clearing a never-sorted table changes nothing."));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.ClearSort(), "The first clear after a real sort changes state.");
                Verify.IsFalse(tableView.ClearSort(), "A second consecutive clear is a no-op.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CanUserSortColumns gates only the gesture, never the programmatic API.")]
        public void VerifyCanUserSortColumnsFalseStillAllowsProgrammaticSort()
        {
            TableView tableView = null;
            var returned = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.CanUserSortColumns = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                returned = tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(returned, "SortByColumn must work while CanUserSortColumns is false (TableView.idl:577-579).");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection,
                    "The column must hold the programmatically applied direction.");

                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "a programmatic sort must reorder rows even with the gesture gate closed");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies no sort affordance is built while CanUserSortColumns is false.")]
        public void VerifyCanUserSortColumnsFalseBuildsNoSortAffordance()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.CanUserSortColumns = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var indicator = FindSortIndicator(tableView, 0);

                // "No affordance" is not spelled out as absent-versus-collapsed, so either satisfies it.
                var absent = indicator == null || indicator.Visibility == Visibility.Collapsed;
                Verify.IsTrue(absent,
                    "No sort affordance may be offered while CanUserSortColumns is false (TableView.idl:577-579).");

                tableView.CanUserSortColumns = true;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var indicator = FindSortIndicator(tableView, 0);
                if (indicator == null)
                {
                    Verify.Fail("Re-opening the gate must build the sort affordance.");
                    return;
                }

                Verify.AreEqual(Visibility.Visible, indicator.Visibility,
                    "The affordance must be visible once the gate is open again.");
            });
        }

        #endregion

        #region 7.3 Sort ordering results

        [TestMethod]
        [TestProperty("Description", "Verifies sorting by a column orders rows by the column's sort member path.")]
        public void VerifySortBySortMemberPathOrdersRows()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].SortMemberPath = "Name";
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "a column sort must order the rows by its sort member path"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies descending order is the exact reverse of ascending order.")]
        public void VerifySortDescendingReversesOrder()
        {
            TableView tableView = null;
            List<string> ascending = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => ascending = RowValues(tableView, person => person.Name));

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var expected = Enumerable.Reverse(ascending).ToList();
                VerifySequence(expected, RowValues(tableView, person => person.Name),
                    "descending must be the exact reverse of ascending");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CustomSortComparer drives the row order and is actually called.")]
        public void VerifyCustomSortComparerOrdersRows()
        {
            TableView tableView = null;
            LengthThenOrdinalComparer comparer = null;

            RunOnUIThread.Execute(() =>
            {
                comparer = new LengthThenOrdinalComparer();

                tableView = CreateSortTable();
                tableView.Columns[0].CustomSortComparer = comparer;

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsGreaterThan(comparer.CompareCount, 0,
                    "A supplied comparer must be the thing doing the comparing.");

                // Deliberately not alphabetical, so a default comparison cannot pass by coincidence.
                VerifySequence(
                    new[] { "Mei", "Asha", "Ines", "Owen", "Rafa", "Diego" },
                    RowValues(tableView, person => person.Name),
                    "the comparer's order must be the row order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CustomSortComparer takes precedence over SortMemberPath.")]
        public void VerifyCustomSortComparerTakesPrecedenceOverSortMemberPath()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                // The path and the comparer produce different orders on this data, so exactly one of
                // them can be the explanation for what the test sees.
                tableView.Columns[0].SortMemberPath = "Name";
                tableView.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(
                    new[] { "Mei", "Asha", "Ines", "Owen", "Rafa", "Diego" },
                    RowValues(tableView, person => person.Name),
                    "the comparer must win over SortMemberPath (TableView.idl:179-180)"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies items with equal sort keys keep their source-relative order.")]
        public void VerifySortIsStableForEqualKeys()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            // Role has three ties: one Architect, two Designers, three Engineers.
            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[1], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(
                    new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name),
                    "tied items must keep their source-relative order"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an item added while sorted keeps the sort and lands at its sorted position.")]
        public void VerifySortSurvivesItemAddition()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateSortTable(items);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            // "Nadia" sorts between "Mei" and "Owen", so appending at the end would be visible.
            RunOnUIThread.Execute(() => items.Add(new ShapedPerson("Nadia", "Engineer", "Platform")));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection,
                    "A collection change must not drop the active sort.");

                VerifySequence(
                    new[] { "Asha", "Diego", "Ines", "Mei", "Nadia", "Owen", "Rafa" },
                    RowValues(tableView, person => person.Name),
                    "an item added while sorted must land at its sorted position");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sorting a column with neither a path nor a comparer leaves the rows alone.")]
        public void VerifyColumnWithNoSortKeyDoesNotReorderRows()
        {
            TableView tableView = null;
            TableViewColumn keyless = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                // A base column with no Binding to fall back on, so it has no sort key at all.
                keyless = new TableViewColumn { Header = "Keyless" };
                tableView.Columns.Add(keyless);

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(keyless, SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "a column with no sort key must not reorder anything (TableView.idl:175-177)");

                // What SortDirection should read here is not specified. Logged, not asserted.
                Log.Comment($"SortDirection on a keyless column after SortByColumn: {keyless.SortDirection}.");
            });
        }

        #endregion

        #region 7.4 Sort events

        [TestMethod]
        [TestProperty("Description", "Verifies Sorting is raised before the state changes, carrying the column and the pending direction.")]
        public void VerifySortingFiresBeforeStateChangeWithColumnAndDirection()
        {
            TableView tableView = null;
            TableViewColumn seenColumn = null;
            var seenDirection = SortDirection.None;
            var directionInsideHandler = SortDirection.Descending;
            var raiseCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                tableView.Sorting += (sender, args) =>
                {
                    raiseCount++;
                    seenColumn = args.Column;
                    seenDirection = args.Direction;
                    directionInsideHandler = tableView.Columns[0].SortDirection;
                };

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, raiseCount, "Sorting must be raised exactly once for one sort.");
                Verify.AreEqual(tableView.Columns[0], seenColumn, "The args must carry the trigger column.");
                Verify.AreEqual(SortDirection.Ascending, seenDirection, "The args must carry the pending direction.");
                Verify.AreEqual(SortDirection.None, directionInsideHandler,
                    "Sorting is raised BEFORE the state change, so the handler must still see the old direction (TableView.idl:595-596).");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies cancelling Sorting leaves both the row order and every column's sort state untouched.")]
        public void VerifySortingCancelLeavesOrderAndSortStateUnchanged()
        {
            TableView tableView = null;
            var returned = true;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Sorting += (sender, args) => args.Cancel = true;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                returned = tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(returned, "A cancelled sort changed no state, so it must not report that it did.");

                foreach (var column in tableView.Columns)
                {
                    Verify.AreEqual(SortDirection.None, column.SortDirection,
                        $"Column '{column.Header}' must be untouched by a cancelled sort (TableView.idl:251-254).");
                }

                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "a cancelled sort must leave the row order alone");

                var header = (Control)GetHeaderCell(tableView, 0);
                var sortState = GetCurrentVisualState(header, "SortStates");
                Verify.IsTrue(sortState == "Unsorted" || sortState == "UnsortedSortAvailable",
                    "A cancelled sort must keep the header unsorted, allowing its hover or keyboard-focus affordance.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Sorted is raised once after the new order has been applied.")]
        public void VerifySortedFiresAfterSuccessfulSort()
        {
            TableView tableView = null;
            TableViewColumn seenColumn = null;
            var seenDirection = SortDirection.None;
            var directionInsideHandler = SortDirection.None;
            List<string> orderInsideHandler = null;
            var raiseCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                tableView.Sorted += (sender, args) =>
                {
                    raiseCount++;
                    seenColumn = args.Column;
                    seenDirection = args.Direction;
                    directionInsideHandler = tableView.Columns[0].SortDirection;
                    orderInsideHandler = RowValues(tableView, person => person.Name);
                };

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, raiseCount, "Sorted must be raised exactly once per successful sort.");
                Verify.AreEqual(tableView.Columns[0], seenColumn, "The args must carry the column that changed.");
                Verify.AreEqual(SortDirection.Ascending, seenDirection, "The args must carry the applied direction.");
                Verify.AreEqual(SortDirection.Ascending, directionInsideHandler,
                    "Sorted is raised AFTER the change, so the handler must see the new direction (TableView.idl:598-599).");

                VerifySequence(NameAscendingOrder, orderInsideHandler,
                    "the new order must already be in place when Sorted runs");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cancelled sort raises no Sorted event.")]
        public void VerifySortedDoesNotFireWhenCanceled()
        {
            TableView tableView = null;
            var sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Sorting += (sender, args) => args.Cancel = true;
                tableView.Sorted += (sender, args) => sortedCount++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending);
                tableView.ToggleSortDirection(tableView.Columns[1]);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual(0, sortedCount,
                    "A cancelled sort must not report completion (TableView.idl:252-253)."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearSort raises both sort events with the null-column clear-all sentinel.")]
        public void VerifySortEventsUseNullColumnSentinelForClearSort()
        {
            TableView tableView = null;
            var sortingRaises = 0;
            var sortedRaises = 0;
            TableViewColumn sortingColumn = null;
            TableViewColumn sortedColumn = null;
            var sortingDirection = SortDirection.Ascending;
            var sortedDirection = SortDirection.Ascending;
            var handlersArmed = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                tableView.Sorting += (sender, args) =>
                {
                    if (!handlersArmed)
                    {
                        return;
                    }

                    sortingRaises++;
                    sortingColumn = args.Column;
                    sortingDirection = args.Direction;
                };

                tableView.Sorted += (sender, args) =>
                {
                    if (!handlersArmed)
                    {
                        return;
                    }

                    sortedRaises++;
                    sortedColumn = args.Column;
                    sortedDirection = args.Direction;
                };

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                handlersArmed = true;
                tableView.ClearSort();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, sortingRaises, "ClearSort must raise Sorting once.");
                Verify.AreEqual(1, sortedRaises, "ClearSort must raise Sorted once.");
                Verify.IsNull(sortingColumn, "A null Column is the clear-all sentinel on Sorting (TableView.idl:245).");
                Verify.IsNull(sortedColumn, "A null Column is the clear-all sentinel on Sorted (TableView.idl:261).");
                Verify.AreEqual(SortDirection.None, sortingDirection, "A clear-all is a change to None.");
                Verify.AreEqual(SortDirection.None, sortedDirection, "A clear-all is a change to None.");
            });
        }

        #endregion

        #region 7.5 Sort edge cases

        [TestMethod]
        [TestProperty("Description", "Verifies two columns naming the same sort member path keep independent sort state.")]
        public void VerifyDuplicateSortMemberPathsTrackColumnIdentity()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);

                // The IDL calls this shape out explicitly: one column formats a value, another ranks
                // the same one. Both name the same path.
                tableView.Columns.Add(new TableViewTemplateColumn
                {
                    Header = "Name",
                    CellTemplate = MakeTextCellTemplate("Name"),
                    SortMemberPath = "Name",
                });

                tableView.Columns.Add(new TableViewTemplateColumn
                {
                    Header = "Name again",
                    CellTemplate = MakeTextCellTemplate("Name"),
                    SortMemberPath = "Name",
                });

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[1], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[1].SortDirection,
                    "The sorted column holds the state.");
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "Sort state is keyed on column identity, not on the path (TableView.idl:177-181).");

                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "a duplicate path must still sort correctly");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing the sorted column leaves the control coherent and still sortable.")]
        public void VerifyRemovingSortedColumnClearsSortState()
        {
            TableView tableView = null;
            TableViewColumn removed = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                removed = tableView.Columns[0];
                tableView.SortByColumn(removed, SortDirection.Ascending);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.Columns.Remove(removed));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                foreach (var column in tableView.Columns)
                {
                    Verify.AreEqual(SortDirection.None, column.SortDirection,
                        $"No remaining column may claim the sort that left with column '{removed.Header}'.");
                }

                // Whether the rows revert to source order or keep the departed column's ordering is
                // not specified anywhere. Logged for the spec question, not asserted.
                Log.Comment($"Row order after removing the sorted column: [{string.Join(", ", RowValues(tableView, person => person.Name))}].");

                // The real risk is a dangling reference to the removed column on the next sort.
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending),
                    "The table must still be sortable after losing the sorted column.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "The post-removal sort must take effect normally."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies moving or re-setting the sorted column in Columns keeps its sort.")]
        public void VerifyMovingSortedColumnKeepsSort()
        {
            TableView tableView = null;
            TableViewColumn name = null;
            var sortedEvents = new List<SortDirection>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                name = tableView.Columns[0];
                Verify.IsTrue(tableView.SortByColumn(name, SortDirection.Ascending), "Precondition: the Name column sorts.");
                tableView.Sorted += (s, e) => sortedEvents.Add(e.Direction);
            });

            SettleLayout(tableView);

            // IObservableVector has no Move: RemoveAt + Insert of the same instance is the move.
            RunOnUIThread.Execute(() =>
            {
                tableView.Columns.RemoveAt(0);
                tableView.Columns.Insert(1, name);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, tableView.Columns.IndexOf(name), "Precondition: the Name column moved.");
                Verify.AreEqual(SortDirection.Ascending, name.SortDirection, "A moved column keeps its sort.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "the rows must stay sorted by the moved column");
                Verify.AreEqual(0, sortedEvents.Count, "A move changes no sort, so Sorted must not be raised.");

                tableView.Columns[1] = name;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, name.SortDirection, "Re-setting the same column keeps its sort.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "the rows must stay sorted after the column is re-set in place");
                Verify.AreEqual(0, sortedEvents.Count, "Re-setting the same column changes no sort.");

                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending),
                    "Another column must still sort after the move.");
                Verify.AreEqual(SortDirection.None, name.SortDirection, "The new sort replaces the moved column's.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort change while the sorted column is out of Columns stops its re-insert from restoring the sort.")]
        public void VerifyMovedColumnSortNotRestoredAfterInterveningChange()
        {
            var cases = new (string Name, Action<TableView, TableViewColumn> Intervene, Action<TableView, TableViewColumn> Reinsert)[]
            {
                ("CanUserSortColumns off", (t, c) => t.CanUserSortColumns = false, (t, c) => t.Columns.Insert(1, c)),
                ("CanSort off while detached", (t, c) => c.CanSort = false, (t, c) => t.Columns.Insert(1, c)),
                ("ClearSort", (t, c) => t.ClearSort(), (t, c) => t.Columns.Insert(1, c)),
                ("another column sorted then cleared", (t, c) =>
                {
                    Verify.IsTrue(t.SortByColumn(t.Columns[0], SortDirection.Ascending), "Precondition: Role sorts.");
                    Verify.IsTrue(t.SortByColumn(t.Columns[0], SortDirection.None), "Precondition: Role's sort clears.");
                }, (t, c) => t.Columns.Insert(1, c)),
                ("Columns cleared", (t, c) =>
                {
                    t.Columns.Clear();
                    t.Columns.Add(new TableViewTemplateColumn { Header = "Role" });
                }, (t, c) => t.Columns.Insert(0, c)),
                ("different column inserted", (t, c) => { }, (t, c) => t.Columns.Insert(0, new TableViewTemplateColumn { Header = "Other" })),
            };

            foreach (var testCase in cases)
            {
                Log.Comment($"Case: {testCase.Name}");
                TableView tableView = null;
                TableViewColumn name = null;
                var sorted = new List<(TableViewColumn Column, SortDirection Direction)>();

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateSortTable();
                    LoadContent(tableView);
                });

                SettleLayout(tableView);

                RunOnUIThread.Execute(() =>
                {
                    name = tableView.Columns[0];
                    Verify.IsTrue(tableView.SortByColumn(name, SortDirection.Ascending), "Precondition: the Name column sorts.");
                    tableView.Sorted += (s, e) => sorted.Add((e.Column, e.Direction));
                });

                SettleLayout(tableView);

                RunOnUIThread.Execute(() =>
                {
                    tableView.Columns.RemoveAt(0);
                    testCase.Intervene(tableView, name);
                    testCase.Reinsert(tableView, name);
                });

                SettleLayout(tableView);

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(SortDirection.None, name.SortDirection, $"{testCase.Name}: the sort must not come back.");
                    foreach (var column in tableView.Columns)
                    {
                        Verify.AreEqual(SortDirection.None, column.SortDirection, $"{testCase.Name}: no column may be lit.");
                    }
                    VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                        $"{testCase.Name}: the rows must return to source order");
                    Verify.IsTrue(sorted.Count > 0 && sorted[sorted.Count - 1].Column == null && sorted[sorted.Count - 1].Direction == SortDirection.None,
                        $"{testCase.Name}: the last Sorted must report the clear.");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing the sorted column in place with another instance clears the sort.")]
        public void VerifyReplacingSortedColumnWithAnotherInstanceClearsSort()
        {
            TableView tableView = null;
            TableViewColumn name = null;
            var sorted = new List<TableViewColumn>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                name = tableView.Columns[0];
                Verify.IsTrue(tableView.SortByColumn(name, SortDirection.Ascending), "Precondition: the Name column sorts.");
                tableView.Sorted += (s, e) => sorted.Add(e.Column);
                tableView.Columns[0] = new TableViewTemplateColumn { Header = "Other" };
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, name.SortDirection, "The replaced column leaves its sort behind.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "the rows must return to source order");
                Verify.AreEqual(1, sorted.Count, "The clear must raise Sorted once.");
                Verify.IsNull(sorted[0], "The clear reports no column.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a column sort and a TableViewSource sort reconcile, last writer winning.")]
        public void VerifySortReconcilesWithTableViewSourceSort()
        {
            TableView tableView = null;
            TableViewSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "the column sort is the last writer so far"));

            // The source now writes the other axis. Exactly one axis is ever in force.
            RunOnUIThread.Execute(() => source.Sort("Role", SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(
                    new[] { "Engineer", "Engineer", "Engineer", "Designer", "Designer", "Architect" },
                    RowValues(tableView, person => person.Role),
                    "the source sort must replace the column sort, not stack with it");

                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A source-owned sort must clear the column indicator (TableView.idl:571-573).");
            });

            // And back the other way: the column writes last and must win again.
            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(
                    Enumerable.Reverse(NameAscendingOrder).ToList(),
                    RowValues(tableView, person => person.Name),
                    "the column sort must replace the source sort when it writes last");

                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "The column that wrote last owns the indicator.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clearing a column lit by an app-declared TableViewSource sort removes that sort.")]
        [TestProperty("data:Clear", "{SortByColumnNone, HeaderInvoke}")]
        public void VerifyClearingColumnLitBySourceSortRemovesSourceSort()
        {
            var clear = ((string)TestContext.DataRow["Clear"]).Trim();
            TableView tableView = null;
            TableViewSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                tableView.Columns[0].SortCycle = TableViewSortCycle.AscendingDescendingNone;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "Precondition: the source sort lights the Name column.");
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(tableView, person => person.Name),
                    "precondition: the source sort orders the rows");

                switch (clear)
                {
                    case "SortByColumnNone":
                        tableView.SortByColumn(tableView.Columns[0], SortDirection.None);
                        break;
                    default:
                        var provider = CreateColumnHeaderPeer(tableView, tableView.Columns[0])
                            .GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                        provider.Invoke();
                        break;
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection, $"{clear}: the column must be cleared.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    $"{clear}: the clear must remove the app-declared sort");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies moving, replacing or ungating the columns keeps an app-declared TableViewSource sort and re-lights the matching column.")]
        [TestProperty("data:Change", "{MoveColumn, ReplaceColumns, CanUserSortColumnsFalse}")]
        public void VerifyImplicitClearKeepsSourceSort()
        {
            var change = ((string)TestContext.DataRow["Change"]).Trim();
            TableView tableView = null;
            TableViewSource source = null;
            var sortedArgs = new List<TableViewSortedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "Precondition: the source sort lights the Name column.");
                tableView.Sorted += (s, e) => sortedArgs.Add(e);

                switch (change)
                {
                    case "MoveColumn":
                        var column = tableView.Columns[0];
                        tableView.Columns.RemoveAt(0);
                        tableView.Columns.Insert(1, column);
                        break;
                    case "ReplaceColumns":
                        tableView.Columns.Clear();
                        tableView.Columns.Add(MakeTextTemplateColumn("Name"));
                        tableView.Columns.Add(MakeTextTemplateColumn("Role"));
                        break;
                    default:
                        tableView.CanUserSortColumns = false;
                        break;
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(tableView, person => person.Name),
                    $"{change}: the app-declared sort must stay on the source");
                var nameColumn = tableView.Columns.First(c => c.Header as string == "Name");
                Verify.AreEqual(SortDirection.Descending, nameColumn.SortDirection,
                    $"{change}: the Name column must re-light from the source sort");
                Verify.IsFalse(sortedArgs.Any(e => e.Column == null && e.Direction == SortDirection.None),
                    $"{change}: a clear that left the source sort in place must not report that sorting was cleared");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies turning CanUserSortColumns off right after the app sorts the source removes only the table's axis and reports no clear.")]
        public void VerifyUngatingAfterAppSourceSortDoesNotReportClear()
        {
            TableView tableView = null;
            TableViewSource source = null;
            var sortedArgs = new List<TableViewSortedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the Name column sorts.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorted += (s, e) => sortedArgs.Add(e);
                // Same tick: the table's own axis is still installed when the gate closes.
                source.Sort("Role", SortDirection.Descending);
                tableView.CanUserSortColumns = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(sortedArgs.Any(e => e.Column == null && e.Direction == SortDirection.None),
                    "Removing the table's axis while the app's sort remains must not report that sorting was cleared.");
                var roleColumn = tableView.Columns.First(c => c.Header as string == "Role");
                var nameColumn = tableView.Columns.First(c => c.Header as string == "Name");
                Verify.AreEqual(SortDirection.Descending, roleColumn.SortDirection, "The app's Role sort must light its column.");
                Verify.AreEqual(SortDirection.None, nameColumn.SortDirection, "The table's own Name sort must be gone.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies filter changes under an app sort no column describes raise no Sorted.")]
        [TestProperty("data:Sort", "{KeySelector, UnmatchedPath}")]
        public void VerifyFilterUnderUnmatchedAppSortRaisesNoSorted()
        {
            var sort = ((string)TestContext.DataRow["Sort"]).Trim();
            TableView tableView = null;
            TableViewSource source = null;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                if (sort == "KeySelector")
                {
                    source.Sort(item => ((ShapedPerson)item).Name, SortDirection.Ascending);
                }
                else
                {
                    source.Sort("DepartmentName", SortDirection.Ascending);
                }
            });

            SettleLayout(tableView);

            for (int i = 0; i < 3; i++)
            {
                RunOnUIThread.Execute(() =>
                {
                    if (i == 0)
                    {
                        tableView.Sorted += (s, e) => sortedCount++;
                    }
                    source.Filter(_ => true);
                });
                SettleLayout(tableView);
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, sortedCount, $"{sort}: a filter change must not report a sort change.");
                Verify.IsTrue(tableView.Columns.All(c => c.SortDirection == SortDirection.None), $"{sort}: no column describes the app's sort.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies turning sorting off for the table or the column leaves a column the app's source sort lit untouched and raises no sort events, also while the table is unloaded.")]
        [TestProperty("data:Change", "{CanUserSortColumnsFalse, CanSortFalse, CanUserSortColumnsFalseUnloaded, CanSortFalseUnloaded}")]
        public void VerifyUngatingKeepsAppLitColumnWithoutEvents()
        {
            var change = ((string)TestContext.DataRow["Change"]).Trim();
            var unloaded = change.EndsWith("Unloaded");
            TableView tableView = null;
            TableViewSource source = null;
            int sortingCount = 0;
            int sortedCount = 0;
            int directionChanges = 0;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "Precondition: the source sort lights the Name column.");
            });

            if (unloaded)
            {
                RunOnUIThread.Execute(() => Content = null);
                IdleSynchronizer.Wait();
            }

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorting += (s, e) => sortingCount++;
                tableView.Sorted += (s, e) => sortedCount++;
                tableView.Columns[0].RegisterPropertyChangedCallback(TableViewColumn.SortDirectionProperty, (s, dp) => directionChanges++);
                if (change.StartsWith("CanSortFalse"))
                {
                    tableView.Columns[0].CanSort = false;
                }
                else
                {
                    tableView.CanUserSortColumns = false;
                }
            });

            if (unloaded)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => LoadContent(tableView));
            }

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, sortingCount, "The table installed no sort, so there is nothing to clear and no Sorting.");
                Verify.AreEqual(0, sortedCount, "No Sorted may be raised.");
                Verify.AreEqual(0, directionChanges, "The column's SortDirection must not flip.");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The column keeps describing the app's sort.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the app's sort stays on the source");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an unloaded table does not treat the sort another table installed on their shared source as app-lit: CanSort = false drops its own stale state and leaves that sort alone.")]
        public void VerifyUngatingWhileUnloadedLeavesOtherTablesSort()
        {
            TableViewSource source = null;
            TableView first = null;
            TableView second = null;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                first = CreateSortTable(source);
                second = CreateSortTable(source);
                LoadContent(first);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Ascending));

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, first.Columns[0].SortDirection, "Precondition: the source sort lights the first table's Name column.");
                LoadContent(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(second.SortByColumn(second.Columns[0], SortDirection.Descending), "Precondition: the loaded table sorts the shared source.");
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                first.Sorted += (s, e) => sortedCount++;
                first.Columns[0].CanSort = false;
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, sortedCount, "The unloaded table clears its own stale sort.");
                Verify.AreEqual(SortDirection.None, first.Columns[0].SortDirection, "The unloaded table's opted-out column drops its sort.");
                Verify.AreEqual(SortDirection.Descending, second.Columns[0].SortDirection, "The owning table keeps its sort.");
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(second, person => person.Name), "the owning table's sort stays on the source");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort opt-out rebuilds the headers even when a Sorted handler throws.")]
        [TestProperty("data:OptOut", "{CanSort, CanUserSortColumns}")]
        public void VerifySortOptOutRebuildsHeadersWhenSortedHandlerThrows()
        {
            var optOut = ((string)TestContext.DataRow["OptOut"]).Trim();
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the column sorts.");
                tableView.Sorted += (s, e) => throw new InvalidOperationException("Sorted handler fault");
                try
                {
                    if (optOut == "CanSort")
                    {
                        tableView.Columns[0].CanSort = false;
                    }
                    else
                    {
                        tableView.CanUserSortColumns = false;
                    }
                }
                catch (Exception e)
                {
                    Log.Comment($"{optOut}: the handler fault surfaced: {e.Message}");
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var indicator = FindSortIndicator(tableView, 0);
                Verify.IsTrue(indicator == null || indicator.Visibility == Visibility.Collapsed,
                    $"{optOut}: the header must drop its sort affordance even though a Sorted handler threw.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a clear queued by removing the sorted column leaves the app-declared sort of the next ItemsSource alone.")]
        public void VerifyColumnRemovalClearKeepsNewItemsSourceSort()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable(TableViewSource.From(MakeShapedPeople()));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[1], SortDirection.Ascending),
                "Precondition: the Role column sorts."));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Columns.RemoveAt(1);
                var next = TableViewSource.From(MakeShapedPeople());
                next.Sort("Name", SortDirection.Descending);
                tableView.ItemsSource = next;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(tableView, person => person.Name),
                    "the new ItemsSource must keep the sort the app declared on it");
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "The Name column must reflect the new source's sort.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies invoking a column header toggles the sort and updates the sort indicator.")]
        public void VerifyHeaderInvokeTogglesSortAndUpdatesIndicator()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var provider = CreateColumnHeaderPeer(tableView, tableView.Columns[0])
                    .GetPattern(PatternInterface.Invoke) as IInvokeProvider;

                if (provider == null)
                {
                    Verify.Fail("A sortable header must offer the Invoke pattern.");
                    return;
                }

                provider.Invoke();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection,
                    "Invoking a header must advance the column's SortCycle by one step.");

                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "the gesture must drive the same pipeline the API drives");

                var indicator = FindSortIndicator(tableView, 0);
                if (indicator == null)
                {
                    Verify.Fail("A sortable header must carry a SortIndicator.");
                    return;
                }

                Verify.AreEqual(SortIndicatorDirection.Ascending, indicator.Direction,
                    "The glyph must follow the sort state.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies invoking a header over a list holding one object twice neither throws nor changes sort state.")]
        public void VerifySortOverDuplicateItemFailsSafely()
        {
            TableView tableView = null;
            int sortingCount = 0;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                var dup = new ShapedPerson("Zoe", "Engineer", "Platform");
                tableView = CreateSortTable(new List<ShapedPerson>
                {
                    dup,
                    new ShapedPerson("Bea", "Designer", "Studio"),
                    dup,
                    new ShapedPerson("Cal", "Architect", "Research"),
                });
                tableView.Sorting += (sender, args) => sortingCount++;
                tableView.Sorted += (sender, args) => sortedCount++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => InvokeHeader(tableView, 0));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A refused sort must leave the column unsorted.");
                VerifySequence(new[] { "Zoe", "Bea", "Zoe", "Cal" }, RowValues(tableView, person => person.Name),
                    "a refused sort must keep the source order");
                Verify.AreEqual(0, sortingCount, "Sorting must not be raised for a refused sort.");
                Verify.AreEqual(0, sortedCount, "Sorted must not be raised for a refused sort.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies invoking a header over a list holding a null item neither throws nor changes sort state.")]
        public void VerifySortOverNullItemFailsSafely()
        {
            TableView tableView = null;
            int sortingCount = 0;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable(new List<ShapedPerson>
                {
                    new ShapedPerson("Zoe", "Engineer", "Platform"),
                    null,
                    new ShapedPerson("Bea", "Designer", "Studio"),
                });
                tableView.Sorting += (sender, args) => sortingCount++;
                tableView.Sorted += (sender, args) => sortedCount++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => InvokeHeader(tableView, 0));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A refused sort must leave the column unsorted.");
                VerifySequence(new[] { "Zoe", "<not a ShapedPerson>", "Bea" }, RowValues(tableView, person => person.Name),
                    "a refused sort must keep the source order");
                Verify.AreEqual(0, sortingCount, "Sorting must not be raised for a refused sort.");
                Verify.AreEqual(0, sortedCount, "Sorted must not be raised for a refused sort.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SortByColumn returns false over duplicate rows, and sorts once the duplicate is removed.")]
        public void VerifySortByColumnReturnsFalseForDuplicateRows()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                var dup = new ShapedPerson("Zoe", "Engineer", "Platform");
                items = new ObservableCollection<ShapedPerson>
                {
                    dup,
                    new ShapedPerson("Bea", "Designer", "Studio"),
                    dup,
                    new ShapedPerson("Cal", "Architect", "Research"),
                };
                tableView = CreateSortTable(items);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending),
                    "Sorting rows that include one object twice must be refused.");
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A refused sort must leave the column unsorted.");

                items.RemoveAt(2);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending),
                    "With the duplicate gone the sort must apply.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection);
                VerifySequence(new[] { "Bea", "Cal", "Zoe" }, RowValues(tableView, person => person.Name),
                    "the rows must sort once every row is a distinct object");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CustomSortComparer's ranks do not keep a removed row alive.")]
        public void VerifyCustomSortReleasesRemovedRow()
        {
            VerifyCustomSortReleasesRemovedRowCore(filtered: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CustomSortComparer's ranks do not keep a row removed under a filter alive.")]
        public void VerifyCustomSortReleasesRowRemovedUnderFilter()
        {
            VerifyCustomSortReleasesRemovedRowCore(filtered: true);
        }

        private void VerifyCustomSortReleasesRemovedRowCore(bool filtered)
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;
            WeakReference weak = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>();
                for (int i = 0; i < 199; i++)
                {
                    items.Add(new ShapedPerson($"P{i:D3}", "Engineer", "Platform"));
                }
                // Longest name, so the comparer sorts it last, and last in the source: never realized.
                weak = AddUntracked(items, "Zed-last-row");

                object source = filtered ? (object)TableViewSource.From(items).Filter(_ => true) : items;
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 600, height: 150);
                tableView.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending),
                    "Precondition: the comparer sort applies."));

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => items.RemoveAt(items.Count - 1));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();

                Verify.IsFalse(weak.IsAlive, "A removed row must not stay alive in the comparer's ranks.");
                GC.KeepAlive(tableView);
            });
        }

        // Not inlined, so no local here keeps the item alive for the caller's GC.
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static WeakReference AddUntracked(ObservableCollection<ShapedPerson> items, string name)
        {
            var person = new ShapedPerson(name, "Engineer", "Platform");
            items.Add(person);
            return new WeakReference(person);
        }

        #endregion
    }

    // A probe exposing the protected GetSortMemberPathCore so a test can assert path resolution
    // directly rather than inferring it from a resulting row order.
    internal partial class SortPathProbeColumn : TableViewColumn
    {
        public string ReadSortMemberPath() => GetSortMemberPathCore();
    }


    // Orders by name length first, then ordinally. Deliberately not alphabetical, so a default
    // comparison cannot produce this order by coincidence.
    internal sealed class ReverseNameComparer : ITableViewSortComparer
    {
        public int Compare(object left, object right)
            => string.CompareOrdinal((right as ShapedPerson)?.Name, (left as ShapedPerson)?.Name);
    }

    internal sealed class LengthThenOrdinalComparer : ITableViewSortComparer
    {
        public int CompareCount { get; private set; }

        public int Compare(object left, object right)
        {
            CompareCount++;

            var leftName = (left as ShapedPerson)?.Name ?? string.Empty;
            var rightName = (right as ShapedPerson)?.Name ?? string.Empty;

            if (leftName.Length != rightName.Length)
            {
                return leftName.Length - rightName.Length;
            }

            return string.CompareOrdinal(leftName, rightName);
        }
    }

    internal static class TableViewSortingTestHelpers
    {
        // The six-person fixture in source order. Named so a test asserting "unchanged" says what
        // unchanged means.
        internal static readonly string[] SourceOrderNames =
            { "Asha", "Diego", "Mei", "Rafa", "Ines", "Owen" };

        internal static readonly string[] NameAscendingOrder =
            { "Asha", "Diego", "Ines", "Mei", "Owen", "Rafa" };

        internal static TableView CreateSortTable() => CreateSortTable(MakeShapedPeople());

        // Two text template columns whose SortMemberPath names the displayed property, the common
        // authoring shape most tests exercise.
        internal static TableView CreateSortTable(object itemsSource)
            => CreateTableView(itemsSource, headers: new[] { "Name", "Role" }, width: 600, height: 700);

        // One projected value per row, in projected order, read from the repeater's view so the
        // result describes the projection rather than whatever happens to be realized.
        internal static List<string> RowValues(TableView tableView, Func<ShapedPerson, string> selector)
            => GetProjectedItems(tableView)
                .Select(item => item is ShapedPerson person ? selector(person) : "<not a ShapedPerson>")
                .ToList();

        // The SortIndicator built into a header, or null when no affordance was built. Named
        // "TableViewSortIndicator" by TableView::AppendSortIndicatorVisual.
        internal static SortIndicator FindSortIndicator(TableView tableView, int columnIndex)
            => FindVisualChildrenByType<SortIndicator>(GetHeaderCell(tableView, columnIndex)).FirstOrDefault();

        // Call on the UI thread.
        internal static void InvokeHeader(TableView tableView, int columnIndex)
        {
            var provider = CreateColumnHeaderPeer(tableView, tableView.Columns[columnIndex])
                .GetPattern(PatternInterface.Invoke) as IInvokeProvider;
            if (provider == null)
            {
                Verify.Fail("A sortable header must offer the Invoke pattern.");
                return;
            }

            provider.Invoke();
        }
    }
}
