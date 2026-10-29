// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2 (the shaping redesign), Milestone 1: a flat source bound end to end.
    // As for TableViewSource, the projection is observed only through a bound TableView:
    // GetRowsRepeater(table).ItemsSourceView is the projection itself, and VerifyProjection reads
    // the realized rows' ShapedPerson names in order.
    [TestClass]
    public class TableViewSource2Tests : TableViewApiTestBase
    {
        private static readonly string[] s_people = { "Asha", "Diego", "Mei", "Rafa", "Ines", "Owen" };

        // Tall enough that every row of MakeShapedPeople realizes.
        private static TableView CreateTable(object itemsSource)
            => CreateTableView(itemsSource, headers: new[] { "Name", "Role" }, width: 500, height: 700);

        private static int ProjectedCount(TableView tableView)
            => GetRowsRepeater(tableView).ItemsSourceView?.Count ?? 0;

        private static void VerifyArgumentException(Action action, string expectedMessage, string context)
        {
            try
            {
                action();
            }
            catch (ArgumentException e)
            {
                Verify.IsTrue(e.Message.Contains(expectedMessage),
                    $"{context}: the message should contain '{expectedMessage}', was '{e.Message}'.");
                return;
            }

            Verify.Fail($"{context}: expected an ArgumentException.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a TableViewSource2 over a flat list projects every item in source order.")]
        public void VerifyFlatBindProjectsSourceOrder()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTable(TableViewSource2.From(MakeShapedPeople()));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(tableView), "Every source item must be one row.");
                VerifyProjection(tableView, s_people, "Flat bind");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a TableViewSource2 over an empty collection projects no rows.")]
        public void VerifyEmptySourceProjectsNoRows()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTable(TableViewSource2.From(new List<ShapedPerson>()));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(GetRowsRepeater(tableView).ItemsSourceView, "An empty source still binds a view.");
                Verify.AreEqual(0, ProjectedCount(tableView), "An empty source projects no rows.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a null item in the source fails From with E_INVALIDARG.")]
        public void VerifyNullItemThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var people = MakeShapedPeople();
                people.Insert(2, null);

                VerifyArgumentException(() => TableViewSource2.From(people),
                    "A row could not be given a stable identity: null item", "Null item");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the same item object twice in the source fails From with E_INVALIDARG.")]
        public void VerifyDuplicateItemThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var people = MakeShapedPeople();
                people.Add(people[1]);

                VerifyArgumentException(() => TableViewSource2.From(people),
                    "The same item object appears in the source more than once", "Duplicate item");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies From rejects null and objects that are not collections.")]
        public void VerifyUnsupportedSourceThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.Throws<ArgumentException>(() => TableViewSource2.From(null), "From(null) must throw.");
                VerifyArgumentException(() => TableViewSource2.From(42),
                    "items must implement a supported collection interface", "From(42)");
                VerifyArgumentException(() => TableViewSource2.From(new ShapedPerson("Asha", "Designer", "Studio")),
                    "items must implement a supported collection interface", "From(single item)");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a plain List still binds through the v1 TableViewSource path.")]
        public void VerifyPlainListStillBindsThroughV1()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTable(MakeShapedPeople());
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(tableView), "A plain list projects every item.");
                VerifyProjection(tableView, s_people, "Plain list");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Refresh re-reads a non-observable source and shows the new rows.")]
        public void VerifyRefreshRereadsSource()
        {
            TableView tableView = null;
            TableViewSource2 source = null;
            List<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                source = TableViewSource2.From(people);
                tableView = CreateTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                people.RemoveAt(0);
                people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
                source.Refresh();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new[] { "Diego", "Mei", "Rafa", "Ines", "Owen", "Zoe" }, "After Refresh");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Refresh that finds a duplicate throws and leaves the projection as it was.")]
        public void VerifyRefreshWithDuplicateKeepsProjection()
        {
            TableView tableView = null;
            TableViewSource2 source = null;
            List<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                source = TableViewSource2.From(people);
                tableView = CreateTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                people.Add(people[0]);
                VerifyArgumentException(() => source.Refresh(),
                    "The same item object appears in the source more than once", "Refresh with duplicate");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, s_people, "A failed Refresh must not change the rows");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies unbinding, unloading and reloading a table keeps or reclaims its TableViewSource2.")]
        public void VerifyUnbindUnloadReloadReclaims()
        {
            TableView tableView = null;
            TableViewSource2 source = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource2.From(MakeShapedPeople());
                tableView = CreateTable(source);
                host = new StackPanel();
                host.Children.Add(tableView);
                LoadContent(host);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.ItemsSource = null);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual(0, ProjectedCount(tableView), "Unbound: no rows."));

            RunOnUIThread.Execute(() => tableView.ItemsSource = source);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyProjection(tableView, s_people, "Rebound"));

            RunOnUIThread.Execute(() => host.Children.Remove(tableView));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => host.Children.Add(tableView));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(tableView), "Reloaded: the table reclaims the source.");
                VerifyProjection(tableView, s_people, "Reloaded");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a second TableView bound to an owned TableViewSource2 renders nothing, then takes over when the owner unloads.")]
        public void VerifySecondTableWaitsThenTakesOver()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource2 source = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource2.From(MakeShapedPeople());
                first = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                second = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host.Children.Add(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(first), "The owner keeps its rows.");
                Verify.AreEqual(0, ProjectedCount(second), "The refused table must render no rows.");
                host.Children.Remove(first);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(second), "The waiting table takes over when the owner unloads.");
                GC.KeepAlive(first);
            });
        }

        // Review Focus: a LINQ result reaches native code as an iterable whose iterator may fail
        // with E_NOINTERFACE; the accessor must retry through IBindableIterable.
        [TestMethod]
        [TestProperty("Description", "Verifies a CLR IEnumerable that is not a list binds through TableViewSource2.")]
        public void VerifyIterableOnlySourceBinds()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                IEnumerable<ShapedPerson> query = MakeShapedPeople().Select(p => p);
                tableView = CreateTable(TableViewSource2.From(query));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyProjection(tableView, s_people, "Iterable-only source"));
        }

        // Review Focus: a managed item crosses into native code through different interface
        // pointers; lookups must resolve it through its canonical IUnknown.
        [TestMethod]
        [TestProperty("Description", "Verifies ItemsSourceView.IndexOf finds a managed item by object identity.")]
        public void VerifyIndexOfResolvesThroughCanonicalIdentity()
        {
            TableView tableView = null;
            List<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                tableView = CreateTable(TableViewSource2.From(people));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var view = GetRowsRepeater(tableView).ItemsSourceView;
                for (int i = 0; i < people.Count; i++)
                {
                    Verify.AreEqual(i, view.IndexOf(people[i]), $"IndexOf({people[i].Name})");
                }

                Verify.AreEqual(-1, view.IndexOf(new ShapedPerson("Asha", "Designer", "Studio")),
                    "An equal-looking but distinct object is not in the source.");
                Verify.AreSame(people[2], view.GetAt(2), "GetAt returns the app's own object.");
            });
        }

        // Review Focus: the same source set again on the same table must be reclaimed, and the
        // release after it must still let another table take over.
        [TestMethod]
        [TestProperty("Description", "Verifies rebinding the same TableViewSource2 to the same TableView reclaims it and later releases it cleanly.")]
        public void VerifyRebindSameSourceToSameTable()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource2 source = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource2.From(MakeShapedPeople());
                first = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                first.ItemsSource = source;
                first.ItemsSource = null;
                first.ItemsSource = source;
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(first), "The table must reclaim its own source.");
                second = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host.Children.Add(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, ProjectedCount(second), "The rebound table still owns the source.");
                first.ItemsSource = null;
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(second), "Unbinding the owner hands the source to the waiting table.");
                Verify.AreEqual(0, ProjectedCount(first), "The unbound table renders no rows.");
            });
        }

        // Review Focus: the source holds its owner as a raw pointer; a table that goes away must
        // not leave it behind.
        [TestMethod]
        [TestProperty("Description", "Verifies a TableViewSource2 that outlives its TableView can be refreshed and bound again.")]
        public void VerifySourceOutlivesTableView()
        {
            TableViewSource2 source = null;
            List<ShapedPerson> people = null;
            WeakReference firstTable = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                source = TableViewSource2.From(people);
                firstTable = LoadTableAndForget(source);
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Content = null);
            IdleSynchronizer.Wait();

            for (int attempt = 0; attempt < 5 && firstTable.IsAlive; attempt++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                IdleSynchronizer.Wait();
            }

            Verify.IsFalse(firstTable.IsAlive, "The first TableView must be collected.");

            TableView second = null;
            RunOnUIThread.Execute(() =>
            {
                // No owner now: a Refresh must not call back into the destroyed table.
                people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
                source.Refresh();

                second = CreateTable(source);
                LoadContent(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length + 1, ProjectedCount(second), "A new table owns the surviving source.");
            });
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private WeakReference LoadTableAndForget(TableViewSource2 source)
        {
            var tableView = CreateTable(source);
            LoadContent(tableView);
            return new WeakReference(tableView);
        }

        // Review Focus: each swap releases the previous source of the other generation.
        [TestMethod]
        [TestProperty("Description", "Verifies swapping ItemsSource from TableViewSource2 to a v1 source and back releases and reclaims ownership.")]
        public void VerifySwapV2ToV1ToV2()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource2 source = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource2.From(MakeShapedPeople());
                first = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() => first.ItemsSource = TableViewSource.From(MakeShapedPeople()));
            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(first), "The v1 source projects its rows.");
                second = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host.Children.Add(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(second),
                    "Swapping to v1 must have released the v2 source, so another table can own it.");
                first.ItemsSource = source;
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, ProjectedCount(first), "The v2 source is owned by the second table now.");
                host.Children.Remove(second);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(first), "Swapped back to v2, the table takes the source over.");
                VerifyProjection(first, s_people, "v2 -> v1 -> v2");
                GC.KeepAlive(second);
            });
        }
    }
}
